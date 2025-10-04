GNUEFI := third_party/gnu-efi
ARCH   := x86_64
OVMF   := /usr/share/qemu/OVMF.fd

INC            := $(GNUEFI)/inc
LIBDIR         := $(GNUEFI)/$(ARCH)/lib
GNUEFI_LIBDIR  := $(GNUEFI)/$(ARCH)/gnuefi
CRT0           := $(GNUEFI_LIBDIR)/crt0-efi-$(ARCH).o
LDS            := $(GNUEFI)/gnuefi/elf_$(ARCH)_efi.lds

CFLAGS := -I$(INC) -fpic -ffreestanding -fno-stack-protector -fno-stack-check \
    -fshort-wchar -mno-red-zone -Wall -Wextra
LDFLAGS := -shared -Bsymbolic -L$(LIBDIR) -L$(GNUEFI_LIBDIR) -T$(LDS)
LIBS := -lgnuefi -lefi
OBJCOPYFLAGS := -j .text -j .sdata -j .data -j .rodata -j .dynamic -j .dynsym \
    -j .rel -j .rela -j .rel.* -j .rela.* -j .reloc --target efi-app-$(ARCH) --subsystem=10

BUILDDIR := build
SRCDIR   := src
ISODIR   := iso
ISO_DIR  := $(ISODIR)/EFI/BOOT
IMG      := fat.img

SRCS := $(wildcard $(SRCDIR)/*.c)
OBJS := $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.o,$(SRCS))
SO   := $(BUILDDIR)/main.so
EFI  := $(BUILDDIR)/main.efi

.DEFAULT_GOAL := run

.PHONY: all image run clean

all: $(EFI)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c | $(BUILDDIR)
	gcc $(CFLAGS) -c $< -o $@

$(SO): $(OBJS) | $(BUILDDIR)
	ld $(LDFLAGS) $(CRT0) $^ -o $@ $(LIBS)

$(EFI): $(SO) | $(BUILDDIR)
	objcopy $(OBJCOPYFLAGS) $< $@

image: $(EFI)
	mkdir -p $(ISO_DIR)
	cp $(EFI) $(ISO_DIR)/BOOTX64.EFI
	truncate -s 64M $(IMG)
	mkfs.vfat -F32 $(IMG)
	mmd -i $(IMG) ::/EFI ::/EFI/BOOT
	mcopy -i $(IMG) $(ISO_DIR)/BOOTX64.EFI ::/EFI/BOOT/

run: image
	qemu-system-x86_64 \
		-bios $(OVMF) \
		-drive format=raw,file=$(IMG)

clean:
	rm -rf $(BUILDDIR) $(ISODIR) $(IMG)

