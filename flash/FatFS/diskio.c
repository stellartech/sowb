/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2007        */
/*-----------------------------------------------------------------------*/
/* This is a stub disk I/O module that acts as front end of the existing */
/* disk I/O modules and attach it to FatFs module with common interface. */
/*-----------------------------------------------------------------------*/

#include "diskio.h"
#include "sowb.h"
#include "user.h"
#include "gpio.h"
#include "gps.h"
#include "ssp0.h"

#define R1_IDLE_STATE           (1 << 0)
#define R1_ERASE_RESET          (1 << 1)
#define R1_ILLEGAL_COMMAND      (1 << 2)
#define R1_COM_CRC_ERROR        (1 << 3)
#define R1_ERASE_SEQUENCE_ERROR (1 << 4)
#define R1_ADDRESS_ERROR        (1 << 5)
#define R1_PARAMETER_ERROR      (1 << 6)

//******************************************************************************************************************
// MBED SPI/CS Select functions.... Modify for your layout.
//**************************************************************************************

//SPI _spi(p5, p6, p7); // mosi, miso, sclk
//DigitalOut _cs(p8);
//DigitalOut P20(p20);
//SPI * _spi;


//******************************************************************************************************************
// Low Level Sector Access Function Prototypes('C' Castrated versions of Simon Ford's C++ MBED SDFileSystem class
//******************************************************************************************************************
int _cmd(int cmd, int arg);
int _cmd8(void);
int _cmdR2(int cmd, int arg);
int _read(BYTE *buffer, int length);
int _write(BYTE *buffer, int length);
int ext_bits(BYTE *data, int msb, int lsb);
int _sd_sectors();
int _sectors;

#define SD_COMMAND_TIMEOUT 5000

void deassert_cs(void) {
    SDCARD_CS_DEASSERT; //_cs = 1;
}

//******************************************************************************************************************
// Sector Access functions for CHAN FatFs 
//******************************************************************************************************************

DRESULT disk_ioctl (
    BYTE drv,        /* Physical drive nmuber (0..) */
    BYTE ctrl,        /* Control code */
    void *buff        /* Buffer to send/receive control data */
)
{
    DRESULT res;

    switch(ctrl)
    {
        case CTRL_SYNC:
             res = RES_OK;
        break;
    
        case GET_SECTOR_SIZE:
              res = RES_OK;
            *(WORD *)buff = 512;
        break;
        
        case GET_SECTOR_COUNT:
            res = RES_OK;
           *(DWORD *)buff = (WORD)_sd_sectors();
        break;
        
        case GET_BLOCK_SIZE:
         res = RES_OK;
          *(DWORD *)buff = 1;
        break;
        
        default:
        res = RES_OK;
        break;
    }
    return res;
}

int _cmd58() {
    SDCARD_CS_ASSERT; //_cs = 0; 
    int arg = 0;
    
    /* Request use of SSP0. */
    while(!SSP0_request()) WHILE_WAITING_DO_PROCESS_FUNCTIONS;
    
    // send a command
    SSP0_FLUSH_RX_FIFO;
    SSP0_WRITE_BYTE(0x40 | 58);
    SSP0_WRITE_BYTE(arg >> 24);
    SSP0_WRITE_BYTE(arg >> 16);
    SSP0_WRITE_BYTE(arg >> 8);
    SSP0_WRITE_BYTE(arg >> 0);
    SSP0_WRITE_BYTE(0x95);
    while(SSP0_IS_BUSY);
    SSP0_FLUSH_RX_FIFO;
    
    //_spi->write(0x40 | 58);
    //_spi->write(arg >> 24);
    //_spi->write(arg >> 16);
    //_spi->write(arg >> 8);
    //_spi->write(arg >> 0);
    //_spi->write(0x95);

    // wait for the repsonse (response[7] == 0)
    SSP0_WRITE_BYTE(0xFF);
    while(SSP0_IS_BUSY);
    for(int i = 0; i < SD_COMMAND_TIMEOUT; i++) {
        int response = LPC_SSP0->DR;
        if(!(response & 0x80)) {
            SSP0_WRITE_BYTE(0xFF);
            while(SSP0_IS_BUSY);
            int ocr = LPC_SSP0->DR << 24;
            SSP0_WRITE_BYTE(0xFF);
            while(SSP0_IS_BUSY);
            ocr |= LPC_SSP0->DR  << 16;
            SSP0_WRITE_BYTE(0xFF);
            while(SSP0_IS_BUSY);
            ocr |= LPC_SSP0->DR  << 8;
            SSP0_WRITE_BYTE(0xFF);
            while(SSP0_IS_BUSY);
            ocr |= LPC_SSP0->DR << 0;
//            printf("OCR = 0x%08X\n", ocr);
            SDCARD_CS_DEASSERT; //_cs = 1;
            SSP0_WRITE_BYTE(0xFF);
            while(SSP0_IS_BUSY);
            SSP0_FLUSH_RX_FIFO;
            // _spi->write(0xFF);
            SSP0_release();
            return response;
        }
        SSP0_WRITE_BYTE(0xFF);
        while(SSP0_IS_BUSY);
    }
    SDCARD_CS_DEASSERT;
    SSP0_WRITE_BYTE(0xFF);
    while(SSP0_IS_BUSY);
    SSP0_release();
    return -1; // timeout
}

int initialise_card_v1() {
    
    /* Request use of SSP0. */
    while(!SSP0_request()) WHILE_WAITING_DO_PROCESS_FUNCTIONS;
    
    for(int i=0; i<SD_COMMAND_TIMEOUT; i++) {
        _cmd(55, 0); 
        if(_cmd(41, 0) == 0) { 
            SSP0_release();
            return 0; //SDCARD_V1;
        }
    }

    //fprintf(stderr, "Timeout waiting for v1.x card\n");
    SSP0_release();
    return STA_NOINIT;
}

int initialise_card_v2() {
    int c41, c58, r;
    
    /* Request use of SSP0. */
    while(!SSP0_request()) WHILE_WAITING_DO_PROCESS_FUNCTIONS;
    
    for(int i=0; i<SD_COMMAND_TIMEOUT; i++) {
        _cmd(55, 0);
        c41 = _cmd(41, 0);
        if(c41 == 0) { 
            SSP0_release();
            c58 = _cmd58();
            //fprintf(stderr, "C41 returned %02x and C58 returned %02x\r\n", c41, c58);
            return 0; //SDCARD_V2;
        }        
    }
    
    
    //fprintf(stderr, "Timeout waiting for v2.x card response=0x%04x\r\n", c41);
    SSP0_release();
    return STA_NOINIT;
}

DSTATUS disk_initialize(BYTE Drive) {
    int i, cmd8_r;
    uint32_t cpsr = LPC_SSP0->CPSR;
    
    //_spi = new SPI(p5, p6, p7);
    
    /* Request use of SSP0. */
    while(!SSP0_request()) WHILE_WAITING_DO_PROCESS_FUNCTIONS;
    
    //_spi->frequency(100000); // Set to 100kHz for initialisation
    LPC_SSP0->CPSR = 0x64;
    SDCARD_CS_ASSERT; //_cs = 1;
    
    // Initialise the card by clocking it a bit (cs = 1)
    for(int i=0; i < 16; i++) {   
        SSP0_WRITE_BYTE(0xFF);
    }
    while(SSP0_IS_BUSY);
    SSP0_FLUSH_RX_FIFO;
    

    
    // send CMD0, should return with all zeros except IDLE STATE set (bit 0)
    if(_cmd(0, 0) != 0x01) { 
        //fprintf(stderr, "Not in idle state\n");
        SSP0_release();
        return STA_NOINIT;
    }
    
    for(i = 0; i < SD_COMMAND_TIMEOUT; i++) {
        cmd8_r = _cmd8();
        if (cmd8_r == 0 || cmd8_r == 1 || cmd8_r == R1_ILLEGAL_COMMAND) break;
        if (cmd8_r == 0 || cmd8_r == R1_ILLEGAL_COMMAND) break;
    }
    
    if ( (cmd8_r & R1_ILLEGAL_COMMAND) != 0) {
        //fprintf(stderr, "V1 %d\r\n", cmd8_r);
        SSP0_release();
        return initialise_card_v1();
    }
    
    if (cmd8_r == 0 || cmd8_r == 1) {
        //fprintf(stderr, "V2 %d\r\n", cmd8_r);
        SSP0_release();
        return initialise_card_v2();
    }
    
    
    //fprintf(stderr, "Not in idle state after sending CMD8 (not an SD card?) response = %d\r\n", cmd8_r);
    SSP0_release();
    return STA_NOINIT;
    
    // ACMD41 to give host capacity support (repeat until not busy)
    // ACMD41 is application specific command, so we send APP_CMD (CMD55) beforehand
    for(int i=0;; i++) {
        _cmd(55, 0); 
        int response = _cmd(41, 0);
        if(response == 0) { 
            break;
        } else if(i > SD_COMMAND_TIMEOUT) {
            //fprintf(stderr, "Timeout waiting for card\n");
            SSP0_release();
            return STA_NOINIT;
        }    
    }

    _sectors = _sd_sectors();

    // Set block length to 512 (CMD16)
    if(_cmd(16, 512) != 0) {
        //fprintf(stderr, "Set block timeout\n");
        SSP0_release();
        return STA_NOINIT;
    }
    
    LPC_SSP0->CPSR = cpsr;    
    SSP0_release();
    //_spi->frequency(10000000); // Set to 10MHz for data transfer
    return 0;
}

DRESULT disk_write(BYTE Drive,const BYTE * Buffer, DWORD SectorNumber, BYTE SectorCount)
{
    BYTE i;
    
    BYTE * MyBufOut = (BYTE *)Buffer;
    
    /* Request use of SSP0. */
    while(!SSP0_request()) WHILE_WAITING_DO_PROCESS_FUNCTIONS;
    
    for(i=0;i<SectorCount;i++)
    {
        // set write address for single block (CMD24)
        if(_cmd(24, (SectorNumber + i) * 512 ) != 0) {
            SSP0_release();
            return RES_ERROR;
        }

        // send the data block
        _write(MyBufOut, 512);    
        
        MyBufOut+=512;
    }
    
    SSP0_release();
    return RES_OK;    
}

DRESULT disk_read(BYTE Drive, BYTE * Buffer,DWORD SectorNumber, BYTE SectorCount)
{        
    BYTE i;
    
    /* Request use of SSP0. */
    while(!SSP0_request()) WHILE_WAITING_DO_PROCESS_FUNCTIONS;
    
    for(i=0;i<SectorCount;i++)
    {
        // set read address for single block (CMD17)
        if(_cmd(17, (SectorNumber+i) * 512) != 0)
        {
            SSP0_release();
            return RES_ERROR;
        }
        // receive the data
        _read(Buffer, 512);
       
        Buffer+=512;
    }
    
    SSP0_release(); 
    return RES_OK;
}


extern "C" DWORD get_fattime(void) {
    GPS_TIME the_time;
    
    gps_get_time(&the_time);
    
    uint32_t year   = (the_time.year - 1980) << 25;
    uint32_t month  = the_time.month << 21;
    uint32_t day    = the_time.day << 16;
    uint32_t hour   = the_time.hour << 11;
    uint32_t minute = the_time.minute << 5;
    uint32_t second = (the_time.second / 2) &0xF;
    return (DWORD)(year | month | day | hour | minute | second);
}

DSTATUS disk_status(BYTE Drive)
{
    return 0;
}

//**************************************************************************************
// Low Level Sector Access Functions (Castrated versions of Simon Fords C++ MBED class
//**************************************************************************************

int _cmd(int cmd, int arg) {
    volatile int delay;
    
    for (delay = 500; delay; delay--);
    SDCARD_CS_ASSERT; //_cs = 0; 
    for (delay = 500; delay; delay--);
    
    // send a command
    SSP0_FLUSH_RX_FIFO;
    SSP0_WRITE_BYTE(0x40 | cmd);
    SSP0_WRITE_BYTE(arg >> 24);
    SSP0_WRITE_BYTE(arg >> 16);
    SSP0_WRITE_BYTE(arg >> 8);
    SSP0_WRITE_BYTE(arg >> 0);
    SSP0_WRITE_BYTE(0x95);
    while(SSP0_IS_BUSY);
    SSP0_FLUSH_RX_FIFO;
    
    //_spi->write(0x40 | cmd);
    //_spi->write(arg >> 24);
    //_spi->write(arg >> 16);
    //_spi->write(arg >> 8);
    //_spi->write(arg >> 0);
    //_spi->write(0x95);

    // wait for the repsonse (response[7] == 0)
    SSP0_WRITE_BYTE(0xFF);
    while(SSP0_IS_BUSY);
    for(int i=0; i<SD_COMMAND_TIMEOUT; i++) {
        int response = LPC_SSP0->DR;
        if(!(response & 0x80)) {
            SDCARD_CS_DEASSERT; //_cs = 1;
            return response;
        }
        SSP0_WRITE_BYTE(0xFF);
        while(SSP0_IS_BUSY);
    }
    
    SDCARD_CS_ASSERT; //_cs = 1;
    return -1; // timeout
}

int _cmdR2(int cmd, int arg) {
    int response;
    
    SDCARD_CS_ASSERT; //_cs = 0; 
    
    // send a command
    SSP0_FLUSH_RX_FIFO;
    SSP0_WRITE_BYTE(0x40 | cmd);
    SSP0_WRITE_BYTE(arg >> 24);
    SSP0_WRITE_BYTE(arg >> 16);
    SSP0_WRITE_BYTE(arg >> 8);
    SSP0_WRITE_BYTE(arg >> 0);
    SSP0_WRITE_BYTE(0x95);
    SSP0_WRITE_BYTE(0xFF);
    while(SSP0_IS_BUSY);
    SSP0_FLUSH_RX_FIFO;
    
    //_spi->write(0x40 | cmd);
    //_spi->write(arg >> 24);
    //_spi->write(arg >> 16);
    //_spi->write(arg >> 8);
    //_spi->write(arg >> 0);
    //_spi->write(0x95);    
    //_spi->write(0xFF);
    
    // wait for the repsonse (response[7] == 0)
    //for(int i=0; i<SD_COMMAND_TIMEOUT; i++) {
        response = 0;
        SSP0_WRITE_BYTE(0x00);
        while(SSP0_IS_BUSY);
        response = (LPC_SSP0->DR & 0xFF);
        response = (response << 8) & 0xFF00;
        SSP0_WRITE_BYTE(0x00);
        while(SSP0_IS_BUSY);
        response |= (LPC_SSP0->DR & 0xFF);
        SDCARD_CS_DEASSERT; //_cs = 1;
        return response;
    //}
    SDCARD_CS_DEASSERT; //_cs = 1;
    return -1; // timeout
}

int _cmd8(void) {
    SDCARD_CS_ASSERT; //_cs = 0; 
    
    // send a command
    SSP0_FLUSH_RX_FIFO;
    SSP0_WRITE_BYTE(0x40 | 8);
    SSP0_WRITE_BYTE(00);
    SSP0_WRITE_BYTE(00);
    SSP0_WRITE_BYTE(01);
    SSP0_WRITE_BYTE(0xAA);
    SSP0_WRITE_BYTE(0x87);
    while(SSP0_IS_BUSY);
    SSP0_FLUSH_RX_FIFO;
    
    //_spi->write(0x40 | 8); // CMD8
    //_spi->write(0x00);     // reserved
    //_spi->write(0x00);     // reserved
    //_spi->write(0x01);     // 3.3v
    //_spi->write(0xAA);     // check pattern
    //_spi->write(0x87);     // crc

    // wait for the repsonse (response[7] == 0)
    SSP0_WRITE_BYTE(0xFF);
    while(SSP0_IS_BUSY);
    for(int i=0; i<SD_COMMAND_TIMEOUT * 1000; i++) {
        char response[5];
        response[0] = LPC_SSP0->DR;
        SSP0_WRITE_BYTE(0xFF);
        while(SSP0_IS_BUSY);
        for(int j=1; j<5; j++) {
            response[i] = LPC_SSP0->DR;
            SSP0_WRITE_BYTE(0xFF);
            while(SSP0_IS_BUSY);
        }
        SDCARD_CS_DEASSERT; //_cs = 1;
        SSP0_WRITE_BYTE(0xFF);
        while(SSP0_IS_BUSY);
        SSP0_FLUSH_RX_FIFO;
        return response[0];
    }
    
    SDCARD_CS_DEASSERT; //_cs = 1;
    SSP0_WRITE_BYTE(0xFF);
    while(SSP0_IS_BUSY);
    SSP0_FLUSH_RX_FIFO;
    //_spi->write(0xFF);
    return -1; // timeout
}

int _cmd8original(void) {
    SDCARD_CS_ASSERT; //_cs = 0; 
    
    // send a command
    SSP0_FLUSH_RX_FIFO;
    SSP0_WRITE_BYTE(0x40 | 8);
    SSP0_WRITE_BYTE(00);
    SSP0_WRITE_BYTE(00);
    SSP0_WRITE_BYTE(01);
    SSP0_WRITE_BYTE(0xAA);
    SSP0_WRITE_BYTE(0x87);
    while(SSP0_IS_BUSY);
    SSP0_FLUSH_RX_FIFO;
    
    //_spi->write(0x40 | 8); // CMD8
    //_spi->write(0x00);     // reserved
    //_spi->write(0x00);     // reserved
    //_spi->write(0x01);     // 3.3v
    //_spi->write(0xAA);     // check pattern
    //_spi->write(0x87);     // crc

    // wait for the repsonse (response[7] == 0)
    SSP0_WRITE_BYTE(0xFF);
    while(SSP0_IS_BUSY);
    for(int i=0; i<SD_COMMAND_TIMEOUT * 1000; i++) {
        char response[5];
        response[0] = LPC_SSP0->DR;
        if(!(response[0] & 0x80)) {
            //fprintf(stderr, "RS = %d\r\n", response[0]);
            SSP0_WRITE_BYTE(0xFF);
            while(SSP0_IS_BUSY);
            for(int j=1; j<5; j++) {
                response[i] = LPC_SSP0->DR;
                SSP0_WRITE_BYTE(0xFF);
                while(SSP0_IS_BUSY);
            }
            SDCARD_CS_DEASSERT; //_cs = 1;
            SSP0_WRITE_BYTE(0xFF);
            while(SSP0_IS_BUSY);
            //_spi->write(0xFF);
            return response[0];
        }
    }
    
    SDCARD_CS_DEASSERT; //_cs = 1;
    SSP0_WRITE_BYTE(0xFF);
    while(SSP0_IS_BUSY);
    SSP0_FLUSH_RX_FIFO;
    //_spi->write(0xFF);
    return -1; // timeout
}

int _read(BYTE *buffer, int length) {
    SDCARD_CS_ASSERT; //_cs = 0;

    // read until start byte (0xFF)
    SSP0_FLUSH_RX_FIFO;
    SSP0_WRITE_BYTE(0xFF);
    while(SSP0_IS_BUSY);
    while(LPC_SSP0->DR != 0xFE) {
        SSP0_WRITE_BYTE(0xFF);
        while(SSP0_IS_BUSY);
    }

    // read data
    SSP0_FLUSH_RX_FIFO;
    SSP0_WRITE_BYTE(0xFF);
    while(SSP0_IS_BUSY);
    for(int i=0; i<length; i++) {
        buffer[i] = LPC_SSP0->DR;
        SSP0_WRITE_BYTE(0xFF);
        while(SSP0_IS_BUSY);
    }
    SSP0_WRITE_BYTE(0xFF);
    SSP0_WRITE_BYTE(0xFF);
    while(SSP0_IS_BUSY);
    SSP0_FLUSH_RX_FIFO;
    //_spi->write(0xFF); // checksum
    //_spi->write(0xFF);

    SDCARD_CS_DEASSERT; //_cs = 1;    
    return 0;
}

int _write(BYTE *buffer, int length) {
    SDCARD_CS_ASSERT; //_cs = 0;
    
    // indicate start of block
    SSP0_WRITE_BYTE(0xFE);
    while(SSP0_IS_BUSY);
    //_spi->write(0xFE);
    
    // write the data
    for(int i=0; i<length; i++) {
        SSP0_WRITE_BYTE(buffer[i]);
        while(SSP0_IS_BUSY);
        //_spi->write(buffer[i]);
    }
    
    // write the checksum
    SSP0_WRITE_BYTE(0xFF);
    SSP0_WRITE_BYTE(0xFF);
    while(SSP0_IS_BUSY);
    //_spi->write(0xFF); 
    //_spi->write(0xFF);

    // check the repsonse token
    SSP0_FLUSH_RX_FIFO;
    SSP0_WRITE_BYTE(0xFF);
    while(SSP0_IS_BUSY);
    if((LPC_SSP0->DR & 0x1F) != 0x05) {
        SDCARD_CS_DEASSERT; //_cs = 1; 
        return 1;
    }

    // wait for write to finish
    SSP0_FLUSH_RX_FIFO;
    SSP0_WRITE_BYTE(0xFF);
    while(SSP0_IS_BUSY);
    while(LPC_SSP0->DR == 0) {
        SSP0_WRITE_BYTE(0xFF);
        while(SSP0_IS_BUSY);
    }

    SDCARD_CS_DEASSERT; //_cs = 1; 
    return 0;
}

int ext_bits(BYTE *data, int msb, int lsb) {
    int bits = 0;
    int size = 1 + msb - lsb; 
    for(int i=0; i<size; i++) {
        int position = lsb + i;
        int byte = 15 - (position >> 3);
        int bit = position & 0x7;
        int value = (data[byte] >> bit) & 1;
        bits |= value << i;
    }
    return bits;
}

int _sd_sectors() {

    // CMD9, Response R2 (R1 byte + 16-byte block read)
    if(_cmd(9, 0) != 0) {
        //fprintf(stderr, "Didn't get a response from the disk\n");
        return 0;
    }
    
    BYTE csd[16];    
    if(_read(csd, 16) != 0) {
        //fprintf(stderr, "Couldn't read csd response from disk\n");
        return 0;
    }

    // csd_structure : csd[127:126]
    // c_size        : csd[73:62]
    // c_size_mult   : csd[49:47]
    // read_bl_len   : csd[83:80] 

    int csd_structure = ext_bits(csd, 127, 126);
    int c_size = ext_bits(csd, 73, 62);
    int c_size_mult = ext_bits(csd, 49, 47);
    int read_bl_len = ext_bits(csd, 83, 80);
    
    if(csd_structure != 0) {
        //fprintf(stderr, "This disk tastes funny! I only know about type 0 CSD structures");
        return 0;
    }
                            
    int blocks = (c_size + 1) * (1 << (c_size_mult + 2));
    int block_size = 1 << read_bl_len;

    if(block_size != 512) {
        //fprintf(stderr, "This disk tastes funny! I only like 512 byte blocks (%d)\r\n",block_size);
        return 0;
    }
    
    return blocks;
}

