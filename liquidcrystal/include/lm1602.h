#ifndef LM1602
#define LM1602
#include<zephyr/kernel.h>
#include<zephyr/drivers/i2c.h>
#include<stdbool.h>

//connections 
/*
p0->RS
p1->RW
p2->E
p3->BT backlight 
p4->D4
p4->D5
p6->D6
p7->D7

*/

#define LCD_CLEAR_DISPLAY    0x01
#define LCD_RETURN_HOME      0x02
#define LCD_ENTRY_MODE_SET   0x04
#define LCD_DISPLAY_CONTROL  0x08
#define LCD_CURSOR_SHIFT     0x10
#define LCD_FUNCTION_SET     0x20
#define LCD_SET_CGRAM_ADDR   0x40
#define LCD_SET_DDRAM_ADDR   0x80

#define LCD_FUNCTION_8BIT     0X30
#define LCD_FUNCTION_4BIT   0X20
#define LCD_FUNCTION_2LINE  0x08
#define LCD_5X8_DOTS        
#define LCD_5X11_DOTS   0x04

/* I2C LCD Pin Mapping (PCF8574) */
#define LCD_RS_PIN           0x01  /* P0 - Register Select */
#define LCD_RW_PIN           0x02  /* P1 - Read/Write */
#define LCD_EN_PIN           0x04  /* P2 - Enable */
#define LCD_BACKLIGHT_PIN    0x08  /* P3 - Backlight */

#define LCD_ENTRYLEFT 0x02

#define LCD_DISPLAYON 0x04
#define LCD_CURSOROFF 0x00
#define LCD_BLINKOFF  0x00
#define LCD_CURSORON 0x02
#define LCD_BLINKON 0x01
#define LCD_DISPLAYCONTROL 0x08

#define LCD_SHIFT_DISPLAY_LEFT 0x18

void lcd_init();
void initialize();
void backlight_on();
void backlight_off();


void lcd_write_instruction(uint8_t inst);
void lcd_write_4bit(uint8_t nibble,bool instruction);
void cursor_on();
void lcd_print(char *character,int len);


void clear();
void home();

void pcf8574_write(uint8_t val);
void set_cursor(uint8_t row,uint8_t col);
void cursor_blinkon();
void cursor_off();
void cursor_blinkoff();
void  create_custom_char(char *rows,uint8_t addr);
void print_custom(uint8_t addr);
void shift_display_left();

#endif