


#![no_std] // exclude std lib
#![no_main] // disable all rust-level entry points

use core::panic::PanicInfo;
mod vga_buffer;

// static HELLO: &[u8] = b"Hello World!";

#[panic_handler]
fn panic(info: &PanicInfo) -> ! {
    println!("{}", info);
    loop {}
}

#[unsafe(no_mangle)]
// -> ! is a divergent function that never returns to the caller
pub extern "C" fn _start() -> ! { // using C calling conventions
    println!("Hello World! {}", "Testing");

    loop {}
}



