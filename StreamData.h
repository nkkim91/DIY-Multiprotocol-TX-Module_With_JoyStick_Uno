#include "config.h"

#define MAX_TX_STREAM_LEN			26
#define MAX_TX_CHANNEL_BUF_LEN		22
#define MAX_TX_CHANNEL				16

#define DEFAULT_XMIT_DELAY	(4000)

#define AILERON_MIN		(0x0CC)
#define AILERON_MID		(0x400)
#define AILERON_MAX		(0x733)

#define ELEVATOR_MIN	(0x0CC)
#define ELEVATOR_MID	(0x400)
#define ELEVATOR_MAX	(0x733)

#define THROTTLE_MIN	(0x0CC) /* 204 */
#define THROTTLE_MID	(0x400)
#define THROTTLE_MAX	(0x733) /* 1843 */

#define RUDDER_MIN		(0x0CC)
#define RUDDER_MID		(0x400)
#define RUDDER_MAX		(0x733)

#define JOYSTICK_AILERON_MIN	(-32767)
#define JOYSTICK_AILERON_MID	(0)
#define JOYSTICK_AILERON_MAX	(32767)

#define JOYSTICK_ELEVATOR_MIN	(32767)
#define JOYSTICK_ELEVATOR_MID	(0)
#define JOYSTICK_ELEVATOR_MAX	(-32767)

#define JOYSTICK_THROTTLE_MIN	(32767)
#define JOYSTICK_THROTTLE_MID	(0)
#define JOYSTICK_THROTTLE_MAX	(-32767)

#define JOYSTICK_RUDDER_MIN		(-32767)
#define JOYSTICK_RUDDER_MID		(0)
#define JOYSTICK_RUDDER_MAX		(32767)


#define JOYSTICK_INPUT_MAX	(32767)
#define CHANNEL_MINUS125P_VALUE	(0)
#define CHANNEL_MINUS100P_VALUE	(204)
#define CHANNEL_PLUSM100P_VALUE	(1843)
#define CHANNEL_PLUSM125P_VALUE	(2047)
#define CHANNEL_MAX_VALUE	(CHANNEL_PLUSM100P_VALUE - CHANNEL_MINUS100P_VALUE + 1)	// 1640

#define BIND_TIME_OUT	(2)	// 3 Sec

//#define DEBUG_NK

#ifdef ARDUINO
typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef signed int int16_t;
typedef unsigned int uint16_t;
typedef long int int32_t;
typedef long unsigned int uint32_t;
#else /* X86 */
typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef signed short int16_t;
typedef unsigned short uint16_t;
typedef signed int int32_t;
typedef unsigned int uint32_t;
#endif

#if defined(CONFIG_SYMAX_X5C)
enum eChanTxProtocol {
	AILERON	= 0,
	ELEVATOR = 1,
	THROTTLE = 2,
	RUDDER = 3,
	FLIP = 4,
	PICTURE = 6,
	VIDEO = 7,
	HEADLESS = 8,
};
#elif defined(CONFIG_V2X2)
enum eChanTxProtocol {
  AILERON = 0,
  ELEVATOR = 1,
  THROTTLE = 2,
  RUDDER = 3,
  TRIM_YAW = 4,
  TRIM_PITCH = 5,
  TRIM_ROLL = 6,
  RX_TX_ADDR1 = 7,
  RX_TX_ADDR2 = 8,
  RX_TX_ADDR3 = 9,
  FLAGS2 = 10,
  CHANNEL_11 = 11,
  CHANNEL_12 = 12,
  CHANNEL_13 = 13,
  FLAGS = 14,
};
#else
#error "No configuration defined !!"
#endif

enum eState {
	BIND_YET = 0,
	BIND_DONE = 1,
	BIND_UNKNOWN = 2,
};


/* Bit 0..4 */
enum eSubProtocol {
	SUB_PROTOCOL_V2X2 = 5,
	SUB_PROTOCOL_MIN = SUB_PROTOCOL_V2X2,
	SUB_PROTOCOL_SYMAX = 10,
	SUB_PROTOCOL_MAX = SUB_PROTOCOL_SYMAX,
};
#define SUB_PROTOCOL_MASK (0x1F)

/* Bit 5 */
enum eRangCheck {
  RANGE_CHECK_NO = 0,
  RANGE_CHECK_YES = 1,
};

/* Bit 6 */
enum eAutoBind {
  AUTO_BIND_OFF = 0,
  AUTO_BIND_ON = 1,
};

/* Bit 7 */
enum eBind {
  BIND_OFF = 0,
  BIND_ON = 1,
};

#define RX_NUM_MASK (0x0F)

/* Bit 4..6 */
enum eSubProtocolType {
	SUB_PROTOCOL_TYPE_SYMAX = 0,
	SUB_PROTOCOL_TYPE_SYMAX5C = 1,
	SUB_PROTOCOL_TYPE_V2X2 = 0,
	SUB_PROTOCOL_TYPE_JXD506 = 1,
};
#define SUB_PROTOCOL_TYPE_MASK (0x07)

/* Bit 7 */
enum ePower {
  POWER_HIGH = 0,
  POWER_LOW = 1,
};



enum InputSourceType {
	IST_REG_FILE = 0,
	IST_CHAR_DEV = 1,
};


struct TxStream {
	uint8_t ucHead;
	uint8_t ucSubProtocol;
	uint8_t	ucType;
	uint8_t	ucOptionProtocol;
	uint8_t ucChan[MAX_TX_CHANNEL_BUF_LEN];	// 22(x unsigned char) => 16 (x unsigned short)
};

#define TX_STREAM_SIZE	sizeof(struct TxStream)

union TxStreamData {
	struct TxStream stTS;
	uint8_t ucByte[TX_STREAM_SIZE];
};

#define TS_HEAD			stTS.ucHead
#define TS_SPROTOCOL	stTS.ucSubProtocol
#define TS_TYPE			stTS.ucType
#define TS_OPROTOCOL 	stTS.ucOptionProtocol
#define TS_CH(x)		stTS.ucChan[x]

