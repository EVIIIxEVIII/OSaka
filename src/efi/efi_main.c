#include "x86_64/efibind.h"
#include <efi.h>
#include <efilib.h>

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

UINT64 file_size(EFI_FILE_HANDLE file_handle) {
    EFI_FILE_INFO *file_info = LibFileInfo(file_handle);
    UINT64 size = file_info->FileSize;
    FreePool(file_info);
    return size;
}

EFI_STATUS exit_boot(EFI_HANDLE image) {
    EFI_STATUS st;
    UINTN mmap_size = 0, map_key, desc_size;
    UINT32 desc_ver;

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

static void dump_bytes(void *addr, UINTN len) {
    UINT8 *p = (UINT8*)addr;
    for (UINTN i = 0; i < len; i += 16) {
        Print(L"%08lx: ", (UINTN)addr + i);
        for (UINTN j = 0; j < 16 && i + j < len; ++j)
            Print(L"%02x ", p[i + j]);
        Print(L"\r\n");
    }
}

EFI_STATUS efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *system_table) {
    InitializeLib(image_handle, system_table);
    Print(L"Loading the kernel!\r\n");

    EFI_FILE_HANDLE volume = get_volume(image_handle);
    Print(L"Loaded the volume!\r\n");

    CHAR16 *kernel_file_name = L"kernel.bin";
    EFI_FILE_HANDLE kernel_handle;

    uefi_call_wrapper(volume->Open, 5, volume, &kernel_handle, kernel_file_name, EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY | EFI_FILE_HIDDEN | EFI_FILE_SYSTEM);
    Print(L"Opened the kernel file!\r\n");

    UINT64 kernel_size = file_size(kernel_handle);
    UINT8  *buffer     = AllocatePool(kernel_size);

    uefi_call_wrapper(kernel_handle->Read, 3, kernel_handle, &kernel_size, buffer);
    uefi_call_wrapper(kernel_handle->Close, 1, kernel_handle);

    Print(L"Loaded the kernel into memory!\r\n");

    EFI_PHYSICAL_ADDRESS phys = 0x100000;
    UINTN pages = (UINTN)((kernel_size + 0xFFF) >> 12); // add 4095 and dividde by 2^12
    EFI_STATUS st_alloc = uefi_call_wrapper(BS->AllocatePages, 4,
                        AllocateAddress, EfiLoaderData, pages, &phys);

    if (EFI_ERROR(st_alloc)) {
        Print(L"AllocatePages at 0x100000 failed: %r\r\n", st_alloc);
        return st_alloc;
    }

    Print(L"Allocated pages at 0x100000!\r\n");
    uefi_call_wrapper(BS->CopyMem, 3, (VOID*)(UINTN)phys, buffer, (UINTN)kernel_size);
    FreePool(buffer);

    Print(L"Copied the kernel into the allocated memory!\r\n");
    uefi_call_wrapper(volume->Close, 1, volume);

    Print(L"Closed the volume!\r\n");

    EFI_STATUS st_exit = exit_boot(image_handle);
    if (EFI_ERROR(st_exit)) {
        Print(L"ExitBootServices failed: %r\r\n", st_exit);
        return st_exit;
    }

    typedef void (*kentry_t)(void);
    kentry_t kentry = (kentry_t)(UINTN)0x100000;
    kentry();

    return EFI_SUCCESS;
}

