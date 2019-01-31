# G5_Bulk_programmer
A simple AVR microcontroller programmer project, it consists of
* A hardware circuit that connects the PC with master programmer and further to the slave devices and manages the voltage between the pins
* An Interface with PC to convert *.c files into binary format and transfare it to the programmer
* Programmer code that recieves the binaries through the USB protocol and programs slave devices using SPI protocol
