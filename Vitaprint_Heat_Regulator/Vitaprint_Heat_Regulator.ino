//ENCODER setup
#include <ClickEncoder.h>
#include <TimerOne.h>

ClickEncoder *encoder;
int16_t last, value;

void timerIsr() {
encoder->service();
}
bool STEPPLUS, STEPMINUS;
double encsum = 0, encprev = 1;

// LCD SETUP
#include "U8glib.h"

U8GLIB_ST7920_128X64_1X u8g(23, 17, 16); // SPI Com: SCK = en = 23, MOSI = rw = 17, CS = di = 16

#define KEY_NONE 0
#define KEY_PREV 1
#define KEY_NEXT 2
#define KEY_SELECT 3
#define KEY_BACK 4



// Button state variables
uint8_t uiKeyPrev = 7;
uint8_t uiKeyNext = 3;
uint8_t uiKeySelect = 2;
uint8_t uiKeyBack = 8;

uint8_t uiKeyCodeFirst = KEY_NONE;
uint8_t uiKeyCode = KEY_NONE;

//TEMPSETUP
long AIN0 = 0;
long AIN1 = 0;
long AIN2 = 0;
long A0INV = 0;
long A1INV = 0;
long A2INV = 0;


double TH1 = 20;
double TH2 = 20;
double TB = 20;
double CH1 = 0;
double CH2 = 0;
double CB = 0;

// Heaters
double DH1 = 8;
double DH2 = 9;
double DB = 10;

//timers
unsigned long Current_time = 0;
unsigned long Prev_time = 0;

//State variables
bool ST = LOW;

//PID variables
#include <PID_v1.h>
// HEAD 1
double H1p=2, H1i=5, H1d=1;
PID H1PID(&CH1, &DH1, &TH1, H1p, H1i, H1d, DIRECT);

// HEAD 2
double H2p=2, H2i=5, H2d=1;
PID H2PID(&CH2, &DH2, &TH2, H2p, H2i, H2d, DIRECT);

//  BED
double Bp=2, Bi=5, Bd=1;
PID BPID(&CB, &DB, &TB, Bp, Bi, Bd, DIRECT);


void uiStep(void) {
  
    value += encoder->getValue();   //reading encoder
  if (value != last) {              // count till 4 for enc direction
    if(value - last > 0){
     encsum ++;
    }
    else if(value - last < 0){
      encsum --;
    }
    last = value;
  }
  if (encsum - encprev >= 4){
    STEPPLUS = HIGH;

    encprev = encsum;
  }
   if (encsum - encprev <= -4){
    STEPMINUS = HIGH;
    encprev = encsum;
  }

  if ( STEPMINUS == HIGH ){
    uiKeyCodeFirst = KEY_PREV;
    STEPMINUS = LOW;
  }
  else if ( STEPPLUS == HIGH ){
    uiKeyCodeFirst = KEY_NEXT;
    STEPPLUS = LOW;
  }
  ClickEncoder::Button b = encoder->getButton();
  if (b != ClickEncoder::Open){
    switch (b) {
      
      case ClickEncoder::Clicked:
      uiKeyCodeFirst = KEY_SELECT;
      break;
      case ClickEncoder::DoubleClicked:
      uiKeyCodeFirst = KEY_BACK;
      break;
      default:
      break;
    }
  }
    uiKeyCode = uiKeyCodeFirst;
    uiKeyCodeFirst = KEY_NONE;
}


#define MENU_ITEMS 3
const char *menu_strings[MENU_ITEMS] = { "HEAD 1", "HEAD 2", "BED"};
uint8_t menu_current = 0;
uint8_t menu_redraw_required = 0;
uint8_t last_key_code = KEY_NONE;


void drawMenu(void) {
  uint8_t i, h;
  u8g_uint_t w, d;

  u8g.setFont(u8g_font_6x13);
  u8g.setFontRefHeightText();
  u8g.setFontPosTop();
  
  h = u8g.getFontAscent()-u8g.getFontDescent();
  w = u8g.getWidth();
  for( i = 0; i < MENU_ITEMS; i++ ) {
    d = (w-u8g.getStrWidth(menu_strings[i]))/2;
    u8g.setDefaultForegroundColor();
    if ( i == menu_current ) {
      u8g.drawBox(0, i*h+1, w, h);
      u8g.setDefaultBackgroundColor();
    }
    u8g.drawStr(d, i*h, menu_strings[i]);
      u8g.setDefaultForegroundColor();
      u8g.drawFrame(20+i*30,34,28,15);
      if(ST == HIGH && i == menu_current){
        u8g.drawBox(20+i*30, 48, 28, 15);
      }else {
        u8g.drawFrame(20+i*30,48,28,15);
    }
  }
  u8g.setDefaultForegroundColor();
  u8g.drawStr(0,36,"CT");
  u8g.drawStr(0,50,"TT");
//  u8g.setPrintPos(110, 36); 
//  u8g.print(menu_current);
  u8g.setPrintPos(23, 36); 
  u8g.print(CH1,1);
  if(ST == HIGH && menu_current == 0){
   u8g.setDefaultBackgroundColor();
  }
  u8g.setPrintPos(23, 50); 
  u8g.print(TH1,1);
  u8g.setDefaultForegroundColor();
  u8g.setPrintPos(53, 36); 
  u8g.print(CH2,1); 
  if(ST == HIGH && menu_current == 1){
   u8g.setDefaultBackgroundColor();
  }
  u8g.setPrintPos(53, 50); 
  u8g.print(TH2,1);
  u8g.setDefaultForegroundColor(); 
  u8g.setPrintPos(83, 36); 
  u8g.print(CB,1);
  if(ST == HIGH && menu_current == 2){
   u8g.setDefaultBackgroundColor();
  }
  u8g.setPrintPos(83,50); 
  u8g.print(TB,1);
  u8g.setDefaultForegroundColor();
  
}

void updateMenu(void) {
  
  last_key_code = uiKeyCode;
  
  switch ( uiKeyCode ) {
    case KEY_NEXT:
      menu_current++;
      if ( menu_current >= MENU_ITEMS )
        menu_current = 0;
      menu_redraw_required = 1;
      break;
    case KEY_PREV:
      if ( menu_current == 0 )
        menu_current = MENU_ITEMS;
      menu_current--;
      menu_redraw_required = 1;
      break;
   
  }
  uiKeyCode = KEY_NONE;
}


void setup() {
  // rotate screen, if required
  // u8g.setRot180();
  Serial.begin(9600);
  encoder = new ClickEncoder(33, 31, 35);
  Timer1.initialize(1000);
  Timer1.attachInterrupt(timerIsr); 
  last = -1;
                                 // setup key detection and debounce algorithm
  menu_redraw_required = 1;     // force initial redraw

  // pid value initialize
  AIN0 = analogRead(A13);
  AIN1 = analogRead(A14);
  AIN2 = analogRead(A15);

  H1PID.SetMode(AUTOMATIC);
  H2PID.SetMode(AUTOMATIC);
  BPID.SetMode(AUTOMATIC);

}

void loop() {  
  
  
  A0INV =  analogRead(A13);
  AIN0 = map(A0INV,0,1024,1024,0);
  A1INV =  analogRead(A14);
  AIN1 = map(A1INV,0,1024,1024,0);
  A2INV =  analogRead(A15);
  AIN2 = map(A2INV,0,1024,1024,0);
  CH1 = Thermister(AIN0);
  CH2 = Thermister(AIN1);
  CB = Thermister(AIN2);
  H1PID.Compute();
  H2PID.Compute();
  BPID.Compute();
  
  uiStep();                                     // check for key press
  if (uiKeyCode == KEY_SELECT){
    TempSelect();  
  }
  if (  menu_redraw_required != 0 ) {
    u8g.firstPage();
    do  {
      drawMenu();
    } while( u8g.nextPage() );
    menu_redraw_required = 0;
  }  
  
  Current_time = millis();                    // Screen refresh
  if (Current_time - Prev_time >= 500){
    menu_redraw_required = 1;
    Prev_time = Current_time;
  }

    updateMenu();                               // update menu bar
  
}

void TempSelect(){
  uiKeyCode = KEY_NONE;
  ST = HIGH;
  while(uiKeyCode != KEY_SELECT){
    uiKeyCode = KEY_NONE;
    uiStep();
    switch (menu_current){
      case 0:
         switch ( uiKeyCode ) {
           case KEY_NEXT:
            TH1 = TH1 + 0.5;
            if(TH1 < 20 ) TH1 = 20;
            if(TH1 > 100 ) TH1 = 100;
            menu_redraw_required = 1;
          break;
          case KEY_PREV:
            TH1 = TH1 - 0.5;
            if(TH1 < 20 ) TH1 = 20;
            if(TH1 > 100 ) TH1 = 100;
            menu_redraw_required = 1;
          break;
        }
      break;    
      case 1:
        switch ( uiKeyCode ) {
          case KEY_NEXT:
            TH2 = TH2 + 0.5;
            if(TH2 < 20 ) TH2 = 20;
            if(TH2 > 100 ) TH2 = 100;
            menu_redraw_required = 1;
          break;
          case KEY_PREV:
            TH2 = TH2 - 0.5;
            if(TH2 < 20 ) TH2 = 20;
            if(TH2 > 100 ) TH2 = 100;
            menu_redraw_required = 1;
          break;
        }
      break;
      case 2:
        switch ( uiKeyCode ) {
          case KEY_NEXT:
            TB = TB + 0.5;
            if(TB < 20 ) TB = 20;
            if(TB > 100 ) TB = 100;
            menu_redraw_required = 1;
          break;
          case KEY_PREV:
            TB = TB - 0.5;
            if(TB < 20 ) TB = 20;
            if(TB > 100 ) TB = 100;
            menu_redraw_required = 1;
          break;
      }
      break;
    }
  
    if (  menu_redraw_required != 0 ) {
    u8g.firstPage();
    do  {
      drawMenu();
    } while( u8g.nextPage() );
    menu_redraw_required = 0;
  }               
   
 }
 uiKeyCode = KEY_NONE;
 ST = LOW;
}

double Thermister(int RawADC) {

 double Temp;
 Temp = 8.098*exp(0.002577*RawADC)-564.2*exp(-0.01456*RawADC)+1.5;
//log(10000.0*((1024.0/RawADC-1))); 
//         =log(10000.0/(1024.0/RawADC-1)) // for pull-up configuration
// Temp = 1 / (0.001453 + (0.0002 + (0.000000015 * Temp * Temp ))* Temp );
// Temp = Temp - 273.15;            // Convert Kelvin to Celcius
  return Temp;
}

