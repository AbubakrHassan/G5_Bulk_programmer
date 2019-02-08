#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// this is libusb, see http://libusb.sourceforge.net/ 
#include <usb.h>

 #include <time.h>

#define USB_LED_OFF 0
#define USB_LED_ON  1
#define USB_READ_STATUS 2
#define USB_DATA_IN 3
#define USB_INIT_PROGRAMMER 4
#define USB_PAGE_WRITE 5
#define USB_WRITE_PAGE_V2 6// version 1 didn't work XD

 int program_size = -1; // stores the size of the latest .bin file opened using Openhex  
 int program_counter=0;

 unsigned char cur_page[128]; // placeholder for the page currently being send . it's used with USB_PAGE_WRITE

void delay(int time_ms){
    double start_time = clock();
    while(clock()<(start_time+time_ms));
}

// used to get descriptor strings for device identification 
static int usbGetDescriptorString(usb_dev_handle *dev, int index, int langid, 
                                  char *buf, int buflen) {
    char buffer[256];
    int rval, i;

    // make standard request GET_DESCRIPTOR, type string and given index 
    // (e.g. dev->iProduct)
    rval = usb_control_msg(dev, 
        USB_TYPE_STANDARD | USB_RECIP_DEVICE | USB_ENDPOINT_IN, 
        USB_REQ_GET_DESCRIPTOR, (USB_DT_STRING << 8) + index, langid, 
        buffer, sizeof(buffer), 1000);
        
    if(rval < 0) // error
        return rval;
    
    // rval should be bytes read, but buffer[0] contains the actual response size
    if((unsigned char)buffer[0] < rval)
        rval = (unsigned char)buffer[0]; // string is shorter than bytes read
    
    if(buffer[1] != USB_DT_STRING) // second byte is the data type
        return 0; // invalid return type
        
    // we're dealing with UTF-16LE here so actual chars is half of rval,
    // and index 0 doesn't count
    rval /= 2;
    
    // lossy conversion to ISO Latin1 
    for(i = 1; i < rval && i < buflen; i++) {
        if(buffer[2 * i + 1] == 0)
            buf[i-1] = buffer[2 * i];
        else
            buf[i-1] = '?'; // outside of ISO Latin1 range
    }
    buf[i-1] = 0;
    
    return i-1;
}

static usb_dev_handle * usbOpenDevice(int vendor, char *vendorName, 
                                      int product, char *productName) {
    struct usb_bus *bus;
    struct usb_device *dev;
    char devVendor[256], devProduct[256];
    
    usb_dev_handle * handle = NULL;
    
    usb_init();
    usb_find_busses();
    usb_find_devices();
    
    for(bus=usb_get_busses(); bus; bus=bus->next) {
        for(dev=bus->devices; dev; dev=dev->next) {         
            if(dev->descriptor.idVendor != vendor ||
               dev->descriptor.idProduct != product)
                continue;
                
            // we need to open the device in order to query strings 
            if(!(handle = usb_open(dev))) {
                fprintf(stderr, "Warning: cannot open USB device: %s\n",
                    usb_strerror());
                continue;
            }
            
            // get vendor name 
            if(usbGetDescriptorString(handle, dev->descriptor.iManufacturer, 0x0409, devVendor, sizeof(devVendor)) < 0) {
                fprintf(stderr, 
                    "Warning: cannot query manufacturer for device: %s\n", 
                    usb_strerror());
                usb_close(handle);
                continue;
            }
            
            // get product name 
            if(usbGetDescriptorString(handle, dev->descriptor.iProduct, 
               0x0409, devProduct, sizeof(devProduct)) < 0) {// devVendeor = devProduct
                fprintf(stderr, 
                    "Warning: cannot query product for device: %s\n", 
                    usb_strerror());
                usb_close(handle);
                continue;
            }
            
            if(strcmp(devVendor, vendorName) == 0 && 
               strcmp(devProduct, productName) == 0)
                return handle;
            else
                usb_close(handle);
        }
    }
    
    return NULL;
}

unsigned char* openHex(char* file_name){
    char command[255]="Hex2bin.exe ";
    strcat(command,file_name);
    strcat(command,".hex");
    
    
    system (command);// create bin file
    char binary_file_name[255];
    int i=0;
    for ( i = 0; *(file_name+i) != 0; i++)
    {
        binary_file_name[i]=*(file_name+i);
    }
    binary_file_name[i++]='.';
    binary_file_name[i++]='b';
    binary_file_name[i++]='i';
    binary_file_name[i++]='n';
    binary_file_name[i++]= 0;
    
    printf("\n%s\n",binary_file_name);
    FILE *binary = fopen(binary_file_name, "rb");
    fseek(binary,0,SEEK_END);
    int sz = ftell(binary);
    rewind(binary);
    void *mall = malloc(sz);
    
    unsigned char *bytesFromBinary = (unsigned char*) mall;
    
   
    fread(bytesFromBinary, 1, sz, binary);
    program_size=sz;    
    return bytesFromBinary;
}

int main(int argc, char **argv) {
    usb_dev_handle *handle = NULL;
    int nBytes = 0;
    char buffer[256];

    if(argc < 2) {
        printf("Usage:\n");
        printf("mainBulk.exe on\n");
        printf("mainBulk.exe off\n");
        printf("mainBulk.exe status\n");
        printf("mainBulk.exe laod_code <file>\n");

        exit(1);
    }
    
    handle = usbOpenDevice(0x16C0, "Mohanad", 0x05DC, "G5BulkProg");
    
    if(handle == NULL) {
        fprintf(stderr, "Could not find USB device!\n");
        exit(1);
    }

    usb_set_configuration(handle,1);
    usb_claim_interface(handle,0);

    if(strcmp(argv[1], "on") == 0) {
        nBytes = usb_control_msg(handle, 
            USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN, 
            USB_LED_ON, 0, 0, (char *)buffer, sizeof(buffer), 5000);
    } else if(strcmp(argv[1], "off") == 0) {
        nBytes = usb_control_msg(handle, 
            USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN, 
            USB_LED_OFF, 0, 0, (char *)buffer, sizeof(buffer), 5000);
    }else if(strcmp(argv[1], "status")==0){
        int reply = usb_control_msg(handle,
                USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN,
                USB_READ_STATUS,0,0, (char *)buffer,sizeof(buffer),20000);
        if (reply<0){
            printf("write %s\n",usb_strerror());
        }
        printf("write_status : %d \n",buffer[0] );
        printf("program_counter : %d \n",(unsigned char)buffer[1] );
        printf("page_counter : %d \n",(unsigned char)buffer[2] );
        printf("program_size : %d \n",(unsigned char)buffer[3] );
    } else if(strcmp(argv[1], "load_code") == 0 && argc==3){
        unsigned char *bin_file= openHex(argv[2]);
        double t1=clock();
        int reply = usb_control_msg(handle,
             USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_OUT, 
            USB_INIT_PROGRAMMER, program_size, program_size, (char *)buffer, program_size , 200000
            );
        if (reply<0){
            printf("USB init error: %s\n", usb_strerror());
        }
        printf("time to enum: %f\n",(clock()-t1)/1000 );

        while(program_counter<=program_size){
            reply = usb_control_msg(handle,
                USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN,
                USB_READ_STATUS,0,0, (char *)buffer,sizeof(buffer),20000);
            if (reply<0){ 
                // indicates device is not functioning/busy  so wait 0.5 second and ask it again
                delay(500);
                continue;
            }
            printf("status : %d \n",buffer[0] );
            printf("program_counter : %d \n",(unsigned char)buffer[1] );
            printf("page_counter : %d \n",(unsigned char)buffer[2] );
            printf("program_size : %d \n",(unsigned char)buffer[3] );

            if (buffer[0]==1){
                int data_left=(program_size-program_counter);
                int to_send = data_left>128?128:data_left;
                int i;
                for (i = 0; i < to_send; ++i){
                    cur_page[i]=bin_file[program_counter+i];
                }
                i=0;
                while(i<data_left && i<128){// send page in chunks of 4 bytes
                    reply = usb_control_msg(handle,
                        USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_OUT,
                        USB_WRITE_PAGE_V2,cur_page[i+1]<<8 | cur_page[i],cur_page[i+3]<<8 | cur_page[i+2], (char *)buffer,sizeof(buffer),20000
                        );

                    if (reply<0){
                        //printf("write %d : %s\n",i,usb_strerror());
                    }
                    i+=4;
                }
                program_counter+=128;
            }else{
                delay(500);
            }

        }
        
        free(bin_file);
    }
        
    usb_close(handle);
    
    return 0;
}