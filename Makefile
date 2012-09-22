.PHONY: make-system clean tools clean-tools

CC := ./tools/bin/i586-elf-gcc
LD := ./tools/bin/i586-elf-ld
AS := ./tools/bin/nasm

CFLAGS  := -std=c99 -pedantic -Wall -Wextra -MMD -Werror
CFLAGS  += -O3 -fomit-frame-pointer
CFLAGS	+= -mno-sse -mno-mmx -mno-sse2 -mno-sse3
CFLAGS  += -I src/include
ASFLAGS := -felf32

all: make-all

##############################################################################
#
# Set up cross-compiler and build environment
#
##############################################################################

tools: clean-tools
	@ echo ""
	@ echo "Starting automatic cross-toolchain build. This may take a while depending on"
	@ echo "your Internet connection and processor speed. I would take this opportunity to"
	@ echo "mix up a batch of muffins if I were you."
	@ echo ""
	@ echo "The cross-toolchain will be completely contained within the tools/ directory. No"
	@ echo "superuser access is required."
	@ echo ""
	@ mkdir -p tools
	@ echo " WGET	tools/binutils-2.22.tar.bz2"
	@ wget -P tools http://ftp.gnu.org/gnu/binutils/binutils-2.22.tar.bz2
	@ echo " WGET	tools/gcc-4.7.0.tar.bz2"
	@ wget -P tools http://ftp.gnu.org/gnu/gcc/gcc-4.7.0/gcc-4.7.0.tar.bz2
	@ echo " WGET	tools/nasm-2.09.10.tar.bz2"
	@ wget -P tools http://www.nasm.us/pub/nasm/releasebuilds/2.09.10/nasm-2.09.10.tar.bz2
	@ echo " UNTAR	tools/binutils-2.22.tar.bz2"
	@ tar -xf tools/binutils-2.22.tar.bz2 -C tools
	@ rm tools/binutils-2.22.tar.bz2
	@ echo " UNTAR	tools/gcc-4.7.0.tar.bz2"
	@ tar -xf tools/gcc-4.7.0.tar.bz2 -C tools
	@ rm tools/gcc-4.7.0.tar.bz2
	@ echo " UNTAR	tools/nasm-2.09.10.tar.bz2"
	@ tar -xf tools/nasm-2.09.10.tar.bz2 -C tools
	@ rm tools/nasm-2.09.10.tar.bz2
	@ mkdir -p tools/build-binutils
	@ echo ""
	@ echo " CONFIGURING BINUTILS"
	@ echo ""
	@ cd tools/build-binutils && ../binutils-2.22/configure \
		--target=i586-elf --prefix=$(PWD)/tools --disable-nls
	@ echo ""
	@ echo " COMPILING BINUTILS"
	@ echo ""
	@ make -C tools/build-binutils all
	@ echo ""
	@ echo " INSTALLING BINUTILS"
	@ echo ""
	@ make -C tools/build-binutils install
	@ echo ""
	@ echo " CLEAN	tools/build-binutils tools/binutils-2.22"
	@ rm -rf tools/build-binutils tools/binutils-2.22
	@ mkdir -p tools/build-gcc
	@ echo ""
	@ echo " CONFIGURING GCC"
	@ echo ""
	@ export PATH=$PATH:$(PWD)/tools/bin
	@ cd tools/build-gcc && ../gcc-4.7.0/configure \
		--target=i586-elf --prefix=$(PWD)/tools --disable-nls \
		--enable-languages=c --without-headers
	@ echo ""
	@ echo " COMPILING GCC"
	@ echo ""
	@ make -C tools/build-gcc all-gcc
	@ echo ""
	@ echo " INSTALLING GCC"
	@ echo ""
	@ make -C tools/build-gcc install-gcc
	@ echo ""
	@ echo " CLEAN	tools/build-gcc tools/gcc-4.7.0"
	@ rm -rf tools/build-gcc tools/gcc-4.7.0
	@ echo ""
	@ echo " CONFIGURING NASM"
	@ echo ""
	@ cd tools/nasm-2.09.10 && ./configure --prefix=$(PWD)/tools
	@ echo ""
	@ echo " COMPILING NASM"
	@ echo ""
	@ make -C tools/nasm-2.09.10
	@ echo ""
	@ echo " INSTALLING NASM"
	@ echo ""
	@ make -C tools/nasm-2.09.10 install
	@ echo ""
	@ echo " CLEAN	tools/nasm-2.09.10"
	@ rm -rf tools/nasm-2.09.10
	
clean-tools:
	@ echo " CLEAN	tools/"
	@ - rm -r tools

##############################################################################
#
# Build components and install them to the system root
#
##############################################################################

# build and install all packages into root/
make-all: skeleton make-kernel make-system

.PHONY: skeleton make-kernel

# re-initialize root/ with a blank skeleton structure
skeleton:
	@ echo " CLEAN	root/"
	@ - rm -r root
	@ echo " CLONE	skel/ -> root/"
	@ cp -r skel root

######################################
#
# Kernel
#
######################################

make-kernel: build-kernel install-kernel

.PHONY: build-kernel install-kernel

build-kernel: build/kernel/kernel

kernel_OBJECTS := $(patsubst src/%.s,build/%.o,$(shell find src/kernel -name "*.s"))
kernel_OBJECTS += $(patsubst src/%.c,build/%.o,$(shell find src/kernel -name "*.c"))

build/kernel/kernel: $(kernel_OBJECTS) src/kernel/kernel.ld
	@ echo " LD	"$@ $(kernel_OBJECTS)
	@ $(LD) -o $@ $(kernel_OBJECTS) -Tsrc/kernel/kernel.ld

build/kernel/%.o: src/kernel/%.s
	@ mkdir -p `dirname $@`
	@ echo " AS	"$<
	@ $(AS) $(ASFLAGS) $< -o $@

build/kernel/%.o: src/kernel/%.c
	@ mkdir -p `dirname $@`
	@ echo " CC	"$<
	@ $(CC) $(CFLAGS) -ffreestanding -c $< -o $@

install-kernel: build-kernel
	@ echo " CP	"build/kernel/kernel -> root/boot/kernel
	@ cp build/kernel/kernel root/boot/kernel

-include $(patsubst %.o,%.d,$(kernel_OBJECTS))

######################################
#
# System / Driver Layer
#
######################################

make-system: build-system install-system

.PHONY: build-system install-system

build-system: build/system/system

system_OBJECTS := $(patsubst src/%.s,build/%.o,$(shell find src/system -name "*.s"))
system_OBJECTS += $(patsubst src/%.c,build/%.o,$(shell find src/system -name "*.c"))

build/system/system: $(system_OBJECTS) src/system/system.ld
	@ echo " LD	"$@ $(system_OBJECTS)
	@ $(LD) -o $@ $(system_OBJECTS) -Tsrc/system/system.ld

build/system/%.o: src/system/%.s
	@ mkdir -p `dirname $@`
	@ echo " AS	"$<
	@ $(AS) $(ASFLAGS) $< -o $@

build/system/%.o: src/system/%.c
	@ mkdir -p `dirname $@`
	@ echo " CC	"$<
	@ $(CC) $(CFLAGS) -Isrc/system/include -ffreestanding -c $< -o $@

install-system: build-system
	@ echo " CP	"build/system/system -> root/boot/system
	@ cp build/system/system root/boot/system

-include $(patsubst %.o,%.d,$(system_OBJECTS))

##############################################################################
#
# Create system images
#
##############################################################################

# create a new CD image in images/ from the contents of root/
cd-image: make-all
	@ mkdir -p images/
	@ echo " IMAGE	images/pinion.iso"
	@ mkisofs -R -b boot/grub/stage2_eltorito -no-emul-boot \
		-quiet -boot-load-size 4 -boot-info-table \
		-o images/pinion.iso root

# run the emulator to test the system
test: cd-image
	@ echo " TEST	"
	@ env test/run.sh

##############################################################################
#
# Clean up object files
#
##############################################################################

clean:
	@ echo " CLEAN	build/"
	@ - rm -r build
