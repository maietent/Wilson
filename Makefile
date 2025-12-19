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

# disk image settings
DISK_SIZE := 512M
DISK_IMAGE := isobuilds/Wilson.img
DISK_IMAGE_DEBUG := isobuilds/WilsonD.img

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

.PHONY: all run-debug run-release debug release clean clean_no_iso qemu-debug qemu-release disk-debug disk-release

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

# build disk image
define make_disk
@rm -f $(1)
@echo "💾 Creating disk image $(1)..."
@dd if=/dev/zero of=$(1) bs=1M count=512 status=none
@LOOP_DEV=$$(sudo losetup -f --show -P $(1)); \
sleep 1; \
\
# BIOS boot partition
sudo parted -s $$LOOP_DEV mklabel gpt; \
sudo parted -s $$LOOP_DEV mkpart primary 1MiB 2MiB; \
sudo parted -s $$LOOP_DEV set 1 bios_grub on; \
\
# FAT32 boot partition
sudo parted -s $$LOOP_DEV mkpart primary fat32 2MiB 102MiB; \
sudo parted -s $$LOOP_DEV set 2 esp on; \
\
# ext2 root partition
sudo parted -s $$LOOP_DEV mkpart primary ext2 102MiB 100%; \
\
# filesystems
sudo mkfs.vfat -F 32 $${LOOP_DEV}p2 > /dev/null 2>&1; \
sudo mkfs.ext2 -F $${LOOP_DEV}p3 > /dev/null 2>&1; \
\
# mount FAT32
mkdir -p mnt_boot mnt_root; \
sudo mount $${LOOP_DEV}p2 mnt_boot; \
\
# install limine BIOS bootloader
sudo cp limine/limine-bios.sys mnt_boot/; \
sudo limine/limine-bios-install $$LOOP_DEV; \
\
# copy kernel and config
sudo mkdir -p mnt_boot/boot; \
sudo cp bin/kernel.elf mnt_boot/boot/; \
sudo cp limine.conf mnt_boot/boot/; \
\
# mount root partition
sudo mount $${LOOP_DEV}p3 mnt_root; \
sudo mkdir -p mnt_root/{bin,boot,dev,etc,home,lib,mnt,opt,proc,root,sbin,srv,sys,tmp,usr,var}; \
sudo mkdir -p mnt_root/usr/{bin,lib,local,sbin,share}; \
sudo mkdir -p mnt_root/var/{log,tmp}; \
\
# cleanup
sudo umount mnt_boot; \
sudo umount mnt_root; \
sudo losetup -d $$LOOP_DEV; \
rm -rf mnt_boot mnt_root; \
echo "✅ Disk image created: $(1)"
endef

disk-debug: EXTRA_CFLAGS := -DDEBUG_BUILD
disk-debug: bin/kernel.elf limine/limine
	$(call make_disk,$(DISK_IMAGE_DEBUG))

disk-release: bin/kernel.elf limine/limine
		@echo "💾 Creating release disk image..."
		@rm -f $(DISK_IMAGE)
		@dd if=/dev/zero of=$(DISK_IMAGE) bs=1M count=512 status=none
		@bash -c '\
		parted -s $(DISK_IMAGE) mklabel gpt; \
		parted -s $(DISK_IMAGE) mkpart primary fat32 1MiB 101MiB; \
		parted -s $(DISK_IMAGE) set 1 esp on; \
		parted -s $(DISK_IMAGE) mkpart primary ext2 101MiB 100%; \
		LOOP_DEV=$$(sudo losetup -f --show -P $(DISK_IMAGE)); \
		sleep 1; \
		sudo mkfs.vfat -F 32 $${LOOP_DEV}p1; \
		sudo mkfs.ext2 -F $${LOOP_DEV}p2; \
		mkdir -p mnt_boot mnt_root; \
		sudo mount $${LOOP_DEV}p1 mnt_boot; \
		sudo mkdir -p mnt_boot/EFI/BOOT; \
		sudo mkdir -p mnt_boot/boot; \
		sudo cp limine/BOOTX64.EFI mnt_boot/EFI/BOOT/; \
		sudo cp bin/kernel.elf mnt_boot/boot/; \
		sudo cp limine.conf mnt_boot/boot/; \
		sudo mount $${LOOP_DEV}p2 mnt_root; \
		sudo mkdir -p mnt_root/{bin,boot,dev,etc,home,lib,mnt,opt,proc,root,sbin,srv,sys,tmp,usr,var}; \
		sudo mkdir -p mnt_root/usr/{bin,lib,local,sbin,share}; \
		sudo mkdir -p mnt_root/var/{log,tmp}; \
		sudo umount mnt_boot; \
		sudo umount mnt_root; \
		sudo losetup -d $${LOOP_DEV}; \
		rm -rf mnt_boot mnt_root; \
		'
		@echo "✅ Release disk image created: $(DISK_IMAGE)"


# build ISO (legacy support)
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
	@rm -rf bin $(KERNEL_OBJ) isobuilds/*.iso isobuilds/*.img limine isodirs mnt_boot mnt_root
	@echo "✅ Clean complete"

clean_no_iso:
	@echo "🧹 Cleaning build artifacts (keeping ISOs and disk images)..."
	@rm -rf bin $(KERNEL_OBJ) limine isodirs mnt_boot mnt_root
	@echo "✅ Clean complete"

# separate build targets
debug: isobuilds/WilsonD.iso
release: isobuilds/Wilson.iso

# run in qemu with ISO (legacy)
run-debug: debug
	@echo "🚀 Launching QEMU (debug mode with ISO)..."
	@qemu-system-x86_64 -M q35 -cdrom isobuilds/WilsonD.iso $(QEMUFLAGS)

run-release: release
	@echo "🚀 Launching QEMU (release mode with ISO)..."
	@qemu-system-x86_64 -M q35 -cdrom isobuilds/Wilson.iso $(QEMUFLAGS)

# run in qemu with disk image
run-disk-debug: disk-debug
	@echo "🚀 Launching QEMU (debug mode with disk image)..."
	@qemu-system-x86_64 -M q35 -drive file=$(DISK_IMAGE_DEBUG),format=raw $(QEMUFLAGS)

run-disk-release: disk-release
	@echo "🚀 Launching QEMU (release mode with disk image)..."
	@qemu-system-x86_64 -M q35 -drive file=$(DISK_IMAGE),format=raw $(QEMUFLAGS)
