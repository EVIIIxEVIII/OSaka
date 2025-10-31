#include <efi.h>
#include <efilib.h>
#include <string.h>
#include "shared/types.h"
#include "shared/shared.h"
#include "x86_64/efibind.h"

static EFI_GRAPHICS_OUTPUT_PROTOCOL *get_gop(void) {
    EFI_GUID gop_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;

    uefi_call_wrapper(BS->LocateProtocol, 3, &gop_guid, NULL, (void**)&gop);
    return gop;
}

EFI_FILE_HANDLE get_volume(EFI_HANDLE image) {
    EFI_LOADED_IMAGE_PROTOCOL *loaded_image = NULL;
    EFI_GUID lip_guid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
    EFI_FILE_IO_INTERFACE *io_volume;
    EFI_GUID fsGuid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
    EFI_FILE_HANDLE volume;

    uefi_call_wrapper(BS->HandleProtocol, 3, image, &lip_guid, (void **) &loaded_image);
    uefi_call_wrapper(BS->HandleProtocol, 3, loaded_image->DeviceHandle, &fsGuid, (VOID*)&io_volume);
    uefi_call_wrapper(io_volume->OpenVolume, 2, io_volume, &volume);

    return volume;
}

u64 file_size(EFI_FILE_HANDLE file_handle) {
    EFI_FILE_INFO *file_info = LibFileInfo(file_handle);
    u64 size = file_info->FileSize;
    FreePool(file_info);
    return size;
}

EFI_STATUS exit_boot(EFI_HANDLE image) {
    EFI_STATUS st;
    u64 mmap_size = 0, map_key, desc_size;
    u32 desc_ver;

    st = uefi_call_wrapper(BS->GetMemoryMap, 5, &mmap_size, NULL, &map_key, &desc_size, &desc_ver);
    if (st != EFI_BUFFER_TOO_SMALL) return st;

    mmap_size += 2 * desc_size;
    EFI_MEMORY_DESCRIPTOR *mmap = AllocatePool(mmap_size);
    if (!mmap) return EFI_OUT_OF_RESOURCES;

    st = uefi_call_wrapper(BS->GetMemoryMap, 5, &mmap_size, mmap, &map_key, &desc_size, &desc_ver);
    if (EFI_ERROR(st)) return st;

    st = uefi_call_wrapper(BS->ExitBootServices, 2, image, map_key);
    return st;
}

static int checksum_ok(const void *p, u64 len) {
    if (len != 36) return 0;
    const i8 *b = p; u8 s = 0;
    for (u64 i = 0; i < len; i++) s += b[i];
    return s == 0;
}

static RSDP* discoverRsdp(EFI_SYSTEM_TABLE* system_table) {
    RSDP* rsdp;
    for (u64 i = 0; i < system_table->NumberOfTableEntries; ++i) {
        EFI_CONFIGURATION_TABLE* t = &system_table->ConfigurationTable[i];

        static EFI_GUID Acpi20Guid = ACPI_20_TABLE_GUID;
        static EFI_GUID AcpiGuid = ACPI_TABLE_GUID;

        if (!CompareGuid(&t->VendorGuid, &Acpi20Guid) && !CompareGuid(&t->VendorGuid, &AcpiGuid)) {
            continue;
        }

        rsdp = (RSDP*)t->VendorTable;
        if (rsdp && checksum_ok(rsdp, rsdp->length)) {
            Print(L"Rsdp signature: %.8a\r\n", rsdp->signature);
            return rsdp;
        }
    }

    return NULL;
}

EFI_STATUS efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *system_table) {
    InitializeLib(image_handle, system_table);
    Print(L"Loading the kernel...\r\n");

    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop = get_gop();
    if (!gop) {
        Print(L"GOP not found\r\n");
        return EFI_UNSUPPORTED;
    }

    u64 BootData_bytes = sizeof(BootData);
    u64 BootData_pages = EFI_SIZE_TO_PAGES(BootData_bytes);
    EFI_PHYSICAL_ADDRESS BootData_physical_address = 0;
    EFI_STATUS s = uefi_call_wrapper(BS->AllocatePages, 4, AllocateAnyPages, EfiLoaderData, BootData_pages, &BootData_physical_address);
    if (EFI_ERROR(s)) {
        Print(L"Failed to allocated memory for the BootData struct!\r\n");
        return s;
    }

    FramebufferInfo framebuffer;
    framebuffer.base = (void*) gop->Mode->FrameBufferBase;
    framebuffer.width = gop->Mode->Info->HorizontalResolution;
    framebuffer.height = gop->Mode->Info->VerticalResolution;
    framebuffer.pitch = framebuffer.pixels_per_scanline * 4;
    framebuffer.pixels_per_scanline = gop->Mode->Info->PixelsPerScanLine;

    BootData* boot_dt = (BootData*)BootData_physical_address;
    boot_dt->fb = framebuffer;
    boot_dt->rsdp = discoverRsdp(system_table);
    if (!boot_dt) {
        Print(L"Failed to discover RSDP!");
        return EFI_SUCCESS;
    }

    EFI_FILE_HANDLE volume = get_volume(image_handle);
    CHAR16 *kernel_file_name = L"kernel.bin";
    EFI_FILE_HANDLE kernel_handle;

    uefi_call_wrapper(volume->Open, 5, volume, &kernel_handle, kernel_file_name, EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY | EFI_FILE_HIDDEN | EFI_FILE_SYSTEM);

    u64 kernel_size = file_size(kernel_handle);
    boot_dt->kernel_size = kernel_size;
    u64 *buffer     = AllocatePool(kernel_size);

    uefi_call_wrapper(kernel_handle->Read, 3, kernel_handle, &kernel_size, buffer);
    uefi_call_wrapper(kernel_handle->Close, 1, kernel_handle);

    EFI_PHYSICAL_ADDRESS phys = 0x100000;
    u64 pages = (u64)EFI_SIZE_TO_PAGES(kernel_size); // add 4095 and dividde by 4096 to get the number of pages
    EFI_STATUS st_alloc = uefi_call_wrapper(BS->AllocatePages, 4,
                        AllocateAddress, EfiLoaderData, pages, &phys);

    if (EFI_ERROR(st_alloc)) {
        Print(L"AllocatePages at 0x100000 failed: %r\r\n", st_alloc);
        return st_alloc;
    }

    uefi_call_wrapper(BS->CopyMem, 3, (VOID*)(u64)phys, buffer, (u64)kernel_size);
    FreePool(buffer);

    uefi_call_wrapper(volume->Close, 1, volume);

    Print(L"Exiting boot...\r\n");

    EFI_STATUS st_exit = exit_boot(image_handle);
    if (EFI_ERROR(st_exit)) {
        Print(L"ExitBootServices failed: %r\r\n", st_exit);
        return st_exit;
    }

    typedef void (*kentry_t)(BootData*);
    kentry_t kentry = (kentry_t)(u64)0x100000;
    kentry(boot_dt);

    while(1);
    return EFI_SUCCESS;
}

