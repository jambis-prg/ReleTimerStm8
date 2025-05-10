#ifndef __SSD1306_H
#define __SSD1306_H

#include <stm8s.h>

#define INVERT_SECONDS 0x01
#define INVERT_MINUTES 0x02
#define INVERT_HOURS 0x04

void ssd1306_init(void);
void ssd1306_power(bool on);
void ssd1306_update_timer(uint32_t timer, uint8_t invert);

#endif