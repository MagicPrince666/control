#!/bin/sh

cd /sys/class/gpio

#lcd_data0
echo 70 > export  
#lcd_data9 
echo 79 > export 
#lcd_data12
echo 8 > export
#lcd_pclk
echo 88 > export

echo in > /sys/class/gpio/gpio8/direction

echo in > /sys/class/gpio/gpio70/direction

echo in > /sys/class/gpio/gpio79/direction

echo in > /sys/class/gpio/gpio88/direction


