
#include"lm1602.h"

const struct i2c_dt_spec lm1602=I2C_DT_SPEC_GET(DT_NODELABEL(lm1602));

 uint8_t lcd_backlight_state=LCD_BACKLIGHT_PIN; // backlight
/**
 * 
 * @brief turns backlight on  for the lm1602
 */
void backlight_on()
{
    lcd_write_4bit(LCD_DISPLAYCONTROL |  LCD_BLINKON , true);
}
void backlight_off()
{
    lcd_backlight_state=~(1<<3);
    i2c_write_dt(&lm1602,&lcd_backlight_state,1);
}
void lcd_init()
{
    /*
    Even with perfect timing in lcd_write_4bit, the LCD will not respond if it hasn't been put into 4-bit mode correctly first.
     When the LCD powers on, it starts in 8-bit mode.

    To get into 4-bit mode, you cannot just call lcd_write_4bit. 
    You must send the "Function Set" command (0x03) as a single nibble three times to "wake it up" before you can start sending dual-nibble commands.

pseudo-code

    Wait > 15ms after power-up.

    Send 0x30 (high nibble only) via I2C, pulse EN.

    Wait > 4.1ms.

    Send 0x30 (high nibble only), pulse EN.

    Wait > 100µs.

    Send 0x30 (high nibble only), pulse EN.

    Send 0x20 (high nibble only), pulse EN. <— This officially switches it to 4-bit mode.

    Now you can use your lcd_write_4bit function for everything else.
    */
     /* Wait for LCD power-up (>40ms) */
   k_msleep(50);

    // Initial state: RS=0, RW=0, EN=0, Backlight=On
    uint8_t startup_flags = LCD_BACKLIGHT_PIN;

    // 2. First special "Wake up" attempt (Instruction 0x30)
    // We send this as a high nibble only.
    // set lcd to 8 bit mode by sending insruction 0x30
    pcf8574_write(LCD_FUNCTION_8BIT | startup_flags);
    k_msleep(5); // Must wait > 4.1ms

    // 3. Second special "Wake up" attempt
    pcf8574_write(LCD_FUNCTION_8BIT | startup_flags);
    k_usleep(150); // Must wait > 100us

    // 4. Third special "Wake up" attempt
    pcf8574_write(LCD_FUNCTION_8BIT | startup_flags);
    k_msleep(1);

    // 5. Set to 4-bit mode (Instruction 0x20)
    // After this command, the LCD is officially in 4-bit mode
    pcf8574_write(LCD_FUNCTION_4BIT | startup_flags);
    k_msleep(1);

    
    
    // Function Set: 2 lines, 5x8 font (0x28)
    lcd_write_4bit(LCD_FUNCTION_4BIT | LCD_FUNCTION_2LINE, true); 
    
    // Display Control: Display ON, Cursor OFF, Blink OFF (0x0C)
    lcd_write_4bit(LCD_DISPLAYCONTROL | LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF , true);
    
    // Clear Display (0x01)
    lcd_write_4bit(LCD_CLEAR_DISPLAY , true);
    k_msleep(2); // Clear command needs ~1.6ms to execute

    // Entry Mode: Increment cursor, no shift (0x06)
    lcd_write_4bit(LCD_ENTRY_MODE_SET | LCD_ENTRYLEFT, true);

   
}
void initialize()
{
    if(!device_is_ready(lm1602.bus))
    {
        printk("i2c driver for lcd not ok!\n");
        return ;
    }
    return ;
}

void lcd_write_instruction(uint8_t inst)
{
    
      lcd_write_4bit(inst,true);
}
void lcd_write_data(uint8_t data)
{
    
    lcd_write_4bit(data,false);
}

// Helper to send a byte to the I2C expander and pulse EN
void pcf8574_write(uint8_t val) {
    uint8_t buf;
    // RW pin has to stay low during write operations 
    buf=val & ~LCD_EN_PIN & ~LCD_RW_PIN;
    
    i2c_write_dt(&lm1602,&buf,1);

    k_usleep(1); // tsu1 (R/W setup time) > 100ns       
    // 1. Send data with EN HIGH
    buf = val | LCD_EN_PIN;
    i2c_write_dt(&lm1602, &buf, 1);
    k_usleep(1); // Enable pulse width > 450ns
    
    // 2. Send data with EN LOW (Falling edge triggers the LCD)
    buf = val & ~LCD_EN_PIN;
    i2c_write_dt(&lm1602, &buf, 1);
    k_usleep(40); // Execution time
    
}

void lcd_write_4bit(uint8_t d, bool is_instruction) {
    uint8_t rs_mask = is_instruction ? 0 : LCD_RS_PIN;
    uint8_t backlight = lcd_backlight_state; // Usually 0x08
    
    uint8_t high_nibble = (d & 0xF0);
    uint8_t low_nibble = ((d << 4) & 0xF0);

    // Write High Nibble
    pcf8574_write(high_nibble | rs_mask | backlight);
    
    // Write Low Nibble
    pcf8574_write(low_nibble | rs_mask | backlight);
}

void cursor_on()
{
    lcd_write_4bit(LCD_DISPLAYCONTROL | LCD_DISPLAYON | LCD_CURSORON | LCD_BLINKOFF , true);
}
void cursor_off()
{
    lcd_write_4bit(LCD_DISPLAYCONTROL | LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF , true);
}
void cursor_blinkon()
{
    lcd_write_4bit(LCD_DISPLAYCONTROL | LCD_DISPLAYON | LCD_CURSORON | LCD_BLINKON, true);
}
void cursor_blinkoff()
{
    lcd_write_4bit(LCD_DISPLAYCONTROL | LCD_DISPLAYON | LCD_CURSORON | LCD_BLINKOFF, true);
}

/**
 * @brief This function sets the location of the cursor
 * 
 *   i.e  DDRAM address
 * @param row row of display either 0 or 1
 * @param col column to write character at between 0 and 39
 * 
 */
void set_cursor(uint8_t row,uint8_t col)
{
    //when in
    if(row==1)
    {
        //addr range is between 0x40 to 0x67
        col=col+0x40;
    }
    lcd_write_4bit(LCD_SET_DDRAM_ADDR|col,true);
    return ;

}
void home()
{
    lcd_write_4bit(LCD_RETURN_HOME , true);
    
}
/**
 * @brief clears entire display 
 */
void clear()
{
   // Clear Display (0x01)
    lcd_write_4bit(LCD_CLEAR_DISPLAY , true);
}

/**
 * @brief writes characters data to the DDRAM address
 *  specified by the set cursor function
 *  
 * @param *character apointer a character array to be displayed 
 * @param len length of the character array 
 */
void lcd_print(char *character,int len)
{
    for(int i=0;i<len;++i)
    {
        lcd_write_4bit(*(character+i),false);
    }
}
/**
 * @brief rows consist of  binary data of the rows 
 * 
 * @param *rows a pointer to the character row array
 * @param addr  this has to be a value between 0 and 7
 */
void  create_custom_char(char *rows,uint8_t addr)
{
    lcd_write_4bit(LCD_SET_CGRAM_ADDR| addr,true);
    // assuming address is automatically increased
    for(uint8_t i=0;i<8;++i)
    {
        lcd_write_4bit(*(rows+i),false);
    }
}
/**
 * @brief prints custom character at the specified address of the CGRAM 
 * 
 * @param addr address of CGRAM where character to be printed id located 
 * 
 *  has to be a value between 0 and 7
 */
void print_custom(uint8_t addr)
{
    lcd_write_4bit(addr,false);
}
void shift_display_left()
{
    lcd_write_4bit(LCD_SHIFT_DISPLAY_LEFT,true);
}
