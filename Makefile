# Based on code by Ring Zero and Lower: http://ringzeroandlower.com/2017/08/08/x86-64-kernel-boot.html

# CC           := clang -target x86_64-elf
# CPP          := clang++ -target x86_64-elf
CC           := x86_64-elf-gcc
CPP          := x86_64-elf-g++
AS           := x86_64-elf-g++
SHARED_FLAGS := -fno-builtin -O3 -nostdlib -ffreestanding -fno-pie -g -Wall -Wextra -Iinclude -mno-red-zone -mcmodel=kernel
CPPCFLAGS    := $(SHARED_FLAGS) -I./include/lib -I./musl/arch/x86_64 -I./musl/arch/generic -I./musl/obj/src/internal -I./musl/src/include -I./musl/src/internal -I./musl/obj/include -I./musl/include -D_GNU_SOURCE
CFLAGS       := $(CPPCFLAGS) -std=c11
CPPFLAGS     := $(CPPCFLAGS) -Iinclude/lib/libcxx -fno-exceptions -fno-rtti -std=c++2a -Drestrict=__restrict__
ASFLAGS      := $(SHARED_FLAGS) -Wa,--divide
GRUB         ?= grub
QEMU_MAIN    ?= -s -drive file=$(ISO_FILE),if=none,media=cdrom,format=raw,id=drive-sata-disk \
                   -device ahci,id=ahci -device ide-cd,drive=drive-sata-disk,id=sata-disk,bus=ahci.1,unit=0 -boot d -serial stdio -m 8G
# QEMU_EXTRA   ?= -drive id=disk,file=disk.img,if=none,format=raw -device ide-hd,drive=disk,bus=ahci.0
QEMU_EXTRA   ?= -drive format=raw,file=disk.img

QEMU_EXTRA   := $(QEMU_EXTRA) -no-shutdown -d cpu_reset,int
# QEMU_EXTRA   := $(QEMU_EXTRA) -device qemu-xhci -device usb-kbd
# QEMU_EXTRA   := $(QEMU_EXTRA) -device usb-mouse
QEMU_EXTRA   := $(QEMU_EXTRA) -enable-kvm
QEMU_EXTRA   := $(QEMU_EXTRA) -cpu host -smp cpus=1,cores=12,maxcpus=12
# QEMU_EXTRA   := $(QEMU_EXTRA) -M accel=tcg
# QEMU_EXTRA   := $(QEMU_EXTRA) -machine q35,kernel-irqchip=split,accel=kvm
# QEMU_EXTRA   := $(QEMU_EXTRA) -S
# QEMU_EXTRA   := $(QEMU_EXTRA) disk.img

ASSEMBLED := $(shell find asm/*.S)
CSRC      := $(shell find src -name \*.c)
CPPSRC    := $(shell find src -name \*.cpp)

SOURCES    = $(ASSEMBLED) $(CPPSRC) $(CSRC)
SPECIAL   := src/arch/x86_64/Interrupts.cpp
OBJS       = $(patsubst %.S,%.o,$(ASSEMBLED)) $(patsubst %.cpp,%.o,$(CPPSRC)) $(patsubst %.c,%.o,$(CSRC))
ISO_FILE  := kernel.iso
LIBS      := musl/lib/libc.a

all: kernel

define CPP_TEMPLATE
$(patsubst %.cpp,%.o,$(1)): $(1)
	$(CPP) $(CPPFLAGS) -DARCHX86_64 -c $$< -o $$@
endef

define C_TEMPLATE
$(patsubst %.c,%.o,$(1)): $(1)
	$(CC) $(CFLAGS) -DARCHX86_64 -c $$< -o $$@
endef

define ASSEMBLED_TEMPLATE
$(patsubst %.S,%.o,$(1)): $(1)
	$(AS) $(ASFLAGS) -DARCHX86_64 -c $$< -o $$@
endef

$(foreach fname,$(filter-out $(SPECIAL),$(CPPSRC)),$(eval $(call CPP_TEMPLATE,$(fname))))
$(foreach fname,$(CSRC),$(eval $(call C_TEMPLATE,$(fname))))
$(foreach fname,$(ASSEMBLED),$(eval $(call ASSEMBLED_TEMPLATE,$(fname))))

src/arch/x86_64/Interrupts.o: src/arch/x86_64/Interrupts.cpp include/arch/x86_64/Interrupts.h
	$(CPP) $(CPPFLAGS) -mgeneral-regs-only -DARCHX86_64 -c $< -o $@

kernel: $(OBJS) kernel.ld $(LIBS)
	x86_64-elf-g++ -z max-page-size=0x1000 $(CPPFLAGS) -Wl,--build-id=none -T kernel.ld -o $@ $(OBJS) $(LIBS)

musl/lib/libc.a:
	$(MAKE) -C musl

iso: $(ISO_FILE)

$(ISO_FILE): kernel
	mkdir -p iso/boot/grub
	cp grub.cfg iso/boot/grub/
	cp kernel iso/boot/
	$(GRUB)-mkrescue -o $(ISO_FILE) iso

run: $(ISO_FILE)
	qemu-system-x86_64 $(QEMU_MAIN) $(QEMU_EXTRA)

pipe: $(ISO_FILE)
	qemu-system-x86_64 $(QEMU_MAIN) $(QEMU_EXTRA) < pipe

clean:
	rm -rf *.o **/*.o `find src -iname "*.o"` kernel iso kernel.iso

destroy: clean
	rm -rf musl/obj

$(OBJS):

DEPFILE  = .dep
DEPTOKEN = "\# MAKEDEPENDS"
DEPFLAGS = -f $(DEPFILE) -s $(DEPTOKEN)

depend:
	@ echo $(DEPTOKEN) > $(DEPFILE)
	makedepend $(DEPFLAGS) -- $(CPP) $(CPPFLAGS) -- $(SOURCES) 2>/dev/null
	@ rm $(DEPFILE).bak

sinclude $(DEPFILE)

.PHONY: all run pipe clean destroy
