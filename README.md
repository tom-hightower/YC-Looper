# YC-Looper
Looper add-on designed with the Yamaha YC88 in mind, but compatible with anything that has an audio output. It can also be built as a standalone unit with input/output jacks or as an effects pedal with minimal modification.

## Hardware

YC Looper is powered by a Teensy 4.0 microcontroller with the Teensy Audio Adapter daughterboard and a custom pcb (design files to be added) for signal routing, mounting of select components, and power management.

The following hardware components are used:

- (1x) [Teensy 4.0](https://www.pjrc.com/store/teensy40.html)
  - Teensy 4.1 can easily be swapped in, 3.x requires code modification
- (1x) [Teensy Audio Adaptor Board](https://www.pjrc.com/store/teensy3_audio.html)
  - Revision D for Teensy 4.x
- (1x) 128x64px I2C OLED display
  - I used [a cheap one off amazon](https://www.amazon.com/HiLetgo-Serial-128X64-Display-Color/dp/B06XRBTBTB), but the [Adafruit 0.96" OLED](https://www.adafruit.com/product/326) might be more reliable (make sure it's set to i2c mode)
- (4x) Momentary pushbutton switches
  - I used 3 of these [19mm Momentary Push Button Switches](https://www.amazon.com/dp/B082M9F69B) for Loop Triggering and a similar but smaller switch with integrated LED for back/tempo display
- (5x) LEDs and appropriate resistors (~47ohm for my red 5mm leds)
  - 4x LEDs / 5x resistors if one of your buttons has a built in lamp like my back button
- (1x) Rotary Encoder with push switch (Keyes ky-040 or similar)
- (2x) Mono 1/4" Jacks for audio in/out
  - You can also hardwire your I/O or use stereo jacks with minimal code modification
- Breadboard, perfboard, or custom PCB (TODO: add pcb design files)
  - hook everything together!
