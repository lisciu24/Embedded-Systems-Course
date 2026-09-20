# Embedded-Systems-Course 
This repository contains laboratory exercises (directories lab0_) and course project done during embedded systems course at university. 
Course focused on low level programming in C in Keil &mu;Vision IDE using Open1768 board with LPC1768 MCU onboard. During course we worked in pairs.
I had pleasure to work with [Patryk Kowalczyk](https://github.com/KowalczykPatryk). 

## Laboratory classes covered following topics
| Directory | Topic | Main task |
| -- | -- | -- |
| lab02 | LED, Buttons and SysTick | Control LED and read buttons using CMSIS libraries, configure and measure time with SysTick. |
| lab03 | UART | Establish UART connection between Open1768 board and PC using registers and CMSIS library. |
| lab04 | Timers, Interrupts, NVIC | Enable and handle interrupts from different sources (buttons, timers, lcd touch). |
| lab05 | LCD, touchpanel, external libraries | Add external libraries to project, draw basic shapes and text on LCD, touchpanel calibration. |
| lab06 | DMA | Create simple LED pattern using DMA and looped LLI |

## Project 
Our course project topic was function generator controlled via UART and LCD with touchpanel. The goal was to achieve following functionalities: 
 * change amplitude and frequency of generated function,
 * control generator through UART interface,
 * generate function from function graph drawn on LCD.

The project was utilizes three main modules: 
 * LCD/TP - interface used for drawing on screen reading input from touchpanel and performing necessary calibration.
 * UART/Command parser - communication with PC, decoding and handling received UART commands.
 * Function generator - generating pre-loaded functions or drawn on LCD using DMA and DAC.

Whole project is described in detail (in Polish) in project_report.pdf. 
