/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include<zephyr/kernel.h>
const struct device *lcm1602=DEVICE_DT_GET(DT_NODELABEL(lm1602));
#include<lcm1602/lcm1602.h>
int main(void)
{
	if(!device_is_ready(lcm1602))
	{
		printk(" Driver failed\n");
		return -1;
	}
	
	//  }
	 char name[]="driver api";
	 lcd_print(lcm1602,name,strlen(name));
	 
	 char kite[8] =
		 {
			 0b00100,
			 0b01010,
			 0b10001,
			 0b01010,
			 0b00100,
			 0b01000,
			 0b00100,
			 0b00000};

	 lcd_create_custom_char(lcm1602,kite, 0);
	 lcd_set_cursor(lcm1602,0, 10);
	 lcd_print_custom(lcm1602,on);
	lcd_cursor(lcm1602,on);
	
	 return 0;
}