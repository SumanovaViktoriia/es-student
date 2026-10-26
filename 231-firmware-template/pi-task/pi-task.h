#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    PI_STATE_NOT_STARTED,
    PI_STATE_RUNNING,
    PI_STATE_DONE
} pi_state_t;

void pi_task_init(void);
bool pi_task_start(uint32_t terms);
void pi_task_handle(void);
pi_state_t pi_task_get_state(void);


bool pi_task_get_result(double *value, uint64_t *duration_ms);


double pi_task_get_value(void);
uint64_t pi_task_get_duration_ms(void);
uint32_t pi_task_get_terms(void);