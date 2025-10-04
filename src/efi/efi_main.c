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

EFI_STATUS efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *system_table) {
    InitializeLib(image_handle, system_table);
    Print(L"Loading the kernel!\r\n");

    EFI_FILE_HANDLE volume = get_volume(image_handle);
    Print(L"Loaded the volume!\r\n");

    CHAR16 *kernel_file_name = L"kernel.bin";
    EFI_FILE_HANDLE kernel_handle;

    uefi_call_wrapper(volume->Open, 5, volume, &kernel_handle, kernel_file_name, EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY | EFI_FILE_HIDDEN | EFI_FILE_SYSTEM);

    Print(L"Opened the kernel, size: %lu!\r\n", file_size(kernel_handle));

    while(1);
}

