/*************************************************************************************
 * 
 * Once the debug mask is enabled and debug message is printed, the loop cycle time increases and consequently, increase the delay for TX.
 * So, it's recommended to enable the mask, only necessary
 * 
 *************************************************************************************/

#include "printf_softserial.h"
#include "StreamData.h"
#include "config.h"

#define TX_PERIOD (11)    // 11 ms, smaller number is not working because of the operation time in the loop

/***************************************************
 *
 * Debug MASK Data structure / Macro       
 *
 ***************************************************/
#define NK_DEBUG_SPOT  (1 << 1)
#define NK_DEBUG_INFO  (1 << 2)
#define NK_DEBUG_DEBUG  (1 << 3)
#define NK_DEBUG_DELTA (1 << 4)

//static uint32_t  gunDebugMask = NK_DEBUG_INFO | NK_DEBUG_SPOT | NK_DEBUG_DELTA | NK_DEBUG_DEBUG;
//static uint32_t  gunDebugMask = NK_DEBUG_INFO | NK_DEBUG_DEBUG;
//static uint32_t  gunDebugMask = NK_DEBUG_INFO | NK_DEBUG_DEBUG | NK_DEBUG_SPOT;
static uint32_t  gunDebugMask = 0;

unsigned long last_tx_time = 0;

union TxStreamData stTxStreamData;

#ifdef CONFIG_SYMAX_X5C
unsigned char gucSubProtocol = SUB_PROTOCOL_SYMAX;
#elif defined(CONFIG_V2X2)
unsigned char gucSubProtocol = SUB_PROTOCOL_V2X2;
#else
#error "No Configuration defined !!"
#endif

unsigned char gucSubProtocolType = SUB_PROTOCOL_TYPE;

void Clear_TxStream_Data(unsigned char *pucTxStreamData)
{
    int i;

    for(i = 0; i < MAX_TX_STREAM_LEN; i++) { // 26
        pucTxStreamData[i] = 0x0;    // Clear
    }
}

void Encode_TxStream_Channel_Data(uint8_t *pucTxStreamData, unsigned int *punChannelData) 
{
    uint8_t *p=pucTxStreamData;
    uint8_t st, ch;

    for(st = 0, ch = 0; ch < MAX_TX_CHANNEL; ch++) {
      switch(st % 11) {
        case 0 :  // st=0,11, ch=0,8
          pucTxStreamData[st] = punChannelData[ch] & 0xFF;
          pucTxStreamData[st+1] = (punChannelData[ch] & 0x700) >> 8;
#if 0
          printf("punChannelData[%d] : 0x%04x\n", ch, punChannelData[ch]);
          printf("punChannelData[%d] & 0x700 : 0x%08x\n", ch, punChannelData[ch] & 0x700);
          printf("punChannelData[%d] & 0x700 >> 8 : 0x%08x\n", ch, (punChannelData[ch] & 0x700) >> 8);          
          printf("[%d] = %u\n", st, pucTxStreamData[st]);
          printf("[%d] = %u\n", st+1, pucTxStreamData[st+1]);
#endif          
          st++;
          break;
        case 1 :  // st=1,12, ch=1,9
          pucTxStreamData[st] |= (punChannelData[ch] & 0x1F) << 3;
          pucTxStreamData[st+1] = (punChannelData[ch] & 0x7E0) >> 5;
          st++;
          break;
        case 2 :  // st=2,13, ch=2,10
          pucTxStreamData[st] |= (punChannelData[ch] & 0x03) << 6;
          pucTxStreamData[st+1] = (punChannelData[ch] & 0x3FC) >> 2;
          pucTxStreamData[st+2] = (punChannelData[ch] & 0x400) >> 10;
          st += 2;
          break;
        case 4 :  // st=4,15 ch=3,11
          pucTxStreamData[st] |= (punChannelData[ch] & 0x7F) << 1;
          pucTxStreamData[st+1] = (punChannelData[ch] & 0x780) >> 7;
          st ++;
          break;
        case 5 :  // st=5,16 ch=4,12
          pucTxStreamData[st] |= (punChannelData[ch] & 0x0F) << 4;
          pucTxStreamData[st+1] = (punChannelData[ch] & 0x7F0) >> 4;
          st ++;
          break;
        case 6 :  // st=6,17 ch=5,13
          pucTxStreamData[st] |= (punChannelData[ch] & 0x01) << 7;
          pucTxStreamData[st+1] = (punChannelData[ch] & 0x1FE) >> 1;
          pucTxStreamData[st+2] = (punChannelData[ch] & 0x600) >> 9;          
          st += 2;
          break;
        case 8 :  // st=8,19 ch=6,14
          pucTxStreamData[st] |= (punChannelData[ch] & 0x3F) << 2;
          pucTxStreamData[st+1] = (punChannelData[ch] & 0x7C0) >> 6;
          st ++;
          break;  
        case 9 :  // st=9,20 ch=7,15
          pucTxStreamData[st] |= (punChannelData[ch] & 0x07) << 5;
          pucTxStreamData[st+1] = (punChannelData[ch] & 0x7F8) >> 3;
          st += 2;
          break; 
        default :
          printf("Logic problem !!\n");
          break;
      }
    }
}

unsigned int    gunChannelData[MAX_TX_CHANNEL] = {             // 16
                            0x0400,  0x0400,  0x00CC,  0x0400,
                            0x00CC,  0x0400,  0x00CC,  0x00CC,
                            0x00CC,  0x0400,  0x0400,  0x0400,
                            0x0400,  0x0400,  0x0400,  0x0400,
                            };

int JS_Throttle_Pin = A0;
int JS_RUDDER_Pin = A1;
int JS_ELEVATOR_Pin = A2;
int JS_AILERON_Pin = A3;

void Joystick_Input()
{
  int nThrottleRawData = 0;
  int nThrottleAdjData = 0;

  int nRudderRawData = 0;
  int nRudderAdjData = 0;

  int nElevatorRawData = 0;
  int nElevatorAdjData = 0;

  int nAileronRawData = 0;
  int nAileronAdjData = 0;
  
  nThrottleRawData = analogRead(JS_Throttle_Pin);
  nThrottleAdjData = map(nThrottleRawData, 0, 1023, THROTTLE_MIN, THROTTLE_MAX);  

  nRudderRawData = analogRead(JS_RUDDER_Pin);
  nRudderAdjData = map(nRudderRawData, 0, 1023, RUDDER_MIN, RUDDER_MAX);  

  nElevatorRawData = analogRead(JS_ELEVATOR_Pin);
  nElevatorAdjData = map(nElevatorRawData, 0, 1023, ELEVATOR_MIN, ELEVATOR_MAX);  

  nAileronRawData = analogRead(JS_AILERON_Pin);
  nAileronAdjData = map(nAileronRawData, 0, 1023, AILERON_MIN, AILERON_MAX);  


#ifdef DEBUG_NK
  printf("nThrottleRawData : %d, nThrottleAdjData : %d\n", nThrottleRawData, nThrottleAdjData);
  printf("nRudderRawData   : %d, nRudderAdjData   : %d\n", nRudderRawData, nRudderAdjData);
  printf("nElevatorRawData : %d, nElevatorAdjData : %d\n", nElevatorRawData, nElevatorAdjData);
  printf("nAileronRawData  : %d, nAileronAdjData  : %d\n", nAileronRawData, nAileronAdjData);
#endif


#if defined(CONFIG_SYMAX_X5C)
  gunChannelData[THROTTLE] = THROTTLE_MAX - nThrottleAdjData + THROTTLE_MIN;
  gunChannelData[RUDDER]   = RUDDER_MAX - nRudderAdjData + RUDDER_MIN;
  gunChannelData[ELEVATOR] = nElevatorAdjData;
  gunChannelData[AILERON]  = AILERON_MAX - nAileronAdjData + AILERON_MIN;
#elif defined(CONFIG_V2X2)
  gunChannelData[THROTTLE] = nThrottleAdjData;  /* CH2 */
  gunChannelData[RUDDER]   = nRudderAdjData;   /* CH3 */
  gunChannelData[ELEVATOR] = nElevatorAdjData;  /* CH1 */
  gunChannelData[AILERON]  = nAileronAdjData;  /* CH0 */

#if 0
  gunChannelData[TRIM_YAW]   = 0x40;
  gunChannelData[TRIM_PITCH] = 0x40;
  gunChannelData[TRIM_ROLL]  = 0x40;

  gunChannelData[RX_TX_ADDR1] = 0x00;
  gunChannelData[RX_TX_ADDR2] = 0x00;
  gunChannelData[RX_TX_ADDR3] = 0x00;

  gunChannelData[FLAGS2]  = 0x00;

  gunChannelData[CHANNEL_11]  = 0x00;
  gunChannelData[CHANNEL_12]  = 0x00;
  gunChannelData[CHANNEL_13]  = 0x00;

  gunChannelData[FLAGS]  = 0x00;
#endif  
#endif
}
                            
void Fill_TxStream_Buf(union TxStreamData *stTmpTSData)
{

    uint8_t ucChannelBuffer[MAX_TX_CHANNEL_BUF_LEN];    // 22
    uint8_t ucRxNum = 0;  /* 0..15 */
    unsigned long ulTmpTime;

#ifdef DEBUG_NK
    Print_TxStream_Channel_Data(unChannelData);
#endif

    stTmpTSData->stTS.ucHead = 0x55;    //
    stTmpTSData->stTS.ucSubProtocol = (AUTO_BIND_OFF << 6) | (RANGE_CHECK_NO << 5) | ((gucSubProtocol & SUB_PROTOCOL_MASK) << 0);
    if( (ulTmpTime = millis()) > 2000 && ulTmpTime < 3000 ) {
      stTmpTSData->stTS.ucSubProtocol |= (BIND_ON << 7);   //
    } else {
      stTmpTSData->stTS.ucSubProtocol |= (BIND_OFF << 7);   //
    }
    
    stTmpTSData->stTS.ucType = (POWER_HIGH << 7) | ((gucSubProtocolType & SUB_PROTOCOL_TYPE_MASK) << 4) | ((ucRxNum & RX_NUM_MASK) << 0);
    stTmpTSData->stTS.ucOptionProtocol = 0; // -128..127

    Joystick_Input();

    Encode_TxStream_Channel_Data(stTmpTSData->stTS.ucChan, gunChannelData);   // dest, src
}

void Print_TxStream_Channel_Data(unsigned int *punChannelData) 
{
    int i;

    if( gunDebugMask & NK_DEBUG_INFO ) {
      printf("%5lu) ## NK [%s:%d] TX Channel Data : ", millis(), __func__, __LINE__);
      for( i = 0; i < MAX_TX_CHANNEL; i++) {
          printf("0x%04x ", punChannelData[i] & 0xFFFF);
      }   
      printf("\n");
    }
}

void Print_TxStream_Data(union TxStreamData *pstTxStreamData) 
{
    int i, j;

    if( gunDebugMask & NK_DEBUG_INFO ) {
      printf("%5lu) ## NK [%s:%d] TxStream Data : ", millis(), __func__, __LINE__);
      for( i = 0; i < MAX_TX_STREAM_LEN; i++) {
        printf("0x%02x ", pstTxStreamData->ucByte[i]);
      }   
      printf("\n");
    }      

#if 0
    for( i = 0; i < MAX_TX_CHANNEL_BUF_LEN; i++) {

        for( j = 0; j < (sizeof(uint8_t) * 8); j++) {

            if( (unsigned char)pucTxStreamData[i] & (1 << j) ) {
                printf("1");
            } else {
                printf("0");
            }
        }
        printf("  ");
    }
    printf("\n");
#endif    
}

int i;

#include <SoftwareSerial.h>

const byte rxPin = 2;
const byte txPin = 3;

// set up a new serial object
SoftwareSerial SerialS(rxPin, txPin);

void setup() {

  pinMode(JS_Throttle_Pin, INPUT);
  pinMode(JS_RUDDER_Pin, INPUT);
  pinMode(JS_ELEVATOR_Pin, INPUT);
  pinMode(JS_AILERON_Pin, INPUT);

  pinMode(rxPin, INPUT);
  pinMode(txPin, OUTPUT);  
  
  // put your setup code here, to run once:
  Serial.begin(100000, SERIAL_8E2);

  SerialS.begin(115200); // NK - if the soft serial baud rate is too high, TX causes thee "Bad frame RX" error.
  printf_begin();

#if 1
  printf("sizeof(unsigned char)         : %d\n", sizeof(char));
  printf("sizeof(uint8_t)               : %d\n", sizeof(uint8_t));
  printf("sizeof(unsigned char *)       : %d\n", sizeof(unsigned char *));

  printf("sizeof(unsigned short)        : %d\n", sizeof(unsigned short));
  printf("sizeof(unsigned short *)      : %d\n", sizeof(unsigned short *));

  printf("sizeof(unsigned int)          : %d\n", sizeof(unsigned int));
  printf("sizeof(int16_t)               : %d\n", sizeof(int16_t));
  printf("sizeof(unsigned int *)        : %d\n", sizeof(unsigned int *));

  printf("sizeof(long int)              : %d\n", sizeof(long int));
  printf("sizeof(int32_t)               : %d\n", sizeof(int32_t));
  printf("sizeof(long int *)            : %d\n", sizeof(long int *));

  printf("sizeof(unsigned long)         : %d\n", sizeof(unsigned long));
  printf("sizeof(unsigned long *)       : %d\n", sizeof(unsigned long *));

  printf("sizeof(unsigned long long )   : %d\n", sizeof(unsigned long long));
  printf("sizeof(unsigned long long *)  : %d\n", sizeof(unsigned long long *));

  printf("sizeof(union TxStreamData)    : %d\n", sizeof(union TxStreamData));
#endif  
}

void loop() {
//  printf("%5lu) ## NK [%s:%d] CK - 1 \n", millis(), __func__, __LINE__);
  if( millis() - last_tx_time >= TX_PERIOD ) {
    last_tx_time = millis();    
#ifdef DEBUG_NK
    if( gunDebugMask & NK_DEBUG_INFO ) {
      printf("Send TX Stream data !! \n");
    }
#endif
    Clear_TxStream_Data(stTxStreamData.ucByte);
    Fill_TxStream_Buf(&stTxStreamData);
#ifdef DEBUG_NK
    Print_TxStream_Data(&stTxStreamData);
#endif

    for( i = 0; i < MAX_TX_STREAM_LEN; i++) {
      Serial.write(stTxStreamData.ucByte[i]);
    }
  }
}
