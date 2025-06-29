# TinyOscilloscop
整了个STM32示波器，目前可以达到1MHz采样率。  
This is tiny STM32 Oscilloscope which can reach 1MHz sample rate. 

## Thank to
Kontor

## Hardware
STM32F103RCT6  
0'96 inch 128*64 OLED  
4 Button 
8 MHz Crystal Oscillator（晶振）  
32.768 RTC Osc

## Pin Definition
GPIOB.9 PWM Sync Output  PWM输出，与ADC采样同步  
GPIOA.6 ADC Channel 0 Input  ADC输入，示波器测量引脚  
GPIOA.7 200Hz SPWM Output  PWM输出，200Hz测试信号SPWM波（菜单显示时按Key0键开启）  
GPIOA.4-5 Sine Function Wave Output  DAC输出，Sin函数测试信号（菜单显示时按Key0键开启）  
GPIOA.9-10 USART1@115200 which was used in Virtual Oscilloscope  
GPIOA.9-10 使用串口1与上位机通信，波特率115200

## 更新日志：

V2.1 2020.11.12 14:32  
添加了我自己也看不懂的注释  
添加了频率计  
添加了异步模式下的提示界面  
给上位机额外添加了两个可用的采样频率  
变更了采样频率可选项为递增模式  
V2.2 2020.11.23 15:59  
添加了FFT功能（菜单不显示时按Key0切换示波器/FFT）  
V2.3 2022.6.28 14:26  
移除了考研时钟功能（普通时钟仍然需要完整的RTC功能）  
将主频修改为56MHz（7倍频）  
添加了1MHz采样功能  
彻底修复了OLED驱动问题（大概）  
修复了部分bug  
添加了部分bug  
Removed Herobrine  

V3.0 Alpha 2023.2.11 23:13
此版本为测试版，UI和代码并未统一到相同风格，不代表最终品质
修改了示波器模式的部分UI设计，添加了波形生成功能，开机/重启时按Key0按键进入
添加了XY模式（双通道同步采样），开机/重启时按Key1按键进入
添加了波形播放模式（仅播放+OLED显示），开机/重启时按Key2按键进入
修复了部分bug
添加了部分bug
Removed Herobrine

注：
AD文件中元件标注不完全正确，请随缘购买

## Demonstration
使用演示可见本人Bilibili主页  
You can learn how to use it here:  
https://www.bilibili.com/video/BV1gV41117Bu
