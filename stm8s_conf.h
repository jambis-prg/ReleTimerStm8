#ifndef __STM8S_CONF_H
#define __STM8S_CONF_H

/* Incluir os headers dos perif�ricos que voc� vai usar */
#include "stm8s_gpio.h"
#include "stm8s_i2c.h"
#include "stm8s_exti.h"
#include "stm8s_tim1.h"
#include "stm8s_clk.h"
#include "stm8s103f3p.h"
// Adicione outros conforme necess�rio: uart, i2c, adc, etc.

/* Ativa o assert_param (opcional, �til para debug) */
// #define USE_FULL_ASSERT    (descomente para ativar as verifica��es em tempo de execu��o)

/* Macro de assert (ativa se USE_FULL_ASSERT estiver definida) */
#ifdef USE_FULL_ASSERT
#include <stdio.h>
#define assert_param(expr) ((expr) ? (void)0 : assert_failed((uint8_t *)__FILE__, __LINE__))
void assert_failed(uint8_t* file, uint32_t line);
#else
#define assert_param(expr) ((void)0)
#endif

#endif /* __STM8S_CONF_H */
