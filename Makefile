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

USER_CFLAGS = -ffreestanding -fno-pic -fno-pie -static -O2 \
              -fno-stack-protector \
              -mno-sse -mno-sse2 -mno-avx \
              -nostdlib -nostartfiles \
              -Wall -Wextra

DOOM_SRCS = $(shell find user_programs/doom/game/doomgeneric -name '*.c' \
    ! -name 'doomgeneric_sdl.c'       \
    ! -name 'doomgeneric_xlib.c'      \
    ! -name 'doomgeneric_win.c'       \
    ! -name 'doomgeneric_allegro.c'   \
    ! -name 'doomgeneric_emscripten.c'\
    ! -name 'doomgeneric_sosox.c'     \
    ! -name 'doomgeneric_soso.c'      \
    ! -name 'doomgeneric_linuxvt.c'   \
    ! -name 'i_sdlmusic.c'            \
    ! -name 'i_sdlsound.c'            \
    ! -name 'i_allegromusic.c'        \
    ! -name 'i_allegrosound.c')

# ========================
# Build Rules
# ========================

INITRD_DIR = initrd
INITRD_TAR = initrd.tar
INITRD_OBJ = initrd.o

$(INITRD_OBJ): initrd/hello.elf initrd/doom.elf $(wildcard $(INITRD_DIR)/*)
	tar --format=ustar -cf $(INITRD_TAR) -C $(INITRD_DIR) .
	x86_64-linux-gnu-objcopy -I binary -O elf64-x86-64 \
		-B i386:x86-64 $(INITRD_TAR) $(INITRD_OBJ)

initrd/hello.elf: user_programs/hello/main.c \
                  src/user/malloc.c \
                  src/user/user_lib.c
	$(CC) $(USER_CFLAGS) -static -T user_linker.ld \
		-Wl,--build-id=none \
		user_programs/hello/main.c \
		src/user/malloc.c \
		src/user/user_lib.c \
		-o initrd/hello.elf

initrd/doom.elf: $(DOOM_SRCS) \
                 user_programs/doom/doomgeneric_myos.c \
                 user_programs/doom/doom_platform.c \
                 src/user/malloc.c \
                 src/user/user_lib.c
	musl-gcc -static -nostartfiles -O2 \
		-fno-stack-protector \
		-DDOOMGENERIC_RESX=320 \
		-DDOOMGENERIC_RESY=200 \
		-T user_linker.ld -Wl,--build-id=none \
		-Iuser_programs/doom/game/doomgeneric \
		-Isrc/user \
		$^ \
		-o initrd/doom.elf

all: $(ISO)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.asm
	$(AS) $(ASFLAGS) $< -o $@

$(KERNEL): $(OBJ) $(INITRD_OBJ)
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
		-ex "set logging file gdb.log" \
		-ex "set logging overwrite off" \
		-ex "set logging enabled on" \
		-ex "set pagination off" \
		-ex "set disassemble-next-line on" \
		-ex "layout asm"\
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
	rm -f $(INITRD_TAR) $(INITRD_OBJ)
	rm -f initrd/hello.elf 
	rm -f initrd/doom.elf 

.PHONY: all run clean test debug