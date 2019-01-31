#include <avr/io.h>
#include <util/delay.h>
#include "spi.h"

#define SCK  7
#define MISO  6
#define MOSI  5
#define SS  4



// Declare your global variables here
int PROG_EN[4] = {0xac,0x53,0xff,0xff};

void send_byte_spi(int b){
    SPDR = b;
    while(!(SPSR &(1<<SPIF)));
}

void spi_disable(){
    SPCR=0;
} 

void spi_enable(){
    SPCR |= (1<<SPE) | (1<<MSTR) | (1<<SPR1) | (1<<SPR0) ;
    //SPSR |= (1<<SPI2X);
}

int start_prog(int device,int err_count){
    int i;
    int err = 0;
                              
    PORTA &= ~(1<<(device*2)) ;  // reset of device 
    PORTB &= ~(1<<SCK);

    _delay_ms(20);
        
    PORTA |= (1<<(device*2));
    _delay_us(1);  
    PORTA &= ~(1<<(device*2));
                 
    spi_enable();
  
   for (i=0;i<4;i++){
         send_byte_spi(PROG_EN[i]);
         if (i==2 && SPDR != PROG_EN[1] ){
            err=1;
            err_count++;
         }
   }
   if (err){  
        spi_disable();
        if (err_count==10){
            return 1;
        }else{
            return start_prog(device,err_count);
        }
   }else{
        //PORTC=(1<<(device*2));
        //_delay_ms(100);
        //PORTC= 0x00;
        return 0;   
   }
}

void  chip_erase(){
    send_byte_spi(0b10101100);
    send_byte_spi(0b10000000);
    send_byte_spi(0xff);
    send_byte_spi(0xff);
    _delay_ms(15);
}
void load_page(int device,unsigned char page[],int len){
    start_prog(device,0);
    PORTA = 0x00;// prevent other slaves from executing by activating RESET
    int byte=0;

    PORTC = (1<<(device*2));
    while (byte<len){
        int word = (byte)/2;
        if (byte%2==0){
            // send low byte
            send_byte_spi(0x40);
        }else{
            // send high byte
            send_byte_spi(0x48);
        }
        send_byte_spi(0);
        send_byte_spi(word & 0b111111);
        send_byte_spi(page[byte]);
        if (SPDR!= word){
             continue; 
        }
        
        byte++;  
    }
    PORTC = 0;

}
void write_page(int page_number){
    int low_byte;
    
    send_byte_spi(0x4c);
    send_byte_spi((page_number)>>2);
    low_byte=(page_number<<6);
    low_byte=low_byte & (3<<6);
    send_byte_spi(low_byte);
    send_byte_spi(0);
    if (SPDR!=low_byte){
        write_page(page_number);
        return;
    } 
    _delay_ms(15);
}

int read_byte(int addr){
    if (addr%2==0){
        send_byte_spi(0x20);
    }else{
        send_byte_spi(0x28);
    }
    send_byte_spi((addr>>8)&0b111111);
    send_byte_spi(addr&&0b11111111);
    send_byte_spi(0xff);
    return SPDR ;
}