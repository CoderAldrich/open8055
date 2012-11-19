/********************************************************************
 FileName:     	HardwareProfile - PIC18F2550.h
 Dependencies:  See INCLUDES section
 Processor:     PIC18F2550 USB Microcontroller
 Hardware:      Open8055 based on original Vellaman K8055
 Compiler:      Microchip C18
 Company:       Microchip Technology, Inc.

 Software License Agreement:

 The software supplied herewith by Microchip Technology Incorporated
 (the �Company�) for its PIC� Microcontroller is intended and
 supplied to you, the Company�s customer, for use solely and
 exclusively on Microchip PIC Microcontroller products. The
 software is owned by the Company and/or its supplier, and is
 protected under applicable copyright laws. All rights are reserved.
 Any use in violation of the foregoing restrictions may subject the
 user to criminal sanctions under applicable laws, as well as to
 civil liability for the breach of the terms and conditions of this
 license.

 THIS SOFTWARE IS PROVIDED IN AN �AS IS� CONDITION. NO WARRANTIES,
 WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED
 TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE COMPANY SHALL NOT,
 IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
 CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.

********************************************************************
 File Description:

 Change History:
  Rev   Date         Description
  1.0   11/19/2004   Initial release
  2.1   02/26/2007   Updated for simplicity and to use common
                     coding style
  2.3   09/15/2008   Broke out each hardware platform into its own
                     "HardwareProfile - xxx.h" file
                     
  Open8055
  
  0.1	11/13/2012	Adjusted for Open8055 Firmware on modified
  					K8055 board
********************************************************************/

#ifndef HARDWARE_PROFILE_PIC18F2550_H
#define HARDWARE_PROFILE_PIC18F2550_H

    /*******************************************************************/
    /******** USB stack hardware selection options *********************/
    /*******************************************************************/
    //This section is the set of definitions required by the MCHPFSUSB
    //  framework.  These definitions tell the firmware what mode it is
    //  running in, and where it can find the results to some information
    //  that the stack needs.
    //These definitions are required by every application developed with
    //  this revision of the MCHPFSUSB framework.  Please review each
    //  option carefully and determine which options are desired/required
    //  for your application.

    //Uncomment the following line to make the output HEX of this 
    //  project work with the HID Bootloader
    #define PROGRAMMABLE_WITH_USB_HID_BOOTLOADER		

    /*******************************************************************/
    /*******************************************************************/
    /*******************************************************************/
    /******** Application specific definitions *************************/
    /*******************************************************************/
    /*******************************************************************/
    /*******************************************************************/

    /** Board definition ***********************************************/
    //These defintions will tell the main() function which board is
    //  currently selected.  This will allow the application to add
    //  the correct configuration bits as wells use the correct
    //  initialization functions for the board.  These defitions are only
    //  required in the stack provided demos.  They are not required in
    //  final application design.
    #define OPEN8055_PIC18F24J50
    #define CLOCK_FREQ 48000000

    /** SELF POWER *****************************************************/

    #define tris_usb_bus_sense  
    #define usb_bus_sense       1
    #define tris_self_power     
    #define self_power          1
    
    /** IO PORT SETTINGS ***********************************************/

    //Port A
    //	RA0, RA1 are analog inputs
    //	RA2 is the pull up/low for address jumpers
    //	RA3, RA5 are digital inputs I1, I2
    #define OPEN8055_TRISA 0xEB
    
    //Port B
    //	RB0..RB7 are digital outputs D1..D8
    #define OPEN8055_TRISB 0x00
    
    //Port C
    //	RC0 is digital input I3
    //	RC1, RC2 are PWM outputs
    //  RC4, RC5 are the USB data pins
    //	RC6, RC7 are digital inputs I4, I5
	//  During boot however we use RC1, RC2 as inputs
	//  to detect the state of the card address jumpers
    #define OPEN8055_TRISC_J 0xC9
    #define OPEN8055_TRISC 0xCF

    /** SWITCHES *******************************************************/

	#define OPEN8055sw1	PORTAbits.RA3
	#define OPEN8055sw2	PORTAbits.RA5
	#define OPEN8055sw3	PORTCbits.RC0
	#define OPEN8055sw4	PORTCbits.RC6
	#define OPEN8055sw5	PORTCbits.RC7

    /** ADDRESS JUMPER *************************************************/

	#define OPEN8055sk56power PORTAbits.RA2
    #define OPEN8055sk5	PORTCbits.RC1
    #define OPEN8055sk6	PORTCbits.RC2
    
    /** OUPUT PORTS ****************************************************/

    #define OPEN8055d1	PORTBbits.RB0
    #define OPEN8055d2	PORTBbits.RB1
    #define OPEN8055d3	PORTBbits.RB2
    #define OPEN8055d4	PORTBbits.RB3
    #define OPEN8055d5	PORTBbits.RB4
    #define OPEN8055d6	PORTBbits.RB5
    #define OPEN8055d7	PORTBbits.RB6
    #define OPEN8055d8	PORTBbits.RB7
    
    /** ADC CONFIG *****************************************************/

	// set up ADC we need to wait a least 2us after this before setting GO/DONE
	// 2us = 8*Tcy (at 4MHz bus clock)
	
	// ADCON1
	// bit 7 ADFM
	// bit 6 ADCAL
	// bit 5-3 ACQT2:ACQT0: A/D Acquisition Time Select bits
	//		111 = 20 TAD
	//		110 = 16 TAD
	//		101 = 12 TAD
	//		100 = 8 TAD
	//		011 = 6 TAD
	//		010 = 4 TAD
	//		001 = 2 TAD
	//		000 = 0 TAD(1)
	// bit 2-0 ADCS2:ADCS0: A/D Conversion Clock Select bits
	//		111 = FRC (clock derived from A/D RC oscillator)(1)
	//		110 = FOSC/64
	//		101 = FOSC/16
	//		100 = FOSC/4
	//		011 = FRC (clock derived from A/D RC oscillator)(1)
	//		010 = FOSC/32
	//		001 = FOSC/8
	//		000 = FOSC/2
	#define OPEN8055_ADCON1 0b10101010 // 20TAD,FOSC/32


	//ADCON0
	// bit7..6 	- Reference voltage (00 = use rails)
	// bit5..2 	- CHS3:0, channel select
	// bit1		- GO/DONE
	// bit1		- ADON
	#define OPEN8055_ADCON0 0x01

	//ANCON0/1
	#define OPEN8055_ANCON0 0xFC
	#define OPEN8055_ANCON1	0x1F
    
    /** PWM CONFIG *****************************************************/
	
	//The original K8055 PWM runs at 23.43 kHz. 
	//At 96 MHz Fosc with a Timer2 prescale of 4 
	// 1 / ((255+1) * 4 * (1/Fosc) * 4) = 23,437
	#define OPEN8055_T2CKPS0	1
	#define OPEN8055_T2CKPS1	0
	#define OPEN8055_PWMPR2		127
	#define OPEN8055_CCP1CON	0x0C
	#define OPEN8055_CCP2CON	0x0C
    
    /** I/O pin definitions ********************************************/
    #define INPUT_PIN 1
    #define OUTPUT_PIN 0
#endif  //HARDWARE_PROFILE_PIC18F2550_H
