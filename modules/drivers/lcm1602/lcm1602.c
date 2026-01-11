#define DT_DRV_COMPAT lcm1602

#include<zephyr/kernel.h>
#include<errno.h>
#include<zephyr/drivers/i2c.h>


#include"lcm1602.h"


int lcd_write_4bit(const struct device *dev,uint8_t d, bool is_instruction);
/***
 * 
 * @brief function the does actual writing of the data to the lcm1602 based on the write 
 * timming diagram ,does actual transfer using i2c hardware
 * 
 * @param *dev pointer to the device instance 
 * 
 * @param nibble to send 
 * 
 * @retval  returns 0 for success or an error code 
 */
 int pcf8574_write(const struct device *dev ,uint8_t val) {
    uint8_t buf;
    // RW pin has to stay low during write operations 
    buf=val & ~LCD_EN_PIN & ~LCD_RW_PIN;
    const struct lcm1602_config *cfg=(const struct lcm1602_config*)dev->config;
    i2c_write_dt(&cfg->i2c,&buf,1);

    k_usleep(1); // tsu1 (R/W setup time) > 100ns       
    // 1. Send data with EN HIGH
    buf = val | LCD_EN_PIN;
    i2c_write_dt(&cfg->i2c, &buf, 1);
    k_usleep(1); // Enable pulse width > 450ns
    
    // 2. Send data with EN LOW (Falling edge triggers the LCD)
    buf = val & ~LCD_EN_PIN;
    i2c_write_dt(&cfg->i2c, &buf, 1);
    k_usleep(40); // Execution time
    
    return 0; // for now 
}

/***
 * @brief write  2 nibbles to the lcm1602 by splitting the 8bit data into nibbles and 
 * calling pcf8574_write
 * 
 * @param *dev pointer to device instance 
 * 
 * @param is_instruction this is set to true for instructions 
 * 
 * @retval int returns 0 for success or an error code for failure
 */
int lcd_write_4bit(const struct device *dev,uint8_t d, bool is_instruction) {
    uint8_t rs_mask = is_instruction ? 0 : LCD_RS_PIN;
    struct lcm1602_data *data=( struct lcm1602_data*)dev->data;
    uint8_t backlight = data->lcd_backlight_state; // Usually 0x08
    
    uint8_t high_nibble = (d & 0xF0);
    uint8_t low_nibble = ((d << 4) & 0xF0);

    // Write High Nibble
    pcf8574_write(dev,high_nibble | rs_mask | backlight);
    
    // Write Low Nibble
    pcf8574_write(dev,low_nibble | rs_mask | backlight);
    return 0; // for now 
}

/***
 * @brief initialize the lcm1602 and set it up for 4 bit mode 
 * 
 * @param *dev pointer to a device instance 
 * 
 * @retval  returns 0 for success or an error code 
 */
int  lcm1602_init(const struct device *dev)
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
    struct lcm1602_data *data=( struct lcm1602_data*)dev->data;
    data->lcd_backlight_state=LCD_BACKLIGHT_PIN;
    // 2. First special "Wake up" attempt (Instruction 0x30)
    // We send this as a high nibble only.
    // set lcd to 8 bit mode by sending insruction 0x30
    pcf8574_write(dev,LCD_FUNCTION_8BIT | startup_flags);
    k_msleep(5); // Must wait > 4.1ms

    // 3. Second special "Wake up" attempt
    pcf8574_write(dev,LCD_FUNCTION_8BIT | startup_flags);
    k_usleep(150); // Must wait > 100us

    // 4. Third special "Wake up" attempt
    pcf8574_write(dev,LCD_FUNCTION_8BIT | startup_flags);
    k_msleep(1);

    // 5. Set to 4-bit mode (Instruction 0x20)
    // After this command, the LCD is officially in 4-bit mode
    pcf8574_write(dev,LCD_FUNCTION_4BIT | startup_flags);
    k_msleep(1);

    
    
    // Function Set: 2 lines, 5x8 font (0x28)
    lcd_write_4bit(dev,LCD_FUNCTION_4BIT | LCD_FUNCTION_2LINE, true); 
    
    // Display Control: Display ON, Cursor OFF, Blink OFF (0x0C)
    lcd_write_4bit(dev,LCD_DISPLAYCONTROL | LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF , true);
    
    // Clear Display (0x01)
    lcd_write_4bit(dev,LCD_CLEAR_DISPLAY , true);
    k_msleep(2); // Clear command needs ~1.6ms to execute

    // Entry Mode: Increment cursor, no shift (0x06)
    lcd_write_4bit(dev,LCD_ENTRY_MODE_SET | LCD_ENTRYLEFT, true);

   return 0; // for now 
}


/***
 * @brief toggle the  blink state of the cursor 
 * 
 *
 */
int lcd_cursor_blink(const struct device *dev,enum state state)
{
    if(state)
    {
        lcd_write_4bit(dev,LCD_DISPLAYCONTROL | LCD_DISPLAYON | LCD_CURSORON | LCD_BLINKON, true);
    }
    else{
        lcd_write_4bit(dev,LCD_DISPLAYCONTROL | LCD_DISPLAYON | LCD_CURSORON | LCD_BLINKOFF, true);
    }
    return 0; // for now 
}


//----------------------------------------------------
//shift display 

 int lcd_shift_display(const struct device *dev,enum shift_dir dir)
 {
    switch (dir)
    {
    case left:
       lcd_write_4bit(dev,LCD_SHIFT_DISPLAY_LEFT,true);
        break;
    case right:
        //Todo
        break;
    default:
        break;
    }
    return 0 ;
 }

//------------------------------------------
//print 
/**
 * @brief writes characters data to the DDRAM address
 *  specified by the set cursor function
 *  
 * @param  *dev pointer to device instance 
 * @param *character apointer a character array to be displayed 
 * @param len length of the character array 
 */
 int lcd_print(const struct device *dev,char *character,int len)
{
    for(int i=0;i<len;++i)
    {
        lcd_write_4bit(dev,*(character+i),false);
    }
    return 0;
}

//--------------------------------------------
//set_cursor 
/**
 * @brief This function sets the location of the cursor
 * 
 *   i.e  DDRAM address
 * @param row row of display either 0 or 1
 * @param col column to write character at between 0 and 39
 * 
 */
int lcd_set_cursor(const struct device *dev,uint8_t row,uint8_t col)
{
    //ensure rows are not greater than 2
    if(row<=1)
    {
        if(row==1)
    {
        //addr range is between 0x40 to 0x67
        col=col+0x40;
    }
    lcd_write_4bit(dev,LCD_SET_DDRAM_ADDR|col,true);
    }
  
    return 0;

}


//--------------------------------
// create custom character 
int lcd_create_custom_char(const struct device *dev,char *rows,uint8_t addr)
{
    lcd_write_4bit(dev,LCD_SET_CGRAM_ADDR| addr,true);
    // assuming address is automatically increased
    for(uint8_t i=0;i<8;++i)
    {
        lcd_write_4bit(dev,*(rows+i),false);
    }
    return 0; 
}

//------------------------------
// print custom char 
/**
 * @brief prints custom character at the specified address of the CGRAM 
 * 
 * @param addr address of CGRAM where character to be printed id located 
 * 
 *  has to be a value between 0 and 7
 */
int  lcd_print_custom(const struct device *dev,uint8_t addr)
{
    lcd_write_4bit(dev,addr,false);
    return 0;
}

//------------------
// clear display 
int lcd_clear(const struct device *dev)
{
   // Clear Display (0x01)
    lcd_write_4bit(dev,LCD_CLEAR_DISPLAY , true);
    return 0;
}

//---------------------
// return cursor to home position 
int lcd_home(const struct device *dev)
{
    lcd_write_4bit(dev,LCD_RETURN_HOME , true);
    return 0;
}




int lcd_cursor(const struct device *dev, enum state state)
{
    switch (state)
    {
    case on:
         lcd_write_4bit(dev,LCD_DISPLAYCONTROL | LCD_DISPLAYON | LCD_CURSORON | LCD_BLINKOFF , true);
        break;
    case off:
        lcd_write_4bit(dev,LCD_DISPLAYCONTROL | LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF , true);
    default:
        break;
    }
    return 0;
}



int lcd_backlight(const struct device *dev,enum state st)
{
     struct lcm1602_data * const data=( struct lcm1602_data* const )dev->data;
    switch (st)
    {
    case on:
          lcd_write_4bit(dev,LCD_DISPLAYCONTROL |  LCD_BLINKON , true);
        break;
    case off:
        const struct lcm1602_config *cfg=(const struct lcm1602_config*)dev->config;
        data->lcd_backlight_state=~(1<<3);
        i2c_write_dt(&cfg->i2c,&data->lcd_backlight_state,1);
        break;
    default:
        break;
    }
    return 0;
}

// void lcd_write_instruction(uint8_t inst)
// {
    
//       lcd_write_4bit(inst,true);
// }
// void lcd_write_data(uint8_t data)
// {
    
//     lcd_write_4bit(data,false);
// }
static DEVICE_API(lcm1602,lcm1602_api)={
    .cursor_blink=&lcd_cursor_blink,
    .shift_display=&lcd_shift_display,
    .print=&lcd_print,
    .set_cursor=&lcd_set_cursor,
    .create_custom_char=&lcd_create_custom_char,
    .print_custom=&lcd_print_custom,
    .clear=&lcd_clear,
    .home=&lcd_home,
    .cursor=&lcd_cursor,
    .backlight=&lcd_backlight
};
static inline int z_impl_lcd_print(const struct device *dev,char *character,int len)
{
    __ASSERT_NO_MSG(DEVICE_API_IS(lcm1602,dev));
    return DEVICE_API_GET(lcm1602,dev)->print(dev,character,len);
}
static inline int z_impl_lcd_set_cursor(const struct device *dev,uint8_t row,uint8_t col)
{
    __ASSERT_NO_MSG(DEVICE_API_IS(lcm1602,dev));
    return DEVICE_API_GET(lcm1602,dev)->set_cursor(dev,row,col);
}
static inline int z_impl_lcd_create_custom_char(const struct device *dev,char *rows,uint8_t addr)
{
    __ASSERT_NO_MSG(DEVICE_API_IS(lcm1602,dev));
    return DEVICE_API_GET(lcm1602,dev)->create_custom_char(dev,rows,addr);
}
static inline int z_impl_lcd_cursor_blink(const struct device *dev,enum state state){
    __ASSERT_NO_MSG(DEVICE_API_IS(lcm1602,dev));
    return DEVICE_API_GET(lcm1602,dev)->cursor_blink(dev,state);
}

static inline int z_impl_lcd_shift_display(const struct device *dev,enum shift_dir dir)
{
    __ASSERT_NO_MSG(DEVICE_API_IS(lcm1602,dev));
    return DEVICE_API_GET(lcm1602,dev)->shift_display(dev,dir);
}


static inline int z_impl_lcd_print_custom(const struct device *dev,uint8_t addr)
{
    __ASSERT_NO_MSG(DEVICE_API_IS(lcm1602,dev));
    return DEVICE_API_GET(lcm1602,dev)->print_custom(dev,addr);
}

static inline int z_impl_lcd_clear(const struct device *dev)
{
      __ASSERT_NO_MSG(DEVICE_API_IS(lcm1602,dev));
    return DEVICE_API_GET(lcm1602,dev)->clear(dev);
}
static inline int z_impl_lcd_home(const struct device *dev)
{
     __ASSERT_NO_MSG(DEVICE_API_IS(lcm1602,dev));
    return DEVICE_API_GET(lcm1602,dev)->home(dev);
}

static inline int z_impl_lcd_cursor(const struct device *dev, enum state state)
{
    __ASSERT_NO_MSG(DEVICE_API_IS(lcm1602,dev));
    return DEVICE_API_GET(lcm1602,dev)->cursor(dev,state);
}
static inline int z_impl_lcd_backlight(const struct device *dev,enum state st)
{
    __ASSERT_NO_MSG(DEVICE_API_IS(lcm1602,dev));
    return DEVICE_API_GET(lcm1602,dev)->backlight(dev,st);
}

#define LCM1602_DEFINE(inst) \
    static const struct lcm1602_config lcm1602_config##inst={ \
        .i2c=I2C_DT_SPEC_INST_GET(inst), \
        .num_lines=DT_PROP(DT_INST(inst,lcm1602),num_lines), \
    }; \
    static  struct lcm1602_data lcm1602_data##inst; \
    DEVICE_DT_INST_DEFINE(inst,\
            lcm1602_init, \
            NULL, \
            &lcm1602_data##inst, \
            &lcm1602_config##inst, \
            POST_KERNEL, \
            CONFIG_I2C_INIT_PRIORITY, \
            &lcm1602_api, \
    );

DT_INST_FOREACH_STATUS_OKAY(LCM1602_DEFINE)

