#ifndef DISKIO_HARDWARE_H
#define	DISKIO_HARDWARE_H

#include <xc.h> // include processor files - each processor file is guarded.  

/* Definitions of physical drive number for each drive */
#define DEV_MMC		0	/* Example: Map MMC/SD card to physical drive 1 */

// #PIC   #MMC
//  RA1 -> INS
//  RA2 -> CS
//  RB0 -> DO
//  RC6 -> DI
//  RC7 -> SCLK

#define MMC_TRISA_MASK  0xFB
#define MMC_TRISB_MASK  0xFF
#define MMC_TRISC_MASK  0x3F

#define MMC_PPSOUT_SCK	RC7PPS
#define MMC_PPSOUT_SDO	RC6PPS
#define MMC_PPSIN_SDI	0x08	// RB0
#define MMC_PPSIN_SCK	0x17	// 0x09 for RC7

#define MMC_CS				(LATAbits.LATA2)
//#define MMC_CE				(LATDbits.LATD7)
#define MMC_INS_PORT		(PORTAbits.RA1)
#define MMC_INS_WPU			(WPUAbits.WPUA1)
#define MMC_INS_IOCP		(IOCAPbits.IOCAP1)
#define MMC_INS_IOCN		(IOCANbits.IOCAN1)
#define MMC_INS_IOCF		(IOCAFbits.IOCAF1)
#define MMC_IsInserted()	(!MMC_INS_PORT)

// If Chip enable is implemented, these macro should be implemented
#define MMC_ChipEnable(on)
#define MMC_IsChipEnable()  (true)

void MMC_Init(void);
void MMC_Interrupt(void);

void MMC_Eject(void);
bool MMC_IsEjected(void);

// need to be mplemented in main.c
void MMC_AccessLamp(bool on);

#endif	/* DISKIO_HARDWARE_H */

