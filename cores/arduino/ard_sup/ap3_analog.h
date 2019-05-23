/*
Copyright (c) 2019 SparkFun Electronics

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef _AP3_ANALOG_H_
#define _AP3_ANALOG_H_

#include "Arduino.h"
#include "ap3_analog_types.h"

extern const ap3_analog_pad_map_elem_t ap3_analog_map[AP3_ANALOG_PADS];
extern const ap3_analog_channel_map_elem_t ap3_analog_channel_map[AP3_ANALOG_PADS];

// ADC Device Handle.
static void *g_ADCHandle;

ap3_err_t analogSetup(ap3_gpio_pad_t padNumber);

ap3_err_t ap3_analog_pad_funcsel(ap3_gpio_pad_t padNumber, uint8_t *funcsel);

ap3_err_t adc_config(ap3_gpio_pad_t padNumber);

uint16_t analogRead(uint8_t padNumber);
void analogReadResolution(uint8_t bits);

#endif // _AP3_ANALOG_H_