#![no_std]
#![no_main]
#![feature(custom_test_frameworks)]
#![reexport_test_harness_main = "test_main"]
#![test_runner(custom_kernel::test_runner)]

use core::panic::PanicInfo;
use custom_kernel::println;

#[unsafe(no_mangle)]
pub extern "C" fn _start() -> ! {
    test_main();

    loop {}
}

fn test_runner(tests: &[&dyn Fn()]) {
    unimplemented!();
}

#[panic_handler]
fn panic(info: &PanicInfo) -> ! {
    custom_kernel::test_panic_handler(info);
}

#[test_case]
fn test_println() {
    println!("test_println output");
}