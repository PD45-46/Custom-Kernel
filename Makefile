CC = x86_64-linux-gnu-gcc
AS = nasm
LD = x86_64-linux-gnu-ld

CFLAGS = -m64 -ffreestanding -fno-stack-protector -fno-pic \
         -mcmodel=kernel -Wall -Wextra \
         -mno-sse -mno-sse2 -mno-mmx -mno-red-zone -mno-avx \
         -Iinclude -std=gnu11 -O2 -nostdlib

ASFLAGS = -f elf64
LDFLAGS = -m elf_x86_64 -T linker.ld -nostdlib

SRC_C   = $(shell find src -name '*.c')
SRC_ASM = $(shell find src -name '*.asm')
TEST_C  = $(shell find tests -name '*.c')

OBJ      = $(SRC_C:.c=.o) $(SRC_ASM:.asm=.o)
TEST_OBJ = $(TEST_C:.c=.o)

KERNEL = kernel.elf
ISO    = kernel.iso

# ========================
# Build Rules
# ========================

all: $(ISO)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.asm
	$(AS) $(ASFLAGS) $< -o $@

$(KERNEL): $(OBJ)
	$(LD) $(LDFLAGS) -o $@ $^

$(ISO): $(KERNEL)
	mkdir -p iso/boot/grub
	cp $(KERNEL) iso/boot/
	echo 'menuentry "kernel" { multiboot2 /boot/$(KERNEL) }' > iso/boot/grub/grub.cfg
	grub-mkrescue -o $@ iso

# ========================
# Test Build
# ========================

test: CFLAGS += -DRUN_TESTS
test: $(OBJ) $(TEST_OBJ)
	$(LD) $(LDFLAGS) -o $(KERNEL) $^
	$(MAKE) $(ISO)

# ========================
# Run / Debug
# ========================

run: $(ISO)
	qemu-system-x86_64 -cdrom $(ISO) -m 256M \
		-serial stdio -no-reboot -no-shutdown

run-headless: $(ISO)
	qemu-system-x86_64 -cdrom $(ISO) -m 256M \
		-serial stdio -display none \
		-no-reboot -no-shutdown

debug: $(ISO)
	qemu-system-x86_64 -cdrom $(ISO) -m 256M \
		-serial stdio -no-reboot -no-shutdown \
		-s -S &
	gdb $(KERNEL) \
		-ex "target remote :1234" \
		-ex "break kernel_main" \
		-ex "continue"

# ========================
# Clean
# ========================

clean:
	find src -name '*.o' -delete
	find tests -name '*.o' -delete
	rm -f $(KERNEL) $(ISO)
	rm -rf iso

.PHONY: all run clean test debug 