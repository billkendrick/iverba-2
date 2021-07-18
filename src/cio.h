/**
 * Function to call cio
 * @param IOCB # to call
 * @return Error
 */

#define IOCB_READ 0x04
#define IOCB_WRITE 0x08

unsigned char __fastcall__ ciov(unsigned char channel);
