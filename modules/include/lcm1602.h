#ifndef AUXDISPLAY_LM1602_H
#define AUXDISPLAY_LM1602_H


#include<zephyr/device.h>
#include<zephyr/toolchain.h>
#include<stdbool.h>
#include<zephyr/drivers/i2c.h>
#include <zephyr/drivers/auxdisplay.h>
#include<math.h>
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



/* --- Basic Commands --- */
#define AUXDISPLAY_CLEAR_DISPLAY        0x01
#define AUXDISPLAY_RETURN_HOME          0x02

/* --- Entry Mode Set --- */
#define AUXDISPLAY_ENTRY_MODE_SET       (1 << 2)
// shift  cursor on read/write 
#define AUXDISPLAY_INCREMENT_DDRAM_ADDR_RW (AUXDISPLAY_ENTRY_MODE_SET | (1 << 1))
#define AUXDISPLAY_DECREMENT_DDRAM_ADDR_RW (AUXDISPLAY_ENTRY_MODE_SET )
// shift entire display depending on above configurations 
#define AUXDISPLAY_SHIFT_DISPLAY_RW         (AUXDISPLAY_ENTRY_MODE_SET ) |(1<<0)

/* --- Display Control (On/Off, Cursor, Blink) --- */
#define AUXDISPLAY_DISPLAY_CONTROL      (1 << 3)
#define AUXDISPLAY_ON                   (AUXDISPLAY_DISPLAY_CONTROL | (1 << 2))
#define AUXDISPLAY_OFF                  AUXDISPLAY_DISPLAY_CONTROL
#define AUXDISPLAY_CURSOR_ON            (AUXDISPLAY_DISPLAY_CONTROL | (1 << 1))
#define AUXDISPLAY_CURSOR_OFF           AUXDISPLAY_DISPLAY_CONTROL
#define AUXDISPLAY_CURSOR_BLINK_ON      (AUXDISPLAY_DISPLAY_CONTROL | (1 << 0))
#define AUXDISPLAY_CURSOR_BLINK_OFF     AUXDISPLAY_DISPLAY_CONTROL

/* --- Cursor or Display Shift --- */
#define AUXDISPLAY_SHIFT_CONTROL        (1 << 4)
#define AUXDISPLAY_DISPLAY_SHIFT_RIGHT  (AUXDISPLAY_SHIFT_CONTROL | (1 << 3) | (1 << 2))
#define AUXDISPLAY_DISPLAY_SHIFT_LEFT   (AUXDISPLAY_SHIFT_CONTROL | (1 << 3))
#define AUXDISPLAY_CURSOR_SHIFT_LEFT    AUXDISPLAY_SHIFT_CONTROL
#define AUXDISPLAY_CURSOR_SHIFT_RIGHT   (AUXDISPLAY_SHIFT_CONTROL | (1 << 2))

/* --- Function Set --- */
#define AUXDISPLAY_FUNCTION_SET         (1 << 5)
#define AUXDISPLAY_FUNCTION_8BIT        (AUXDISPLAY_FUNCTION_SET | (1 << 4))
#define AUXDISPLAY_FUNCTION_4BIT        AUXDISPLAY_FUNCTION_SET
#define AUXDISPLAY_FUNCTION_2LINE       (AUXDISPLAY_FUNCTION_SET | (1 << 3))
#define AUXDISPLAY_FUNCTION_1LINE       AUXDISPLAY_FUNCTION_SET
#define AUXDISPLAY_FUNCTION_5X8         AUXDISPLAY_FUNCTION_SET
#define AUXDISPLAY_FUNCTION_5X11        (AUXDISPLAY_FUNCTION_SET | (1 << 2))

/* --- CGRAM & DDRAM Addressing --- */
#define AUXDISPLAY_SET_CGRAM_ADDR       (1 << 6)
#define AUXDISPLAY_SET_DDRAM_ADDR       (1 << 7)

/* --- I2C Pin Mapping --- */
#define AUXDISPLAY_RS_PIN               0x01  /* P0 - Register Select */
#define AUXDISPLAY_RW_PIN               0x02  /* P1 - Read/Write */
#define AUXDISPLAY_EN_PIN               0x04  /* P2 - Enable */
#define AUXDISPLAY_BACKLIGHT_PIN        0x08  /* P3 - Backlight */

#define AUXDISPLAY_MODE_1_LINE_1_MAX_ADDR 0X4F
#define AUXDISPLAY_MODE_2_LINE_1_MAX_ADDR 0X27
#define AUXDISPLAY_MODE_2_LINE_2_MAX_ADDR 0X67
#define AUXDISPLAY_LINE_1_START_ADDR 0x0
#define AUXDISPLAY_LINE_2_START_ADDR 0x40


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

struct lcm1602_config{
    struct i2c_dt_spec i2c;
    uint8_t display_mode; // can either be  2 line or 1 line 
    uint8_t idl;  // interface data length  either 8 bit or 4 bit 
    const char *dots;

};

enum ddram_addr_dir
{
    increase=0,decrease
};
struct lcm1602_data
{
    uint8_t backlight;
    // holds cursor psosition data 
    uint8_t x,y ;
    // registers 
    /** 
     * @brief entry_mode stores bit states of    
                1. increment or decrement of cursor position on write 
                2. shifting display left or right on write 

     */
    uint8_t entry_mode;

    /** 
    * @brief display_control store bit info on 
    *        1. display on/off
    *        2. cursor on/off
    *        3. cursor blink on/off
    */
   uint8_t display_control;
   /**
    * @brief funstion_set: stores bit info on 
    *       1.  data length 8/4 bit 
    *       2.  line numbers 
    *       3.  5x8 or 5x11 dots 
    */
   uint8_t  function_set ; 
    enum auxdisplay_direction dir ;
    // count the number of times the display has been shifted
    // 0 for non , a negative number for left and a positive number for right
    int8_t display_shift_count;
};

int lcm1602_cursor_shift_set(const struct device *dev, uint8_t direction, bool display_shift);
int lcm1602_write(const struct device *dev,
                  const uint8_t *data,
                  uint16_t len);

uint8_t compute_ddram_addr(const struct device *dev, uint8_t x, uint8_t y);
int lcm1602_cursor_position_set (const struct device *dev, enum auxdisplay_position type, int16_t x, int16_t y);

#endif  /* AUXDISPLAY_LM1602_H */