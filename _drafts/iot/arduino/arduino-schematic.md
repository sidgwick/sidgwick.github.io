---
title: "如何阅读 Arduino 原理图"
date: 2022-09-01 01:34:04
tags: arduino schematic
---

> 英文原文: https://learn.circuit.rocks/the-basic-arduino-schematic-diagram

![](https://learn.circuit.rocks/wp-content/uploads/2019/12/arduinogardware-e1600234850301.png)

Get deeper into the Arduino craft by looking into a reference design. In here I will get into details with the basic arduino schematic diagram using one of their more popular development board, the Arduino UNO.

<!--more-->

So what is an Arduino? It can basically mean two things:

One, it is an open-source hardware and software company that creates circuit boards that makes microcontrollers easier to use. Open-source means its source code and hardware design is freely available for anyone to modify. With the code and design free, public collaboration can develop the product way faster than any proprietary product. The only downside of giving the public access is that it gives birth to competition. It allows clones. This is the also the reason why we can only do a diagram analysis of Arduino, and not a Raspberry Pi.

Two and the more popular use of the term is that the board itself is called an Arduino.

There are a lot of tutorials for Arduino in the internet. Nearly all of them explains how to make it work but, how does it work? A good understanding of the hardware design will help you learn how to incorporate them on your machines, small-scale to large.

Here we have the schematic diagram of the latest revision of Arduino UNO. Don’t worry if you can’t see it much right now. We’ll chop it up after and analyze every section according to their function.

![](https://learn.circuit.rocks/wp-content/uploads/2019/12/arduino.png)

# Components

![](https://learn.circuit.rocks/wp-content/uploads/2019/12/Arduino_Parts_diag.jpg)

Here we can visibly identify each component on board, besides the obvious Atmega328P. Depending on the IC package, these components can appear differently. If you want to learn more about IC packages. Visit this [SparkFun](https://learn.sparkfun.com/tutorials/integrated-circuits/all) webpage.

We will divide the schematic diagram into four major sections: Power Supply, Main Microcontroller, USB bridge and lastly the Input and Output.

![](https://learn.circuit.rocks/wp-content/uploads/2019/12/arduino-1.png)

# POWER SUPPLY

![](https://learn.circuit.rocks/wp-content/uploads/2019/12/Power-1.png)

This section of the schematic diagram powers our Arduinos. For this, we can use either DC power supply or USB connection as a source. To trace how the circuit works, let us start with the 5V linear voltage regulator(5V 线性稳压器) [NCP1117ST50T3G](https://www.onsemi.com/pub/Collateral/NCP1117-D.PDF). This regulator has a pretty straightforward function. It takes voltage input up to 20V and converts it to 5V. The Vin of this regulator is connected to the DC power supply via the [M7](https://www.vtrons.com/images/DIODE%20M7.pdf) diode. This diode provides reverse polarity protection, which means current can only flow from the power supply to the regulator and not the other way around.

There are two kinds of capacitors used in this circuit. One, the polarized capacitors PC1 and PC2. They are used to filter supply noise. Two, the 100nF capacitor on the far right side acting as a decoupling capacitor. Decoupling capacitors “disconnects” circuit elements to reduce its effect on the rest of the circuit. Also, notice the +5V terminals circled in blue? You can see them around the schematic diagram. That is basically a connection. Designers often use these terminals so that they can fit their design in the least amount of pages. Labels like Vin and GND also works the same.

![](https://learn.circuit.rocks/wp-content/uploads/2019/12/power1-4-1024x378.png)

## Choosing the Power Source

If you use the USB source, power will enter through the USBVCC section into a P-channel MOSFET [FDN340P](https://www.mouser.com/datasheet/2/149/FDN340P-103694.pdf). Explaining briefly, MOSFETS are transistors(晶体管) that are normally closed (connected) until a certain amount of voltage triggers the gate. Consequently, triggering the MOSFET opens the transistor (disconnected). "触发晶体管会导致其断开". Using this characteristic, a comparator is connected to the gate terminal of the MOSFET to function as a switch mechanism. If you use the USB source, it will just power up the 3.3V regulator since the MOSFET is normally connected. If you plug a dc supply, the [LMV358](https://www.ti.com/lit/ds/symlink/lmv358-n-q1.pdf) op-amp(运算放大器) will produce a high output triggering the gate of the MOSFET, disconnecting the USB source from the 3.3V regulator. So if both are plugged, the board will use the DC power supply.

> 注意电源选择这块只使用了 LMV358 的 1/2/3 引脚. 引脚 4 (GND) 和引脚 8 (VCC) 分别接入电路里面的地线和 +5V(虽然 +5V 是经过 LMV358 比较完才知道电源是来自 USB 口还是来自 DC 口).
> TODO: LVM358 的 5/6/7 引脚是干啥的? 貌似和 USB 有关系

![](https://learn.circuit.rocks/wp-content/uploads/2019/12/power3-1.png)
![](https://learn.circuit.rocks/wp-content/uploads/2019/12/power2-3.png)

The 5V terminal here indicates a connection to the circuit powered by the DC power supply earlier. This means that the 3.3V regulator is powered by whichever powers the circuit.

![](https://learn.circuit.rocks/wp-content/uploads/2019/12/power4-2-370x223.png)

Lastly, the LED indicator circuit. Put simply, it lights the green LED up when it detects power from the 5V terminal.

# MICROCONTROLLER

![](https://learn.circuit.rocks/wp-content/uploads/2019/12/mcu.png)

This section is the main microcontroller circuit of the Arduino UNO. It has the [ATmega328P](http://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7810-Automotive-Microcontrollers-ATmega328P_Datasheet.pdf) microcontroller. We won’t go into details with the microcontroller’s specs. The only thing you need to know is that it is an 8-bit device which means it can handle 8 data lines simultaneously.

![](https://learn.circuit.rocks/wp-content/uploads/2019/12/mcuheaders-1.png)

These three are the black headers you see on board. As you can see, the TX and RX from the ATmega16U2 are connected into pins 0 and 1. The IOL and IOH headers are for pins 0 to 13 in addition to the I2C pins, GND, and AREF.

![](https://learn.circuit.rocks/wp-content/uploads/2019/12/mcucrystak.png)

There’s a [CSTCE16M0V53-R0](https://www.murata.com/en-eu/api/pdfdownloadapi?cate=&partno=CSTCE16M0V53-R0) 16MHz ceramic(陶瓷) resonator(谐振器(振荡器
) of the microcontroller. This circuit serves as the clock.

The capacitors here has the same function with the decoupling capacitors in the previous section.

![](https://learn.circuit.rocks/wp-content/uploads/2019/12/mcuicsp-2.png)

The ICSP (In-Circuit Serial Programming, 在线串行编程) Header is used to program the ATmega328P using an external programmer (e.g. Raspberry Pi). Usually, we only use this for remote programming or for flashing of the microcontroller for the first time. We don’t use this way of programming often since we can already do it via USB, which we will discuss in the next section.

# USB

![](https://learn.circuit.rocks/wp-content/uploads/2019/12/usball-1.png)

This section is the USB to UART bridge. Since the main microcontroller, ATmega328, does not have a USB transceiver, we use the [ATmega16U2](http://ww1.microchip.com/downloads/en/DeviceDoc/doc7799.pdf) microcontroller as a bridge to translate the USB signals from your computer to UART, the communication protocol the ATmega328 uses.

This is basically the same with the main microcontroller section. This MCU has capacitors, an ICSP header and a crystal as well.

![](https://learn.circuit.rocks/wp-content/uploads/2019/12/usbter-1.png)

These RN3 resistors are termination resistors. They are used so that the total impedance of the line matches with the USB specification. It slows the speed of the USB pulses lessening EMI interference. On the other hand, Z1 and Z2 are varistors. They are used for ESD (Electrostatic Discharges) protection.

> 这些 RN3 电阻器是终端电阻器。使用它们是为了使线路的总阻抗符合 USB 规范。它减慢了 USB 脉冲的速度，从而减少了 EMI 干扰。另一方面，Z1 和 Z2 是压敏电阻。它们用于 ESD（静电放电）保护。

![](https://learn.circuit.rocks/wp-content/uploads/2019/12/usbter-2.png)

When the ATmega 16U2 transmits or receives a signal from the ATmega 328P, yellow LEDs light up. We can also see here the two lines where the two microcontrollers communicate.

# I/O

![](https://learn.circuit.rocks/wp-content/uploads/2019/12/ARDUINO_V2.png)

Image courtesy of [MarcusJenkins](http://marcusjenkins.com/arduino-pinout-diagrams/)

Quite refreshing to see the board again right? By now, you should be able to appreciate how these are connected to the several electronic components in our schematic diagram. We don’t need to explain them anymore. This is an article for the basic arduino schematic diagram after all.

# 参考资料

- [Understanding Arduino UNO Hardware Design](https://www.allaboutcircuits.com/technical-articles/understanding-arduino-uno-hardware-design/)
- [ISP/ICSP - Installing an Arduino Bootloader](https://learn.sparkfun.com/tutorials/installing-an-arduino-bootloader)
- [ISP/ICSP - Using an In-System Programmer](https://www.instructables.com/Using-an-In-System-Programmer/)
- [电路图的基本符号](https://arduinotogo.com/2016/08/22/chapter-2-the-schematic/)

# TODO

- 去耦电容/降噪电容
- LMV358 5/6/7 脚在电路图中的原理是什么?
