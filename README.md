# Custom-Kernel

Writing from scratch in C and Assembly.

## Early Kernel Bootstrapping and Long Mode Initialisation
### 1. Module Overview

The target directory: ``` src/boot ```.  
The ```boot.asm``` module serves as the primary entry point for my custom x86_64 kernel. Since the [__Multiboot2 specification__](https://www.gnu.org/software/grub/manual/multiboot2/html_node/Machine-state.html) requires the bootloader to transfer control to a 32-bit operating system in Protected Mode with paging disabled, while leaving critical execution state such as the stack pointer and descriptor tables undefined, an assembly bootstrap stub is required to establish the minimal execution environment needed before higher-level kernel code can execute.  
Its primary duties include:  

1. Validating the image via a Multiboot2 compliant signature.
2. Configuring a temporary early-boot stack.  
3. Constructing a basic 4-level paging hierarchy to map early physical memory.  
4. Transitioning the CPU control state from 32-bit Protected Mode into 64-bit Long Mode.  
5. Initialising the execution segment registers and aligning the stack to conform to the System V AMD64 ABI before jumping into C code — ```kernel_main()``` in ```kernel.c```.  

### 2. Multiboot2 Header Verification

```
MAGIC    equ 0xe85250d6
ARCH     equ 0           ; x86
HDRLEN   equ header_end - header_start
CHECKSUM equ -(MAGIC + ARCH + HDRLEN)
```

__Magic Number__ ```0xe85250d6``` identifies this binary as Multiboot2 executable.  
__Architecture__ ```0``` signals to the bootloader that the kernel expects to be initialised in i386 32-bit Protected Mode.  
__Header Length__ and __Checksum__ formally validate the structural integrity of the block, by ensuring that the bootloader fields sum to exactly zero.  
[__Reference for values__](https://www.gnu.org/software/grub/manual/multiboot2/html_node/Header-magic-fields.html)

### 3. Early Page Table Construction and Identity Mapping

Before 64-bit execution can be toggled via control registers — ```cr0-cr6``` — the x86_64 architecture requires paging to be active, and a valid 4-level transition tree must be present in the CPU, [__source__](https://stackoverflow.com/questions/77665641/x86-64-running-in-long-mode-64-bit-submode). ```boot.asm``` reservers three contiguous 4096-byte blocks within uninitalised data storage (```.bss```) to build the initial page layout.  

``` 
section .bss
align 4096
pml4_table: resb 4096
pdpt_table: resb 4096
pd_table:   resb 4096
align 16 
stack_bottom: resb 16384
stack_top:
```

1. ```pml4_table```: Page Map Level 4. The first entry is pointed directly to the base physical address of ```pdpt_table```, combined with control flags ```0b11``` — __Present__ and __Read/Write__.
2. ```pdpt_table```: Page Directory Pointer Table. The first entry is pointed to the physical address of ```pd_table```, also flagged with ```0b11```.  
3. ```pd_table```: Page Directory. Rather than managing the fourth tier of mapping (Page Tables) for granular 4KB allocations, the kernel establishes optimised 2MB Huge Pages by setting Page Size (bit 7) directly within the Page Directory entries. 

Setup loop maps the first 16MB of physical memory: 

```
; Link PD entry 0 to a 2MB Huge Page (starts at address 0)
    mov ecx, 0          ; index
    mov ebx, 0          ; physical addr

    .map_loop:
        mov eax, ebx
        or eax, 0b10000011
        mov [pd_table + ecx*8], eax

        add ebx, 0x200000    ; next 2MB
        inc ecx
        cmp ecx, 8           ; map 16MB
        jl .map_loop
```

Identity mapping is vital as the physical location of the code currently executing the memory must correspond exactly to its virtual block. If the virtual address doesn't map directly to the physical address, the instruction pointer would fetch arbitrary instructions when paging is enabled, resulting in a CPU crash.  

### 4. Hardware Long Mode Execution 

The steps to toggling the processor from 32-bit architecture to 64-bit follows:  

1. The physical starting address of ```pml4_table``` is loaded into the translation base (```cr3```) register. 
2. Bit 5 of the physical address translation (```cr4```) register is flagged. Physical address translation changes the width of the page table records from 32-bits to 64-bits to accommodate for the wider physical address pointers.  
3. The kernel queries the Extended Feature Enable Register (EFER) at address `0xc0000080` via the `rdmsr` instruction. It sets bit 8 (Long Mode Enable), and writes it back to the CPU execution core using `wrmsr`.  
4. Bit 31 of `cr0` register (Paging - PG) is flagged. Soon after, the Memory Management Unit begins translating all the memory operations using the loaded PML4 tree.  
5. Although paging is active, the internal execution segment cache is still operating user 32-bit limits. To resolve this, a preliminary Global Descriptor Table layout `gdt64.layout` is loaded via `lgdt`, followed by an explicit long jump `jmp gdt64.code:long_mode_start`. This flushes the processor prefetch queue and locks the instruction decoder into native 64-bit execution.  

### 5. Execution Environment Sanitation and ABI Handover  

These are just cleanup measures that are taken to ensure platform predictability. This entails clearing out segment registers (ie. setting them to `0`). Also, stack pointer realignment, where `rsp` is moved to `stack_top` and the lowest 4 bits are cleared to align it with the new architecture. And finally, permanent kernel handoff via `call kernel_main` which is in `kernel.c`.  

## CPU Architecture Configuration and Segment Lifecycle Management  

### 1. Architectural Overview  

Note that though `boot.asm` establishes a temporary state to execute 64-bit code, it doesn't configure any kind of finalised execution environment. This module `src/cpu` establishes important hierarchies including User and Kernel spaces, setting up a Hardware Task Management System for safe transitions, and managing interruptions (IDT/ISR) to trap processor exceptions, timer ticks, and hardware interrupts.  

### 2. Segment Control and Ring Isolation (Global Descriptor Table)

![GDT Table - https://wiki.osdev.org/Global_Descriptor_Table](assets/gdt_table.png)
__Note that the 64-bit System Segment Descriptor can be represented as a C struct:__

```
typedef struct __attribute__((packed)) {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_mid;
    uint8_t type;
    uint8_t limit_high_flags;
    uint8_t base_high;
    uint32_t base_upper;
    uint32_t reserved;
} tss_descriptor_t;
```

The __Base Address__ represents where the Task State Segment (TSS) is located in memory. Note that the address is split into four sections: `base_low`, `base_mid`, `base_high`, and `base_upper` — all adding up to a 64-bit value.  
The __Limit__ is the size of the TSS descriptor. Follows a similar seperation pattern as __Base Address__ but instead this will add up to a total of 20 bits. Notice that the last 4 bits of __Limit__ is combined with the __Flag__ bits, this is done as to avoid C bitfields and their related compiler uncertainty issues — endianness and bitfield ordering.  
The __Type__ bits (Access Byte) are used  

### TODO ^ 


Loading a new GDT layout into memory via `lgdt` doesn't immediately update the CPU's internal cached segment states. The kernel executes `gdt_flush.asm` to enforce those boundaries. This process simply just loads the GDT pointer, overwrites the data selectors with Ring 0 data token `0x10`, and then those code segments are reloaded with the proper privileges. 

### 3. Privilege Ring Escalation Security (Task State Segment)

In 64-bit mode, the Task State Segment (TSS) hardware task-switching features are deprecated. Instead, the TSS serves to define the target stack pointer during unprivileged (user) ring transition.  
When a Ring 3 user process triggers a hardware interrupt, exception, or system call (syscall) instruction, the CPU must immediately leave the (untrusted) user stack and jump to a secure kernel-allocated stack area to execute the associated handler.  
A more technical perspective: 
- The kernel populates `tss.rsp0` field with the physicall address of the executing process's private kernel stack frame. 
- When an exception or syscall triggers a transition into Ring 0, the CPU's execution unit reads `tss.rsp0` from the active task block, automatically swaps the stack pointer `rsp` to the safe address, and he user's original stack parameters `ss` `rsp` are pushed onto it for a clean return later.  

### 4. Interrupt Routing and Exception De-multiplexing (Interrupt Descriptor Table)  

The Interrupt Descriptor Table is what the CPU uses to located the correct handler routine when an exception or hardware interrupt fires. Each of the 256 possible interrupt vectors maps to a unique gate descriptor that encodes the handler's address, privilege level, and gate type:  

```
typedef struct __attribute__((packed)) { 
    uint16_t offset_low; 
    uint16_t segment_selector; 
    uint8_t  ist; 
    uint8_t  type_attr;
    uint16_t offset_mid; 
    uint32_t offset_high; 
    uint32_t zero; 
} idt_entry_t; 
```
The `type_attr` field controls the gate type — `0xE` for an interrupt gate, which automatically clears RLFLAGS. ... __TODO__ 

## Physical Memory Management 
### 1. Module Overview 
The target directory: `src/memory/pmm.c`  

The __Physical Memory Manager__ is the lowest-level allocator in the kernel. Its only job is to track which 4KB physical frames of RAM are free and handing them out on demand. Every higher-level subsystem — the VMM, the heap, the ELF loader — ultimately calls into the PMM to obtain raw physical memory.  

### 2. Allocator Design  
The PMM uses a __bitmap allocator__: a flat array of bits where each bit corresponds to one 4KB physical frame — where `0` is free, and `1` means it is in use. 

```
static uint8_t bitmap[BITMAP_SIZE]; 

void *pmm_alloc(void) { 
    for(uint32_t i = 0; i < TOTAL_FRAMES; i++) { 
        if(!bitmap_test(i)) { 
            bitmap_set(i); 
            free_frame_count--; 
            return (void *)(uintptr_t)(i * PAGE_SIZE); 
        }
    }
    return NULL; 
}
```

### 3. Why Not Use a More Sophisticated Allocator? 
A buddy system or slab allocator would reduce memory fragmentation for mixed-sized allocations, but the PMM only ever hands out 4KB frames in this system. All higher level fragmentation is handled by the __Virtual Memory Manager__ and the kernel heap.  

## Virtual Memory Management: TODO ADD Page Map Level diagram 
### 1. Module Overview 
The target directory: `src/memory/vmm.c` 

The __Virtual Memory Manager__ owns the 4-level page table hierarchy at runtime. It provides my kernel with the ability to create isolated address spaces for user programs, install arbitrary virtual-to-physical mappings, and switch between address spaces on context switches.  

### 2. Page Table Traversal 
`vmm_map()` walks the live PML4 tree (read from register `cr3`) four levels deep, allocating intermediate tables using `pmm_alloc()` whenever an entry is absent, and installs the final 4KB mapping at PT level. 

```
void vmm_map(uint64_t virt, uint64_t phys, uint64_t flags) { 
    uint64_t cur_cr3; 
    asm volatile("mov %%cr3, %0" : "=r"(cur_cr3)); 
    uint64_t *pml4 = (uint64_t *)ENTRY_ADDR(cur_cr3); 

    uint64_t *pdpt = get_or_create_table(&pml4[PML4_INDEX(virt)], PTE_WRITABLE | PTE_USER);
    uint64_t *pd = get_or_create_table(&pdpt[PDPT_INDEX(virt)], PTE_WRITABLE | PTE_USER); 
    uint64_t *pt = get_or_create_table(&pd[PD_INDEX(virt)], PTE_WRITABLE | PTE_USER);
    pt[PT_INDEX(virt)] = phys | flags | PTE_PRESENT; 

    asm volatile("invlpg (%0)" : : "r"(virt) : "memory");
}
```

`vmm_map_in()` temporarily switches `cr3` to a target address space, calls `vmm_map()`, then restores the original `cr3`. This is used during process creation to install mappings into a process's private page table before it scheduled. 

### 3. Address Space Isolation 
`vmm_create_address_space()` allocates a fresh PML4 frame and copies all 512 entries from the current kernel PML4 into it, making each present entry as `PTE_USER`. This gives every user process a private lower-half address space while sharing the kernel's upper-half mappings — ensuring that the kernel remains reachable after syscall without switching to `cr3`.  

### 4. Identity Mapping Extension 
GRUB's initial page tables only identity-map a small region of physical RAM. Once the kernel embeds the full initrd (containing `doom.elf` and `doom1.wad`, totalling about 15MB), PMM frame allocations land above GRUB's mapped region. `vmm_init()` extends GRUB's existing Page Directory to cover the full first 1GB via 512 × 2MB huge pages, ensuring that every physical frame returned by `pmm_alloc()` is directly accessible as a virtual address throughout kernel execution.

## Kernel Heap 
### 1. Memory Allocation Overview 
The target directory: `src/memory/heap.c`  

The kernel heap provides `kmalloc`, `kfree`, and `kcalloc`. It sits above the PMM — obtaining large contiguous regions from `pmm_alloc()` and subdividing them into various sizes. I won't be going into too much detail about dynamic memory allocation, as if you're reading this, there is a very high chance you already know how most of it works under the hood.  

## Process Management 
### 1. Subsystem Overview 
The target directory: `src/process/`  

This subsystem defines __process abstraction__ — the data structure representing an execution context, the logic for creating kernel and user programs, and the assembly trampolines that perform ring transitions and context switches. _Note that this section is also has high dependency on syscalls._  

### 2. The Process Descriptor  
Every process is made up of the struct:  
```
typedef struct process { 
    uint32_t        pid; 
    process_state_t state; 
    cpu_state_t     context; 
    uint64_t        kernel_stack; 
    uint64_t        user_stack; 
    uint64_t        page_table;
    uint64_t        wake_tick;
    wait_reason_t   wait_reason; 
    struct process  *next; 
    uint64_t        heap_start; 
    uint64_t        heap_end; 
    uint8_t         use_linux_abi; 
} process_t; 
```
The `page_table` field's offset within the struct is verified by a `_Static_assert` and referenced directly in `context.asm` during context switches — keeping the Assembly and C definitions in synch at compile time.  

### 3. Kernel and User Processes 
__Kernel processes__ (`process_create`) run entirely in Ring 0 and share the kernel's page table (`page_table = 0`). They're used for internal background tasks.  
__User processes__ (`process_create_user`, `process_create_elf`) run entirely in Ring 3 with a private address space. Their creation involves:  
1. Allocating a private PML4 via `vmm_create_address_space`. 
2. Mapping a 128KB user stack (32 pages) below `USER_STACK_VIRT = 0x8000040000`.  
3. Installing the entry point (either a kernel function mapped at `USER_CODE_VIRT`, or ELF segments loaded from the ramdisk).  
4. Setting up the kernel stack with a fake initial frame so the `trampoline` function can perform the first Ring 3 entry via `iretq`.  

### 4. Ring Transition Trampolines 
The first time a process is scheduled, the context switch `ret`s into `process_trampoline_fn` (in `process_asm.asm`). The trampoline builds a synthetic `iretq` frame on the kernel stack:  
```
; Stack layout before iretq:
; [user SS]  [user RSP]  [RFLAGS]  [user CS]  [user RIP]
push USER_SS
push [user_rsp from stack]
push 0x202          ; RFLAGS: interrupts enabled
push USER_CS
push [entry_rip from stack]
iretq               ; drops into Ring 3
```
This separates first-run intialisation from stead-state context switching, which only ever saves and restores the the general-purpose register file. 
_Note that the extract here is a simplified version of what is in `process_asm.asm`_.  


## Process Scheduler  
### 1. Module Overview 
The target directory: `src/process/scheduler.c`  

The system uses round-robin scheduling over a singly linked list of `process_t` nodes. The timer IRQ calls `scheduler_tick()` to preempt the current process and select the next runnable one.  

### 2. Context Switching  
`context_switch` (in `context.asm`) saves the full GPR of the set outgoing process into its `context` field, swaps the `cr3` if the incoming process has a different page table, then restores the incoming process's registers state and returns — landing at whatever that process's last called `context_switch` from, or at its trampoline on first run.  
```
TODO ADD EXAMPLE CODE  
```

### 3. Sleep and Block 
Processes can voluntarily block via `SYS_SLEEP` (setting `wake_tick` and `wait_reason = WAIT_SLEEP`) or block on keyboard input (`WAIT_KEY`). On each scheduler tick, the scheduler scans the list and wakes any process whose `wait_tick <= timer_ticks()` or whose waited resource is available, transitioning it back to `PROCESS_READY`. 

## System Call Interface 
### Syscall Overview 
The target directory: `src/syscall/`

The syscall layer is the median that allows for Ring 3 processes to access privileged instructions (Ring 0). It uses the `syscall`/`sysret` fast-path instruction pair rather than software interrupts. 

### 2. Syscall Entry 
`syscall_init()` programs MSRs:  
- STAR — encodes the kernel CS/SS selectors loaded on syscall entry.  
- LSTAR — the 64-bit handler address (`syscall_entry` in `syscall_asm.asm`).  
- SFMASK — clears RFLAGS.IF on entry so the handler runs with interrupts enabled.  

`syscall_asm.asm` switches from the user stack to the per-CPU kernel stack (`kernel_stack_top`), saves the caller-saved registers, then calls `syscall_dispatch(num, arg1, arg2, arg3)` with arguments taken from `rax`, `rdi`, `rsi`, `rdx` respectively.  

### 3. Dual ABI Dispatch 
Because DOOM is linked against musl libc, which issues syscalls using __Linux ABI numbers__ (`openat=275`, `mmap=9`, `brk=12`, etc.), while native kernel processes use a custom compact numbering, the dispatcher checks a per-process flag:  
``` 
int64_t syscall_dispatch(uint64_t num, uint64_t arg1, uint64_t arg2, uint64_t arg3) { 

    process_t *curr = scheduler_current(); 
    if(curr && curr->use_linux_abi) { 
        return linux_syscall_dispatch(num, arg1, arg2, arg3); 
    }
    /* Custom ABI switch statement */
} 
```
`linux_syscall_dispatch()` maps the Linux numbers to the correct kernel handlers — including translating `mmap`/`brk` into `sys_brk`, `openat` into `ramdisk_open`, and `writev` into `sys_write` — while preserving the custom extension numbers (map_fb, setpalette, gettime) at non-conflicting slots. This design avoids modifying musl and requires zero changes to the doomgeneric source tree.  

### 4. Syscall Table 

| Number | Name | Notes |
| :--- | :--- | :--- |
| 0 | SYS_WRITE | VGA + serial output |
| 1 | SYS_EXIT | Mark process dead, reschedule |
| 4 | SYS_SLEEP | Block until tick |
| 6 | SYS_MAP_FB | Map VGA framebuffer into user VA |
| 13 | SYS_SBRK | Grow user heap |
| 14 | SYS_GETTIME | Timer ticks &rarr; milliseconds |
| 15 | SYS_SETPALETTE | Program VGA DAC registers |
| 16 | SYS_SET_FS_BASE | WRMSR for musl TLS |
| 177 | SYS_GETKEY | Raw PS/2 scancode + press/release |


## File System 
### 1. System Overview 
The target directory: `src/filesystem/`  

The file system layer consists of two components: a __ramdisk__ that provides read-only file access to an in-memory archive, and an __ELF loader__ that parses and maps executable images into the user address spaces.  

### 2. Ramdisk  
The initrd is a USTAR-format tar archive embedded directly into the kernel binary at link time through `objcopy`. The archive contains `hello.elf`, `doom.elf`, and `doom1.wad`. A small USTAR parsee walks the 512-byte headers to locate files by name:  
```
int ramdisk_open(const char *path) { 
    const uint8_t *p = _binary_initrd_tar_start;
    
    while(p + 512 <= _binary_initrd_tar_end && p[0] != '\0') { 
        const char *name = (const char *)p; 
        uint64_t size = octal((const char *)(p + 124), 11); 
        char type = (char)p[156]; 
    
        if((type == '0' || type == '\0') && path_match(name, path)) { 
            for(int i = 0; i < MAX_FDS; i++) { 
                if(!fds[i].in_use) { 
                    fds[i].data = p + 512; 
                    fds[i].size = size; 
                    fds[i].pos = 0; 
                    fds[i].in_use = 1; 
                    return i + 3; 
                }
            }
            return -1; 
        }
        p += 512 + ((size + 511) & ~511ULL); 
    }
    return -1; 
}
``` 
The fd table (`fs[]`) tracks up to `MAX_FDS` open files simultaneously, each with a `data` pointer into the tar image and a `pos` cursor for each sequential reads and seeks.  

### 3. ELF Loader 
`process_create_elf()` parses an ELF64 executable from the ramdisk and constructs a fully mapped user process:  

1. Validates the ELF magic, class (`ELFCLASS64`), and machine (`EM_X86_64`).
2. Iterates `PT_LOAD` program headers, allocating physical frames and mapping them into the process's address space at the specified virtual addresses.
3. Switches to the process's page table, zeroes each segment's memory region (handling BSS), then copies the file image from the ramdisk.
4. Sets `heap_start` to the page-aligned end of the last loaded segment so `sys_sbrk` has a correct starting boundary.
Builds the initial kernel stack frame for the trampoline and records the ELF entry point.

## User Space Library
### 1. Module Overview
The target directory: `src/user_programs/`

The library provides the thin syscall wrappers used by all user-space programs, and `malloc` implementation for programs that doesn't link against the full libc.  

### 2. Syscall Wrappers  
Each wrapper uses inline assembly to load the syscall number into `rax` and invoke the `syscall` instruction, following the kernel's ABI convention: 
```
void u_sleep(uint64_t ticks) {
    asm volatile("mov $4,%%rax\nsyscall\n"
        :: "D"(ticks)
        : "rax","rcx","rdx","rsi","r8","r9","r10","r11","memory");
}
```
The clobber list ensures the compiler doens't assume any caller-saved register survives the syscall boundary. 

### 3. User Malloc
`src/user_programs/malloc.c` mplements a free-list heap allocator driven by `u_sbrk()`. It is intentionally separate from the kernel heap (`heap.c`) and is compiled into both `hello.elf` and linked ahead of musl in `doom.elf` so that `malloc`/`calloc`/`free` symbols take precedence — bypassing musl's internal `mmap`-based allocator, which would otherwise issue raw Linux `mmap` syscalls that conflict with the kernel's custom ABI.

## DOOM Port 
### 1. Overview 
DOOM is ported via the [doomgeneric](https://github.com/ozkl/doomgeneric) abstraction layer, which reduces the the platform requirements to five functions: `DG_Init`, `DG_DrawFrame`, `DG_SetPalette`, `DG_GetKey`, and `DG_SleepMs`.

## 2. Platform Layer (`doom_platform.c`) 
Because `doom.elf` is linked against musl libc (`musl-gcc -static`), musl's internals issue raw Linux syscalls via the `syscall` instruction. Rather than modifying doomgeneric, the kernel's `linux_syscall_dispatch()` translates Linux ABI numbers to the corrrect kernel handlers.  
`doom_platform.c` provides overrides for `_exit`, `read`, `write`, `open`, `lseek`, and `close` at the C function level, handling the cases where the musl does call through public POSIX symbols rather than internal `__syscall`. 

### 3. Display (`doomgeneric_myos.c`)
DOOM renders into `I_VideoBuffer` — a 320x200 byte array in indexed-colour Mode 13h format. `DG_Init` maps the VGA framebuffer (physical 0xA0000) into the process's virtual address space via `SYS_MAP_FB`. `DG_DrawFrame` copies `I_VideoBuffer` directly into the mapped region. `DG_SetPalette` sends the 768-byte palette (256 × RGB) to the kernel via `SYS_SETPALETTE`, which programs the VGA DAC registers directly.  

### 4. Input (`doomgeneric_myos.c`)
The keyboard driver was extended to maintain a ring buffer of raw PS/2 make/break events alongside the existing character queue. A new `SYS_GET_RAW_KEY` syscall (177) dequeues one event per call, returning the scancode and a pressed/released flag. `DG_GetKey` translates PS/2 Set-1 scancodes to DOOM key constants and — critically — delivers both keydown and keyup events, which DOOM requires to correctly clear held-key state:
```
int DG_GetKey(int *pressed, unsigned char *doomKey) {
    raw_key_t evt;
    while (u_get_raw_key(&evt)) {
        unsigned char dk = sc_to_doom(evt.scancode);
        if (!dk) continue;
        *pressed = evt.pressed;
        *doomKey = dk;
        return 1;
    }
    return 0;
}
```

### 5. Result 
DOOM boots, loads `doom1.wad` from the ramdisk, renders at 320x200 in Mode 13h with correct palette, and accepts keyboard interrupts for movement, shooting, and interaction. The system runs entirely on my custom bare-metal kernel written from scratch — no host operating system involvement.  







## Running the Code
### Libs
```
sudo apt install build-essential nasm gcc-x86-64-linux-gnu \
    binutils-x86-64-linux-gnu musl-tools grub-pc-bin \
    grub-common xorriso qemu-system-x86 mtools
```
### Running the system
```
../Custom-Kernel$ make clean && make && make run
```
```
../Custom-Kernel$ make clean && make debug && make run
```
```
../Custom-Kernel$ make clean && make test && make run
```

### BUGS TO ADDRESS... 
> When you init the process scheduler with just a user process, the scheduler will bug out after the first print. The same doesn't happen when I start with a kernel process and append a user process after in the scheduler.
 
> Technically not a bug; change files to be cleaner, for example I have to keep adding header files all across different sections. I need to find a cleaner method for sharing information between different 'modules'. E.g Scheduler has ```scheduler_wake_key_waiter()``` which I then have to link in ```keyboard.c```.

> I need to init a kernel level process A before being able to also run process pong. Adding onto that, pong will run for a while in this state then it will just pause --- perhaps crashing. 

> Add comments to EVERYTHING

> Remove magic numbers in all code.

> Add testing for folders: user, filesystems, ... 

> Include more error resolution/print statements in oth vga & serial. 