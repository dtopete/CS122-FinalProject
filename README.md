# CS122-FinalProject
Implement a multi-stage RC (Remote-Controlled) signal converter and a dashboard for small-robotics and ESC (Electronic Speed Controller) testing

# Hardware
* Raspberry Pi Pico 2W
* FPGA: [iCE Sugar Pro](https://github.com/wuxx/icesugar-pro)
* [4.3 inch TFT LCD 480*272 resolution RGB565 and PMOD](https://www.tindie.com/products/johnnywu/pmod-rgblcd-expansion-board/)
* ESP32-S3 LoRA Development Board [(ESP32-S3-LR1121-HF)](https://www.waveshare.com/esp32-s3-lr1121.htm), [documentation](https://docs.waveshare.com/ESP32-S3-LR1121-XF?variant=ESP32-S3-LR1121-HF-Kit)
* Backup plan if the Development board doesn't work, RadioMaster ELRS receiver 

# Software
* For text processing from Pico to FPGA, we are using an[ framebuffer with LVGL provided to us](https://github.com/UCR-CS122A/icesugar-pro-framebuffer/tree/main)
* we used a custom ELRS reciever PCB which had to have its own custom ELRS configuration made for it by Troy. (Link github later)