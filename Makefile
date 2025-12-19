# vibecoded make file i think im a vibecoder now because i vibecoded a makefile help me

# nuke built-in rules and variables
MAKEFLAGS += -rR
.SUFFIXES:

# toolchain
CC := x86_64-elf-gcc
AS := nasm
LD := x86_64-elf-ld
AR := x86_64-elf-ar

# qemu flags
QEMUFLAGS := -m 512M
IMAGE_NAME := Wilson

# compiler and linker flags
CFLAGS := -m64 -mcmodel=kernel -ffreestanding -fno-stack-protector -fno-stack-check -nostdlib -O2 -g -Wall -Wextra
LDFLAGS := -T linker.ld -nostdlib -static

# source and asm files
KERNEL_SRC := $(shell find src -type f -name "*.c")
KERNEL_ASM := $(shell find src -type f -name "*.asm")
KERNEL_OBJ := $(patsubst src/%, bin/%, $(KERNEL_SRC:.c=.o) $(KERNEL_ASM:.asm=.o))

# include dirs
INCLUDE_DIRS := $(shell find include -type d)
INCLUDES := $(foreach dir, $(INCLUDE_DIRS), -I$(dir))

# ensure bin and isobuilds directories
$(shell mkdir -p bin isobuilds)

# silent mode by default
.SILENT:

.PHONY: all run-debug run-release debug release clean clean_no_iso qemu-debug qemu-release

# default target
all: release

# compile C files
bin/%.o: src/%.c
	@mkdir -p $(dir $@)
	@echo "🔨 Compiling $<..."
	@$(CC) $(CFLAGS) $(INCLUDES) $(EXTRA_CFLAGS) -c $< -o $@ 2>&1 | \
		while IFS= read -r line; do \
			if echo "$$line" | grep -qi "error:"; then \
				echo "❌ $$line"; \
			elif echo "$$line" | grep -qi "warning:"; then \
				echo "⚠️  $$line"; \
			else \
				echo "$$line"; \
			fi; \
		done; \
		exit $${PIPESTATUS[0]}

# compile assembly files with nasm
bin/%.o: src/%.asm
	@mkdir -p $(dir $@)
	@echo "⚙️  Assembling $<..."
	@$(AS) -f elf64 -o $@ $< 2>&1 | \
		while IFS= read -r line; do \
			if echo "$$line" | grep -qi "error:"; then \
				echo "❌ $$line"; \
			elif echo "$$line" | grep -qi "warning:"; then \
				echo "⚠️  $$line"; \
			else \
				echo "$$line"; \
			fi; \
		done; \
		exit $${PIPESTATUS[0]}

# link kernel
bin/kernel.elf: $(KERNEL_OBJ)
	@echo "🔗 Linking kernel..."
	@mkdir -p bin
	@$(LD) $(LDFLAGS) -o $@ $^ 2>&1 | \
		while IFS= read -r line; do \
			if echo "$$line" | grep -qi "error:"; then \
				echo "❌ $$line"; \
			elif echo "$$line" | grep -qi "warning:"; then \
				echo "⚠️  $$line"; \
			else \
				echo "$$line"; \
			fi; \
		done; \
		exit $${PIPESTATUS[0]}
	@echo "✅ Kernel linked successfully"

# fetch limine
limine/limine:
	@if [ ! -d "limine" ]; then \
		echo "📦 Fetching limine bootloader..."; \
		git clone https://github.com/limine-bootloader/limine.git --branch=v8.x-binary --depth=1 > /dev/null 2>&1; \
		echo "🔧 Building limine..."; \
		$(MAKE) -C limine > /dev/null 2>&1; \
		echo "✅ Limine ready"; \
	else \
		echo "✅ Limine already present"; \
	fi

# build ISO
isobuilds/WilsonD.iso: EXTRA_CFLAGS := -DDEBUG_BUILD
isobuilds/WilsonD.iso: bin/kernel.elf limine/limine
	@echo "📀 Building debug ISO..."
	@rm -rf isodirs
	@mkdir -p isodirs/boot/limine
	@cp bin/kernel.elf isodirs/boot/
	@cp limine.conf isodirs/boot/limine/
	@cp limine/limine-bios.sys limine/limine-bios-cd.bin limine/limine-uefi-cd.bin isodirs/boot/limine/
	@mkdir -p isodirs/EFI/BOOT
	@cp limine/BOOTX64.EFI isodirs/EFI/BOOT/
	@xorriso -as mkisofs -R -J -b boot/limine/limine-bios-cd.bin -no-emul-boot \
		-boot-load-size 4 -boot-info-table -hfsplus -apm-block-size 2048 \
		--efi-boot boot/limine/limine-uefi-cd.bin -efi-boot-part --efi-boot-image \
		--protective-msdos-label isodirs -o isobuilds/WilsonD.iso > /dev/null 2>&1
	@./limine/limine bios-install isobuilds/WilsonD.iso > /dev/null 2>&1
	@rm -rf isodirs
	@echo "✅ Debug ISO created: isobuilds/WilsonD.iso"

isobuilds/Wilson.iso: bin/kernel.elf limine/limine
	@echo "📀 Building release ISO..."
	@rm -rf isodirs
	@mkdir -p isodirs/boot/limine
	@cp bin/kernel.elf isodirs/boot/
	@cp limine.conf isodirs/boot/limine/
	@cp limine/limine-bios.sys limine/limine-bios-cd.bin limine/limine-uefi-cd.bin isodirs/boot/limine/
	@mkdir -p isodirs/EFI/BOOT
	@cp limine/BOOTX64.EFI isodirs/EFI/BOOT/
	@xorriso -as mkisofs -R -J -b boot/limine/limine-bios-cd.bin -no-emul-boot \
		-boot-load-size 4 -boot-info-table -hfsplus -apm-block-size 2048 \
		--efi-boot boot/limine/limine-uefi-cd.bin -efi-boot-part --efi-boot-image \
		--protective-msdos-label isodirs -o isobuilds/Wilson.iso > /dev/null 2>&1
	@./limine/limine bios-install isobuilds/Wilson.iso > /dev/null 2>&1
	@rm -rf isodirs
	@echo "✅ Release ISO created: isobuilds/Wilson.iso"

# cleanup
clean:
	@echo "🧹 Cleaning all build artifacts..."
	@rm -rf bin $(KERNEL_OBJ) isobuilds/*.iso limine isodirs mnt_boot mnt_root
	@echo "✅ Clean complete"

clean_no_iso:
	@echo "🧹 Cleaning build artifacts (keeping ISOs)..."
	@rm -rf bin $(KERNEL_OBJ) limine isodirs mnt_boot mnt_root
	@echo "✅ Clean complete"

# separate build targets
debug: isobuilds/WilsonD.iso
release: isobuilds/Wilson.iso

# run in qemu with ISO
run-debug: debug
	@echo "🚀 Launching QEMU..."
	@qemu-system-x86_64 -M q35 -cdrom isobuilds/WilsonD.iso $(QEMUFLAGS)

run-release: release
	@echo "🚀 Launching QEMU..."
	@qemu-system-x86_64 -M q35 -cdrom isobuilds/Wilson.iso $(QEMUFLAGS)
