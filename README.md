# SHEGO16-QMK-Hall-Effect-Macropad
An RP2040-powered 4x4 15 key macropad with a knob on the top row, that runs custom QMK firmware that allows analogue hall effect sensors to work, also has SOCD, works with VIA and SignalRGB
![ Alt text](/images/shego16.jpg) ![ Alt text](/images/shego16-2.jpg) ![ Alt text](/images/shego16-3.jpg)

### Features
- QMK Firmware
- VIA Compatible
- Hall effect (adjustable actuation also)
- SOCD (last input wins on A and D)
- Works with SignalRGB
- ST7735 TFT Screen *(currently disabled/broken due to complications and implementing another way)*
- Per-key RGB
- Ambient strip
- Custom keycodes to send to UART to control esp32 *(for tft screen on my future build)* etc
- Multiple GPIO broken out for testing purposes
- Blink LED on GPIO25

I cant find a Hall effect keyboard that i genuinely like that has all the features that i want so i thought to make one myself. Technically i am still in prototype phase for the 'endgame' board i yearn to have, this was a originally a little test board that I was making so I could figure out how to do firmware. I've been working months on end on this project with many pcbs and components surrounding me. The end goal is a 75% keyboard, knob, a tft screen that can play gifs, **north** facing RGB (my goodness i hate south facing), and magnetic switches with socd. Sadly I think it would be too difficult to create a software/web utility to control certain features of the board so thats why after weeks on end trying to build firmware from scratch with C, then scrapped that and tried Rust, then scrapped again and thought to use heavily edited qmk, i am now here.
For my big 75%, the screen will now be implemented by using an [ESP32 running a GIF decoder](https://github.com/Charading/ESP32-GIF-Player ) that accepts UART commands and it uses SD card for storage

Thankyou [riskable](https://github.com/riskable)  for your amazing resources and being my inspiration for this passion project. 


This was one of my first ever pcbs
![ Alt text](/images/shego16_pcb-front.jpg)
![ Alt text](/images/shego16_pcb-back.jpg)

I recommend using [this version](https://github.com/Charading/qmk_picosdk
) of qmk for easy building of firmware, i could not get pico/stdlib.h to be located at all with normal qmk_firmware :(. 
