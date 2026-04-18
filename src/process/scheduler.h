#pragma once
#include "process.h"

void scheduler_init(void); 
void scheduler_add(process_t *proc); 
void scheduler_tick(void); 
process_t *scheduler_current(void); 
void context_switch(process_t *current, process_t *next); 
void scheduler_start(void); 