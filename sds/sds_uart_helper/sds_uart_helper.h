#ifndef UART_HELPER_H_
#define UART_HELPER_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include "app_uart.h"


void uart_helper_init(void);
uint32_t set_baud(uint32_t baud_rate);

#endif
