// DIY AnalogPad - PSpad to MD converter
// by takuya matsubara
// for Arduino pro/pro mini(ATmega328 , 5V , 16MHz)

// XE-1AP MegaDrive mode only!

// +-------------------+  Megadrive connector
// |(5) (4) (3) (2) (1)|
//  |                 |
//   |(9) (8) (7) (6)|
//   +---------------+

// (1)DATA0:port D2 
// (2)DATA1:port D3
// (3)DATA2:port D4
// (4)DATA3:port D5
// (5)VCC+5V
// (6)LH   :port D6
// (7)REQ  :port B0
// (8)GND  
// (9)ACK  :port D7


// REQ
// ---+   +--------------------------------------------------------------------
//    |   |  
//    +---+

// DATA   +0    +1    +2    +3    +4    +5    +6    +7    +8    +9    +10
// D3-D0   
// -----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+----
//      |E*FG |ABCD |ch1H |ch0H |     |ch2H |ch1L |ch0L |     |ch2L | ?   |   
//      +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
//      <16us><34us>
//      <  50usec  >

// ACK
// -----+  +--+  +--+  +--+  +--+  +--+  +--+  +--+  +--+  +--+  +--+  +-------  
//      |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |       
//      +--+  +--+  +--+  +--+  +--+  +--+  +--+  +--+  +--+  +--+  +--+       

// LH
// ---+       +-----+     +-----+     +-----+     +-----+     +-----+     +----
//    |       |     |     |     |     |     |     |     |     |     |     |    
//    +-------+     +-----+     +-----+     +-----+     +-----+     +-----+    

// stick/throttle
#define REVERSE_CH1 0 //  ch.1 stick X( 0=normal / 1=reverse )
#define REVERSE_CH0 0 //  ch.0 stick Y( 0=normal / 1=reverse ) 
#define REVERSE_CH2 0 //  ch.2 throttle( 0=normal / 1=reverse )

#define MD_PORT PORTD     // data/LH/ACK用
#define MD_DDR DDRD       // data/LH/ACK用 向き
#define MD_DAT_SHIFT 2    // dataビットシフト数
#define MD_DAT_MASK 0x0f  // 4bit通信用マスク
#define MD_BITLH (1<<6)   // LH用マスク
#define MD_BITACK (1<<7)  // ACK用マスク

#define REQ_PORT PORTB  // REQ用
#define REQ_PIN PINB    // REQ用 入力
#define REQ_DDR DDRB    // REQ用 向き
#define REQ_BIT (1<<0)  // REQ用 マスク

// Play Station Gamepad
#define PAD_PORT  PORTB
#define PAD_DDR   DDRB
#define PAD_PIN   PINB

#define PAD_DATBIT  (1<<1)
#define PAD_CMDBIT  (1<<2)
#define PAD_SELBIT  (1<<3)
#define PAD_CLKBIT  (1<<4)

#define PAD_DISABLE PAD_PORT|=PAD_SELBIT
#define PAD_ENABLE  PAD_PORT&=~PAD_SELBIT
#define PAD_CLK_H PAD_PORT|=PAD_CLKBIT
#define PAD_CLK_L PAD_PORT&=~PAD_CLKBIT
#define PAD_CMD_H PAD_PORT|=PAD_CMDBIT
#define PAD_CMD_L PAD_PORT&=~PAD_CMDBIT

#define CPUHZ 16000000
#define TIMER_1USEC (unsigned int)(CPUHZ/1000000)
#define TIMER_2USEC (unsigned int)(CPUHZ/500000)
#define TIMER_4USEC (unsigned int)(CPUHZ/250000)
#define TIMER_12USEC (unsigned int)(CPUHZ/83333)
#define TIMER_22USEC (unsigned int)(CPUHZ/45454)
#define TIMER_50USEC (unsigned int)(CPUHZ/20000)

const unsigned char configmode_enter[]={0x01,0x43,0x00,0x01,0x00,0x00,0x00,0x00,0x00};
const unsigned char configmode_exit[] ={0x01,0x43,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
const unsigned char get_querymodel[]  ={0x01,0x45,0x00,0x5a,0x5a,0x5a,0x5a,0x5a,0x5a};
const unsigned char set_analogmode[]  ={0x01,0x44,0x00,0x01,0x03,0x00,0x00,0x00,0x00};
const unsigned char get_paddata[]     ={0x01,0x42,0x00,0x00,0x00,0x00,0x00,0x00,0x00}; 

unsigned char padinput[30]; // PAD data

//----- wait micro second 
void timer_uswait(unsigned int limitcnt){
  TCNT1 = 0;                    
  while(TCNT1 < limitcnt){      
  }                             
}

//------ PS pad init
void pad_init(void){
  PAD_DDR |= (PAD_CLKBIT + PAD_SELBIT + PAD_CMDBIT);  // output
  PAD_PORT |= (PAD_CLKBIT + PAD_SELBIT + PAD_CMDBIT);
}

//------ send/recv 1Byte
unsigned char pad_send1byte(unsigned char sendbyte){
  unsigned char recvbyte,bitmask;

  recvbyte = 0;
  PAD_CLK_H;
  PAD_CMD_H;
  timer_uswait(TIMER_4USEC);

  bitmask = 0x01;
  while(bitmask){
    if(sendbyte & bitmask){
      PAD_CMD_H;
    }else{
      PAD_CMD_L;
    }
    PAD_CLK_L;
    timer_uswait(TIMER_2USEC);

    if(PAD_PIN & PAD_DATBIT) recvbyte |= bitmask;
    PAD_CLK_H;
    bitmask <<= 1;
  }
  PAD_CMD_H;
  return recvbyte;
}

//------ send/recv command
void pad_sendcommand(unsigned char *cmd,char sendcnt){
  unsigned char i,j;

  PAD_ENABLE;
  timer_uswait(TIMER_4USEC);
  for(i=0; i < sendcnt; i++){
    timer_uswait(TIMER_4USEC);
    padinput[i] = pad_send1byte(*(cmd+i));
  }
  PAD_DISABLE;
}

//------ change analog mode
void pad_analogmode(void){
  timer_uswait(TIMER_4USEC);
  pad_sendcommand(configmode_enter,sizeof(configmode_enter)); // config mode
  timer_uswait(TIMER_4USEC);
  pad_sendcommand(configmode_enter,sizeof(configmode_enter)); // config mode
  timer_uswait(TIMER_4USEC);

  //query
  pad_sendcommand(get_querymodel,sizeof(get_querymodel));
  timer_uswait(TIMER_4USEC);
  pad_sendcommand(get_querymodel,sizeof(get_querymodel));
  timer_uswait(TIMER_4USEC);
  
  //analog mode
  pad_sendcommand(set_analogmode,sizeof(set_analogmode));
  timer_uswait(TIMER_4USEC);
  pad_sendcommand(set_analogmode,sizeof(set_analogmode));
  timer_uswait(TIMER_4USEC);

  //exit
  pad_sendcommand(configmode_exit,sizeof(configmode_exit));
  timer_uswait(TIMER_4USEC);
  pad_sendcommand(configmode_exit,sizeof(configmode_exit));
}

//------ read PS pad
void pad_read(void){
  pad_sendcommand(get_paddata,sizeof(get_paddata)); 
  if(padinput[2] == 0x5a){
    if(padinput[1]==0x73){
      return; //正常
    }
    //analog modeではない場合、
    pad_analogmode(); // change analog mode
  }else{
    // fatal error？
  }
  //dummy data
  padinput[3] = 0xff;
  padinput[4] = 0xff;
  padinput[5] = 0x80;
  padinput[6] = 0x80;
  padinput[7] = 0x80;
  padinput[8] = 0x80;
}

//------
void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  
  REQ_DDR &= ~REQ_BIT;  // input
  REQ_PORT |= REQ_BIT;  // pullup

  MD_DDR |= (MD_DAT_MASK << MD_DAT_SHIFT) + (MD_BITLH + MD_BITACK);  // output
  MD_PORT |= (MD_DAT_MASK << MD_DAT_SHIFT) + (MD_BITLH + MD_BITACK);  // high

  TCCR1A = 0;
  TCCR1B = 1;
// 0 No clock source (Timer/Counter stopped).
// 1 clk /1 (No prescaling)
// 2 clk /8 (From prescaler)
// 3 clk /64 (From prescaler)
// 4 clk /256 (From prescaler)
// 5 clk /1024 (From prescaler)

  pad_init();
}
//------ debug
void padtest(void) {
  unsigned char i;

  while(1){
    pad_read();
    for(i=0;i<9;i++){
        Serial.print(padinput[i],HEX);
        Serial.write(' ');
        delay(1);
    }
    Serial.write('\n');
    delay(100);
  }
}

//------ debug
void reqtest(void)
{
  char count;

  count=0;
  while(1){
    while(1){
      if((REQ_PIN & REQ_BIT)==0)break;   // REQ == L
    }
    while(1){
      if((REQ_PIN & REQ_BIT)!=0)break;   // REQ == H
    }
    count++;
    if(count >= 60){
      count = 0;
      Serial.write('.');            // debug
    }
  }
}

//----- debug
void timertest(){
  long i;
  while(1){
    for(i=0;i<1000000;i++){
      TCNT1 = 0;                        // wait
      while(TCNT1 <= TIMER_1USEC){      // wait
      }                                 // wait
    }
    Serial.write('.');            // debug
    Serial.write('\n');           // debug
  }
}

//------
void loop() {
  unsigned char sendbuf[11];
  char datanum;
  unsigned char ch0,ch1,ch2;
  unsigned char temp;

//  padtest();    // debug
//  timertest();  // debug
//  reqtest();    // debug

  while(1){
    pad_read();
//               b7    b6    b5    b4    b3    b2    b1    b0  
// padinput[3]:LEFT / DOWN/RIGHT/UP   /START/   L3/   R3/SELECT
// padinput[4]:KAKU4/ BATU/ MARU/KAKU3/   R1/   L1/   R2/    L2
#define PSPAD_MARU    (padinput[3]&(1<<5))
#define PSPAD_BATU    (padinput[3]&(1<<6))
#define PSPAD_START   (padinput[3]&(1<<3))
#define PSPAD_SELECT  (padinput[3]&(1<<0))
#define PSPAD_R1      (padinput[4]&(1<<3))
#define PSPAD_L1      (padinput[4]&(1<<2))
#define PSPAD_R2      (padinput[4]&(1<<1))
#define PSPAD_L2      (padinput[4]&(1<<0))
#define PSPAD_RX   padinput[5]
#define PSPAD_RY   padinput[6]
#define PSPAD_LX   padinput[7]
#define PSPAD_LY   padinput[8]

    temp = 0x0f;
//    if(PSPAD_MARU == 0) temp &= ~(1<<3);  // E1 button
//    if(PSPAD_BATU == 0) temp &= ~(1<<2);  // E2 button
    if(PSPAD_START  == 0) temp &= ~(1<<1);  // F(START) button
    if(PSPAD_SELECT == 0) temp &= ~(1<<0);  // G(SELECT) button
    sendbuf[0] = temp;

    temp = 0x0f;
    if(PSPAD_R1 ==0) temp &= ~(1<<3);  // A button
    if(PSPAD_R2 ==0) temp &= ~(1<<2);  // B button
    if(PSPAD_L1 ==0) temp &= ~(1<<1);  // C button
    if(PSPAD_L2 ==0) temp &= ~(1<<0);  // D button
    sendbuf[1] = temp;

#if REVERSE_CH1
    ch1 = 255-PSPAD_LX;     //reverse
#else
    ch1 = PSPAD_LX;  //normal
#endif

#if REVERSE_CH0
    ch0 = 255-PSPAD_LY;     //reverse
#else
    ch0 = PSPAD_LY;  //normal
#endif
   
#if REVERSE_CH2
    ch2 = 255-PSPAD_RY;     //reverse
#else
    ch2 = PSPAD_RY;  //normal
#endif

    sendbuf[2] = ch1 >> 4;  // CH1 H
    sendbuf[3] = ch0 >> 4;  // CH0 H
    sendbuf[4] = 0;
    sendbuf[5] = ch2 >> 4;  // CH2 H
    sendbuf[6] = ch1 & 0x0f;  // CH1 L
    sendbuf[7] = ch0 & 0x0f;  // CH0 L
    sendbuf[8] = 0;
    sendbuf[9] = ch2 & 0x0f;  // CH2 L
    sendbuf[10] = 0xf;  //未調査

    while(1){
      if((REQ_PIN & REQ_BIT)!=0)break;   // REQ == H
    }
    while(1){
      if((REQ_PIN & REQ_BIT)==0)break;   // REQ == L
    }

    timer_uswait(TIMER_4USEC); //適切なウエイト時間が不明

    for(datanum=0 ;datanum<11; datanum++){
      if((datanum & 1)==0){
        MD_PORT &= ~MD_BITLH;   // LH = L
      }else{
        MD_PORT |= MD_BITLH;    // LH = H
      }

      temp = MD_PORT;
      temp &= ~(MD_DAT_MASK << MD_DAT_SHIFT);
      temp |= sendbuf[datanum] << MD_DAT_SHIFT;   // D0,D1,D2,D3
      MD_PORT = temp;

      MD_PORT &=  ~MD_BITACK; // ACK = L
      timer_uswait(TIMER_12USEC);
      MD_PORT |=  MD_BITACK;  // ACK = H

      // ループ回数の奇数／偶数で周期が違う
      if((datanum & 1)==0){
        timer_uswait(TIMER_4USEC);
      }else{
        timer_uswait(TIMER_22USEC);
      }
    }
    MD_PORT |= (MD_BITLH + (MD_DAT_MASK << MD_DAT_SHIFT));    // LH = H , D3-D0=H
  }
}
