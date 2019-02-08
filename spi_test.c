#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>

#include <avr/interrupt.h>
#include <avr/wdt.h>

#include "usbdrv.h"

#include "spi.h"

#define SCK  7
#define MISO  6
#define MOSI  5
#define SS  4

int program_counter=0;

int DEVICES[] = {1,0,0,0};// slave devices status

unsigned char small_prog[] = {12, 148, 42, 0, 12, 148, 0, 0, 12, 148, 0, 0, 12, 148
, 0, 0, 12, 148, 0, 0, 12, 148, 0, 0, 12, 148, 0, 0, 12, 148, 0, 0, 12, 148,
 0, 0, 12, 148, 0, 0, 12, 148, 0, 0, 12, 148, 0, 0, 12, 148, 0, 0, 12, 148,
  0, 0, 12, 148, 0, 0, 12, 148, 0, 0, 12, 148, 0, 0, 12, 148, 0, 0, 12, 148,
   0, 0, 12, 148, 0, 0, 12, 148, 0, 0, 248, 148, 238, 39, 236, 187, 241, 224,
    245, 191, 229, 191, 141, 224, 162, 224, 187, 39, 237, 147, 138, 149, 233,
     247, 128, 224, 152, 224, 160, 230, 237, 147, 1, 151, 233, 247, 239, 229,
      237, 191, 232, 224, 238, 191, 192, 230, 210, 224, 12, 148, 68, 0, 239,
       239, 234, 187, 225, 224, 4, 208, 224, 224, 2, 208, 251, 207, 255, 207,
        235, 187, 164, 239, 177, 224, 0, 192, 16, 150, 57, 240, 168, 149, 128,
         237, 151, 224, 1, 151, 241, 247, 17, 151, 201, 247, 8, 149
         };

void enum_devices(){
    int i;
    for (i=0 ;i<4;i++){
        if (start_prog(i,0)==0){
            DEVICES[i] = 1;
        }    
    }
}


// Declare your global variables here
USB_PUBLIC uchar usbFunctionWrite(uchar *data, uchar len) {
    return 0;
}

USB_PUBLIC uchar usbFunctionSetup(uchar data[8])
 {
    return 0;
}
int main(void){
    int index;
    DDRC = 0xff;
    DDRD = 0xff;
    DDRA = 0xff;
    DDRB |= (1<<MOSI) | (1<<SCK) | (1<<SS);
    enum_devices();

    
    for (index=0;index<4;index++){
        if (DEVICES[index]==1){
            start_prog(index,0);
            chip_erase();
            _delay_ms(1000);
    
            spi_disable();
    
            while (program_counter<sizeof(small_prog)){
                int diff=(sizeof(small_prog)-program_counter);
                load_page(index,&small_prog[program_counter],diff>128?128:diff);
                write_page(program_counter/128);
                program_counter+=128;     
            }
            program_counter=0;
    
            PORTD=0xff;                 
        }  
    }
   
    
    while (1){
        PORTD=0x00;
        _delay_ms(1000);
        PORTD=0xff;
        _delay_ms(1000);
    };   
    return 0;        
}