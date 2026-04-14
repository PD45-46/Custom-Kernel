#pragma once
#include "process.h"

void scheduler_init(); 
void scheduler_add(process *proc); 
void scheduler_tick(); 
process_t *scheduler_current(); 