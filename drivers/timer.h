#ifndef TIMER_H
#define TIMER_H
#include <stdint.h>
void timer_init(uint32_t freq);
void timer_handler();
uint32_t timer_get_ticks();
void sleep_ms(uint32_t ms);
#endif