# SDCardDriver
### Description
Implementation of an SD card driver for STM32 via a SPI. Tested on stm32f103c8t6.
'SD Specifications Part 1 Physical Layer Simplified Specification Version 9.00' was used as documentation.

Implemented functions:
* Complete initialization process (including all types of SD cards);
* Request basic information about the SD card (size, transfer speed, etc.);
* Single and multiple reading;
* Single and multiple write;
* Erasing sectors of SD card;
* Other low-level work functions.

### Launch
* Copy this repository;
* Go to the SDCardDriver folder;
* Run the build using ```make .```;
* flash your microcontroller (I used openosd).

Or you can simply copy the [external folder](https://github.com/MatveyMelnikov/SDCardDriver/tree/master/External) into your project...

### Usage
The driver files are located [here](https://github.com/MatveyMelnikov/SDCardDriver/tree/master/External/SDCard_Driver). 
It is also necessary to implement the CRC, which located [here](https://github.com/MatveyMelnikov/SDCardDriver/tree/master/External/CRC).

The [main.с](https://github.com/MatveyMelnikov/SDCardDriver/blob/master/Core/Src/main.c) file presents the use of basic functions of working with an SD card. 
The global variable sd_card_status displays the result of card initialization (version, size, presence of errors)

Note: in [this file](https://github.com/MatveyMelnikov/SDCardDriver/blob/master/External/SDCard_Driver/Inc/sd_driver_secondary.h) you need to change the pin in the following lines, which in your case is responsible for the CS:
```
#define SELECT_SD() \
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET)

#define DISELECT_SD() \
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET)
```
### Hardware
Used during development:
* stm32f103c8t6;
* ST-LINK v2;
* Micro SD card adapter;
* SD card 16 gb;
* Logic analyzer - Kingst LA1010.

![изображение](https://github.com/MatveyMelnikov/SDCardDriver/assets/55649891/550515b3-28a6-4311-9c46-4ed95544e5bc)

![изображение](https://github.com/MatveyMelnikov/SDCardDriver/assets/55649891/d9a84951-addb-41ba-b0d5-3500cf6d137d)

![изображение](https://github.com/MatveyMelnikov/SDCardDriver/assets/55649891/65eea189-19a2-4c16-90dd-420f53f5e921)
