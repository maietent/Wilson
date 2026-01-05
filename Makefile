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

# disk image paths
DISK_DEBUG := builds/img/WilsonD.img
DISK_RELEASE := builds/img/Wilson.img
DISK_SIZE_MB := 96

# ensure bin and builds directories
$(shell mkdir -p bin/debug bin/release bin/limine builds/isos builds/img)

# silent mode by default
.SILENT:

.PHONY: all run-debug run-release debug release clean clean_no_iso qemu-disk-debug qemu-disk-release debug-complete release-complete

# default target - builds debug complete, then release complete
all: debug-complete release-complete

# debug-complete builds debug kernel, then both debug iso and img
debug-complete: bin/debug/kernel.elf
	@$(MAKE) builds/isos/WilsonD.iso
	@$(MAKE) $(DISK_DEBUG)

# release-complete builds release kernel, then both release iso and img
release-complete: bin/release/kernel.elf
	@$(MAKE) builds/isos/Wilson.iso
	@$(MAKE) $(DISK_RELEASE)

# ------------------------
# Compile rules
# ------------------------

bin/debug/%.o: src/%.c $(KERNEL_HDR)
	@mkdir -p $(dir $@)
	@echo "Compiling (debug) $<..."
	@$(CC) $(CFLAGS_DEBUG) $(INCLUDES) -c $< -o $@

bin/release/%.o: src/%.c $(KERNEL_HDR)
	@mkdir -p $(dir $@)
	@echo "Compiling (release) $<..."
	@$(CC) $(CFLAGS_RELEASE) $(INCLUDES) -c $< -o $@

bin/debug/%.o: src/%.asm
	@mkdir -p $(dir $@)
	@echo "Assembling (debug) $<..."
	@$(AS) -f elf64 -o $@ $<

bin/release/%.o: src/%.asm
	@mkdir -p $(dir $@)
	@echo "Assembling (release) $<..."
	@$(AS) -f elf64 -o $@ $<

# ------------------------
# Link kernel
# ------------------------

bin/debug/kernel.elf: $(KERNEL_OBJ_DEBUG)
	@echo "Linking debug kernel..."
	@mkdir -p bin/debug
	@$(LD) $(LDFLAGS) -o $@ $^
	@echo "Debug kernel linked successfully"

bin/release/kernel.elf: $(KERNEL_OBJ_RELEASE)
	@echo "Linking release kernel..."
	@mkdir -p bin/release
	@$(LD) $(LDFLAGS) -o $@ $^
	@echo "Release kernel linked successfully"

# ------------------------
# Limine bootloader
# ------------------------

bin/limine/limine:
	@if [ ! -f "bin/limine/limine" ]; then \
		echo "Fetching limine bootloader..."; \
		mkdir -p bin; \
		cd bin && git clone https://github.com/limine-bootloader/limine.git --branch=v8.x-binary --depth=1 > /dev/null 2>&1; \
		echo "Building limine..."; \
		$(MAKE) -C limine > /dev/null 2>&1; \
		echo "Limine ready"; \
	else \
		echo "Limine already present"; \
	fi

# ------------------------
# ISO build rules
# ------------------------

builds/isos/WilsonD.iso: bin/debug/kernel.elf bin/limine/limine
	@echo "Building debug ISO..."
	@rm -rf bin/wilsonfs
	@mkdir -p bin/wilsonfs/boot/limine
	@cp bin/debug/kernel.elf bin/wilsonfs/boot/kernel.elf
	@cp limine.conf bin/wilsonfs/boot/limine/
	@cp bin/limine/limine-bios.sys bin/limine/limine-bios-cd.bin bin/limine/limine-uefi-cd.bin bin/wilsonfs/boot/limine/
	@mkdir -p bin/wilsonfs/EFI/BOOT
	@cp bin/limine/BOOTX64.EFI bin/wilsonfs/EFI/BOOT/
	@mkdir -p bin/wilsonfs/root
	@xorriso -as mkisofs -R -J -b boot/limine/limine-bios-cd.bin -no-emul-boot \
		-boot-load-size 4 -boot-info-table -hfsplus -apm-block-size 2048 \
		--efi-boot boot/limine/limine-uefi-cd.bin -efi-boot-part --efi-boot-image \
		--protective-msdos-label bin/wilsonfs -o builds/isos/WilsonD.iso 2>&1 | grep -v "NOTE"
	@./bin/limine/limine bios-install builds/isos/WilsonD.iso 2>/dev/null
	@rm -rf bin/wilsonfs
	@echo "Debug ISO created: builds/isos/WilsonD.iso"

builds/isos/Wilson.iso: bin/release/kernel.elf bin/limine/limine
	@echo "Building release ISO..."
	@rm -rf bin/wilsonfs
	@mkdir -p bin/wilsonfs/boot/limine
	@cp bin/release/kernel.elf bin/wilsonfs/boot/kernel.elf
	@cp limine.conf bin/wilsonfs/boot/limine/
	@cp bin/limine/limine-bios.sys bin/limine/limine-bios-cd.bin bin/limine/limine-uefi-cd.bin bin/wilsonfs/boot/limine/
	@mkdir -p bin/wilsonfs/EFI/BOOT
	@cp bin/limine/BOOTX64.EFI bin/wilsonfs/EFI/BOOT/
	@mkdir -p bin/wilsonfs/root
	@xorriso -as mkisofs -R -J -b boot/limine/limine-bios-cd.bin -no-emul-boot \
		-boot-load-size 4 -boot-info-table -hfsplus -apm-block-size 2048 \
		--efi-boot boot/limine/limine-uefi-cd.bin -efi-boot-part --efi-boot-image \
		--protective-msdos-label bin/wilsonfs -o builds/isos/Wilson.iso 2>&1 | grep -v "NOTE"
	@./bin/limine/limine bios-install builds/isos/Wilson.iso 2>/dev/null
	@rm -rf bin/wilsonfs
	@echo "Release ISO created: builds/isos/Wilson.iso"

# ------------------------
# Raw disk images (with proper partition)
# ------------------------

$(DISK_DEBUG): bin/debug/kernel.elf bin/limine/limine
	@echo "Creating debug disk image..."
	@mkdir -p builds/img
	@rm -f $(DISK_DEBUG)
	@dd if=/dev/zero of=$(DISK_DEBUG) bs=1M count=$(DISK_SIZE_MB) 2>/dev/null
	@parted -s $(DISK_DEBUG) mklabel msdos 2>/dev/null
	@parted -s $(DISK_DEBUG) mkpart primary fat32 1MiB 100% 2>/dev/null
	@parted -s $(DISK_DEBUG) set 1 boot on 2>/dev/null
	@LOOPDEV=$$(sudo losetup --show -f -P $(DISK_DEBUG)); \
	PART=$${LOOPDEV}p1; \
	sudo mkfs.fat -F 32 $$PART > /dev/null 2>&1; \
	sudo mkdir -p tmp_mnt; \
	sudo mount $$PART tmp_mnt; \
	sudo mkdir -p tmp_mnt/boot/limine; \
	sudo cp bin/debug/kernel.elf tmp_mnt/boot/kernel.elf; \
	sudo cp bin/limine/limine-bios.sys bin/limine/limine-bios-cd.bin bin/limine/limine-uefi-cd.bin tmp_mnt/boot/limine/; \
	sudo cp limine.conf tmp_mnt/boot/limine/limine.conf; \
	# Copy contents of wilsonfs to root of disk \
	sudo cp -r wilsonfs/* tmp_mnt/; \
	sudo umount tmp_mnt; \
	sudo losetup -d $$LOOPDEV; \
	rm -rf tmp_mnt
	@./bin/limine/limine bios-install $(DISK_DEBUG) 2>/dev/null
	@echo "Debug disk image created: $(DISK_DEBUG)"

$(DISK_RELEASE): bin/release/kernel.elf bin/limine/limine
	@echo "Creating release disk image..."
	@mkdir -p builds/img
	@rm -f $(DISK_RELEASE)
	@dd if=/dev/zero of=$(DISK_RELEASE) bs=1M count=$(DISK_SIZE_MB) 2>/dev/null
	@parted -s $(DISK_RELEASE) mklabel msdos 2>/dev/null
	@parted -s $(DISK_RELEASE) mkpart primary fat32 1MiB 100% 2>/dev/null
	@parted -s $(DISK_RELEASE) set 1 boot on 2>/dev/null
	@LOOPDEV=$$(sudo losetup --show -f -P $(DISK_RELEASE)); \
	PART=$${LOOPDEV}p1; \
	sudo mkfs.fat -F 32 $$PART > /dev/null 2>&1; \
	sudo mkdir -p tmp_mnt; \
	sudo mount $$PART tmp_mnt; \
	sudo mkdir -p tmp_mnt/boot/limine; \
	sudo cp bin/release/kernel.elf tmp_mnt/boot/kernel.elf; \
	sudo cp bin/limine/limine-bios.sys bin/limine/limine-bios-cd.bin bin/limine/limine-uefi-cd.bin tmp_mnt/boot/limine/; \
	sudo cp limine.conf tmp_mnt/boot/limine/limine.conf; \
	# Copy contents of wilsonfs to root of disk \
	sudo cp -r wilsonfs/* tmp_mnt/; \
	sudo umount tmp_mnt; \
	sudo losetup -d $$LOOPDEV; \
	rm -rf tmp_mnt
	@./bin/limine/limine bios-install $(DISK_RELEASE) 2>/dev/null
	@echo "Release disk image created: $(DISK_RELEASE)"

# ------------------------
# Clean
# ------------------------

clean:
	@echo "Cleaning all build artifacts..."
	@rm -rf bin $(KERNEL_OBJ_DEBUG) $(KERNEL_OBJ_RELEASE) builds mnt_boot mnt_root
	@echo "Clean complete"

clean_no_iso:
	@echo "Cleaning build artifacts (keeping ISOs)..."
	@rm -rf bin $(KERNEL_OBJ_DEBUG) $(KERNEL_OBJ_RELEASE) mnt_boot mnt_root
	@echo "Clean complete"

# ------------------------
# Build targets
# ------------------------

debug: builds/isos/WilsonD.iso
release: builds/isos/Wilson.iso

run-debug: debug
	@echo "Launching QEMU..."
	@qemu-system-x86_64 -serial stdio -M q35 -cdrom builds/isos/WilsonD.iso $(QEMUFLAGS)

run-release: release
	@echo "Launching QEMU..."
	@qemu-system-x86_64 -serial stdio -M q35 -cdrom builds/isos/Wilson.iso $(QEMUFLAGS)

qemu-disk-debug: $(DISK_DEBUG)
	@echo "Booting debug disk in QEMU..."
	@qemu-system-x86_64 -serial stdio -hda $(DISK_DEBUG) $(QEMUFLAGS)

qemu-disk-release: $(DISK_RELEASE)
	@echo "Booting release disk in QEMU..."
	@qemu-system-x86_64 -serial stdio -hda $(DISK_RELEASE) $(QEMUFLAGS)
