/********************************************************************
 FileName:     main.c
 Dependencies: See INCLUDES section
 Processor:		PIC18F USB Microcontroller
 Hardware:		Velleman K8055(N) with PIC18F2x(J)50
 Complier:  	Microchip C18 (for PIC18)
 Company:		Microchip Technology, Inc.

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
  Rev   Description
  ----  -----------------------------------------
  1.0   Initial release
  2.1   Updated for simplicity and to use common
                     coding style
  2.7b  Improvements to USBCBSendResume(), to make it easier to use.
  
  Open8055
  
  0.1	11/13/1012	Adopted for Open8055
  
********************************************************************/

/** INCLUDES *******************************************************/

#include "HardwareProfile.h"

#include "./USB/usb.h"
#include "./USB/usb_function_hid.h"

#include "usb_config.h"
#include "open8055_hid_protocol.h"

/** CONFIGURATION **************************************************/


/* ----
 * If you get this error, please add a line
 * OPEN8055_PLLDIV=n
 * according to your boards crystal speed under
 * Project->Build Options->Project in the tab MPLAB C18 tab.
 */
#if !defined(OPEN8055_PLLDIV)
    #error "OPEN8055_PLLDIV has not been defined in the Project's preprocessor settings"
#endif

#if defined(OPEN8055_PIC18F2550)
        #pragma config CPUDIV   = OSC1_PLL2	
        #pragma config PLLDIV	= OPEN8055_PLLDIV
        #pragma config USBDIV   = 2         // Clock source from 96MHz PLL/2
        #pragma config FOSC     = HSPLL_HS
        #pragma config FCMEN    = OFF
        #pragma config IESO     = OFF
        #pragma config PWRT     = OFF
        #pragma config BOR      = ON
        #pragma config BORV     = 3
        #pragma config VREGEN   = ON		//USB Voltage Regulator
        #pragma config WDT      = OFF
        #pragma config WDTPS    = 32768
        #pragma config MCLRE    = ON
        #pragma config LPT1OSC  = OFF
        #pragma config PBADEN   = OFF
//      #pragma config CCP2MX   = ON
        #pragma config STVREN   = ON
        #pragma config LVP      = OFF
//      #pragma config ICPRT    = OFF       // Dedicated In-Circuit Debug/Programming
        #pragma config XINST    = OFF       // Extended Instruction Set
        #pragma config CP0      = OFF
        #pragma config CP1      = OFF
//      #pragma config CP2      = OFF
//      #pragma config CP3      = OFF
        #pragma config CPB      = OFF
//      #pragma config CPD      = OFF
        #pragma config WRT0     = OFF
        #pragma config WRT1     = OFF
//      #pragma config WRT2     = OFF
//      #pragma config WRT3     = OFF
        #pragma config WRTB     = OFF       // Boot Block Write Protection
        #pragma config WRTC     = OFF
//      #pragma config WRTD     = OFF
        #pragma config EBTR0    = OFF
        #pragma config EBTR1    = OFF
//      #pragma config EBTR2    = OFF
//      #pragma config EBTR3    = OFF
        #pragma config EBTRB    = OFF
        
#elif defined(OPEN8055_PIC18F24J50)
        #pragma config WDTEN = OFF          //WDT disabled (enabled by SWDTEN bit)
        #pragma config PLLDIV = OPEN8055_PLLDIV
        #pragma config STVREN = ON            //stack overflow/underflow reset enabled
        #pragma config XINST = OFF          //Extended instruction set disabled
        #pragma config CPUDIV = OSC1        //No CPU system clock divide
        #pragma config CP0 = OFF            //Program memory is not code-protected
        #pragma config OSC = HSPLL          //HS oscillator, PLL enabled, HSPLL used by USB
        #pragma config T1DIG = OFF          //Sec Osc clock source may not be selected, unless T1OSCEN = 1
        #pragma config LPT1OSC = OFF        //high power Timer1 mode
        #pragma config FCMEN = OFF          //Fail-Safe Clock Monitor disabled
        #pragma config IESO = OFF           //Two-Speed Start-up disabled
        #pragma config WDTPS = 32768        //1:32768
        #pragma config DSWDTOSC = INTOSCREF //DSWDT uses INTOSC/INTRC as clock
        #pragma config RTCOSC = T1OSCREF    //RTCC uses T1OSC/T1CKI as clock
        #pragma config DSBOREN = OFF        //Zero-Power BOR disabled in Deep Sleep
        #pragma config DSWDTEN = OFF        //Disabled
        #pragma config DSWDTPS = 8192       //1:8,192 (8.5 seconds)
        #pragma config IOL1WAY = OFF        //IOLOCK bit can be set and cleared
        #pragma config MSSP7B_EN = MSK7     //7 Bit address masking
        #pragma config WPFP = PAGE_1        //Write Protect Program Flash Page 0
        #pragma config WPEND = PAGE_0       //Start protection at page 0
        #pragma config WPCFG = OFF          //Write/Erase last page protect Disabled
        #pragma config WPDIS = OFF          //WPFP[5:0], WPEND, and WPCFG bits ignored 
#else
	    #error "Unsupported Processor type file __FILE__ line __LINE__"
#endif

/** USB IO Buffers *************************************************/
#if defined(OPEN8055_PIC18F2550)
    #pragma udata USB_VARIABLES=0x500
#elif defined(OPEN8055_PIC18F24J50)
	#pragma udata
#else
    #error "Unsupported processor in file __FILE__, line __LINE__"
#endif

Open8055_hidMessage_t receivedDataBuffer;
Open8055_hidMessage_t toSendDataBuffer;

/** VARIABLES ******************************************************/
#pragma udata

USB_HANDLE	outputHandle = 0;
USB_HANDLE 	inputHandle = 0;

uint8_t		cardAddress = 0;
uint8_t		cardConnected = 0;

uint8_t		tickCounter = 0;
uint8_t		tickMillisecond = 0;
uint16_t	tickSecond = 0;


struct Open8055_status_t {
    uint8_t		outputBits;
} status;	


/** PRIVATE PROTOTYPES *********************************************/
void highPriorityISRCode();
void lowPriorityISRCode();

static void initializeSystem(void);
static void userInit(void);
static void processIO(void);

void USBCBSendResume(void);

/** VECTOR REMAPPING ***********************************************/

//On PIC18 devices, addresses 0x00, 0x08, and 0x18 are used for
//the reset, high priority interrupt, and low priority interrupt
//vectors.  However, the current Open8055 boot loader occupies
//addresses 0x0000-0x11FF. Therefore,
//the bootloader code remaps these vectors to new locations
//as indicated below.  This remapping is only necessary if you
//wish to program the hex file generated from this project with
//the USB bootloader.  If no bootloader is used, edit the
//usb_config.h file and comment out the following define:
//#define PROGRAMMABLE_WITH_USB_HID_BOOTLOADER

#if defined(PROGRAMMABLE_WITH_USB_HID_BOOTLOADER)
  #if defined(OPEN8055_PIC18F2550)
	#define REMAPPED_RESET_VECTOR_ADDRESS			0x1200
	#define REMAPPED_HIGH_INTERRUPT_VECTOR_ADDRESS	0x1208
	#define REMAPPED_LOW_INTERRUPT_VECTOR_ADDRESS	0x1218
  #elif defined(OPEN8055_PIC18F24J50)
	#define REMAPPED_RESET_VECTOR_ADDRESS			0x1400
	#define REMAPPED_HIGH_INTERRUPT_VECTOR_ADDRESS	0x1408
	#define REMAPPED_LOW_INTERRUPT_VECTOR_ADDRESS	0x1418
  #else
    #error "Unsupported processor in file __FILE__, line __LINE__"
  #endif
#else	
	#define REMAPPED_RESET_VECTOR_ADDRESS			0x00
	#define REMAPPED_HIGH_INTERRUPT_VECTOR_ADDRESS	0x08
	#define REMAPPED_LOW_INTERRUPT_VECTOR_ADDRESS	0x18
#endif

#if defined(PROGRAMMABLE_WITH_USB_HID_BOOTLOADER)
extern void _startup (void);        // See c018i.c in your C18 compiler dir
#pragma code REMAPPED_RESET_VECTOR = REMAPPED_RESET_VECTOR_ADDRESS
void _reset (void)
{
    _asm goto _startup _endasm
}
#endif

#pragma code REMAPPED_HIGH_INTERRUPT_VECTOR = REMAPPED_HIGH_INTERRUPT_VECTOR_ADDRESS
void remappedHighISR (void)
{
     _asm goto highPriorityISRCode _endasm
}

#pragma code REMAPPED_LOW_INTERRUPT_VECTOR = REMAPPED_LOW_INTERRUPT_VECTOR_ADDRESS
void remappedLowISR (void)
{
     _asm goto lowPriorityISRCode _endasm
}

#if defined(PROGRAMMABLE_WITH_USB_HID_BOOTLOADER)
//Note: If this project is built while one of the bootloaders has
//been defined, but then the output hex file is not programmed with
//the bootloader, addresses 0x08 and 0x18 would end up programmed with 0xFFFF.
//As a result, if an actual interrupt was enabled and occured, the PC would jump
//to 0x08 (or 0x18) and would begin executing "0xFFFF" (unprogrammed space).  This
//executes as nop instructions, but the PC would eventually reach the REMAPPED_RESET_VECTOR_ADDRESS
//(0x1000 or 0x800, depending upon bootloader), and would execute the "goto _startup".  This
//would effective reset the application.

//To fix this situation, we should always deliberately place a 
//"goto REMAPPED_HIGH_INTERRUPT_VECTOR_ADDRESS" at address 0x08, and a
//"goto REMAPPED_LOW_INTERRUPT_VECTOR_ADDRESS" at address 0x18.  When the output
//hex file of this project is programmed with the bootloader, these sections do not
//get bootloaded (as they overlap the bootloader space).  If the output hex file is not
//programmed using the bootloader, then the below goto instructions do get programmed,
//and the hex file still works like normal.  The below section is only required to fix this
//scenario.
#pragma code HIGH_INTERRUPT_VECTOR = 0x08
void highISR (void)
{
     _asm goto REMAPPED_HIGH_INTERRUPT_VECTOR_ADDRESS _endasm
}
#pragma code LOW_INTERRUPT_VECTOR = 0x18
void lowISR (void)
{
     _asm goto REMAPPED_LOW_INTERRUPT_VECTOR_ADDRESS _endasm
}
#endif	//end of "#if defined(PROGRAMMABLE_WITH_USB_HID_BOOTLOADER)||defined(PROGRAMMABLE_WITH_USB_LEGACY_CUSTOM_CLASS_BOOTLOADER)"

#pragma code


//This is the actual high priority interrupt hander.
#pragma interrupt highPriorityISRCode
void highPriorityISRCode()
{
	//Check which interrupt flag caused the interrupt.
	//Service the interrupt
	//Clear the interrupt flag
	//Etc.
	
	if (PIR2bits.TMR3IF)
	{
		unsigned int increment;
		
		increment = OPEN8055_TICK_TIMER_CYCLES;
		
		// Set Timer3 to the next ticker timeout. We do all the math
		// with the timer disabled to avoid roll-over issues.
		T3CONbits.TMR3ON = 0;
		increment = ~(increment - (TMR3L | ((unsigned int)TMR3H << 8))); 
		TMR3L = increment & 0xFF;
		TMR3H = (increment >> 8) & 0xFF;
		T3CONbits.TMR3ON = 1;
		
		// Count ticks seen and clear the interrupt flag.
		tickCounter++;
		PIR2bits.TMR3IF = 0;
	}	

}	//This return will be a "retfie fast", since this is in a #pragma interrupt section 


//This is the actual low priority interrupt hander.
#pragma interruptlow lowPriorityISRCode
void lowPriorityISRCode()
{
	//Check which interrupt flag caused the interrupt.
	//Service the interrupt
	//Clear the interrupt flag
	//Etc.

    #if defined(USB_INTERRUPT)
    	USBDeviceTasks();
    #endif
}	//This return will be a "retfie", since this is in a #pragma interruptlow section 


/** DECLARATIONS ***************************************************/
#pragma code

/********************************************************************
 * Function:        void main(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        Main program entry point.
 *
 * Note:            None
 *******************************************************************/
#if defined(__18CXX)
void main(void)
#else
int main(void)
#endif
{   
    #if defined(OPEN8055_PIC18F24J50)
	//On the PIC18F87J50 Family of USB microcontrollers, the PLL will not power up and be enabled
	//by default, even if a PLL enabled oscillator configuration is selected (such as HS+PLL).
	//This allows the device to power up at a lower initial operating frequency, which can be
	//advantageous when powered from a source which is not gauranteed to be adequate for 48MHz
	//operation.  On these devices, user firmware needs to manually set the OSCTUNE<PLLEN> bit to
	//power up the PLL.
    {
        unsigned int pll_startup_counter = 600;
        OSCTUNEbits.PLLEN = 1;  //Enable the PLL and wait 2+ms until the PLL locks before enabling USB module
        while(pll_startup_counter--);
    }
    //Device switches over automatically to PLL output after PLL is locked and ready.
    #endif

    initializeSystem();

    #if defined(USB_INTERRUPT)
        USBDeviceAttach();
    #endif

    while(1)
    {
        #if defined(USB_POLLING)
		// Check bus status and service USB interrupts.
        USBDeviceTasks(); // Interrupt or polling method.  If using polling, must call
        				  // this function periodically.  This function will take care
        				  // of processing and responding to SETUP transactions 
        				  // (such as during the enumeration process when you first
        				  // plug in).  USB hosts require that USB devices should accept
        				  // and process SETUP packets in a timely fashion.  Therefore,
        				  // when using polling, this function should be called 
        				  // regularly (such as once every 1.8ms or faster** [see 
        				  // inline code comments in usb_device.c for explanation when
        				  // "or faster" applies])  In most cases, the USBDeviceTasks() 
        				  // function does not take very long to execute (ex: <100 
        				  // instruction cycles) before it returns.
        #endif
 
		// Application-specific tasks.
		// Application related code may be added here, or in the ProcessIO() function.
        processIO();        
    }//end while
}//end main


/********************************************************************
 * Function:        static void initializeSystem(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        InitializeSystem is a centralize initialization
 *                  routine. All required USB initialization routines
 *                  are called from here.
 *
 *                  User application initialization routine should
 *                  also be called from here.                  
 *
 * Note:            None
 *******************************************************************/
static void initializeSystem(void)
{
    userInit();			//Initialize according to Hardware Profile
    
    USBDeviceInit();	//usb_device.c.  Initializes USB module SFRs and firmware
    					//variables to known states.
}//end initializeSystem



/******************************************************************************
 * Function:        static void userInit(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This routine should take care of all of the code
 *                  initialization that is required.
 *
 * Note:            
 *
 *****************************************************************************/
static void userInit(void)
{
	#if defined(USB_USER_DEVICE_DESCRIPTOR_INCLUDE)
		USB_USER_DEVICE_DESCRIPTOR_INCLUDE;
	#endif

    //initialize the variable holding the handle for the last
    // transmission
    outputHandle = 0;
    inputHandle = 0;

	//Set all ports to digital
	#if defined(OPEN8055_PIC18F2550)
	    ADCON1 |= OPEN8055_ADCON1_ALL_DIGITAL_MASK;
	#elif defined(OPEN8055_PIC18F24J50)
		ANCON0 = OPEN8055_ANCON0;
		ANCON1 = OPEN8055_ANCON1;
	#else
		#error "Unsupported processor in file __FILE__, line __LINE__"
	#endif

    //Configure all IO ports (definitions are in "HardwareProfile_PIC18F*.h")
    TRISA = OPEN8055_TRISA;
    TRISB = OPEN8055_TRISB;
    TRISC = OPEN8055_TRISC;
    
    PORTB = 0x00;
    CCPR1L = 0;
    CCPR2L = 0;
    
	//Configure ADC (definitions are in "HardwareProfile_PIC18F*.h")
	#if defined(OPEN8055_PIC18F2550)
		ADCON1 = OPEN8055_ADCON1;
		ADCON2 = OPEN8055_ADCON2;
		ADCON0 = OPEN8055_ADCON0;
	#elif defined(OPEN8055_PIC18F24J50)
		ADCON1 = OPEN8055_ADCON1;
		ADCON0 = OPEN8055_ADCON0;
	#else
		#error "Unsupported processor in file __FILE__, line __LINE__"
	#endif
	

	//Make sure that interrupt priotities and high priority interrupts
	//are enabled.
	RCONbits.IPEN	= 1;
	INTCONbits.GIEH	= 1;
/*
	//Setup PWM configuration including timer2
	T2CONbits.T2CKPS0 = OPEN8055_T2CKPS0;
	T2CONbits.T2CKPS1 = OPEN8055_T2CKPS1;
	PR2 = OPEN8055_PWMPR2;

	CCPR1 = 0x0000;
	CCPR2 = 0x0000;
	CCP1CON = OPEN8055_CCP1CON;
	CCP2CON = OPEN8055_CCP2CON;

    T2CONbits.TMR2ON = 1;
*/
	//Enable Timer3 for our internal ticker
	T3CON = 0x04;				//Timer3 is using internal 12 MHz clock
	TMR3H = 0xFB;
	TMR3L = 0x50;
	IPR2bits.TMR3IP = 1;		//Timer3 will trigger high priority interrupt
	PIR2bits.TMR3IF = 0;		//Reset the timer1 flag
	PIE2bits.TMR3IE = 1;
	
	T3CONbits.TMR3ON = 1;		//and turn it on.


	//Determine the cardAddress depending on sk5 and sk6.
	#if defined(OPEN8055_BOARD1)
		cardAddress = ((OPEN8055sk6) ? 2 : 0) + ((OPEN8055sk5) ? 1 : 0);
	#elif defined(OPEN8055_BOARD2)
		OPEN8055sk56power = 1;
		{int i = 100; while(--i);}
		cardAddress = ((OPEN8055sk6) ? 0 : 2) + ((OPEN8055sk5) ? 0 : 1);
		OPEN8055sk56power = 0;
		TRISC = OPEN8055_TRISC_2;
	#else
		#error "Unsupported Board type file __FILE__ line __LINE__"
	#endif
	#if defined(USB_USER_DEVICE_DESCRIPTOR_INCLUDE)
		device_dsc.idProduct = (device_dsc.idProduct & 0xFFF0) | cardAddress;
	#endif
}//end userInit

/********************************************************************
 * Function:        static void processIO(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This function is a place holder for other user
 *                  routines. It is a mixture of both USB and
 *                  non-USB tasks.
 *
 * Note:            None
 *******************************************************************/
static void processIO(void)
{
	uint8_t	ticksSeen;
	
    // Check if we are USB connected
    if((USBDeviceState < CONFIGURED_STATE)||(USBSuspendControl==1))
    	cardConnected = 0;
	else
    	cardConnected = 1;
    
    // Check if data was received from the host.
    if(cardConnected && !HIDRxHandleBusy(outputHandle))
    {   
        // Process host message
        switch (receivedDataBuffer.msgType)
		{
			// OUTPUT message containing digital output ports and
			// PWM values.
			case OPEN8055_HID_MESSAGE_OUTPUT:
				status.outputBits = receivedDataBuffer.outputBits;
				PORTB = status.outputBits;
				break;
			
			// Ignore unknown message types.	
			default:
				break;
		}	 
        
        //Re-arm the OUT endpoint for the next packet
        outputHandle = HIDRxPacket(HID_EP, (BYTE*)&receivedDataBuffer, sizeof(receivedDataBuffer));
    }

	// Count timer ticks
	ticksSeen = tickCounter;
	tickCounter -= ticksSeen;
	
	// Timer related code
	while (ticksSeen-- > 0)
	{
		// Per 100 microsecond code comes here
		
		if (++tickMillisecond >= OPEN8055_TICKS_PER_MS)
		{
			tickMillisecond = 0;
			
			// Per 1 millisecond code comes here

			if (++tickSecond >= 1000)
			{
				tickSecond = 0;
				
				// Per 1 second code comes here
			}	
		}	
	}	
	
	if (cardConnected && !HIDTxHandleBusy(inputHandle))
	{
		// Time to send a report.
		
		// Construct a standard input state report.
		memset(&toSendDataBuffer, 0, sizeof(toSendDataBuffer));
		toSendDataBuffer.msgType		= OPEN8055_HID_MESSAGE_INPUT;
		toSendDataBuffer.inputBits		= OPEN8055sw1 |
										  (OPEN8055sw2 << 1) |
										  (OPEN8055sw3 << 2) |
										  (OPEN8055sw4 << 3) |
										  (OPEN8055sw5 << 4);
		
		inputHandle = HIDTxPacket(HID_EP, (BYTE*)&toSendDataBuffer, sizeof(toSendDataBuffer));	
	}

}//end processIO


// ******************************************************************************************************
// ************** USB Callback Functions ****************************************************************
// ******************************************************************************************************
// The USB firmware stack will call the callback functions USBCBxxx() in response to certain USB related
// events.  For example, if the host PC is powering down, it will stop sending out Start of Frame (SOF)
// packets to your device.  In response to this, all USB devices are supposed to decrease their power
// consumption from the USB Vbus to <2.5mA each.  The USB module detects this condition (which according
// to the USB specifications is 3+ms of no bus activity/SOF packets) and then calls the USBCBSuspend()
// function.  You should modify these callback functions to take appropriate actions for each of these
// conditions.  For example, in the USBCBSuspend(), you may wish to add code that will decrease power
// consumption from Vbus to <2.5mA (such as by clock switching, turning off LEDs, putting the
// microcontroller to sleep, etc.).  Then, in the USBCBWakeFromSuspend() function, you may then wish to
// add code that undoes the power saving things done in the USBCBSuspend() function.

// The USBCBSendResume() function is special, in that the USB stack will not automatically call this
// function.  This function is meant to be called from the application firmware instead.  See the
// additional comments near the function.

/******************************************************************************
 * Function:        void USBCBSuspend(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        Call back that is invoked when a USB suspend is detected
 *
 * Note:            None
 *****************************************************************************/
void USBCBSuspend(void)
{
	//Example power saving code.  Insert appropriate code here for the desired
	//application behavior.  If the microcontroller will be put to sleep, a
	//process similar to that shown below may be used:
	
	//ConfigureIOPinsForLowPower();
	//SaveStateOfAllInterruptEnableBits();
	//DisableAllInterruptEnableBits();
	//EnableOnlyTheInterruptsWhichWillBeUsedToWakeTheMicro();	//should enable at least USBActivityIF as a wake source
	//Sleep();
	//RestoreStateOfAllPreviouslySavedInterruptEnableBits();	//Preferrably, this should be done in the USBCBWakeFromSuspend() function instead.
	//RestoreIOPinsToNormal();									//Preferrably, this should be done in the USBCBWakeFromSuspend() function instead.

	//IMPORTANT NOTE: Do not clear the USBActivityIF (ACTVIF) bit here.  This bit is 
	//cleared inside the usb_device.c file.  Clearing USBActivityIF here will cause 
	//things to not work as intended.	
	

    #if defined(__C30__)
        //This function requires that the _IPL level be something other than 0.
        //  We can set it here to something other than 
        #ifndef DSPIC33E_USB_STARTER_KIT
        _IPL = 1;
        USBSleepOnSuspend();
        #endif
    #endif
}


/******************************************************************************
 * Function:        void _USB1Interrupt(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This function is called when the USB interrupt bit is set
 *					In this example the interrupt is only used when the device
 *					goes to sleep when it receives a USB suspend command
 *
 * Note:            None
 *****************************************************************************/
#if 0
void __attribute__ ((interrupt)) _USB1Interrupt(void)
{
    #if !defined(self_powered)
        if(U1OTGIRbits.ACTVIF)
        {
            IEC5bits.USB1IE = 0;
            U1OTGIEbits.ACTVIE = 0;
            IFS5bits.USB1IF = 0;
        
            //USBClearInterruptFlag(USBActivityIFReg,USBActivityIFBitNum);
            USBClearInterruptFlag(USBIdleIFReg,USBIdleIFBitNum);
            //USBSuspendControl = 0;
        }
    #endif
}
#endif

/******************************************************************************
 * Function:        void USBCBWakeFromSuspend(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        The host may put USB peripheral devices in low power
 *					suspend mode (by "sending" 3+ms of idle).  Once in suspend
 *					mode, the host may wake the device back up by sending non-
 *					idle state signalling.
 *					
 *					This call back is invoked when a wakeup from USB suspend 
 *					is detected.
 *
 * Note:            None
 *****************************************************************************/
void USBCBWakeFromSuspend(void)
{
	// If clock switching or other power savings measures were taken when
	// executing the USBCBSuspend() function, now would be a good time to
	// switch back to normal full power run mode conditions.  The host allows
	// a few milliseconds of wakeup time, after which the device must be 
	// fully back to normal, and capable of receiving and processing USB
	// packets.  In order to do this, the USB module must receive proper
	// clocking (IE: 48MHz clock must be available to SIE for full speed USB
	// operation).
}

/********************************************************************
 * Function:        void USBCB_SOF_Handler(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        The USB host sends out a SOF packet to full-speed
 *                  devices every 1 ms. This interrupt may be useful
 *                  for isochronous pipes. End designers should
 *                  implement callback routine as necessary.
 *
 * Note:            None
 *******************************************************************/
void USBCB_SOF_Handler(void)
{
    // No need to clear UIRbits.SOFIF to 0 here.
    // Callback caller is already doing that.
}

/*******************************************************************
 * Function:        void USBCBErrorHandler(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        The purpose of this callback is mainly for
 *                  debugging during development. Check UEIR to see
 *                  which error causes the interrupt.
 *
 * Note:            None
 *******************************************************************/
void USBCBErrorHandler(void)
{
    // No need to clear UEIR to 0 here.
    // Callback caller is already doing that.

	// Typically, user firmware does not need to do anything special
	// if a USB error occurs.  For example, if the host sends an OUT
	// packet to your device, but the packet gets corrupted (ex:
	// because of a bad connection, or the user unplugs the
	// USB cable during the transmission) this will typically set
	// one or more USB error interrupt flags.  Nothing specific
	// needs to be done however, since the SIE will automatically
	// send a "NAK" packet to the host.  In response to this, the
	// host will normally retry to send the packet again, and no
	// data loss occurs.  The system will typically recover
	// automatically, without the need for application firmware
	// intervention.
	
	// Nevertheless, this callback function is provided, such as
	// for debugging purposes.
}


/*******************************************************************
 * Function:        void USBCBCheckOtherReq(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        When SETUP packets arrive from the host, some
 * 					firmware must process the request and respond
 *					appropriately to fulfill the request.  Some of
 *					the SETUP packets will be for standard
 *					USB "chapter 9" (as in, fulfilling chapter 9 of
 *					the official USB specifications) requests, while
 *					others may be specific to the USB device class
 *					that is being implemented.  For example, a HID
 *					class device needs to be able to respond to
 *					"GET REPORT" type of requests.  This
 *					is not a standard USB chapter 9 request, and 
 *					therefore not handled by usb_device.c.  Instead
 *					this request should be handled by class specific 
 *					firmware, such as that contained in usb_function_hid.c.
 *
 * Note:            None
 *******************************************************************/
void USBCBCheckOtherReq(void)
{
    USBCheckHIDRequest();
}//end


/*******************************************************************
 * Function:        void USBCBStdSetDscHandler(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        The USBCBStdSetDscHandler() callback function is
 *					called when a SETUP, bRequest: SET_DESCRIPTOR request
 *					arrives.  Typically SET_DESCRIPTOR requests are
 *					not used in most applications, and it is
 *					optional to support this type of request.
 *
 * Note:            None
 *******************************************************************/
void USBCBStdSetDscHandler(void)
{
    // Must claim session ownership if supporting this request
}//end


/*******************************************************************
 * Function:        void USBCBInitEP(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This function is called when the device becomes
 *                  initialized, which occurs after the host sends a
 * 					SET_CONFIGURATION (wValue not = 0) request.  This 
 *					callback function should initialize the endpoints 
 *					for the device's usage according to the current 
 *					configuration.
 *
 * Note:            None
 *******************************************************************/
void USBCBInitEP(void)
{
    //enable the HID endpoint
    USBEnableEndpoint(HID_EP,USB_IN_ENABLED|USB_OUT_ENABLED|USB_HANDSHAKE_ENABLED|USB_DISALLOW_SETUP);
    //Re-arm the OUT endpoint for the next packet
    outputHandle = HIDRxPacket(HID_EP, (BYTE*)&receivedDataBuffer, sizeof(receivedDataBuffer));
}

/********************************************************************
 * Function:        void USBCBSendResume(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        The USB specifications allow some types of USB
 * 					peripheral devices to wake up a host PC (such
 *					as if it is in a low power suspend to RAM state).
 *					This can be a very useful feature in some
 *					USB applications, such as an Infrared remote
 *					control	receiver.  If a user presses the "power"
 *					button on a remote control, it is nice that the
 *					IR receiver can detect this signalling, and then
 *					send a USB "command" to the PC to wake up.
 *					
 *					The USBCBSendResume() "callback" function is used
 *					to send this special USB signalling which wakes 
 *					up the PC.  This function may be called by
 *					application firmware to wake up the PC.  This
 *					function will only be able to wake up the host if
 *                  all of the below are true:
 *					
 *					1.  The USB driver used on the host PC supports
 *						the remote wakeup capability.
 *					2.  The USB configuration descriptor indicates
 *						the device is remote wakeup capable in the
 *						bmAttributes field.
 *					3.  The USB host PC is currently sleeping,
 *						and has previously sent your device a SET 
 *						FEATURE setup packet which "armed" the
 *						remote wakeup capability.   
 *
 *                  If the host has not armed the device to perform remote wakeup,
 *                  then this function will return without actually performing a
 *                  remote wakeup sequence.  This is the required behavior, 
 *                  as a USB device that has not been armed to perform remote 
 *                  wakeup must not drive remote wakeup signalling onto the bus;
 *                  doing so will cause USB compliance testing failure.
 *                  
 *					This callback should send a RESUME signal that
 *                  has the period of 1-15ms.
 *
 * Note:            This function does nothing and returns quickly, if the USB
 *                  bus and host are not in a suspended condition, or are 
 *                  otherwise not in a remote wakeup ready state.  Therefore, it
 *                  is safe to optionally call this function regularly, ex: 
 *                  anytime application stimulus occurs, as the function will
 *                  have no effect, until the bus really is in a state ready
 *                  to accept remote wakeup. 
 *
 *                  When this function executes, it may perform clock switching,
 *                  depending upon the application specific code in 
 *                  USBCBWakeFromSuspend().  This is needed, since the USB
 *                  bus will no longer be suspended by the time this function
 *                  returns.  Therefore, the USB module will need to be ready
 *                  to receive traffic from the host.
 *
 *                  The modifiable section in this routine may be changed
 *                  to meet the application needs. Current implementation
 *                  temporary blocks other functions from executing for a
 *                  period of ~3-15 ms depending on the core frequency.
 *
 *                  According to USB 2.0 specification section 7.1.7.7,
 *                  "The remote wakeup device must hold the resume signaling
 *                  for at least 1 ms but for no more than 15 ms."
 *                  The idea here is to use a delay counter loop, using a
 *                  common value that would work over a wide range of core
 *                  frequencies.
 *                  That value selected is 1800. See table below:
 *                  ==========================================================
 *                  Core Freq(MHz)      MIP         RESUME Signal Period (ms)
 *                  ==========================================================
 *                      48              12          1.05
 *                       4              1           12.6
 *                  ==========================================================
 *                  * These timing could be incorrect when using code
 *                    optimization or extended instruction mode,
 *                    or when having other interrupts enabled.
 *                    Make sure to verify using the MPLAB SIM's Stopwatch
 *                    and verify the actual signal on an oscilloscope.
 *******************************************************************/
void USBCBSendResume(void)
{
    static WORD delay_count;
    
    //First verify that the host has armed us to perform remote wakeup.
    //It does this by sending a SET_FEATURE request to enable remote wakeup,
    //usually just before the host goes to standby mode (note: it will only
    //send this SET_FEATURE request if the configuration descriptor declares
    //the device as remote wakeup capable, AND, if the feature is enabled
    //on the host (ex: on Windows based hosts, in the device manager 
    //properties page for the USB device, power management tab, the 
    //"Allow this device to bring the computer out of standby." checkbox 
    //should be checked).
    if(USBGetRemoteWakeupStatus() == TRUE) 
    {
        //Verify that the USB bus is in fact suspended, before we send
        //remote wakeup signalling.
        if(USBIsBusSuspended() == TRUE)
        {
            USBMaskInterrupts();
            
            //Clock switch to settings consistent with normal USB operation.
            USBCBWakeFromSuspend();
            USBSuspendControl = 0; 
            USBBusIsSuspended = FALSE;  //So we don't execute this code again, 
                                        //until a new suspend condition is detected.

            //Section 7.1.7.7 of the USB 2.0 specifications indicates a USB
            //device must continuously see 5ms+ of idle on the bus, before it sends
            //remote wakeup signalling.  One way to be certain that this parameter
            //gets met, is to add a 2ms+ blocking delay here (2ms plus at 
            //least 3ms from bus idle to USBIsBusSuspended() == TRUE, yeilds
            //5ms+ total delay since start of idle).
            delay_count = 3600U;        
            do
            {
                delay_count--;
            }while(delay_count);
            
            //Now drive the resume K-state signalling onto the USB bus.
            USBResumeControl = 1;       // Start RESUME signaling
            delay_count = 1800U;        // Set RESUME line for 1-13 ms
            do
            {
                delay_count--;
            }while(delay_count);
            USBResumeControl = 0;       //Finished driving resume signalling

            USBUnmaskInterrupts();
        }
    }
}


/*******************************************************************
 * Function:        BOOL USER_USB_CALLBACK_EVENT_HANDLER(
 *                        USB_EVENT event, void *pdata, WORD size)
 *
 * PreCondition:    None
 *
 * Input:           USB_EVENT event - the type of event
 *                  void *pdata - pointer to the event data
 *                  WORD size - size of the event data
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This function is called from the USB stack to
 *                  notify a user application that a USB event
 *                  occured.  This callback is in interrupt context
 *                  when the USB_INTERRUPT option is selected.
 *
 * Note:            None
 *******************************************************************/
BOOL USER_USB_CALLBACK_EVENT_HANDLER(USB_EVENT event, void *pdata, WORD size)
{
    switch(event)
    {
        case EVENT_TRANSFER:
            //Add application specific callback task or callback function here if desired.
            break;
        case EVENT_SOF:
            USBCB_SOF_Handler();
            break;
        case EVENT_SUSPEND:
            USBCBSuspend();
            break;
        case EVENT_RESUME:
            USBCBWakeFromSuspend();
            break;
        case EVENT_CONFIGURED: 
            USBCBInitEP();
            break;
        case EVENT_SET_DESCRIPTOR:
            USBCBStdSetDscHandler();
            break;
        case EVENT_EP0_REQUEST:
            USBCBCheckOtherReq();
            break;
        case EVENT_BUS_ERROR:
            USBCBErrorHandler();
            break;
        case EVENT_TRANSFER_TERMINATED:
            //Add application specific callback task or callback function here if desired.
            //The EVENT_TRANSFER_TERMINATED event occurs when the host performs a CLEAR
            //FEATURE (endpoint halt) request on an application endpoint which was 
            //previously armed (UOWN was = 1).  Here would be a good place to:
            //1.  Determine which endpoint the transaction that just got terminated was 
            //      on, by checking the handle value in the *pdata.
            //2.  Re-arm the endpoint if desired (typically would be the case for OUT 
            //      endpoints).
            break;
        default:
            break;
    }      
    return TRUE; 
}

/** EOF main.c *************************************************/
