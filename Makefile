# Based on code by Ring Zero and Lower: http://ringzeroandlower.com/2017/08/08/x86-64-kernel-boot.html

CC            = x86_64-elf-g++
AS            = x86_64-elf-g++
SHARED_FLAGS  = -fno-builtin -O0 -nostdlib -ffreestanding -g -Wall -Wextra -Iinclude -mno-red-zone\
                -mcmodel=kernel -fno-pie
CFLAGS        = $(SHARED_FLAGS) -fno-exceptions -fno-rtti
ASFLAGS       = $(SHARED_FLAGS) -Wa,--divide
GRUB         ?= grub
# QEMU_EXTRA  ?= -usb -device usb-kbd

ASSEMBLED := $(shell find asm/*.S)
COMPILED  := $(shell find . -name \*.cpp)
SOURCES    = $(ASSEMBLED) $(COMPILED)

OBJS       = $(patsubst %.S,%.o,$(ASSEMBLED)) $(patsubst %.cpp,%.o,$(COMPILED))
ISO_FILE  := kernel.iso

all: kernel

define COMPILED_TEMPLATE
$(patsubst %.cpp,%.o,$(1)): $(1)
	$(CC) $(CFLAGS) -c $$< -o $$@
endef

define ASSEMBLED_TEMPLATE
$(patsubst %.S,%.o,$(1)): $(1)
	$(AS) $(ASFLAGS) -c $$< -o $$@
endef

$(foreach fname,$(COMPILED),$(eval $(call COMPILED_TEMPLATE,$(fname))))
$(foreach fname,$(ASSEMBLED),$(eval $(call ASSEMBLED_TEMPLATE,$(fname))))

kernel: $(OBJS) kernel.ld Makefile
	$(CC) -z max-page-size=0x1000 $(CFLAGS) -no-pie -Wl,--build-id=none -T kernel.ld -o $@ $(OBJS)

iso: $(ISO_FILE)

$(ISO_FILE): kernel
	mkdir -p iso/boot/grub
	cp grub.cfg iso/boot/grub/
	cp kernel iso/boot/
	$(GRUB)-mkrescue -o $(ISO_FILE) iso

run: $(ISO_FILE)
	qemu-system-x86_64 -cdrom $(ISO_FILE) -enable-kvm -cpu host -smp cpus=1,cores=12,maxcpus=12 -serial stdio -m 8192M $(QEMU_EXTRA)

clean:
	rm -rf *.o **/*.o kernel iso kernel.iso

$(OBJS): Makefile

DEPFILE  = .dep
DEPTOKEN = "\# MAKEDEPENDS"
DEPFLAGS = -f $(DEPFILE) -s $(DEPTOKEN)

depend:
	@ echo $(DEPTOKEN) > $(DEPFILE)
	makedepend $(DEPFLAGS) -- $(CC) $(CFLAGS) -- $(SOURCES) 2>/dev/null
	@ rm $(DEPFILE).bak

sinclude $(DEPFILE)
