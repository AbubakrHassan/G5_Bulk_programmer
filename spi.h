void send_byte_spi(int b);

void spi_disable();

void spi_enable();

int start_prog(int device,int err_count);

void  chip_erase();

void load_page(int device,unsigned char page[],int len);

void write_page(int page_number);

int read_byte(int addr);