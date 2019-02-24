#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>

#include "usbdrv.h"
#include "spi.h"

#include <util/delay.h>

#define USB_LED_OFF 0
#define USB_LED_ON  1
#define USB_READ_STATUS 2
#define USB_DATA_IN 3
#define USB_INIT_PROGRAMMER 4
#define USB_PAGE_WRITE 5
#define USB_WRITE_PAGE_V2 6// version 1 didn't work XD


#define SCK  7
#define MISO  6
#define MOSI  5
#define SS  4

static uchar buffer[128] ;
static uchar write_status[] = {0,0,0,0} ; 
static int program_counter = 0; // for USB_PAGE_WRITE
static int page_counter = 0;

static int program_size;

int DEVICES[] = {0,0,0,0};// slave devices status

int cur_stat;


void enum_devices(){
    int i;
    for (i=0 ;i<4;i++){
        if (start_prog(i,0)==0){
            DEVICES[i] = 1;
        }else{
            PORTC |= (1<<((i*2)+1));
        }    
    }
}

void send_page(){
    uchar device;
    for(device=0;device<4;device++){
        if (DEVICES[device]==1){
            if (program_counter==0){
                start_prog(device,0);
                chip_erase();
                _delay_ms(1000);
    
                spi_disable();
                }
            int byte;
            load_page(device,buffer,page_counter);
            write_page(program_counter/128);
        }
    }
    page_counter=0;
    program_counter += 128;

    write_status[0]=1;
    if(program_counter >= program_size){
        spi_disable();
        PORTA = 0xff;
        PORTC=0;

        // Done programming
        uchar i;
        for(i=0;i<4;i++){
            program_counter=0;
            page_counter=0;
            if (DEVICES[i]==1){
                PORTC |= 1<<(i*2);
            }
        }
    }
}

// this gets called when custom control message is received
USB_PUBLIC uchar usbFunctionSetup(uchar data[8]) {
	usbRequest_t *rq = (void *)data; // cast data to correct type
	
	switch(rq->bRequest) { // custom command is in the bRequest field
    case USB_LED_ON:
		PORTB |= 1; // turn LED on
		return 0;
	case USB_LED_OFF: 
		PORTB &= ~1; // turn LED off
		return 0;
    case USB_READ_STATUS: // send data to PC
        usbMsgPtr = write_status;
        return sizeof(write_status);

    case USB_INIT_PROGRAMMER:
        program_size = (int)rq->wValue.word;
        write_status[3]=program_size;
        cur_stat=USB_INIT_PROGRAMMER;
        return USB_NO_MSG;
    case USB_PAGE_WRITE:
        write_status[0]=0;
        cur_stat=USB_PAGE_WRITE;
        return USB_NO_MSG;
    case USB_WRITE_PAGE_V2:
        write_status[0]=0;
        buffer[page_counter++]=rq->wValue.bytes[0];
        buffer[page_counter++]=rq->wValue.bytes[1];
        buffer[page_counter++]=rq->wIndex.bytes[0];
        buffer[page_counter++]=rq->wIndex.bytes[1];
        if (page_counter>=128 || (program_counter+page_counter)>=program_size)
        {
            write_status[1]=program_counter;
            write_status[2]=page_counter;
            cur_stat=USB_WRITE_PAGE_V2;
            return USB_NO_MSG;
        }
        
        return 0;

    }

    return 0; // should not get here
}

// This gets called when data is sent from PC to the device
USB_PUBLIC uchar usbFunctionWrite(uchar *data, uchar len) {
    

    if (cur_stat==USB_INIT_PROGRAMMER)
    {
        PORTB |= 1<<1;
        enum_devices();
        write_status[0]=1;
        PORTB &= ~(1<<1);
       
        cur_stat = -1;
        return 1;
    }else if (cur_stat==USB_WRITE_PAGE_V2){
            PORTB |= 1<<2;
            send_page();
            PORTB &= ~(1<<2);
            cur_stat = -1;
            return 1;
    }else if (cur_stat==USB_PAGE_WRITE){
            // DDRB |= 1<<2;
            // PORTB |= 1<<2;
            // send_page();
            // PORTB &= ~(1<<2);
            // cur_stat = -1;
            // return 1;
    }
    return 1; // 1 if we received it all, 0 if not
	
}


int main() {
	uchar i;

    DDRA = 0xff;
    DDRC = 0xff;


	 DDRB |= (1<<MOSI) | (1<<SCK) | (1<<SS);

     DDRB |= 1;
     DDRB |= 1<<1;

    //wdt_enable(WDTO_1S); // enable 1s watchdog timer

    usbInit();
	
    usbDeviceDisconnect(); // enforce re-enumeration
    for(i = 0; i<250; i++) { // wait 250 ms
      //  wdt_reset(); // keep the watchdog happy
        _delay_ms(1);
    }
    usbDeviceConnect();
	
    sei(); // Enable interrupts after re-enumeration
	
    while(1) {
        usbPoll();
    }
	
    return 0;
}