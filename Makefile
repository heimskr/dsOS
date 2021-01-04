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
SPECIAL   := ./src/arch/x86_64/Interrupts.cpp

OBJS       = $(patsubst %.S,%.o,$(ASSEMBLED)) $(patsubst %.cpp,%.o,$(COMPILED))
ISO_FILE  := kernel.iso

all: kernel

define COMPILED_TEMPLATE
$(patsubst %.cpp,%.o,$(1)): $(1)
	$(CC) $(CFLAGS) -DARCHX86_64 -c $$< -o $$@
endef

define ASSEMBLED_TEMPLATE
$(patsubst %.S,%.o,$(1)): $(1)
	$(AS) $(ASFLAGS) -DARCHX86_64 -c $$< -o $$@
endef

$(foreach fname,$(filter-out $(SPECIAL),$(COMPILED)),$(eval $(call COMPILED_TEMPLATE,$(fname))))
$(foreach fname,$(ASSEMBLED),$(eval $(call ASSEMBLED_TEMPLATE,$(fname))))

src/arch/x86_64/Interrupts.o: src/arch/x86_64/Interrupts.cpp include/arch/x86_64/Interrupts.h
	$(CC) $(CFLAGS) -mgeneral-regs-only -DARCHX86_64 -c $< -o $@

kernel: $(OBJS) kernel.ld Makefile
	$(CC) -z max-page-size=0x1000 $(CFLAGS) -no-pie -Wl,--build-id=none -T kernel.ld -o $@ $(OBJS)

iso: $(ISO_FILE)

$(ISO_FILE): kernel
	mkdir -p iso/boot/grub
	cp grub.cfg iso/boot/grub/
	cp kernel iso/boot/
	$(GRUB)-mkrescue -o $(ISO_FILE) iso

run: $(ISO_FILE)
	qemu-system-x86_64 -s -cdrom $(ISO_FILE) -enable-kvm -cpu host -smp cpus=1,cores=12,maxcpus=12 -serial stdio -m 8G $(QEMU_EXTRA)

clean:
	rm -rf *.o **/*.o `find src -iname "*.o"` kernel iso kernel.iso

$(OBJS): Makefile

DEPFILE  = .dep
DEPTOKEN = "\# MAKEDEPENDS"
DEPFLAGS = -f $(DEPFILE) -s $(DEPTOKEN)

depend:
	@ echo $(DEPTOKEN) > $(DEPFILE)
	makedepend $(DEPFLAGS) -- $(CC) $(CFLAGS) -- $(SOURCES) 2>/dev/null
	@ rm $(DEPFILE).bak

sinclude $(DEPFILE)
