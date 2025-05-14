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

#define SLEEP_TIME 15000

// Estado
typedef enum State { SET_TIMER , RUNNING };
uint8_t current_state = SET_TIMER;

// Button
bool old_btn_state = false;
uint32_t t_btn_down = 0; // Instância de tempo da descida do botão

// Rotator
bool gate_a = true; // Portao A do encoder
bool gate_b = true;	// Portao B do encoder
bool next_value = 0; // Proximo valor que o portao deve assumir para contar rotação

// OLED
bool update_lcd = false; // Variavel de controle para atualizar o lcd por interrupção
uint8_t selected = INVERT_SECONDS; // Indica quem esta selecionado no LCD { SEGUNDOS, MINUTOS, HORAS }
uint32_t timer = 0, old_timer = 0; // Timer do LCD
uint16_t timer_incrementer = 1;	// Incrementador do timer que pode ser 1 segundo, 1 minuto (60s) ou 1h (3600s)
uint16_t power_accum = 0; // Acumulador de tempo para quando chegar a 10 sec sem atualização desligar OLED

// Buzzer
bool beeping = false;	// Variavel de controle do buzzer para indicar a duração do de tempo ligado e desligado
uint16_t beep_accum = 0; // Acumulador de tempo para controlar a duração do som do buzzer

// Aux Variables
uint16_t accum = 0; // Acumulador de 1 segundo para o contador

// Clock
uint16_t old_clock_time = 0; // Indica qual o valor antigo do contador o clock em ms para ajustar casos de overflow
uint32_t absolute_time = 0;	// Indica o tempo absoluto em ms desde o começo da aplicação

// Chamada para obter quantos ms se passaram desde o inicio da aplicação
static uint32_t millis(void);

// Funções auxiliares para as interrupções do butão e da rotação do encoder respectivamente
static void sw_handler(bool state);
static void rot_handler(void);

static void CLK_Config(void);

static void GPIO_Config(void);

// Função de configuração do timer 1 de clock
static void TIM1_Config(void);

// Função para mudança de estados da aplicação
// Responsável por mudar os valores de variáveis globais dependendo do estado
static void change_state(uint8_t state);

int main() 
{
		uint32_t start = millis();
		uint16_t ticks = 0;
		
		CLK_Config();
		TIM1_Config();
		GPIO_Config();
		
		gate_a = (bool)(GPIO_ReadInputPin(GPIOD, GPIO_PIN_6) != 0);
		gate_b = (bool)(GPIO_ReadInputPin(GPIOD, GPIO_PIN_5) != 0);
		next_value = (bool)(gate_a ^ 1);
		
    ssd1306_init();
    ssd1306_update_timer(0, selected);

    while (true) 
    {
        uint32_t end = millis();
        uint16_t delta = (uint16_t)(end - start); // Estou considerando que end - start nao sera maior que 65536 ms
        start = end;

				switch(current_state)
				{
				case SET_TIMER:
						if (update_lcd)
						{
								ssd1306_update_timer(timer, selected);
								update_lcd = false;
								
								if (power_accum >= SLEEP_TIME)
									ssd1306_power(true);
								power_accum = 0;
						}
						
						if (power_accum < SLEEP_TIME)
						{
							power_accum += delta;
							if (power_accum >= SLEEP_TIME)
								ssd1306_power(false);
						}
					break;
				case RUNNING:
						if (timer < DAY)
						{
								accum += delta;
                if (accum >= 1000)
                {
										timer--;
										accum -= 1000;
										ssd1306_update_timer(timer, selected);
                }

                if (timer < MINUTE)
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
						else
								change_state(SET_TIMER);
					break;
				default:
					continue;
				}
    }

    return 0;
}

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

static void sw_handler(bool btn_state)
{
		uint32_t delta = 0;
		
		// Se o botão foi pra baixo e antes estava alto pega o tempo
    if (btn_state && !old_btn_state) t_btn_down = millis();
    else if (!btn_state && old_btn_state) // Se o botão foi pra alto e antes estava baixo calcula o delta
		{
			delta = millis() - t_btn_down;
			
			// Se delta for maior que o tempo minimo de estabilização então é contado 1 pressionamento
			if (delta > SW_DELAY_CURSOR)
			{
					if (delta > SW_DELAY_RUN && timer != 0)
							change_state((uint8_t)(current_state ^ 1));
					else if (current_state == RUNNING)
							change_state(RUNNING);
					else
					{
							timer_incrementer = timer_incrementer == 3600 ? 1 : timer_incrementer * 60;
							update_lcd = true;
							selected = (uint8_t)(selected >= INVERT_HOURS ? INVERT_SECONDS : selected << 1);
					}
			}
		}

    old_btn_state = btn_state;
}

static void rot_handler()
{
		bool state_a = (bool)((PD_IDR & GPIO_PIN_6) != 0);
		bool state_b = (bool)((PD_IDR & GPIO_PIN_5) != 0);
		
    if ((gate_a != state_a) && state_a == next_value)
    {
        gate_a = next_value;
        next_value = (bool)(next_value ^ 1);

        if (gate_a != gate_b)
        {
            timer += timer_incrementer;
            if (timer >= DAY)
                timer = DAY - 1;
            update_lcd = true;
        }
    }
    else if ((gate_b != state_b) && state_b == next_value)
    {
        gate_b = next_value;
        next_value = (bool)(next_value ^ 1);

        if (gate_a != gate_b)
        {
            timer -= timer_incrementer;
            // Overflow
            if (timer >= DAY)
                timer = 0;
            update_lcd = true;
        }
    }
}

@far @interrupt @svlreg void PD_IRQHandler()
{
		bool state = (bool)((PD_IDR & GPIO_PIN_4) == 0);
		if(state != old_btn_state)
				sw_handler(state);
		else if (current_state == SET_TIMER)
				rot_handler();
}

static void CLK_Config(void)
{
	CLK_DeInit();
	CLK_HSECmd(DISABLE);
	CLK_LSICmd(DISABLE);
	
	// Desabilita a saida de clock do pino de teste
	CLK_CCOConfig(CLK_OUTPUT_HSI);
	CLK_CCOCmd(DISABLE);
	
	// Divide o clock por 4 saindo entao 4mhz que é o clock
	// Mínimo para que o ssd1306 funcione no i2c
	CLK_HSIPrescalerConfig(CLK_PRESCALER_HSIDIV4);
	CLK_HSICmd(ENABLE);

	// Habilita o clock para o I2C
	CLK_PeripheralClockConfig(CLK_PERIPHERAL_I2C, ENABLE);
}

static void GPIO_Config(void)
{
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
	
	enableInterrupts();
}

static void TIM1_Config()
{
	TIM1_DeInit();
	// Primeiro parametro e o divisor do clock, basicamente ele diz a quantos pulsos de clock
	// Devemos incrementar o contador assim como 4mhz é o valor do clock configurado anteriormente
	// Entao: 4mhz / 4k = 1000 -> 1 / 1000 = 1ms
	// Segundo parametro indica qual o valor de estouro que pode ser ate no maximo 65535 pois ele so
	// Vai ate 16 bits
	TIM1_TimeBaseInit(4000, TIM1_COUNTERMODE_UP, 65535, 0);
	TIM1_Cmd(ENABLE);
}

static void change_state(uint8_t state)
{
	
	switch(state)
	{
	case SET_TIMER:
		timer = old_timer;
		
		// Rele
		GPIO_WriteLow(GPIOD, GPIO_PIN_3);

		// Buzzer
		GPIO_WriteLow(GPIOA, GPIO_PIN_3);
		
		accum = beep_accum = 0;
		beeping = false;
		update_lcd = true;
		break;
	case RUNNING:
		accum = 0;
		beep_accum = 0;
		beeping = false;
		
		// Se já estiver em running reinicia o contador
		if (current_state == RUNNING) timer = old_timer;
		else old_timer = timer;
		
		// Rele
		GPIO_WriteHigh(GPIOD, GPIO_PIN_3);

		// Buzzer
		GPIO_WriteLow(GPIOA, GPIO_PIN_3);
		break;
	default:
		return;
	}
	
	current_state = state;
}