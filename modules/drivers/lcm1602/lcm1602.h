#ifndef LCM1602_API
#define LCM1602_API


#include<zephyr/device.h>
#include<zephyr/toolchain.h>
#include<stdbool.h>
#include<zephyr/drivers/i2c.h>

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
#define LCD_FUNCTION_SET    0x20
#define LCD_FUNCTION_2LINE  0x28
#define LCD_FUNCTION_1LINE  0x20
#define LCD_5X8_DOTS        0x20
#define LCD_5X11_DOTS   0x04

/* I2C LCD Pin Mapping (PCF8574) */
#define LCD_RS_PIN           0x01  /* P0 - Register Select */
#define LCD_RW_PIN           0x02  /* P1 - Read/Write */
#define LCD_EN_PIN           0x04  /* P2 - Enable */
#define LCD_BACKLIGHT_PIN    0x08  /* P3 - Backlight */

#define LCD_ENTRYRIGHT 0x06
#define LCD_ENTRYLEFT   0x04
#define LCD_SHIFT_ENTIRE_DISPLAY_RIGHT 0x05 
#define LCD_SHIFT_ENTIRE_DISPLAY_LEFT  0x04

#define LCD_DISPLAYON 0x04
#define LCD_CURSOROFF 0x00
#define LCD_BLINKOFF  0x00
#define LCD_CURSORON 0x02
#define LCD_BLINKON 0x01
#define LCD_DISPLAYCONTROL 0x08

#define LCD_SHIFT_DISPLAY_LEFT 0x18
#define LCD_SHIFT_DISPLAY_RIGHT 0x1c
#define LCD_SHIFT_CURSOR_LEFT 0x10
#define LCD_SHIFT_CURSOR_RIGHT 0x14
enum shift_dir
{
    left,
    right
};
enum state
{
    on,
    off
};
__subsystem struct lcm1602_driver_api

{
    /***
     * @brief set the state of the cursor either on or off 
     * 
     * @param *dev pointer to the device instance 
     * @param  state true to turn cursor on , false to turn it off
     * 
     * @retval 0 if successful
     * @retval -errno for failure 
     */
    int (*cursor_blink)(const struct device *dev,enum state state);
   
    /***
     * @brief turn cursor on and off 
     */
    int (*cursor)(const struct device *dev, enum state st);

    /***
     * @brief turn backlight on or off 
     */
    int  (*backlight)(const struct device *dev,enum state st);
    /***
     * @brief shifts entire display 
     * 
     * @param *dev pointer to device instance 
     * @param  dir direction to shift display  can either be left of right
     * @retval 0 if successful
     * @retval -errno for failure 
     */
    int (*shift_display)(const struct device *dev,enum shift_dir dir);
    
    /***
     * @brief writes characters data to the DDRAM address
    *  specified by the set cursor function
     * 
     * @param pointer to the device intstance
     * @param char* pointer to character array
     * @param len length of character array 
     * 
     * @retval 0 if successful
     * @retval -errno for failure
     */
    int (*print)(const struct device *dev,char *character,int len);

    /***
     * @brief location to set cursor 
     * 
     * @param *dev pointer to device instance
     * @param row row to set cursor at 
     * @param col column to set cursor at 
     * 
     * @retval 0 if successful
     * @retval -errno for failure
     * @
     */
    int (*set_cursor)(const struct device *dev,uint8_t row,uint8_t col);
    /***
     * @brief store values for a custom character in memory 
     * 
     * @param *dev pointer to a device instance 
     * @param  *rows a pointer to a 5X8 array with the character definition 
     *          see ref manual on how to define a custom char 
     * @param addr CCRAM  address to write to can be a value between 0 and 7 
     *  only seven custom characters can be supported address greater than 7 
     *  will generate and error 
     * 
     * @retval 0 if successful
     * @retval -errno for failure
     */
    int   (*create_custom_char)(const struct device *dev,char *rows,uint8_t addr);

    /***
     * @brief print custom character at the current cursor location 
     * 
     * @param addr address of the CGRAM crange [0,7] out of range error will be thrown 
     *  for values not in range 
     * 
     * @retval 0 if successful
     * @retval -errno for failure
     */
    int (*print_custom)(const struct device *dev,uint8_t addr);

    /***
     * @brief clear entire display
     * 
     * @param *dev pointer to device instance 
     */
    int (*clear)(const struct device *dev);

    /***
     * @brief move cursor ot home position 
     * 
     * @param *dev pointer to device instance 
     * 
     * @retval 0 if successful
     * @retval -errno for failure
     */
    int (*home)(const struct device *dev);

    /***
     * @brief shift cursor either left or right s
     */
    int (*shift_cursor)(const struct device *dev,enum shift_dir dir);

   
};
struct lcm1602_config{
    struct i2c_dt_spec i2c;
    uint8_t num_lines;
};

struct lcm1602_data
{
    uint8_t lcd_backlight_state;
};

int lcd_cursor_blink(const struct device *dev,enum state state);
int lcd_display_shift(const struct device *dev,enum shift_dir dir);
int lcd_print(const struct device *dev,char *character,int len);
int lcd_set_cursor(const struct device *dev,uint8_t row,uint8_t col);
int lcd_create_custom_char(const struct device *dev,char *rows,uint8_t addr);
int lcd_clear(const struct device *dev);
int lcd_home(const struct device *dev);
int lcd_cursor(const struct device *dev, enum state state);
int lcd_backlight(const struct device *dev,enum state st);
int  lcd_print_custom(const struct device *dev,uint8_t addr);
int lcd_cursor_shift(const struct device *dev,enum shift_dir dir);


#endif