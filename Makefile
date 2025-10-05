GNUEFI := third_party/gnu-efi
OVMF   := /usr/share/qemu/OVMF.fd
ARCH   := x86_64

BUILDDIR := build
EFISRC   := src/efi
KSRC     := src/kernel
ISODIR   := iso/EFI/BOOT
IMG      := fat.img
NASM 	 := nasm

INC            := $(GNUEFI)/inc
LIBDIR         := $(GNUEFI)/$(ARCH)/lib
GNUEFI_LIBDIR  := $(GNUEFI)/$(ARCH)/gnuefi
CRT0           := $(GNUEFI_LIBDIR)/crt0-efi-$(ARCH).o
LDS            := $(GNUEFI)/gnuefi/elf_$(ARCH)_efi.lds

EFI_CFLAGS := -I$(INC) -fpic -ffreestanding -fno-stack-protector -fno-stack-check \
    -fshort-wchar -mno-red-zone -Wall -Wextra
EFI_LDFLAGS := -shared -Bsymbolic -L$(LIBDIR) -L$(GNUEFI_LIBDIR) -T$(LDS)
EFI_LIBS    := -lgnuefi -lefi
EFI_OBJCOPY := -j .text -j .sdata -j .data -j .rodata -j .dynamic -j .dynsym \
    -j .rel -j .rela -j .rel.* -j .rela.* -j .reloc --target efi-app-$(ARCH) --subsystem=10

K_CFLAGS := -ffreestanding -fno-pic -fno-stack-protector -mno-red-zone -Wall -Wextra
K_LDS    := $(KSRC)/kernel.ld

EFI_SRCS := $(wildcard $(EFISRC)/*.c)
EFI_OBJS := $(patsubst $(EFISRC)/%.c,$(BUILDDIR)/efi/%.o,$(EFI_SRCS))
EFI_SO   := $(BUILDDIR)/efi/main.so
EFI_BIN  := $(BUILDDIR)/efi/BOOTX64.EFI

K_CSRCS := $(wildcard $(KSRC)/*.c)
K_ASMS  := $(wildcard $(KSRC)/*.asm)
K_OBJS  := $(patsubst $(KSRC)/%.c,$(BUILDDIR)/kernel/%.o,$(K_CSRCS)) \
           $(patsubst $(KSRC)/%.asm,$(BUILDDIR)/kernel/%.o,$(K_ASMS))
K_ELF   := $(BUILDDIR)/kernel/kernel.elf
K_BIN   := $(BUILDDIR)/kernel/kernel.bin

.DEFAULT_GOAL := run
.PHONY: all image run clean

all: $(EFI_BIN) $(K_BIN)

image: all
	mkdir -p $(ISODIR)
	cp $(EFI_BIN) $(ISODIR)/BOOTX64.EFI
	truncate -s 64M $(IMG)
	mkfs.vfat -F32 $(IMG)
	mmd   -i $(IMG) ::/EFI ::/EFI/BOOT
	mcopy -i $(IMG) $(ISODIR)/BOOTX64.EFI ::/EFI/BOOT/
	mcopy -i $(IMG) $(K_BIN) ::/kernel.bin

run: image
	qemu-system-x86_64 \
		-bios $(OVMF) \
		-drive format=raw,file=$(IMG) \
		-device isa-debugcon,iobase=0xE9,chardev=dbg \
		-chardev stdio,id=dbg

clean:
	rm -rf $(BUILDDIR) $(dir $(ISODIR)) $(IMG)

$(BUILDDIR) $(BUILDDIR)/efi $(BUILDDIR)/kernel:
	mkdir -p $@

$(BUILDDIR)/efi/%.o: $(EFISRC)/%.c | $(BUILDDIR)/efi
	gcc $(EFI_CFLAGS) -c $< -o $@

$(EFI_SO): $(EFI_OBJS)
	ld $(EFI_LDFLAGS) $(CRT0) $^ -o $@ $(EFI_LIBS)

$(EFI_BIN): $(EFI_SO)
	objcopy $(EFI_OBJCOPY) $< $@

$(BUILDDIR)/kernel/%.o: $(KSRC)/%.c | $(BUILDDIR)/kernel
	gcc $(K_CFLAGS) -c $< -o $@

$(BUILDDIR)/kernel/%.o: $(KSRC)/%.asm | $(BUILDDIR)/kernel
	$(NASM) -f elf64 $< -o $@

$(K_ELF): $(K_OBJS) $(K_LDS)
	ld -T $(K_LDS) -nostdlib $(K_OBJS) -o $@

$(K_BIN): $(K_ELF)
	objcopy -O binary $< $@

