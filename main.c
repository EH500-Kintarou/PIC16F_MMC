// PIC16F18857 Configuration Bit Settings

// 'C' source line config statements

// CONFIG1
#pragma config FEXTOSC = OFF    // External Oscillator mode selection bits (Oscillator not enabled)
#pragma config RSTOSC = HFINT32 // Power-up default value for COSC bits (HFINTOSC with OSCFRQ= 32 MHz and CDIV = 1:1)
#pragma config CLKOUTEN = OFF   // Clock Out Enable bit (CLKOUT function is disabled; i/o or oscillator function on OSC2)
#pragma config CSWEN = OFF      // Clock Switch Enable bit (The NOSC and NDIV bits cannot be changed by user software)
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor Enable bit (FSCM timer disabled)

// CONFIG2
#pragma config MCLRE = ON       // Master Clear Enable bit (MCLR pin is Master Clear function)
#pragma config PWRTE = ON       // Power-up Timer Enable bit (PWRT enabled)
#pragma config LPBOREN = OFF    // Low-Power BOR enable bit (ULPBOR disabled)
#pragma config BOREN = ON       // Brown-out reset enable bits (Brown-out Reset Enabled, SBOREN bit is ignored)
#pragma config BORV = LO        // Brown-out Reset Voltage Selection (Brown-out Reset Voltage (VBOR) set to 1.9V on LF, and 2.45V on F Devices)
#pragma config ZCD = OFF        // Zero-cross detect disable (Zero-cross detect circuit is disabled at POR.)
#pragma config PPS1WAY = OFF    // Peripheral Pin Select one-way control (The PPSLOCK bit can be set and cleared repeatedly by software)
#pragma config STVREN = ON      // Stack Overflow/Underflow Reset Enable bit (Stack Overflow or Underflow will cause a reset)

// CONFIG3
#pragma config WDTCPS = WDTCPS_31// WDT Period Select bits (Divider ratio 1:65536; software control of WDTPS)
#pragma config WDTE = OFF       // WDT operating mode (WDT Disabled, SWDTEN is ignored)
#pragma config WDTCWS = WDTCWS_7// WDT Window Select bits (window always open (100%); software control; keyed access not required)
#pragma config WDTCCS = SC      // WDT input clock selector (Software Control)

// CONFIG4
#pragma config WRT = OFF        // UserNVM self-write protection bits (Write protection off)
#pragma config SCANE = available// Scanner Enable bit (Scanner module is available for use)
#pragma config LVP = ON         // Low Voltage Programming Enable bit (Low Voltage programming enabled. MCLR/Vpp pin function is MCLR.)

// CONFIG5
#pragma config CP = OFF         // UserNVM Program memory code protection bit (Program Memory code protection disabled)
#pragma config CPD = OFF        // DataNVM code protection bit (Data EEPROM code protection disabled)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#define _XTAL_FREQ 32e6
#include <xc.h>
#include <stdbool.h>
#include <string.h>
#include "FatFs/diskio_hardware.h"
#include "FatFs/ff.h"

#define	LED_TRIS	(TRISBbits.TRISB5)
#define	LED_LAT		(LATBbits.LATB5)
#define	LED(on)		(LED_LAT = (on))

static FATFS fs;
static FIL fp;

void MMC_AccessLamp(bool on)
{
	LED(on);
}

void setup()
{
	ANSELA = 0x00;
	ANSELB = 0x00;
	ANSELC = 0x00;

	TRISA = 0xFF;
	TRISB = 0xFF;
	TRISC = 0xFF;
	
	LED(false);
	LED_TRIS = 0;
	
	MMC_Init();
}

void loop()
{
	f_mount(&fs, "0:", 0);
	if(f_open(&fp, "TEST.TXT", FA_OPEN_APPEND | FA_WRITE | FA_READ) == FR_OK) {
		f_puts("Hello, world!!\n", &fp);

		f_close(&fp);
	}

	f_unmount("0:");
	
	__delay_ms(1000);
}

void main(void)
{
	setup();
	while(1)
		loop();
}

void __interrupt() isr(void)
{
    MMC_Interrupt();
}
