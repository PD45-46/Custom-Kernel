


#![no_std] // exclude std lib
#![no_main] // disable all rust-level entry points

use core::panic::PanicInfo;

static HELLO: &[u8] = b"Hello World!";

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    loop {}
}

#[unsafe(no_mangle)]
// -> ! is a divergent function that never returns to the caller
pub extern "C" fn _start() -> ! { // using C calling conventions
    let vga_buffer = 0xb8000 as *mut u8;
    for(i, &byte) in HELLO.iter().enumerate() {
        unsafe {
            *vga_buffer.offset(i as isize * 2) = byte;
            *vga_buffer.offset(i as isize * 2 + 1) = 0xb;
        }
    }

    loop {}
}
