    #include "ssd1306.h"
    #include <string.h>

    #define ADDR 0x78

    // register definitions
    #define SET_CONTRAST            0x81
    #define SET_ENTIRE_ON           0xA4
    #define SET_NORM_INV            0xA6
    #define SET_DISP                0xAE
    #define SET_MEM_ADDR            0x20
    #define SET_COL_ADDR            0x21
    #define SET_PAGE_ADDR           0x22
    #define SET_DISP_START_LINE     0x40
    #define SET_SEG_REMAP           0xA0
    #define SET_MUX_RATIO           0xA8
    #define SET_IREF_SELECT         0xAD
    #define SET_COM_OUT_DIR         0xC0
    #define SET_DISP_OFFSET         0xD3
    #define SET_COM_PIN_CFG         0xDA
    #define SET_DISP_CLK_DIV        0xD5
    #define SET_PRECHARGE           0xD9
    #define SET_VCOM_DESEL          0xDB
    #define SET_CHARGE_PUMP         0x8D

    #define WIDTH 128
    #define HEIGHT 64
    #define PAGES 8

		#define DISPLAY_PORT GPIOB
    #define SCL GPIO_PIN_4
    #define SDA GPIO_PIN_5

    static const uint8_t s_digits_8x8[10][8] = {
        // Dï¿½gito 0
        { 0x00, 0x3E, 0x7F, 0x49, 0x45, 0x7F, 0x3E, 0x00 },
        // Dï¿½gito 1
        { 0x00, 0x00, 0x42, 0x7F, 0x7F, 0x40, 0x00, 0x00 },
        // Dï¿½gito 2
        { 0x00, 0x42, 0x63, 0x71, 0x59, 0x4F, 0x46, 0x00 },
        // Dï¿½gito 3
        { 0x00, 0x22, 0x63, 0x49, 0x49, 0x7F, 0x36, 0x00 },
        // Dï¿½gito 4
        { 0x00, 0x18, 0x1C, 0x16, 0x7F, 0x7F, 0x10, 0x00 },
        // Dï¿½gito 5
        { 0x00, 0x27, 0x67, 0x45, 0x45, 0x7D, 0x39, 0x00 },
        // Dï¿½gito 6
        { 0x00, 0x3C, 0x7E, 0x4B, 0x49, 0x79, 0x30, 0x00 },
        // Dï¿½gito 7
        { 0x00, 0x01, 0x71, 0x79, 0x0D, 0x07, 0x03, 0x00 },
        // Dï¿½gito 8
        { 0x00, 0x36, 0x7F, 0x49, 0x49, 0x7F, 0x36, 0x00 },
        // Dï¿½gito 9
        { 0x00, 0x06, 0x4F, 0x49, 0x69, 0x3F, 0x1E, 0x00 },
    };

		static void ssd1306_begin_i2c(uint8_t control_byte);
		static void ssd1306_write(uint8_t data);
		static void ssd1306_end_i2c(void);
		static void ssd1306_draw_page(uint8_t collumn, uint8_t number, uint8_t inverted);

		static void ssd1306_begin_i2c(uint8_t control_byte)
		{
			while(I2C_GetFlagStatus(I2C_FLAG_BUSBUSY));
			
			I2C_GenerateSTART(ENABLE);
			while (!I2C_CheckEvent(I2C_EVENT_MASTER_MODE_SELECT));
			
			I2C_Send7bitAddress(ADDR, I2C_DIRECTION_TX);
			while (!I2C_CheckEvent(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));
			
			I2C_SendData(control_byte);
			while (!I2C_CheckEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED));
		}

    static void ssd1306_write(uint8_t data)
    {
			I2C_SendData(data);
			while(!I2C_CheckEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED));
    }

		static void ssd1306_end_i2c()
		{
			I2C_GenerateSTOP(ENABLE);
		}

    void ssd1306_init()
    {
			volatile uint32_t n = 100000;
			uint16_t i = 0;
			uint8_t column = 85;
			
			GPIO_DeInit(DISPLAY_PORT);
			GPIO_Init(DISPLAY_PORT, SCL, GPIO_MODE_OUT_OD_HIZ_FAST);
			GPIO_Init(DISPLAY_PORT, SDA, GPIO_MODE_OUT_OD_HIZ_FAST);
		
			I2C_DeInit();
			I2C_Init(400000, 1, I2C_DUTYCYCLE_2, I2C_ACK_CURR, I2C_ADDMODE_7BIT, 4);
			I2C_Cmd(ENABLE);
			
			while(n--);
			
			// Enviando comandos
			ssd1306_begin_i2c(0x00);

			ssd1306_write(SET_DISP);
			ssd1306_write(SET_MEM_ADDR);
			ssd1306_write(0x00);
			ssd1306_write(SET_DISP_START_LINE);
			ssd1306_write(SET_SEG_REMAP | 0x01);
			ssd1306_write(SET_MUX_RATIO);
			ssd1306_write(HEIGHT - 1);
			ssd1306_write(SET_COM_OUT_DIR | 0x08);
			ssd1306_write(SET_DISP_OFFSET);
			ssd1306_write(0x00);
			ssd1306_write((WIDTH> 2 * HEIGHT) ? 0x02 : 0x12);
			ssd1306_write(SET_DISP_CLK_DIV);
			ssd1306_write(0x80);
			ssd1306_write(SET_PRECHARGE);
			ssd1306_write(0xF1); // 0x22 if self.external_vcc else 0xF1, ?
			ssd1306_write(SET_VCOM_DESEL);
			ssd1306_write(0x30);
			ssd1306_write(SET_CONTRAST);
			ssd1306_write(0xFF);
			ssd1306_write(SET_ENTIRE_ON);
			ssd1306_write(SET_NORM_INV);
			ssd1306_write(SET_IREF_SELECT);
			ssd1306_write(0x30);
			ssd1306_write(SET_CHARGE_PUMP);
			ssd1306_write(0x14); // 0x10 if self.external_vcc else 0x14 ?
			ssd1306_write(SET_DISP | 0x01);

			ssd1306_write(SET_COM_OUT_DIR | (1 << 3));
			ssd1306_write(SET_SEG_REMAP | 1);
			
			// Enviando comandos para limpar a tela
			ssd1306_write(SET_COL_ADDR);
			ssd1306_write(0);
			ssd1306_write(WIDTH - 1);

			ssd1306_write(SET_PAGE_ADDR);
			ssd1306_write(0);
			ssd1306_write(PAGES - 1);
			
			ssd1306_end_i2c();
			
			// Enviando dados
			ssd1306_begin_i2c(0x40);
			
			for (i = 0; i < WIDTH * PAGES; i++)
				ssd1306_write(0x00);
				
			ssd1306_end_i2c();
			
			for (i = 0; i < 2; i++, column -= 20)
			{
				// Enviando comandos
				ssd1306_begin_i2c(0x00);
				
				ssd1306_write(SET_COL_ADDR);
				ssd1306_write(column - 12);
				ssd1306_write(column - 11);

				// Desenha na página 3
				ssd1306_write(SET_PAGE_ADDR);
				ssd1306_write(3);
				ssd1306_write(3);
				
				ssd1306_end_i2c();
				
				// Enviando dados
				ssd1306_begin_i2c(0x40);
				
				ssd1306_write(0x24);
				ssd1306_write(0x24);
				
				ssd1306_end_i2c();
			}
    }

    void ssd1306_power(bool on)
    {
			while(I2C_GetFlagStatus(I2C_FLAG_BUSBUSY));
			
			I2C_GenerateSTART(ENABLE);
			while (!I2C_CheckEvent(I2C_EVENT_MASTER_MODE_SELECT));
			
			I2C_Send7bitAddress(ADDR, I2C_DIRECTION_TX);
			while (!I2C_CheckEvent(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));
			
			// Envia apenas 1 byte
			I2C_SendData(0x80);
			while (!I2C_CheckEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED));
			
			I2C_SendData(SET_DISP | on);
			while(!I2C_CheckEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED));
			
			I2C_GenerateSTOP(ENABLE);
    }

    static void ssd1306_draw_page(uint8_t collumn, uint8_t number, uint8_t inverted)
    {
			uint8_t i;
			
			// Enviando comandos
			ssd1306_begin_i2c(0x00);
				
			ssd1306_write(SET_COL_ADDR);
			ssd1306_write(collumn);
			ssd1306_write(collumn + 7);

			// Desenha na página 3 e 4
			ssd1306_write(SET_PAGE_ADDR);
			ssd1306_write(3);
			ssd1306_write(4);
			
			ssd1306_end_i2c();

			// Enviando dados
			ssd1306_begin_i2c(0x40);
			
			// Número
			for (i = 0; i < 8; i++)
				ssd1306_write(s_digits_8x8[number][i]);
			// Sublinhado, caso nao seja o selecionado limpa e se for desenha
			for (i = 0; i < 8; i++)
				ssd1306_write(inverted);
				
			ssd1306_end_i2c();
    }

    void ssd1306_update_timer(uint32_t timer, uint8_t inverted)
    {
			uint8_t desconstructed_timer[3] = { (timer % 60), (timer / 60) % 60, (timer / 3600) };
			uint8_t i, column = 85;
			
			for (i = 0; i < 3; i++)
			{
				ssd1306_draw_page(column - 10, desconstructed_timer[i] / 10, inverted & 1); // Dezenas
				ssd1306_draw_page(column, desconstructed_timer[i] % 10, inverted & 1); // Unidades

				column -= 20;
				inverted >>= 1;
			}
    }