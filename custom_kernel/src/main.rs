#![no_std]
#![no_main]
#![feature(custom_test_frameworks)]
#![test_runner(custom_kernel::test_runner)]
#![reexport_test_harness_main = "test_main"]

use core::panic::PanicInfo;

mod vga_buffer;

#[unsafe(no_mangle)]
pub extern "C" fn _start() -> ! {
    println!("Hello World{}", "!");

    custom_kernel::init();

    // // page fault
    // unsafe {
    //     *(0xdeadbeef as *mut u8) = 42;
    // };
    // x86_64::instructions::interrupts::int3();

    fn stack_overflow() {
        stack_overflow();
    }

    stack_overflow(); 

    #[cfg(test)]
    test_main();

    println!("It didn't crash!");

    loop {}
}

/** This function is called on panic. */
#[panic_handler]
fn panic(info: &PanicInfo) -> ! {
    println!("{}", info);
    loop {}
}

#[test_case]
fn trivial_assertion() {
    assert_eq!(1, 1);
}