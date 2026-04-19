#include "ktest.h"
#include "../src/drivers/serial.h"

int ktest_current_failed = 0; 
int ktest_pass_count = 0; 
int ktest_fail_count; 

void ktest_run(const char *suite_name, ktest_t *tests, int count) { 
    serial_print("\n=== TEST SUITE: "); 
    serial_print(suite_name); 
    serial_print(" ===\n"); 

    for(int i = 0; i < count; i++) { 
        ktest_current_failed = 0; 
        serial_print("  "); 
        serial_print(tests[i].name); 
        serial_print("... "); 

        tests[i].fn(); 

        if(ktest_current_failed) { 
            serial_print("[FAIL]\n"); 
            ktest_fail_count++; 
        } else { 
            serial_print("[PASS]\n"); 
            ktest_pass_count++; 
        }
    }
    serial_print("--- ");
    serial_print_int(ktest_pass_count);
    serial_print(" passed, ");
    serial_print_int(ktest_fail_count);
    serial_print(" failed ---\n\n");
}

