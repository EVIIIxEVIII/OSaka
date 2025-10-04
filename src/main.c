#include <efi.h>
#include <efilib.h>

EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    InitializeLib(ImageHandle, SystemTable);
    Print(L"Hello, UEFI World!\r\nPress any key to exit...\r\n");

    UINTN idx;
    SystemTable->BootServices->WaitForEvent(
        1, &SystemTable->ConIn->WaitForKey, &idx);

    Print(L"The index: %lu\n", idx);

    EFI_INPUT_KEY k;
    SystemTable->ConIn->ReadKeyStroke(SystemTable->ConIn, &k);

    Print(L"The key: %lu\n", k);

    return EFI_SUCCESS;
}

