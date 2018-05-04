#include <LiquidCrystal.h>

//http://brilldea.net/assets_files/LPII-8/LED%20Painter%20II-8%20Schematic%20Rev0.pdf
//http://brilldea.net/assets_files/LPII-8/LED%20Painter%20II-8-Datasheet-Rev011.pdf
//A6281

//#define DEBUG
#define EN_SER_PR

#include <SPI.h>
#include "FastLED.h"
#include <Bounce2.h>
#include "animations.h"
#include "pixel.h"
#include "data_layout.h"

#ifdef DEBUG
#include "printf.h"
#endif

#define OB_RX0         0  // Hardware USART 0 RX Pin
#define OB_TX0         1  // Hardware USART 0 TX Pin
#define ATSHA204_SDA   2  // Shield SDA for ATSHA204 Auth Chip
#define GREEN_OB_LED   3  // Shield Green LED
#define DATA           4  // LED Painter II-8 ~ Serial Data In Pin
#define YELLOW_OB_LED  5  // Shield Yellow LED
#define RED_OB_LED     6  // Shield Red LED
#define LATCH          7  // LED Painter II-8 ~ Latch In Pin
#define EN             8  // LED Painter II-8 ~ Chip Enable In Pin (Active LOW)
#define CLK            9  // LED Painter II-8 ~ Clock In Pin
#define DS3234_CS     10  // Chip Select for DS3234 RTC
#define SOFT_MOSI     11  // MOSI Pin for DS3234
#define SOFT_MISO     12  // MISO Pin for DS3234
#define SOFT_SCK      13  // SCK  Pin for DS3234

#define BRIGHTNESS  10

unsigned long top_sw_fts;

unsigned long ts;

// LED Defines
#define ROWS 10
#define COLS 4
#define NUM_LEDS ROWS*COLS

CRGB leds[NUM_LEDS];
CRGB final_leds[NUM_LEDS]; // final
lp2_pixel disp_leds[NUM_LEDS];


/********************** Setup *********************/

uint8_t state =ANIM_DISP_SPRIAL; //ANIM_DISP_OFF;
uint8_t state_step = 0;
uint8_t state_init = 0;
uint8_t state_change_requested = 0;
uint8_t state_next_state = 0xff;
uint8_t palette_step = 0;
uint8_t tx_fail_ct = 0;
#define MAX_TX_FAIL 10
uint8_t bt_anim_mode = 1;
uint8_t bt_anim_cfg = 0;

unsigned long last_state_change = 0;
unsigned long state_change_timeout = 21 * 1000; // in seconds


void setup() {
  // Initialize the random number generator
  CreateTrulyRandomSeed();

  // Enable the serial device
  #ifdef EN_SER_PR
  Serial.begin(115200);
  #ifdef DEBUG 
    printf_begin();
  #endif
  #endif

  /* Setup shift regs */
  pinMode(CLK,OUTPUT);
  pinMode(LATCH, OUTPUT);
  pinMode(EN,OUTPUT);
  pinMode(DATA,OUTPUT);

  pinMode(RED_OB_LED, OUTPUT);
  pinMode(YELLOW_OB_LED, OUTPUT);
  pinMode(GREEN_OB_LED, OUTPUT);

  /* Set random pin as input */
  pinMode(A2, INPUT); // for random numbers

  /* init shift regs */
  //turn everything off
  digitalWrite(CLK,LOW);
  digitalWrite(LATCH,LOW);
  digitalWrite(EN,LOW);//active low pin
  digitalWrite(DATA,LOW);//init at 0

  digitalWrite(RED_OB_LED, LOW);
  digitalWrite(YELLOW_OB_LED, LOW);
  digitalWrite(GREEN_OB_LED, LOW);
  
  // LED Matrix setup
  // populate w/ cfg data
  // write out
  // copy with 0 data
  // write out

  #ifdef EN_SER_PR
    Serial.print("Setup");
  #endif
  // A6281 cfg Reg
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    disp_leds[i].data = 0b01000111111100011111110001111111;
  }
  write_display();

  // 0 color value
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    disp_leds[i].data = 0;
  }
  write_display();
  
  // Load default palette to memory and init backup palette
  LoadPalette(PALETTE_RAINBOW);
  BackupPalette();
  
  last_state_change = millis();
  // Enable first state transition
  state_init=1;
  state_change_requested = 1;
}


/********************** Main Loop *********************/
void loop() {
  // Transition state on timer
  // TODO add disable for node mcu control
  if (millis() >= (last_state_change + state_change_timeout))
  {
    Serial.println("Timer State Transition");
    state_change_requested = 1;
  }

  // State Change logic
  if (state_change_requested == 1 && switchAnimation(state, bt_anim_mode, state_step) == true)
  {
    #ifdef EN_SER_PR
    Serial.print("State Change from "); Serial.print(state);
    #endif
    // no defined next state
    if (state_next_state == 0xff)
    {
      state++;
      if(state >= ANIM_COUNT)
      {
        state = 1;  // 0 is the off state. we dont want to automatically transition to off
      }
    }
    // defined next state
    else
    {
      // Allow external events (nrf incoming, button presses, etc to set the state)
      state = state_next_state;
      state_next_state = 0xff;
    }
    #ifdef EN_SER_PR
    Serial.print(" to "); Serial.println(state);
    #endif

    // state var cleanup
    state_change_requested = 0;  // clear state change request
    state_step = 0;  // reset state step
    RestorePalette();  // restore previous color palette - TBD May not be needed
    // save current time
    last_state_change = millis();

    state_init = 1;  // we changed states.. perform state init (execution later)
  }
  
  // State machine - select animation mode, cfg, wrist state, and palette
  // TODO load palette per state
  if (state == ANIM_DISP_OFF) {
    bt_anim_mode = 0;
    bt_anim_cfg = 0;
  }
  else if (state == ANIM_DISP_RAINBOW) {
    bt_anim_mode = 1;
    bt_anim_cfg = 0;
    LoadPalette(PALETTE_RAINBOW);
  }
  else if (state == ANIM_DISP_FUSCIA) {
    bt_anim_mode = 1;
    bt_anim_cfg = 0;
    LoadPalette(PALETTE_PURPLE);
  }
  else if (state == ANIM_DISP_SWEEP_DOWN) {
    bt_anim_mode = 2;
    bt_anim_cfg = 0;
    LoadPalette(PALETTE_RAINBOW);
  }
  else if (state == ANIM_DISP_FADE_SWEEP_DOWN) {
    bt_anim_mode = 2;
    bt_anim_cfg = 1;
    LoadPalette(PALETTE_RAINBOW);
  }
  else if (state == ANIM_DISP_SPRIAL) {
    bt_anim_mode = 3;
    bt_anim_cfg = 0;
    LoadPalette(PALETTE_RAINBOW);
    palette_step += 10;
  }
  else if (state == ANIM_DISP_SPACE_ODD) {
    bt_anim_mode = 4;
    bt_anim_cfg = 0;
    LoadPalette(PALETTE_RAINBOW);
  }
  
  // perform animation init
  if(state_init != 0)
  {
    state_init = 0;
    initAnimation(bt_anim_mode, bt_anim_cfg);
  }

  // run animation
  if(animAnimation(bt_anim_mode, state_step) == true) {
    state_step = 0;
  }
  else
  {
    // increment animation step
    state_step++;
  }
}
