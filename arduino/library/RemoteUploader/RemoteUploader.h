#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

#include <inttypes.h>
#include <SoftwareSerial.h>
#include <extEEPROM.h>

// The remaining config should be fine for vast majority of cases
// Currently it goes out of memory on atmega328/168 with VERBOSE true. TODO shorten strings so it doesn't go out of memory
// Must also enable a debug option (USBDEBUG or NSSDEBUG) with VERBOSE true. With atmega328/168 you may only use NSSDEBUG as the only serial port is for flashing
#define VERBOSE false
#define DEBUG_BAUD_RATE 19200
#define USBDEBUG false

// only 115200 works with optiboot
#define OPTIBOOT_BAUD_RATE 115200
// how long to wait for a reply from optiboot before timeout (ms)
#define OPTIBOOT_READ_TIMEOUT 1000

// Currently it goes out of memory on atmega328/168 with VERBOSE true. TODO shorten strings so it doesn't go out of memory
// Must also enable a debug option (USBDEBUG or NSSDEBUG) with VERBOSE true. With atmega328/168 you may only use NSSDEBUG as the only serial port is for flashing
#define VERBOSE false
#define DEBUG_BAUD_RATE 19200
// WARNING: never set this to true for a atmega328/168 as it needs Serial(0) for programming. If you do it will certainly fail on flash()
// Only true for Leonardo (defaults to Serial(0) for debug) 
// IMPORTANT: you must have the serial monitor open when usb debug enabled or will fail after a few packets!
#define USBDEBUG false
// UNTESTED!
#define NSSDEBUG false
#define NSSDEBUG_TX 6
#define NSSDEBUG_RX 7

// NOTE: ONLY SET THIS TRUE FOR TROUBLESHOOTING. FLASHING IS NOT POSSIBLE IF SET TO TRUE
#define USE_SERIAL_FOR_DEBUG false

// this can be reduced to the maximum packet size + header bytes
// memory shouldn't be an issue on the programmer since it only should ever run this sketch!

#define BUFFER_SIZE 150
#define READ_BUFFER_SIZE 150

// TODO not implemented yet
#define PROG_PAGE_RETRIES 2
// the address to start writing the hex to the eeprom
#define EEPROM_OFFSET_ADDRESS 16

// ==================================================================END CONFIG ==================================================================

// TODO define negative error codes for sketch only. translate to byte error code and send to XBee

// we don't need magic bytes if we're not proxying but they are using only 5% of packet with nordic and much less with xbee
// any packet that has byte1 and byte 2 that equals these is a programming packet
#define MAGIC_BYTE1 0xef
#define MAGIC_BYTE2 0xac

#define START_PROGRAMMING 0xa0
#define PROGRAM_DATA 0xa1
#define STOP_PROGRAMMING 0xa2

#define CONTROL_PROG_REQUEST 0x10
#define CONTROL_PROG_DATA 0x20
#define CONTROL_FLASH_START 0x40

#define PROG_START_HEADER_SIZE 9
#define PROG_DATA_HEADER_SIZE 6
#define FLASH_START_HEADER_SIZE 6

#define VERSION = 1;

// host reply codes
// every packet must return exactly one reply: OK or ERROR. is anything > OK
// TODO make sure only one reply code is sent!
// timeout should be the only reply that is not sent immediately after rx packet received
#define OK 1
//got prog data but no start. host needs to start over
#define START_OVER 2
#define TIMEOUT 3
#define FLASH_ERROR 0x82
#define EEPROM_ERROR 0x80
#define EEPROM_WRITE_ERROR 0x81
// TODO these should be bit sets on FLASH_ERROR
#define EEPROM_READ_ERROR 0xb1
// serial lines not connected or reset pin not connected
#define NOBOOTLOADER_ERROR 0xc1
#define BOOTLOADER_REPLY_TIMEOUT 0xc2
#define BOOTLOADER_UNEXPECTED_REPLY 0xc3

// STK CONSTANTS
#define STK_OK              0x10
#define STK_INSYNC          0x14  // ' '
#define CRC_EOP             0x20  // 'SPACE'
#define STK_GET_PARAMETER   0x41  // 'A'
#define STK_ENTER_PROGMODE  0x50  // 'P'
#define STK_LEAVE_PROGMODE  0x51  // 'Q'
#define STK_LOAD_ADDRESS    0x55  // 'U'
#define STK_PROG_PAGE       0x64  // 'd'
#define STK_READ_PAGE       0x74  // 't'
#define STK_READ_SIGN       0x75  // 'u'

class RemoteUploader {
public:
	RemoteUploader();
	HardwareSerial* getProgrammerSerial();
	void dumpBuffer(uint8_t arr[], char context[], uint8_t len);	
	int handlePacket(uint8_t packet[]);
	int setup(HardwareSerial* _serial, extEEPROM* _eeprom, uint8_t _resetPin);
	bool inProgrammingMode();
	long getLastPacketMillis();
	void reset();
	// can only use USBDEBUG with Leonardo or other variant that supports multiple serial ports
	#if (USBDEBUG || NSSDEBUG)
	  Stream* getDebugSerial();
	#endif	
	bool isProgrammingPacket(uint8_t packet[], uint8_t packet_length);
	bool isFlashPacket(uint8_t packet[]);	  
private:
	void clearRead();
	int readOptibootReply(uint8_t len, int timeout);
	int sendToOptiboot(uint8_t command, uint8_t *arr, uint8_t len, uint8_t response_length);
	int flashInit();
	int sendPageToOptiboot(uint8_t *addr, uint8_t *buf, uint8_t data_len);
	void bounce();
	int flash(int start_address, int size);
	extEEPROM* eeprom;
	// ** IMPORTANT! **
	// For Leonardo use Serial1 (UART) or it will try to program through usb-serial
	// For atmega328 use Serial
	// For megas other e.g. Serial2 should work -- UNTESTED!
	HardwareSerial* progammerSerial;
	HardwareSerial* debugSerial;

	#if (NSSDEBUG) 
	  SoftwareSerial nssDebug;
	#endif

	uint8_t cmd_buffer[1];
	uint8_t addr[2];

	// TODO revisit these sizes
	uint8_t buffer[BUFFER_SIZE];
	uint8_t readBuffer[READ_BUFFER_SIZE];

	uint8_t resetPin;
	int packetCount;
	int numPackets;
	int programSize;
	bool inProgramming;
	long programmingStartMillis;
	long lastUpdateAtMillis;
	int currentEEPROMAddress;
};
