# CS122-FinalProject
Implement a multi-stage RC (Remote-Controlled) signal converter and a dashboard for small-robotics and ESC (Electronic Speed Controller) testing

### Full Typed report provided [dtope004-dres002_custom_lab_report.pdf](https://github.com/dtopete/CS122-FinalProject/blob/main/dtope004-dres002_custom_lab_report.pdf)

# Hardware
* **Microcontroller**: [Raspberry Pi Pico 2W (x2)](https://www.raspberrypi.com/products/raspberry-pi-pico-2/?variant=pico-2-w), [pinout](https://pip-assets.raspberrypi.com/categories/1088-raspberry-pi-pico-2-w/documents/RP-008305-DS-1-pico-2-w-pinout.pdf?disposition=inline), [C/C++ SDK](https://pip-assets.raspberrypi.com/categories/609-microcontroller-boards/documents/RP-009085-KB-1-raspberry-pi-pico-c-sdk.pdf)
* **FPGA**: [iCE Sugar Pro](https://github.com/wuxx/icesugar-pro), [pinout](https://www.jjhorton.co.uk/img/iCESugarpro-pinmap.png)
* **Display**: [4.3 inch TFT LCD 480*272 resolution RGB565 and PMOD](https://www.tindie.com/products/johnnywu/pmod-rgblcd-expansion-board/)
* **Wireless input Microcontroller**: ESP32-S3 LoRA Development Board [(ESP32-S3-LR1121-HF)](https://www.waveshare.com/esp32-s3-lr1121.htm), [documentation](https://docs.waveshare.com/ESP32-S3-LR1121-XF?variant=ESP32-S3-LR1121-HF-Kit), [custom firmware](https://github.com/KingElrond/ExpressLRS-4.0.1-Custom/tree/main)
* Backup plan if the Development board doesn't work, RadioMaster ELRS receiver (Not used)

# Software
### Software that was
* For text processing from Pico to FPGA, we are using an [framebuffer with LVGL provided to us](https://github.com/UCR-CS122A/icesugar-pro-framebuffer/tree/main)
* We used a custom ELRS reciever PCB which had to have its own [custom ELRS configuration](https://github.com/KingElrond/ExpressLRS-4.0.1-Custom/tree/main) made by Troy.

# Wiring
## Pico 2W[0]
### Codebase in [icesugar-pro-framebuffer/sw/](https://github.com/dtopete/CS122-FinalProject/tree/main/icesugar-pro-framebuffer/sw)
* **UART1** - Receives ELRS from ESP32-S3 LoRA Dev Board
* **GP15** - Sends Display Data for Pico[1]
* **SPI0** - Sends Display Data for FPGA for LCD
    * Displays Channel data, using LVGL, displays the 8 channels on the FPGA's Display
* **GP15** - Encodes channel data into PPM Signal to Pico[1] 

## Pico 2W[1]
### Codebase in ppmRead/
* Decodes PPM from Pico[0] and splits it across 8 PWM Channels

## FPGA
### Codebase in [icesugar-pro-framebuffer/hw/](https://github.com/dtopete/CS122-FinalProject/tree/main/icesugar-pro-framebuffer/hw)
* Runs icesugarpro Framebuffer

## Stage 1
Expected progress for the project

* 1. Program ELRS input to PPM output in the Pico (First half done)
* 2. This PPM output goes to the FPGA
* 3. Pico SPI0 CRFS -> PPM to FPGA

## Stage 2
* 1. Pico SPI1 CRSF -> Channel Values to FPGA-Display (done)
* 2. FPGA -> 2 channels of PWM

# Testing / Debug
## Check the CRSF via ELRS (UART)
### Used to debug incoming data from both Pico0(ELRS) and Pico1(PPM)
* `pip install pyserial`
* Locate the Pico's serial `ls /dev/cu.usbmodem11*`
* Example for my machine: `python3 -m serial.tools.miniterm /dev/cu.usbmodem113401  113200`