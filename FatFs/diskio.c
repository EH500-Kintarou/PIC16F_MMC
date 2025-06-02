/*-----------------------------------------------------------------------*/
/* Low level disk I/O module SKELETON for FatFs     (C)ChaN, 2019        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include <xc.h>
#include <stdbool.h>
#include "ff.h"			/* Obtains integer types */
#include "diskio.h"		/* Declarations of disk functions */
#include "diskio_hardware.h"


/* Definitions for MMC/SDC command */
#define CMD0	(0)			/* GO_IDLE_STATE */
#define CMD1	(1)			/* SEND_OP_COND (MMC) */
#define	ACMD41	(0x80+41)	/* SEND_OP_COND (SDC) */
#define CMD8	(8)			/* SEND_IF_COND */
#define CMD9	(9)			/* SEND_CSD */
#define CMD10	(10)		/* SEND_CID */
#define CMD12	(12)		/* STOP_TRANSMISSION */
#define ACMD13	(0x80+13)	/* SD_STATUS (SDC) */
#define CMD16	(16)		/* SET_BLOCKLEN */
#define CMD17	(17)		/* READ_SINGLE_BLOCK */
#define CMD18	(18)		/* READ_MULTIPLE_BLOCK */
#define CMD23	(23)		/* SET_BLOCK_COUNT (MMC) */
#define	ACMD23	(0x80+23)	/* SET_WR_BLK_ERASE_COUNT (SDC) */
#define CMD24	(24)		/* WRITE_BLOCK */
#define CMD25	(25)		/* WRITE_MULTIPLE_BLOCK */
#define CMD55	(55)		/* APP_CMD */
#define CMD58	(58)		/* READ_OCR */


/* MMC card type flags (MMC_GET_TYPE) */
#define CT_MMC		0x01		/* MMC ver 3 */
#define CT_SD1		0x02		/* SD ver 1 */
#define CT_SD2		0x04		/* SD ver 2 */
#define CT_SDC		(CT_SD1|CT_SD2)	/* SD */
#define CT_BLOCK	0x08		/* Block addressing */


DSTATUS DiskStat = STA_NOINIT;  /* Disk status */
bool ejected = false;
BYTE CardType = 0;              /* Detected card type */


void MMC_Init(void)
{
    TRISA &= MMC_TRISA_MASK;
    TRISB &= MMC_TRISB_MASK;
    TRISC &= MMC_TRISC_MASK;

    MMC_INS_WPU = 1;
    MMC_INS_IOCF = 0;
    MMC_INS_IOCP = 1;
    MMC_INS_IOCN = 1;
    
    if(MMC_IsInserted()) {   // 差し込まれた
        CardType = 0;
        DiskStat = STA_NOINIT;
    } else {    // 抜き取られた
        CardType = 0;
        DiskStat = STA_NOINIT | STA_NODISK;
    }
    
    PIE0bits.IOCIE = 1;
}

void MMC_Interrupt(void)
{
    if(MMC_INS_IOCF) {
        if(MMC_IsInserted()) {   // 差し込まれた
            CardType = 0;
            DiskStat = STA_NOINIT;
        } else {    // 抜き取られた
            CardType = 0;
            DiskStat = STA_NOINIT | STA_NODISK;
            MMC_ChipEnable(false);
            ejected = false;
        }
        MMC_INS_IOCF = 0;
    }
}

void MMC_Eject(void)
{
    DiskStat = STA_NOINIT | STA_NODISK;
    MMC_ChipEnable(false);
    ejected = true;
}

bool MMC_IsEjected(void)
{
    return ejected;
}

void MMC_SPIInit()
{
    SSP2DATPPS = MMC_PPSIN_SDI;
    SSP2CLKPPS = MMC_PPSIN_SCK;
    MMC_PPSOUT_SCK = 0x16;      // SCK2
    MMC_PPSOUT_SDO = 0x17;      // SDO2

    SSP2STAT = 0xC0;
    SSP2CON2 = 0x00;
    SSP2CON1 = 0x20;
}

BYTE MMC_SendSPI(BYTE d)
{
    MMC_AccessLamp(true);
    
    SSP2BUF = d;
    while(!SSP2STATbits.BF)
        ;
    d = SSP2BUF;
    
    MMC_AccessLamp(false);
    
    return d;
}

void MMC_ReceiveBytesSPI(BYTE *ptr, UINT cnt)
{
    while(cnt--)
        *ptr++ = MMC_SendSPI(0xFF);
}

void MMC_SendBytesSPI(const BYTE *p, UINT cnt)
{
    while(cnt--)
        MMC_SendSPI(*p++);
}


/*-----------------------------------------------------------------------*/
/* Wait for card ready                                                   */
/*-----------------------------------------------------------------------*/

uint8_t MMC_wait_ready(UINT wt)
{
	BYTE d;

	wt *= 10;
	while(wt--) {
		d = MMC_SendSPI(0xFF);
		if(d == 0xFF)
			return 1;
		__delay_us(100);
	}
	return 0;
}


/*-----------------------------------------------------------------------*/
/* Deselect the card and release SPI bus                                 */
/*-----------------------------------------------------------------------*/

void MMC_deselect (void)
{
    MMC_CS = 1;
	MMC_SendSPI(0xFF);	/* Dummy clock (force DO hi-z for multiple slave SPI) */
}


/*-----------------------------------------------------------------------*/
/* Select the card and wait for ready                                    */
/*-----------------------------------------------------------------------*/

int MMC_select (void)	/* 1:Successful, 0:Timeout */
{
	MMC_CS = 0;
	MMC_SendSPI(0xFF);	/* Dummy clock (force DO enabled) */

	if(MMC_wait_ready(500))
        return 1;	/* OK */
	MMC_deselect();
	return 0;	/* Timeout */
}




BYTE MMC_send_cmd_internal(BYTE cmd, DWORD arg)
{
	BYTE n, res, crc;

	/* Select the card and wait for ready */
	MMC_deselect();
	if(!MMC_select())
        return 0xFF;

	/* Send command packet */
	MMC_SendSPI(0x40 | cmd);			/* Start + Command index */
	MMC_SendSPI((BYTE)(arg >> 24));		/* Argument[31..24] */
	MMC_SendSPI((BYTE)(arg >> 16));		/* Argument[23..16] */
	MMC_SendSPI((BYTE)(arg >> 8));		/* Argument[15..8] */
	MMC_SendSPI((BYTE)arg);				/* Argument[7..0] */

	if(cmd == CMD0)
        crc = 0x95;			/* Valid CRC for CMD0(0) + Stop */
	else if(cmd == CMD8)
        crc = 0x87;			/* Valid CRC for CMD8(0x1AA) Stop */
    else
        crc = 0x01;         /* Dummy CRC + Stop */
	MMC_SendSPI(crc);

	/* Receive command response */
	if (cmd == CMD12) MMC_SendSPI(0xFF);		/* Skip a stuff byte when stop reading */
	n = 10;								/* Wait for a valid response in timeout of 10 attempts */
	do {
		res = MMC_SendSPI(0xFF);
	} while ((res & 0x80) && --n);

	return res;			/* Return with the response value */
}



BYTE MMC_send_cmd(BYTE cmd, DWORD arg)
{
	BYTE res;

	if (cmd & 0x80) {	/* ACMD<n> is the command sequense of CMD55-CMD<n> */
		cmd &= 0x7F;
		res = MMC_send_cmd_internal(CMD55, 0);
		if (res > 1) return res;
	}

    res = MMC_send_cmd_internal(cmd, arg);
    return res;
}

/*-----------------------------------------------------------------------*/
/* Receive a data packet from MMC                                        */
/*-----------------------------------------------------------------------*/

int MMC_ReceiveDataBlock(BYTE *buff, UINT btr)
{
	BYTE token;

	for(UINT i = 0; i < 2000; i++) {	/* Wait for data packet in timeout of 200ms */
		token = MMC_SendSPI(0xFF);
		if(token != 0xFF)
			break;
		__delay_us(100);
	}
	if(token != 0xFE)
        return 0;	/* If not valid data token, retutn with error */

	MMC_ReceiveBytesSPI(buff, btr);		/* Receive the data block into buffer */
	MMC_SendSPI(0xFF);					/* Discard CRC */
	MMC_SendSPI(0xFF);

	return 1;						/* Return with success */
}



/*-----------------------------------------------------------------------*/
/* Send a data packet to MMC                                             */
/*-----------------------------------------------------------------------*/

int MMC_SendDataBlock(const BYTE *buff, BYTE token)
{
	BYTE resp;

	if(!MMC_wait_ready(500)) return 0;

	MMC_SendSPI(token);					/* Xmit data token */
	if (token != 0xFD) {	/* Is data token */
		MMC_SendBytesSPI(buff, 512);		/* Xmit the data block to the MMC */
		MMC_SendSPI(0xFF);					/* CRC (Dummy) */
		MMC_SendSPI(0xFF);
		resp = MMC_SendSPI(0xFF);			/* Reveive data response */
		if((resp & 0x1F) != 0x05)		/* If not accepted, return with error */
			return 0;
	}

	return 1;
}






/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
    if(pdrv != DEV_MMC)
        return STA_NOINIT;
    
    return DiskStat;
}

/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
    BYTE n, cmd, ty, ocr[4];
	UINT tmr;

    if(pdrv != DEV_MMC)
        return STA_NOINIT;    
	if(DiskStat & STA_NODISK)
        return DiskStat;
    
    MMC_ChipEnable(true);
    MMC_SPIInit();
	__delay_ms(5);

	for (n = 10; n; n--)
        MMC_SendSPI(0xFF);  /* 80 dummy clocks */

	ty = 0;
	if(MMC_send_cmd(CMD0, 0) == 1) {			/* Enter Idle state */
		if(MMC_send_cmd(CMD8, 0x1AA) == 1) {	/* SDv2? */
			for(n = 0; n < 4; n++)
				ocr[n] = MMC_SendSPI(0xFF);		/* Get trailing return value of R7 resp */
			if(ocr[2] == 0x01 && ocr[3] == 0xAA) {		/* The card can work at vdd range of 2.7-3.6V */
				for(tmr = 1000; tmr; tmr--) {			/* Wait for leaving idle state (ACMD41 with HCS bit) */
					if(MMC_send_cmd(ACMD41, 1UL << 30) == 0)
						break;
					__delay_ms(1);
				}
				if(tmr && MMC_send_cmd(CMD58, 0) == 0) {		/* Check CCS bit in the OCR */
					for(n = 0; n < 4; n++)
						ocr[n] = MMC_SendSPI(0xFF);
					ty = (ocr[0] & 0x40) ? CT_SD2 | CT_BLOCK : CT_SD2;	/* SDv2 */
				}
			}
		} else {							/* SDv1 or MMCv3 */
			if(MMC_send_cmd(ACMD41, 0) <= 1) 	{
				ty = CT_SD1;
				cmd = ACMD41;	/* SDv1 */
			} else {
				ty = CT_MMC;
				cmd = CMD1;	/* MMCv3 */
			}
			for(tmr = 1000; tmr; tmr--) {			/* Wait for leaving idle state */
				if(MMC_send_cmd(cmd, 0) == 0)
					break;
				__delay_ms(1);
			}
			if(!tmr || MMC_send_cmd(CMD16, 512) != 0)	/* Set R/W block length to 512 */
				ty = 0;
		}
	}
	CardType = ty;
	MMC_deselect();

	if (ty) {			/* Initialization succeded */
		DiskStat &= ~STA_NOINIT;		/* Clear STA_NOINIT */
	}

	return DiskStat;
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	LBA_t sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
    if((pdrv != DEV_MMC) || (count == 0))
        return RES_PARERR;
    if(DiskStat & STA_NOINIT)
        return RES_NOTRDY;
    
    if(!(CardType & CT_BLOCK))
        sector *= 512;	/* Convert to byte address if needed */

	if(count == 1) {	/* Single block read */
		if((MMC_send_cmd(CMD17, sector) == 0) && MMC_ReceiveDataBlock(buff, 512))
			count = 0;
	} else {				/* Multiple block read */
		if(MMC_send_cmd(CMD18, sector) == 0) {	/* READ_MULTIPLE_BLOCK */
			do {
				if(!MMC_ReceiveDataBlock(buff, 512))
                    break;
				buff += 512;
			} while (--count);
			MMC_send_cmd(CMD12, 0);				/* STOP_TRANSMISSION */
		}
	}
	MMC_deselect();

	return (count > 0) ? RES_ERROR : RES_OK;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	LBA_t sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
    if((pdrv != DEV_MMC) || (count == 0))
        return RES_PARERR;
	if(DiskStat & STA_NOINIT)
        return RES_NOTRDY;
	if(DiskStat & STA_PROTECT)
        return RES_WRPRT;

	if(!(CardType & CT_BLOCK))
        sector *= 512;	/* Convert to byte address if needed */

	if(count == 1) {	/* Single block write */
		if((MMC_send_cmd(CMD24, sector) == 0) && MMC_SendDataBlock(buff, 0xFE))
			count = 0;
	} else {				/* Multiple block write */
		if(CardType & CT_SDC) MMC_send_cmd(ACMD23, count);
		if(MMC_send_cmd(CMD25, sector) == 0) {	/* WRITE_MULTIPLE_BLOCK */
			do {
				if(!MMC_SendDataBlock(buff, 0xFC)) break;
				buff += 512;
			} while (--count);
			if (!MMC_SendDataBlock(0, 0xFD))	/* STOP_TRAN token */
				count = 1;
		}
	}
	MMC_deselect();

	return count ? RES_ERROR : RES_OK;
}

#endif

/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE pdrv,		// Physical drive nmuber (0..)
	BYTE cmd,		// Control code
	void *buff		// Buffer to send/receive control data
)
{
	DRESULT res;
	BYTE n, csd[16];
    DWORD csize;
#if CMD_FATFS_NOT_USED
	BYTE *ptr = buff;
#endif
#if _USE_ERASE
	DWORD *dp, st, ed;
#endif
    
    if(pdrv != DEV_MMC)
        return RES_PARERR;

	res = RES_ERROR;

	if(DiskStat & STA_NOINIT)
		return RES_NOTRDY;

	switch (cmd) {
	case CTRL_SYNC :		// Make sure that no pending write process. Do not remove this or written sector might not left updated. 
		if(MMC_select())
			return RES_OK;
		break;

	case GET_SECTOR_COUNT :	// Get number of sectors on the disk (DWORD) 
		if ((MMC_send_cmd(CMD9, 0) == 0) && MMC_ReceiveDataBlock(csd, 16)) {
			if ((csd[0] >> 6) == 1) {	// SDC ver 2.00 
				csize = csd[9] + ((WORD)csd[8] << 8) + ((DWORD)(csd[7] & 63) << 16) + 1;
				*(DWORD*)buff = csize << 10;
			} else {					// SDC ver 1.XX or MMC
				n = (BYTE)((csd[5] & 15) + ((csd[10] & 128) >> 7) + ((csd[9] & 3) << 1) + 2);
				csize = (csd[8] >> 6) + ((WORD)csd[7] << 2) + ((WORD)(csd[6] & 3) << 10) + 1;
				*(DWORD*)buff = csize << (n - 9);
			}
			res = RES_OK;
		}
		break;

	case GET_SECTOR_SIZE :	// Get sector size (WORD) 
		*(WORD*)buff = 512;
		res = RES_OK;
		break;

	case GET_BLOCK_SIZE :	// Get erase block size in unit of sector (DWORD) 
		if (CardType & CT_SD2) {	// SDv2? 
			if (MMC_send_cmd(ACMD13, 0) == 0) {	// Read SD status 
				MMC_SendSPI(0xFF);
				if (MMC_ReceiveDataBlock(csd, 16)) {				// Read partial block 
					for (n = 64 - 16; n; n--) MMC_SendSPI(0xFF);	// Purge trailing data 
					*(DWORD*)buff = 16UL << (csd[10] >> 4);
					res = RES_OK;
				}
			}
		} else {					// SDv1 or MMCv3 
			if ((MMC_send_cmd(CMD9, 0) == 0) && MMC_ReceiveDataBlock(csd, 16)) {	// Read CSD 
				if (CardType & CT_SD1) {	// SDv1 
					*(DWORD*)buff = (((WORD)(csd[10] & 63) << 1) + ((WORD)(csd[11] & 128) >> 7) + 1) << ((csd[13] >> 6) - 1);
				} else {					// MMCv3 
					*(DWORD*)buff = ((DWORD)((csd[10] & 124) >> 2) + 1) * ((BYTE)(((csd[11] & 3) << 3) + ((csd[11] & 224) >> 5) + 1));
				}
				res = RES_OK;
			}
		}
		break;
#if _USE_ERASE
	case CTRL_ERASE_SECTOR :	// Erase a block of sectors (used when _USE_ERASE == 1) 
		if (!(CardType & CT_SDC)) break;				// Check if the card is SDC 
		if (disk_ioctl(drv, MMC_GET_CSD, csd)) break;	// Get CSD 
		if (!(csd[0] >> 6) && !(csd[10] & 0x40)) break;	// Check if sector erase can be applied to the card 
		dp = buff; st = dp[0]; ed = dp[1];				// Load sector block 
		if (!(CardType & CT_BLOCK)) {
			st *= 512; ed *= 512;
		}
		if (send_cmd(CMD32, st) == 0 && send_cmd(CMD33, ed) == 0 && send_cmd(CMD38, 0) == 0 && wait_ready(30000))	// Erase sector block 
			res = RES_OK;	// FatFs does not check result of this command 
		break;
#endif
#if CMD_FATFS_NOT_USED
	// Following commands are never used by FatFs module 
	case MMC_GET_TYPE :		// Get card type flags (1 byte)
		*ptr = CardType;
		res = RES_OK;
		break;

	case MMC_GET_CSD :		// Receive CSD as a data block (16 bytes)
		if (MMC_send_cmd(CMD9, 0) == 0		// READ_CSD
			&& MMC_ReceiveDataBlock(ptr, 16))
			res = RES_OK;
		break;

	case MMC_GET_CID :		// Receive CID as a data block (16 bytes)
		if (MMC_send_cmd(CMD10, 0) == 0		// READ_CID
			&& MMC_ReceiveDataBlock(ptr, 16))
			res = RES_OK;
		break;

	case MMC_GET_OCR :		// Receive OCR as an R3 resp (4 bytes)
		if (MMC_send_cmd(CMD58, 0) == 0) {	// READ_OCR
			for (n = 4; n; n--) *ptr++ = MMC_SendSPI(0xFF);
			res = RES_OK;
		}
		break;

	case MMC_GET_SDSTAT :	// Receive SD statsu as a data block (64 bytes)
		if (MMC_send_cmd(ACMD13, 0) == 0) {	// SD_STATUS
			MMC_SendSPI(0xFF);
			if (MMC_ReceiveDataBlock(ptr, 64))
				res = RES_OK;
		}
		break;
	case CTRL_POWER:
		switch (ptr[0]) {
		case 0:		// Sub control code (POWER_OFF) 
			MMC_ChipEnable(false);
			res = RES_OK;
			break;
		case 1:		// Sub control code (POWER_GET) 
			ptr[1] = MMC_IsChipEnable();
			res = RES_OK;
			break;
		default :
			res = RES_PARERR;
		}
		return res;
#endif
	default:
		res = RES_PARERR;
	}

	MMC_deselect();

	return res;
}
