#include <LiquidCrystal.h>

//http://brilldea.net/assets_files/LPII-8/LED%20Painter%20II-8%20Schematic%20Rev0.pdf
//http://brilldea.net/assets_files/LPII-8/LED%20Painter%20II-8-Datasheet-Rev011.pdf
//A6281

#define EN_SER_PR

#include <SPI.h>
#include "FastLED.h"
#include <Bounce2.h>
#include "animations.h"
#include "pixel.h"
#include "data_layout.h"

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

// LED Defines
#define ROWS 10
#define COLS 4
#define NUM_LEDS ROWS*COLS

CRGB leds[NUM_LEDS];  // led display buffer - 8 bit (40 LEDs -> 3 bytes -> 120bytes)
CRGB final_leds[NUM_LEDS]; // temp led disply buffer - 8bit (120 bytes)
lp2_pixel disp_leds[NUM_LEDS]; // led display buffer - 10 bit (160 bytes)
lp2_pixel cmd_leds[NUM_LEDS]; // led display buffer - 10 bit (160 bytes)

/********************** Setup *********************/
// 11 bytes
uint8_t state = ANIM_DISP_OFF; // Current state
uint8_t state_step = 0;  // Position in current state
uint8_t state_init = 0;  // Initialize state
uint8_t state_change_requested = 0;  // next state requested
uint8_t state_next_state = 0xff;  // next state override
uint8_t state_transition_anim_allowed = 0;  // allow next state
uint8_t reset_state_step = 0;  // global to allow state_step to be reset to 0
uint8_t palette_step = 0;  // step in palette / color blending
uint8_t bt_anim_mode = 1;  // animation mode (state mode)
uint8_t bt_anim_cfg = 0;   // animation config (state config)

// 4*2 -> 8 bytes
unsigned long last_state_change = 0;  // timestamp of last state change
const unsigned long state_change_timeout = 20 * 1000; // in seconds  // Transion period


void setup() {
  // Initialize the random number generator
  CreateTrulyRandomSeed();

  // Enable the serial device
  #ifdef EN_SER_PR
  Serial.begin(115200);
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
  
  #ifdef EN_SER_PR
    Serial.print("Setup");
  #endif

  // A6281 cfg Reg
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    // see data_layout.h for details
    // Set cfg reg (will always be this value)
    cmd_leds[i].data = A6281_CFG_REG;
    // Set data reg (software may change)
    disp_leds[i].data = 0;
  }
  write_display();  // write both cfg reg and data reg
  
  // Load default palette to memory and init backup palette
  LoadPalette(PALETTE_RAINBOW);
  BackupPalette();
  
  last_state_change = millis();
  // Enable first state transition
  state_init=1;
  state_change_requested = 1; // allow initial state to change
  state_transition_anim_allowed = 1;  // allow inital state to change
  #ifdef EN_SER_PR
    Serial.println("... End Setup.");
  #endif
}


/********************** Main Loop *********************/
void loop() {
  // Transition state on timer
  // TODO add disable for node mcu control
  if (state_change_requested == 0)
  {
    if (millis() >= (last_state_change + state_change_timeout))
    {
      #ifdef EN_SER_PR
      Serial.println("Timer State Transition");
      #endif
      state_change_requested = 1;
    }
  }
  else
  {
    // State Change logic
    switchAnimation(state, bt_anim_mode, state_step);
    if (state_transition_anim_allowed == 1)
    {
      #ifdef EN_SER_PR
      Serial.print("State Change from "); Serial.print(state);
      #endif

      // no override for next state
      if (state_next_state == 0xff)
      {
        state++;
      }
      else
      {
        // handle state override
        state = state_next_state;
        state_next_state = 0xff;  // reset override
      }

      // Handle state out of bounds
      if (state >= ANIM_COUNT)
      {
        state = 1;
      }

      #ifdef EN_SER_PR
      Serial.print(" to "); Serial.println(state);
      #endif

      // setup state change
      last_state_change = millis(); // store state change time
      state_init = 1;  // we changed states.. perform state init (execution later)
      #ifdef EN_SER_PR
      Serial.println("Clearing state change request");
      #endif
      state_change_requested = 0;  // clear state change request
      state_transition_anim_allowed = 0; // clear state change allowed
      state_step = 0;  // start at the beginning of the next state
      RestorePalette();  // restore previous color palette - TBD May not be needed
    } // state_transmission_anim_allowed
    else
    {
      #ifdef EN_SER_PR
      Serial.print("State change not allowed --- ");
      Serial.println(state);
      #endif
    }
  } // state_change_requested

  // State machine - select animatioin mode (init only), cfg, and palette
  if (state_init != 0) {
    #ifdef EN_SER_PR
    Serial.println("Init state");
    #endif
    if (state == ANIM_DISP_OFF) {
      bt_anim_mode = 0;
      bt_anim_cfg = 0;
      LoadPalette(PALETTE_RAINBOW);
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
    }
    else if (state == ANIM_DISP_SPACE_ODD) {
      bt_anim_mode = 4;
      bt_anim_cfg = 0;
      LoadPalette(PALETTE_RAINBOW);
    }

    initAnimation(bt_anim_mode, bt_anim_cfg);
    state_init = 0;
  }

  // run animation
  animAnimation(bt_anim_mode, state_step);

  // handle animation step
  if(reset_state_step == 1) {
    #ifdef EN_SER_PR
    Serial.println("Resetting state step");
    #endif
    state_step = 0;
    reset_state_step = 0;
  }
  else
  {
    // increment animation step
    state_step++;
  }
}
