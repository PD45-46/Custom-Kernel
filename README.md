# Custom-Kernel
Writing from scratch in C and Assembly.

### What is the GDT
The Global Descriptor Table (GDT) is a table in memory that defines memory segments, providing context to which segments are allowed to perform certain actions --- privilege rings. Ring 0 is called the kernel mode which has full hardware access and ring 3 is the user mode which has certain restrictions.cIn the modern day the GDT is mostly just a formality --- which must still be loaded correctly --- the CPU requires before the OS is able to do anything of use.  

### IDT and Interrupt Stubs 
The Interrupt Descriptor Table (IDT) is the CPU's lookup table for handing interrupts and exceptions. When anything unexpected happens --- divide 0, page fault, timer firing, keypress --- the CPU stops what it's doing and has to handle the interrupt type by look it up on the table. 
There are 256 possible entries. 0-31 are CPU exceptions, 32-47 are hardware interrupts, 48+ are for syscalls. 

Each entry of the table must point to an assembly stub because the CPU pushes specific things onto the stack when an interrupt fires and expects a specific ```iretq``` call after the interrupt. An example of an assembly stub is just a small ```.asm``` file containing a small set of instructions; the goal is to not write the entire system is assembly. 



