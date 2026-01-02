# this makefile is vibecoded!!!!!!!!!!!!!!!!!!1

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

# compiler and linker flags (base flags)
CFLAGS_BASE := -m64 -mcmodel=kernel -ffreestanding -fno-stack-protector -fno-stack-check -nostdlib -O2 -Wall -Wextra
CFLAGS_DEBUG := $(CFLAGS_BASE) -g -DDEBUG_BUILD
CFLAGS_RELEASE := $(CFLAGS_BASE) -g

LDFLAGS := -T linker.ld -nostdlib -static

# source and asm files
KERNEL_SRC := $(shell find src -type f -name "*.c")
KERNEL_ASM := $(shell find src -type f -name "*.asm")
KERNEL_HDR := $(shell find include -type f -name "*.h")

# object files (separate for debug and release)
KERNEL_OBJ_DEBUG := $(patsubst src/%, bin/debug/%, $(KERNEL_SRC:.c=.o) $(KERNEL_ASM:.asm=.o))
KERNEL_OBJ_RELEASE := $(patsubst src/%, bin/release/%, $(KERNEL_SRC:.c=.o) $(KERNEL_ASM:.asm=.o))

# include dirs
INCLUDE_DIRS := $(shell find include -type d)
INCLUDES := $(foreach dir, $(INCLUDE_DIRS), -I$(dir))

# ensure bin and isobuilds directories
$(shell mkdir -p bin/debug bin/release isobuilds)

# silent mode by default
.SILENT:

.PHONY: all run-debug run-release debug release clean clean_no_iso qemu-debug qemu-release

# default target
all: release

# compile C files for DEBUG
bin/debug/%.o: src/%.c $(KERNEL_HDR)
	@mkdir -p $(dir $@)
	@echo "Compiling (debug) $<..."
	@$(CC) $(CFLAGS_DEBUG) $(INCLUDES) -c $< -o $@ 2>&1 | \
	while IFS= read -r line; do \
		if echo "$$line" | grep -qi "error:"; then \
			echo "$$line"; \
		elif echo "$$line" | grep -qi "warning:"; then \
			echo "$$line"; \
		else \
			echo "$$line"; \
		fi; \
	done; \
	exit $${PIPESTATUS[0]}

# compile C files for RELEASE
bin/release/%.o: src/%.c $(KERNEL_HDR)
	@mkdir -p $(dir $@)
	@echo "Compiling (release) $<..."
	@$(CC) $(CFLAGS_RELEASE) $(INCLUDES) -c $< -o $@ 2>&1 | \
	while IFS= read -r line; do \
		if echo "$$line" | grep -qi "error:"; then \
			echo "$$line"; \
		elif echo "$$line" | grep -qi "warning:"; then \
			echo "$$line"; \
		else \
			echo "$$line"; \
		fi; \
	done; \
	exit $${PIPESTATUS[0]}

# compile assembly files for DEBUG
bin/debug/%.o: src/%.asm
	@mkdir -p $(dir $@)
	@echo "Assembling (debug) $<..."
	@$(AS) -f elf64 -o $@ $< 2>&1 | \
	while IFS= read -r line; do \
		if echo "$$line" | grep -qi "error:"; then \
			echo "$$line"; \
		elif echo "$$line" | grep -qi "warning:"; then \
			echo "$$line"; \
		else \
			echo "$$line"; \
		fi; \
	done; \
	exit $${PIPESTATUS[0]}

# compile assembly files for RELEASE
bin/release/%.o: src/%.asm
	@mkdir -p $(dir $@)
	@echo "Assembling (release) $<..."
	@$(AS) -f elf64 -o $@ $< 2>&1 | \
	while IFS= read -r line; do \
		if echo "$$line" | grep -qi "error:"; then \
			echo "$$line"; \
		elif echo "$$line" | grep -qi "warning:"; then \
			echo "$$line"; \
		else \
			echo "$$line"; \
		fi; \
	done; \
	exit $${PIPESTATUS[0]}

# link kernel (DEBUG)
bin/debug/kernel.elf: $(KERNEL_OBJ_DEBUG)
	@echo "Linking debug kernel..."
	@mkdir -p bin/debug
	@$(LD) $(LDFLAGS) -o $@ $^ 2>&1 | \
	while IFS= read -r line; do \
		if echo "$$line" | grep -qi "error:"; then \
			echo "$$line"; \
		elif echo "$$line" | grep -qi "warning:"; then \
			echo "$$line"; \
		else \
			echo "$$line"; \
		fi; \
	done; \
	exit $${PIPESTATUS[0]}
	@echo "Debug kernel linked successfully"

# link kernel (RELEASE)
bin/release/kernel.elf: $(KERNEL_OBJ_RELEASE)
	@echo "Linking release kernel..."
	@mkdir -p bin/release
	@$(LD) $(LDFLAGS) -o $@ $^ 2>&1 | \
	while IFS= read -r line; do \
		if echo "$$line" | grep -qi "error:"; then \
			echo "$$line"; \
		elif echo "$$line" | grep -qi "warning:"; then \
			echo "$$line"; \
		else \
			echo "$$line"; \
		fi; \
	done; \
	exit $${PIPESTATUS[0]}
	@echo "Release kernel linked successfully"

# fetch limine
limine/limine:
	@if [ ! -d "limine" ]; then \
		echo "Fetching limine bootloader..."; \
		git clone https://github.com/limine-bootloader/limine.git --branch=v8.x-binary --depth=1 > /dev/null 2>&1; \
		echo "Building limine..."; \
		$(MAKE) -C limine > /dev/null 2>&1; \
		echo "Limine ready"; \
	else \
		echo "Limine already present"; \
	fi

# build DEBUG ISO
isobuilds/WilsonD.iso: bin/debug/kernel.elf limine/limine
	@echo "Building debug ISO..."
	@rm -rf isodirs
	@mkdir -p isodirs/boot/limine
	@cp bin/debug/kernel.elf isodirs/boot/kernel.elf
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
	@echo "Debug ISO created: isobuilds/WilsonD.iso"

# build RELEASE ISO
isobuilds/Wilson.iso: bin/release/kernel.elf limine/limine
	@echo "Building release ISO..."
	@rm -rf isodirs
	@mkdir -p isodirs/boot/limine
	@cp bin/release/kernel.elf isodirs/boot/kernel.elf
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
	@echo "Release ISO created: isobuilds/Wilson.iso"

# cleanup
clean:
	@echo "Cleaning all build artifacts..."
	@rm -rf bin $(KERNEL_OBJ_DEBUG) $(KERNEL_OBJ_RELEASE) isobuilds/*.iso limine isodirs mnt_boot mnt_root
	@echo "Clean complete"

clean_no_iso:
	@echo "Cleaning build artifacts (keeping ISOs)..."
	@rm -rf bin $(KERNEL_OBJ_DEBUG) $(KERNEL_OBJ_RELEASE) limine isodirs mnt_boot mnt_root
	@echo "Clean complete"

# separate build targets
debug: isobuilds/WilsonD.iso

release: isobuilds/Wilson.iso

# run in qemu with ISO
run-debug: debug
	@echo "Launching QEMU..."
	@qemu-system-x86_64 -serial stdio -M q35 -cdrom isobuilds/WilsonD.iso $(QEMUFLAGS)

run-release: release
	@echo "Launching QEMU..."
	@qemu-system-x86_64 -serial stdio -M q35 -cdrom isobuilds/Wilson.iso $(QEMUFLAGS)
