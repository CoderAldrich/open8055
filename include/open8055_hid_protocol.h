#ifndef OPEN8055_HID_PROTOCOL_H
#define OPEN8055_HID_PROTOCOL_H


// We need a couple declarations from stdint.h, but
// C18 doesn't have that header.

#ifndef _STDINT_H

#ifndef _INT8_T_DECLARED
typedef char int8_t;
#define _INT8_T_DECLARED
#endif

#ifndef _UINT8_T_DECLARED
typedef unsigned char uint8_t;
#define _UINT8_T_DECLARED
#endif

#ifndef _INT16_T_DECLARED
typedef short int16_t;
#define _INT16_T_DECLARED
#endif

#ifndef _UINT16_T_DECLARED
typedef unsigned short uint16_t;
#define _UINT16_T_DECLARED
#endif

#endif /* _STDINT_H */


#define OPEN8055_HID_MESSAGE_SIZE	32

#define OPEN8055_HID_MESSAGE_OUTPUT	0x01	// Setting output values
#define OPEN8055_HID_MESSAGE_GETINPUT	0x02	// Request a forced input report
#define OPEN8055_HID_MESSAGE_SETCONFIG1	0x03	// Change configuration
#define OPEN8055_HID_MESSAGE_GETCONFIG	0x04	// Request current config
#define OPEN8055_HID_MESSAGE_SAVECONFIG	0x05	// Save current config to EEPROM
#define OPEN8055_HID_MESSAGE_SAVEALL	0x06	// Save config and values to EEPROM

#define OPEN8055_HID_MESSAGE_RESET	0x7F	// Restart PIC

#define OPEN8055_HID_MESSAGE_INPUT	0x81	// Report current input values


#define OPEN8055_MODE_ADC		10	// A1,A2 - port is in ADC mode
#define OPEN8055_MODE_INPUT		20	// I1..I5 - port is digital input
#define OPEN8055_MODE_FREQUENCY		21	// I1..I5 - port is a frequency counter
#define OPEN8055_MODE_EUSART		22	// I4&I5 - ports used as EUSART
#define OPEN8055_MODE_OUTPUT		30	// O1..O8 - port is digital output
#define OPEN8055_MODE_SERVO		31	// O1..O8 - port is in servo mode
#define OPEN8055_MODE_ISERVO		32	// O1..O8 - port is in inverted servo mode
#define OPEN8055_MODE_I2C		33	// O1&O2 - ports used as I2C bus.
#define OPEN8055_MODE_PWM		40	// PWM1,PWM2 - port used as PWM output


typedef union {
	uint8_t				raw[OPEN8055_HID_MESSAGE_SIZE];
	uint8_t				msgType;
	
	struct {
		uint8_t			_msgType_input;
				
		uint8_t			inputBits;
		uint16_t		inputCounter[5];
		uint16_t		inputAdcValue[2];
	};
	
	struct {
		uint8_t			_msgType_output;
		
		uint8_t			outputBits;
		uint16_t		outputValue[8];
		uint16_t		outputPwmValue[2];
		uint8_t			resetCounter;
	};	

	struct {
		uint8_t			_msgType_config1;

		uint8_t			modeADC[2];
		uint8_t			modeInput[5];
		uint8_t			modeOutput[8];
		uint8_t			modePWM[2];

		uint16_t		debounceValue[5];
		uint8_t			cardAddress;
	};
	
} Open8055_hidMessage_t;	


#endif // OPEN8055_HID_PROTOCOL_H
