/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include<zephyr/kernel.h>
const struct device *lcm1602=DEVICE_DT_GET(DT_NODELABEL(lm1602));
#include<zephyr/drivers/auxdisplay.h>

int main(void)
{
	if(!device_is_ready(lcm1602))
	{
		printk(" Driver failed\n");
		return -1;
	}
	
	//  }
	
	
	auxdisplay_cursor_set_enabled(lcm1602,true);
	auxdisplay_position_blinking_set_enabled(lcm1602,false);
	 char name[]="Room Temp  ";
	auxdisplay_write(lcm1602,name,strlen(name));

	uint8_t Character[8] = {
  0b01111,
  0b01001,
  0b01111,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000
};
	struct auxdisplay_character ch =
		{
			.character_code = 0,
			.data = Character,
			.index =0,
	};
	//auxdisplay_cursor_position_set(lcm1602,AUXDISPLAY_POSITION_ABSOLUTE,1,3);
	auxdisplay_custom_character_set(lcm1602,&ch);
	
// 	uint8_t Character2[8] = {
//   0b00000,
//   0b01110,
//   0b10001,
//   0b10001,
//   0b10001,
//   0b00000,
//   0b01110,
//   0b00000
// };
	
// 	struct auxdisplay_character ch2=
// 		{
// 			.character_code = 1,
// 			.data = Character2,
// 			.index =1,
// 	};
// 	auxdisplay_custom_character_set(lcm1602,&ch2);
	char c='c';
	auxdisplay_write(lcm1602,&c,1);
	
	// auxdisplay_display_position_set(lcm1602,AUXDISPLAY_POSITION_RELATIVE_DIRECTION,-3,0);
	// for(int i=0;i<27;++i)
	//  {
	// 	auxdisplay_display_position_set(lcm1602,AUXDISPLAY_POSITION_RELATIVE_DIRECTION,-1,0);
	// 	k_msleep(300);
	//  }
	k_msleep(5000);
	// auxdisplay_clear(lcm1602);
	auxdisplay_display_off(lcm1602);
	k_msleep(2000);
	auxdisplay_display_on(lcm1602);
	 return 0;
}