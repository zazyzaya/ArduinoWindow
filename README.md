# Synthetic Window
To be affixed to a fake window with frosted glass. 

Add a `secrets.h` file containing

    #define LAT [your latitude]
    #define LON [your longitude]

and upload to your preferred microcontroller. (Code assumes you are using an Arduino Nano Every)

Tracks the position of the sun and changes the colors of the LEDs to match below: 

![colors](img/led_colors.png)

Wire everything up according to the following circuit diagram and your apartment will finally have a window: 

![diagram](img/schematic.png)
