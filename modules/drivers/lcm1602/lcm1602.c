#define DT_DRV_COMPAT lcm1602

#include <zephyr/kernel.h>
#include <errno.h>
#include <zephyr/drivers/i2c.h>

#include <lcm1602.h>


int auxdisplay_write_4bit(const struct device *dev, uint8_t d, bool is_instruction);


/***
 *
 * @brief function the does actual writing of the data to the lcm1602 based on the write
 * timming diagram ,does actual transfer using i2c hardware
 *
 * @param *dev pointer to the device instance
 *
 * @param val nibble to send
 *
 * @retval  returns 0 for success or an error code
 */
int controller_write(const struct device *dev, uint8_t val)
{
    // buffer and return value from an i2c write 
    uint8_t buf = 0, r = 0;
    // RW pin has to stay low during write operations
    buf = val & ~AUXDISPLAY_EN_PIN & ~AUXDISPLAY_RW_PIN;
    const struct lcm1602_config *cfg = (const struct lcm1602_config *)dev->config;
    r = i2c_write_dt(&cfg->i2c, &buf, 1);
    if (r < 0)
    {
        return r;
    }
    k_usleep(1); // tsu1 (R/W setup time) > 100ns
    // 1. Send data with EN HIGH
    buf = val | AUXDISPLAY_EN_PIN;
    r = i2c_write_dt(&cfg->i2c, &buf, 1);
    if (r < 0)
    {
        return r;
    }
    k_usleep(1); // Enable pulse width > 450ns

    // 2. Send data with EN LOW (Falling edge triggers the AUXDISPLAY)
    buf = val & ~AUXDISPLAY_EN_PIN;
    r = i2c_write_dt(&cfg->i2c, &buf, 1);
    if (r < 0)
    {
        return r;
    }
    k_usleep(40); // Execution time

    return 0;
}

/***
 * @brief write  2 nibbles to the lcm1602 by splitting the 8bit data into nibbles and
 * calling controller_write
 *
 * @param *dev pointer to device instance
 *
 * @param is_instruction this is set to true for instructions
 *
 * @retval int returns 0 for success or an error code for failure
 */
int auxdisplay_write_4bit(const struct device *dev, uint8_t d, bool is_instruction)
{
    uint8_t rs_mask = is_instruction ? 0 : AUXDISPLAY_RS_PIN;
    struct lcm1602_data *data = (struct lcm1602_data *)dev->data;
    uint8_t backlight = data->backlight; // Usually 0x08

    uint8_t high_nibble = (d & 0xF0);
    uint8_t low_nibble = ((d << 4) & 0xF0);
    int ret = 0;
    // Write High Nibble
    ret = controller_write(dev, high_nibble | rs_mask | backlight);
    if (ret < 0)
    {
        return ret;
    }
    // Write Low Nibble
    ret = controller_write(dev, low_nibble | rs_mask | backlight);
    if (ret < 0)
    {
        return ret;
    }
    return 0;
}

int update_ddram_addr(const struct device *dev, uint8_t offset)
{
    const struct lcm1602_config *cfg = (const struct lcm1602_config *)dev->config;
    struct lcm1602_data *const drv_data = (struct lcm1602_data *const)dev->data;
    
    // Compute new position in one line based on direction
    int16_t new_x = drv_data->x + (drv_data->dir==AUXDISPLAY_DIRECTION_RIGHT? offset : -offset);

    if (cfg->display_mode == 1) 
    {
        new_x=fmax(0,fmod(new_x,AUXDISPLAY_MODE_1_LINE_1_MAX_ADDR ));
    } else 
    {
        if (drv_data->y == 0) 
        {
            if (new_x > AUXDISPLAY_MODE_2_LINE_1_MAX_ADDR)
            {
                // position data is not stored as addresses but as x,y positions
            // e.g x=0,y=1 represents address 0x40
            new_x=fmax(0,fmod(new_x,AUXDISPLAY_MODE_2_LINE_1_MAX_ADDR))-1;
                // update y to  next line 
               drv_data->y = 1;
            } 
              
        } 
        else {
            if (new_x < AUXDISPLAY_LINE_2_START_ADDR) {
                drv_data->y=0;
            } 
            else if(new_x>AUXDISPLAY_MODE_2_LINE_2_MAX_ADDR)
            {
                 new_x=fmod(new_x,AUXDISPLAY_MODE_2_LINE_1_MAX_ADDR)-1;
                 drv_data->y=0;
            }
            else{
                new_x=fmod(new_x,AUXDISPLAY_MODE_2_LINE_2_MAX_ADDR);
            }
            
        }
    }

    drv_data->x = (uint8_t)new_x;
    return 0;
}
int lcm1602_write ( 	const struct device * 	dev,
		const uint8_t * 	data,
		uint16_t 	len )
        {
             int r;
            const struct lcm1602_config *cfg = (const struct lcm1602_config *)dev->config;
            struct lcm1602_data *const drv_data = (struct lcm1602_data *const)dev->data;
            if(cfg->idl==4)
            {
                for(int i=0; i<len; ++i)
                {
                    r=auxdisplay_write_4bit(dev, *(data+i),false);
                    if(r<0)
                    {
                        return -EINVAL;
                    }

                }
            // 
          
            }
            else
            {
                //  ignore 8 bit mode for now
                return -ENOSYS;
            }
            r=update_ddram_addr(dev,len);
            return r;
        }
/***
 * @brief initialize the lcm1602 and set it up for 4 bit mode
 *
 * @param *dev pointer to a device instance
 *
 * @retval  returns 0 for success or an error code
 */
int lcm1602_init(const struct device *dev)
{

    /*
    Even with perfect timing in auxdisplay_write_4bit, the AUXDISPLAY will not respond if it hasn't been put into 4-bit mode correctly first.
     When the AUXDISPLAY powers on, it starts in 8-bit mode.

    To get into 4-bit mode, you cannot just call auxdisplay_write_4bit.
    You must send the "Function Set" command (0x03) as a single nibble three times to "wake it up" before you can start sending dual-nibble commands.

    pseudo-code

    Wait > 15ms after power-up.

    Send 0x30 (high nibble only) via I2C, pulse EN.

    Wait > 4.1ms.

    Send 0x30 (high nibble only), pulse EN.

    Wait > 100µs.

    Send 0x30 (high nibble only), pulse EN.

    Send 0x20 (high nibble only), pulse EN. <— This officially switches it to 4-bit mode.

    Now you can use your auxdisplay_write_4bit function for everything else.
    */
    /* Wait for AUXDISPLAY power-up (>40ms) */
    k_msleep(50);
    const struct lcm1602_config *cfg = (const struct lcm1602_config *)dev->config;
    struct lcm1602_data *const data = (struct lcm1602_data *const)dev->data;
    //  reset all data variables 
    data->function_set=0;
    data->display_control=0;
    data->entry_mode=0;
   data->x=0;
   data->y=0;
    
    // Initial state: RS=0, RW=0, EN=0, Backlight=On
    uint8_t startup_flags = AUXDISPLAY_BACKLIGHT_PIN;
    data->backlight = AUXDISPLAY_BACKLIGHT_PIN;

    if(cfg->idl==4){
        // 2. First special "Wake up" attempt (Instruction 0x30)
    // We send this as a high nibble only.
    // set AUXDISPLAY to 8 bit mode by sending insruction 0x30
    int r = 0;
    r = controller_write(dev, AUXDISPLAY_FUNCTION_8BIT | startup_flags);
    if (r < 0)
    {
        return r;
    }
    k_msleep(5); // Must wait > 4.1ms

    // 3. Second special "Wake up" attempt
    r = controller_write(dev, AUXDISPLAY_FUNCTION_8BIT | startup_flags);
    if (r < 0)
    {
        return r;
    }
    k_usleep(150); // Must wait > 100us

    // 4. Third special "Wake up" attempt
    r = controller_write(dev, AUXDISPLAY_FUNCTION_8BIT | startup_flags);
    if (r < 0)
    {
        return r;
    }
    k_msleep(1);

    // 5. Set to 4-bit mode (Instruction 0x20)
    // After this command, the AUXDISPLAY is officially in 4-bit mode
    r = controller_write(dev, AUXDISPLAY_FUNCTION_4BIT | startup_flags);
    if (r < 0)
    {
        return r;
    }
    k_msleep(1);
    }
    else
    {
        // ignore 8 bit mode for now
        return -ENOSYS;
    }
    

    // Function Set: 2 lines, 5x8 font (0x20)
    uint8_t bit_mask=0;
    bit_mask|=cfg->idl==4?AUXDISPLAY_FUNCTION_4BIT :AUXDISPLAY_FUNCTION_8BIT;
    bit_mask |= strcmp((const char*)cfg->dots,"5x8")==0? AUXDISPLAY_FUNCTION_5X8 :AUXDISPLAY_FUNCTION_5X11;
    bit_mask |= cfg->display_mode==2 ? AUXDISPLAY_FUNCTION_2LINE :AUXDISPLAY_FUNCTION_1LINE;
     int r = auxdisplay_write_4bit(dev,bit_mask, true);
        if (r < 0)
        {
            return r;
        }
    
    // Display Control: Display ON
    data->display_control|=AUXDISPLAY_ON;
    r = auxdisplay_write_4bit(dev, data->display_control  , true);
    if (r < 0)
    {
        return r;
    }
    // Clear Display (0x01)
    r = auxdisplay_write_4bit(dev, AUXDISPLAY_CLEAR_DISPLAY, true);
    if (r < 0)
    {
        return r;
    }
    k_msleep(2); // Clear command needs ~1.6ms to execute

    // Entry Mode: Increment cursor, no shift (0x06)
    data->entry_mode|=AUXDISPLAY_DECREMENT_DDRAM_ADDR_RW;
    r = auxdisplay_write_4bit(dev, data->entry_mode, true);
    if (r < 0)
    {
        return r;
    }
    lcm1602_cursor_shift_set(dev,1,false);
    // set the auxiliaty display cursor position to  row 0 , column 0
    // this will help with tracking the cursor position
    lcm1602_cursor_position_set(dev,AUXDISPLAY_POSITION_ABSOLUTE,0,0);
    return 0;
}
int lcm1602_clear 	( 	const struct device * 	dev	) 	
{
    return auxdisplay_write_4bit(dev, AUXDISPLAY_CLEAR_DISPLAY, true);

}
int lcm1602_display_on(const struct device *dev)
{
    int r = 0;
    struct lcm1602_data *const data = (struct lcm1602_data *const)dev->data;
    //const struct lcm1602_config *cfg = (const struct lcm1602_config *)dev->config;
    
        // update the entry mode variable
         data->display_control |=AUXDISPLAY_ON;

        r = auxdisplay_write_4bit(dev,data->display_control,true);

    return r;
}
/**
 * @brief turn display bakclight off
 */
int lcm1602_display_off(const struct device *dev)
{
    int r = 0;
    struct lcm1602_data *const data = (struct lcm1602_data *const)dev->data;
    //const struct lcm1602_config *cfg = (const struct lcm1602_config *)dev->config;
    
   
        // change state to off  rest  display on bit 
        // this clears the  display control bit 
        data->display_control &=~AUXDISPLAY_ON;
        // set  display control bit 
        data->display_control |=AUXDISPLAY_DISPLAY_CONTROL;

        r = auxdisplay_write_4bit(dev,data->display_control,true);
     
    return r;
}


int	lcm1602_cursor_set_enabled (const struct device *dev, bool enabled)

{
    int r = 0;
    struct lcm1602_data *const data = (struct lcm1602_data *const)dev->data;
    //const struct lcm1602_config *cfg = (const struct lcm1602_config *)dev->config;
    if(enabled)
    {
         data->display_control |= AUXDISPLAY_CURSOR_ON;

    }
    else 
    {
        //clear the cursor on bit , this clears the display control bit 
       data->display_control &= ~AUXDISPLAY_CURSOR_ON;
       // set the display control bit 
       data->display_control |=AUXDISPLAY_DISPLAY_CONTROL;

    }
     r = auxdisplay_write_4bit(dev,data->display_control,true);
   
    return r;

}

// -----------------------------------------------------

int	lcm1602_position_blinking_set_enabled (const struct device *dev, bool enabled)
{
    int err = 0;
   struct lcm1602_data *const data = (struct lcm1602_data *const)dev->data;
    //const struct lcm1602_config *cfg = (const struct lcm1602_config *)dev->config;
    if(enabled)
    {
        data->display_control |=AUXDISPLAY_CURSOR_BLINK_ON;
    }
    else{
        data->display_control&=~AUXDISPLAY_CURSOR_BLINK_OFF;
        data->display_control |=AUXDISPLAY_DISPLAY_CONTROL;
    }
    err = auxdisplay_write_4bit(dev,data->display_control,true);
    return err;
}

//----------------------------------------------------


int lcm1602_cursor_shift_set (const struct device *dev, uint8_t direction, bool display_shift)
{
        int err = 0;
   struct lcm1602_data *const data = (struct lcm1602_data *const)dev->data;
    //const struct lcm1602_config *cfg = (const struct lcm1602_config *)dev->config;
    // direction to left 
    if(direction==0)
    {
        data->entry_mode &=~AUXDISPLAY_INCREMENT_DDRAM_ADDR_RW;
        data->entry_mode |=AUXDISPLAY_ENTRY_MODE_SET;
        // set ddram address dir to decreasing ;
        data->dir=AUXDISPLAY_DIRECTION_LEFT;
    }
    // shift right
    else if (direction==1)
    {
        data->entry_mode |=AUXDISPLAY_INCREMENT_DDRAM_ADDR_RW;
        // set ddram address to increasing
        data->dir=AUXDISPLAY_DIRECTION_RIGHT;
    }
    else{
        return -ENOSYS;
    }
    // display shift 
    if(display_shift==true)
    {
        data->entry_mode|=AUXDISPLAY_SHIFT_DISPLAY_RW ;
    }
    else{
        data->entry_mode&=~AUXDISPLAY_SHIFT_DISPLAY_RW ;
        data->entry_mode |=AUXDISPLAY_ENTRY_MODE_SET;
    }
    
     err = auxdisplay_write_4bit(dev,data->entry_mode,true);
    return err;
    
}

//----------------------------------------------------------
// set cursor position 
uint8_t compute_ddram_addr(const struct device *dev,uint8_t x,uint8_t y)
{
    const struct lcm1602_config *const cfg = (const struct lcm1602_config *)dev->config;
    //starting addr
    uint8_t addr=0x00;
   
    if (cfg->display_mode == 2)
    {
        // ensure the x  and y  are within range
        if(y==0)
        {
            addr=x;
            if(addr>AUXDISPLAY_MODE_2_LINE_1_MAX_ADDR)
            {
                addr=AUXDISPLAY_MODE_2_LINE_1_MAX_ADDR;
            }
        }
        else
        {
            addr=AUXDISPLAY_LINE_2_START_ADDR +x;
            if(addr>AUXDISPLAY_MODE_2_LINE_2_MAX_ADDR)
            {
                addr=AUXDISPLAY_MODE_2_LINE_2_MAX_ADDR;
            }
        }
    }
    else
    {
        addr=x;
            if(addr>AUXDISPLAY_MODE_1_LINE_1_MAX_ADDR )
            {
                addr=AUXDISPLAY_MODE_1_LINE_1_MAX_ADDR ;
            }
    }
    return addr;
}
//--------------------------------------------
// set_cursor

int lcm1602_cursor_position_set (const struct device *dev, enum auxdisplay_position type, int16_t x, int16_t y)
{
    //const struct lcm1602_config *const cfg = (const struct lcm1602_config *)dev->config;
    struct lcm1602_data *const data = (struct lcm1602_data *)dev->data;
    uint8_t addr;


    // Todo add checks to ensure  x and y fall within the limits of screen positions

    switch (type)
    {
    case AUXDISPLAY_POSITION_ABSOLUTE :
        addr=compute_ddram_addr(dev,x,y);
        
        data->x=(uint8_t)x;
        data->y=(uint8_t)y;
        break;
    case AUXDISPLAY_POSITION_RELATIVE:
         addr=compute_ddram_addr(dev,data->x+x,data->y+y);
         data->x +=(uint8_t)x;
         data->y+=(uint8_t)y;
         break;
    // case AUXDISPLAY_POSITION_RELATIVE_DIRECTION :
    // break;
    default:
        return -ENOSYS;
        break;
    }
    int r = auxdisplay_write_4bit(dev, AUXDISPLAY_SET_DDRAM_ADDR| addr , true);
    
    return r;
}

int lcm1602_cursor_position_get 	( 	const struct device * 	dev,
		int16_t * 	x,
		int16_t * 	y )

        {
            struct lcm1602_data *const data = (struct lcm1602_data *)dev->data;
            *x=(int16_t)data->x;
            *y=(int16_t)data->y;
            return 0;
        }
int lcm1602_custom_character_set( 	const struct device * 	dev,
		struct auxdisplay_character * 	character ) 
{
    
    int r = 0;
    //Each custom character slot in the LCD's CGRAM requires 8 bytes of memory (one byte per row). 
   //if  character index is 1, this sets the address to 0x41 instead of 0x48 (0x40 + (1 * 8)).
    //This unaligned write spills pixel data across adjacent character slots,
    //corrupting the memory layout hence need to multiply by 8 or <<3
    r = auxdisplay_write_4bit(dev, AUXDISPLAY_SET_CGRAM_ADDR | (character->index*8), true);
    struct lcm1602_data *const data = (struct lcm1602_data *)dev->data;
    if (r < 0)
    {
        return r;
    }
    
   // ddram address has to be set to increase after write 
    // for now  5x8 mode only is supported
    /// Todo  add support for 5x8 and 5x11 mode 
    for (uint8_t i = 0; i < 8; ++i)
    {
        r = auxdisplay_write_4bit(dev,*(character->data + i), false);
        if (r < 0)
        {
            return r;
        }
    }
    r=lcm1602_cursor_position_set(dev,AUXDISPLAY_POSITION_ABSOLUTE,data->x, data->y);
    r=auxdisplay_write_4bit(dev,character->index,false);
    r=lcm1602_cursor_position_set(dev,AUXDISPLAY_POSITION_ABSOLUTE,data->x+1, data->y);
    return r;
}

int lcm1602_display_position_set 	( 	const struct device * 	dev,
		enum auxdisplay_position 	type,
		int16_t 	x,
		int16_t 	y )

{

    //ignore type and y for now 
    [[maybe_unused]] y;
    [[maybe_unused]] type;
    int r;
    uint8_t cmd= x<0? AUXDISPLAY_DISPLAY_SHIFT_LEFT :AUXDISPLAY_DISPLAY_SHIFT_RIGHT;
	for(uint8_t i=0;i<fabs(x);++i)
	{
		r=auxdisplay_write_4bit(dev,cmd ,true);
        if (r<0)
        {
            return r;
        }
	}
  
    return 0;
	
}




static DEVICE_API(auxdisplay, lcm1602_api) = {
    .display_on=lcm1602_display_on,
    .display_off=lcm1602_display_off,
    .cursor_set_enabled=lcm1602_cursor_set_enabled,
    .position_blinking_set_enabled=lcm1602_position_blinking_set_enabled,
    .cursor_shift_set=lcm1602_cursor_shift_set,
    .cursor_position_set=lcm1602_cursor_position_set,
    .custom_character_set=lcm1602_custom_character_set,
    .write=lcm1602_write,
    .display_position_set=lcm1602_display_position_set,
    .cursor_position_get=lcm1602_cursor_position_get,
    .clear=lcm1602_clear,


};

#define LCM1602_DEFINE(inst)                                     \
    static const struct lcm1602_config lcm1602_config##inst = {  \
        .i2c = I2C_DT_SPEC_INST_GET(inst),                       \
        .display_mode = DT_PROP(DT_INST(inst, lcm1602), display_mode), \
        .idl=DT_PROP(DT_INST(inst, lcm1602), idl) , \
        .dots=DT_PROP(DT_INST(inst, lcm1602), format),\
    };                                                           \
    static struct lcm1602_data lcm1602_data##inst;               \
    DEVICE_DT_INST_DEFINE(inst,                                  \
                          lcm1602_init,                          \
                          NULL,                                  \
                          &lcm1602_data##inst,                   \
                          &lcm1602_config##inst,                 \
                          POST_KERNEL,                           \
                          CONFIG_I2C_INIT_PRIORITY,              \
                          &lcm1602_api, );

DT_INST_FOREACH_STATUS_OKAY(LCM1602_DEFINE)
