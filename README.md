# Custom-Kernel
Writing from scratch in C and Assembly.

## Early Kernel Bootstrapping and Long Mode Initialisation
### 1. Module Overview   
The target directory: ``` src/boot ```.  
The ```boot.asm``` module serves as the primary entry point for my custom x86_64 kernel. Since the [Multiboot2 specification](https://www.gnu.org/software/grub/manual/multiboot2/html_node/Machine-state.html) requires the bootloader to transfer control to a 32-bit operating system in Protected Mode with paging disabled, while leaving critical execution state such as the stack pointer and descriptor tables undefined, an assembly bootstrap stub is required to establish the minimal execution environment needed before higher-level kernel code can execute.  
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

### What is the GDT
The Global Descriptor Table (GDT) is a table in memory that defines memory segments, providing context to which segments are allowed to perform certain actions --- privilege rings. Ring 0 is called the kernel mode which has full hardware access and ring 3 is the user mode which has certain restrictions.cIn the modern day the GDT is mostly just a formality --- which must still be loaded correctly --- the CPU requires before the OS is able to do anything of use.  

### IDT and Interrupt Stubs 
The Interrupt Descriptor Table (IDT) is the CPU's lookup table for handing interrupts and exceptions. When anything unexpected happens --- divide 0, page fault, timer firing, keypress --- the CPU stops what it's doing and has to handle the interrupt type by look it up on the table. 
There are 256 possible entries. 0-31 are CPU exceptions, 32-47 are hardware interrupts, 48+ are for syscalls. 

Each entry of the table must point to an assembly stub because the CPU pushes specific things onto the stack when an interrupt fires and expects a specific ```iretq``` call after the interrupt. An example of an assembly stub is just a small ```.asm``` file containing a small set of instructions; the goal is to not write the entire system is assembly. 

### ROADMAP 
- Complete user space 
    - SYS_READ: blocking keyboard input to a user buffer. User process calls, kernel blocks the process until the key arrives, wakes the process and returns the char. 
    - SYS_SLEEP: yield for ```n``` ticks. Kernel sets a wake-up tick and then sets it as PROCESS_BLOCKED until time to start again. 
    - PROCESS_BLOCKED: ... 
    - SYS_SBRK: lets the user process call malloc. Implement sbrk as a syscall that bumps a per-process heap pointer and maps new pages. Then a minimal malloc/free in ulib builds on top of it. 
- Graphics 
    - Consider VESA/VBE for better colours/resolution. 
    - Double buffering: user process draws to a back buffer in its own memory, then syscall to blit it to the real framebuffer. Supposedly prevents tearing issues. 
- Storage and filesystem 
    - If i want to run and load games, i will need a file system.
- ELF loader
- Load up DOOM from doomgeneric and map all ports and elements.  

### BUGS TO ADDRESS
> When you init the process scheduler with just a user process, the scheduler will bug out after the first print. The same doesn't happen when I start with a kernel process and append a user process after in the scheduler.
 
> Technically not a bug; change files to be cleaner, for example I have to keep adding header files all across different sections. I need to find a cleaner method for sharing information between different 'modules'. E.g Scheduler has ```scheduler_wake_key_waiter()``` which I then have to link in ```keyboard.c```.

> I need to init a kernel level process A before being able to also run process pong. Adding onto that, pong will run for a while in this state then it will just pause --- perhaps crashing. 

> Add comments to EVERYTHING

> Remove magic numbers in all code.

> Add testing for folders: user, filesystems, ... 

> Include more error resolution/print statements in oth vga & serial. 