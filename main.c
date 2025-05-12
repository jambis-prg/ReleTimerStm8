#include <stm8s.h>
#include "ssd1306.h"
#include "interruptions.h"

#define true 1
#define false 0

#define SW_DELAY_CURSOR 25
#define SW_DELAY_RUN 1000

#define DAY 86400
#define HOUR 3600
#define MINUTE 60
#define SECOND 1

bool running = false;
bool next_value = 0;

bool old_state = true;
uint32_t tDown = 0, tUp = 0;

bool gate_a = true;
bool gate_b = true;

bool update_lcd = false;

uint32_t timer_uint = 0, old_timer_uint = 0;
uint16_t timer_incrementer = 1;
uint8_t selected = INVERT_SECONDS;

bool reset = false;

uint16_t old_clock_time = 0;
uint32_t absolute_time = 0;

static uint32_t millis(void);
static void sw_handler(bool state);
static void rot_handler(void);
static void TIM1_Config(void);

static uint32_t millis()
{
		uint16_t current_time = TIM1_GetCounter();
		if (current_time < old_clock_time)
			absolute_time += 65536 - old_clock_time + current_time;
		else
			absolute_time += current_time - old_clock_time;
			
		old_clock_time = current_time;
    return absolute_time;
}

static void sw_handler(bool state)
{
		uint32_t delta;
		
    if (state && !old_state) tDown = millis();
    else if (!state && old_state) tUp = millis();

    delta = tUp - tDown;
    if (!state && old_state && delta > SW_DELAY_CURSOR)
    {
        if (delta > SW_DELAY_RUN && timer_uint != 0)
        {
            if (running) timer_uint = old_timer_uint;
            else old_timer_uint = timer_uint;

            running = !running;
            update_lcd = true;
						
						// Rele
						running ? GPIO_WriteHigh(GPIOD, GPIO_PIN_3) : GPIO_WriteLow(GPIOD, GPIO_PIN_3);
		
						// Buzzer
						GPIO_WriteLow(GPIOA, GPIO_PIN_3);
        }
        else if (running)
        {
            timer_uint = old_timer_uint;
            update_lcd = true;
            reset = true;
        }
        else
        {
            timer_incrementer = timer_incrementer == 3600 ? 1 : timer_incrementer * 60;
            update_lcd = true;
            selected = selected >= INVERT_HOURS ? INVERT_SECONDS : selected << 1;
        }
    }

    old_state = state;
}

static void rot_handler()
{
    if ((gate_a != (PD_IDR & GPIO_PIN_6)) && (PD_IDR & GPIO_PIN_6) == next_value)
    {
        gate_a = next_value;
        next_value = next_value ^ 1;

        if (gate_a != gate_b)
        {
            timer_uint = timer_uint + timer_incrementer;
            if (timer_uint >= DAY)
                timer_uint = DAY - 1;
            update_lcd = true;
        }
    }
    else if ((gate_b != (PD_IDR & GPIO_PIN_5)) && (PD_IDR & GPIO_PIN_5) == next_value)
    {
        gate_b = next_value;
        next_value = next_value ^ 1;

        if (gate_a != gate_b)
        {
            timer_uint -= timer_incrementer;
            // Overflow
            if (timer_uint >= DAY)
                timer_uint = 0;
            update_lcd = true;
        }
    }
}

@far @interrupt @svlreg void PD_IRQHandler()
{
		if((PD_IDR & GPIO_PIN_4) != old_state)
				sw_handler((PD_IDR & GPIO_PIN_4) == 0);
		else if (!running)
				rot_handler();
}

static void TIM1_Config()
{
	TIM1_DeInit();
	TIM1_TimeBaseInit(2000, TIM1_COUNTERMODE_UP, 65535, 0);
	TIM1_Cmd(ENABLE);
}

int main() 
{
		uint32_t start = millis();
    uint16_t accum = 0, beep_accum = 0;
		bool beeping = false;
		
		TIM1_Config();
	
		// Rele
		GPIO_DeInit(GPIOD);
		GPIO_Init(GPIOD, GPIO_PIN_3, GPIO_MODE_OUT_PP_LOW_FAST);
		GPIO_WriteLow(GPIOD, GPIO_PIN_3);
		
		// Buzzer
		GPIO_DeInit(GPIOA);
		GPIO_Init(GPIOA, GPIO_PIN_3, GPIO_MODE_OUT_PP_LOW_FAST);
		GPIO_WriteLow(GPIOA, GPIO_PIN_3);

		
		GPIO_Init(GPIOD, GPIO_PIN_4, GPIO_MODE_IN_PU_IT); // SW
		GPIO_Init(GPIOD, GPIO_PIN_5, GPIO_MODE_IN_PU_IT); // DT
		GPIO_Init(GPIOD, GPIO_PIN_6, GPIO_MODE_IN_PU_IT); // CLK
		EXTI_SetExtIntSensitivity(EXTI_PORT_GPIOD, EXTI_SENSITIVITY_RISE_FALL);

		gate_a = GPIO_ReadInputPin(GPIOD, GPIO_PIN_6);
		gate_b = GPIO_ReadInputPin(GPIOD, GPIO_PIN_5);
		next_value = gate_a ^ 1;
		
		enableInterrupts();
		
		CLK_DeInit();
		CLK_HSECmd(DISABLE);
		CLK_LSICmd(DISABLE);
		
		CLK_CCOConfig(CLK_OUTPUT_HSI);
		CLK_CCOCmd(DISABLE);
		
		CLK_HSIPrescalerConfig(CLK_PRESCALER_HSIDIV4);
		CLK_HSICmd(ENABLE);
	
		// Habilita o clock para o I2C
		CLK_PeripheralClockConfig(CLK_PERIPHERAL_I2C, ENABLE);
    ssd1306_init();
    ssd1306_update_timer(0, selected);

    while (true) 
    {
        uint32_t end = millis();
        uint32_t delta = end - start;
        start = end;

        if (running)
        {
            if (timer_uint >= DAY)
            {
                timer_uint = old_timer_uint;
                running = false;
                // Rele
								GPIO_WriteLow(GPIOD, GPIO_PIN_3);
				
								// Buzzer
								GPIO_WriteLow(GPIOA, GPIO_PIN_3);
                accum = beep_accum = 0;
                beeping = false;
                update_lcd = true;
            }
            else
            {
                accum += delta;
                if (accum >= 1000)
                {
                    update_lcd = true;
                    accum -= 1000;
                    timer_uint--;
                }

                if (reset)
                {
                    // Buzzer
										GPIO_WriteLow(GPIOA, GPIO_PIN_3);
                    accum = beep_accum = 0;
                    beeping = false;
                    reset = false;
                }

                if (timer_uint < MINUTE)
                {
                    beep_accum += delta;

                    if (!beeping && beep_accum >= 5000)
                    {
                        // Buzzer
												GPIO_WriteHigh(GPIOA, GPIO_PIN_3);
                        beeping = true;
                        beep_accum -= 5000;
                    }
                    else if (beeping && beep_accum >= 500)
                    {
                        // Buzzer
												GPIO_WriteLow(GPIOA, GPIO_PIN_3);
                        beeping = false;
                        beep_accum -= 500;
                    }
                }
            }
        }
        else
        {
            accum = beep_accum = 0;
            beeping = false;
        }
        
        if (update_lcd)
        {
            ssd1306_update_timer(timer_uint, selected);
            update_lcd = false;
        }
    }

    return 0;
}