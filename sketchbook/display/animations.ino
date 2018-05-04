
#include "pixel.h"
#include "animations.h"
#include "data_layout.h"

// All animation functions have three functions:
// 	Initialization - Sets variables
//		- cfg - Configuration for different modes of the same animation
//		- Mode_steps - Number of steps to complete one cycle of this animation
//			A value of 0 is infinite
//		- ms - ms per frame for the animation
//			0 indicates that the animation should run as fast as possible
//	Animation - Performs the animation at the current step
//	Switch - Returns true when the animation can be switched

uint8_t mode_cfg = 0;
uint8_t mode_steps = 0;
uint16_t ms = 0;

#define FPS(x) (1000/x)

// This is a flag for allowing the mode to change
uint8_t mode_change_allowed = 1;
// This is a flag for when a button requests a mode change
uint8_t mode_change_requested = 0;

// Function for blanking the LEDs
void blankLEDs(void) {
	for (uint8_t i = 0; i < NUM_LEDS; i++) {
  		leds[i] = CRGB::Black;
  	}
}

// **********************************************
// * Turns the Display completely off
// **********************************************
void init_DispOff(uint8_t cfg) {
	mode_steps = 1;	
	ms = 0;
	mode_cfg = cfg;
}

void anim_DispOff(uint8_t step) {
	if (step == 0) {
		blankLEDs();
	}
}

void switch_DispOff(uint8_t step) {
	state_transition_anim_allowed = 1;
}


// **********************************************
// * "PaletteFade" Animation
// **********************************************
void init_PaletteFade(int cfg) {
	mode_cfg = cfg;
	mode_steps = 255;
	ms = 0;
}

void anim_PaletteFade(uint8_t step) {
	for (uint8_t i=0; i<PIXEL_CT; i++) {
		// Half brightness
		setPixel(i, step + i, 255);
	}
}

void  switch_PaletteFade(uint8_t step) {
	state_transition_anim_allowed = 1;
}

// **********************************************
// * "Sweep Down" Animation
// **********************************************
void init_SweepDown(int cfg) {
  mode_cfg = cfg;
  mode_steps = 10;
  ms = FPS(4);
  blankLEDs();
}

void anim_SweepDown(uint8_t step) {
  if (mode_cfg==0) {
    blankLEDs();
  }
  for (uint8_t i=0; i<4; i++) {
    setPixel(layout[step][i], palette_step, 255);  
  }
  palette_step += 20;
}

bool switch_SweepDown(uint8_t step) {
  if (step == (mode_steps -1)) { 
    state_transition_anim_allowed = 1;
  } else {
    state_transition_anim_allowed = 0;
  }
}

// **********************************************
// * "Sprial" Animation
// **********************************************
void init_Sprial(int cfg) {
  mode_cfg = cfg;
  mode_steps = NUM_LEDS;
  ms = FPS(10);
  blankLEDs();
}

void anim_Sprial(uint8_t step) {
  const uint8_t pattern[NUM_LEDS] =  {39,29,19,9,8,7,6,5,4,3,2,1,0,10,20,30,31,32,33,34,35,36,37,38,28,18,17,16,15,14,13,12,11,21,22,23,24,25,26,27};
  setPixel(pattern[step], palette_step, 255);  
}

void switch_Sprial(uint8_t step) {
  if (step == (mode_steps-1)) {
    state_transition_anim_allowed = 1;
  }
  else
  {
    state_transition_anim_allowed = 0;
  }
}

// **********************************************
// * "Space Odd" Animation
// **********************************************
//CRGB leds[NUM_LEDS]; // current / transition
//CRGB final_leds[NUM_LEDS]; // final
void init_SpaceOdd(int cfg) {
  mode_cfg = cfg;
  mode_steps = 255;
  ms = 0;
  blankLEDs();
}

void anim_SpaceOdd(uint8_t step) {
  // pick random color per pixel
  // fade from current state to new color (all at once)
  if (step == 0) {
    for (uint8_t ir = 0; ir < NUM_LEDS; ir++)
    {
      // pick random color per led
      // rand 0-255 palette_step
      palette_step = (uint8_t)(random(0, 255));
      setPixelFinal(ir, palette_step, 255);
    }
  } // step=0

  uint8_t leds_changed = 0;
  for (uint8_t i = 0; i < NUM_LEDS; i++)
  {
    // update each pixel (per step)
    // RED PIXEL
    if (final_leds[i].red > leds[i].red){
      leds[i].red += 1;
      leds_changed = leds_changed & (1<<i);
    }
    else if (final_leds[i].red < leds[i].red){
      leds[i].red -= 1;
      leds_changed = leds_changed & (1<<i);
    }

    // GREEN PIXEL
    if (final_leds[i].green > leds[i].green){
      leds[i].green += 1;
      leds_changed = leds_changed & (1<<i);
    }
    else if (final_leds[i].green < leds[i].green){
      leds[i].green -= 1;
      leds_changed = leds_changed & (1<<i);
    }

    //BLUE PIXEL
    if (final_leds[i].blue > leds[i].blue){
      leds[i].blue += 1;
      leds_changed = leds_changed & (1<<i);
    }
    else if (final_leds[i].blue < leds[i].blue){
      leds[i].blue -= 1;
      leds_changed = leds_changed & (1<<i);
    }

  }
  
  if (leds_changed == 0)
  {
    // no leds have changed.. Animation run done. force step0
    mode_steps = step + 1;
    Serial.println("Space Odd resetting to setp 0");
  }
}

void switch_SpaceOdd(uint8_t step) {
    state_transition_anim_allowed = 1;
}

// ****************************************************************************
// Animation modes
// ****************************************************************************
typedef void (*Animation_Init_t)(uint8_t);
Animation_Init_t Animation_Inits[5] = {
	init_DispOff,
  init_PaletteFade,
  init_SweepDown,
  init_Sprial,
  init_SpaceOdd
};

typedef void (*Animation_Func_t)(uint8_t);
Animation_Func_t Animation_Funcs[5] = {
	anim_DispOff,
	anim_PaletteFade,
  anim_SweepDown,
  anim_Sprial,
  anim_SpaceOdd
};

typedef void (*Animation_Switch_t)(uint8_t);
Animation_Switch_t Animation_Switches[5] = {
	switch_DispOff,
	switch_PaletteFade,
  switch_SweepDown,
  switch_Sprial,
  switch_SpaceOdd
};

// ****************************************************************************
// Animation functions
// ****************************************************************************
void initAnimation(uint8_t anim, uint8_t cfg) {
	blankLEDs();
	(Animation_Inits[anim])(cfg);
}

bool animAnimation(uint8_t anim, uint8_t step) {
	(Animation_Funcs[anim])(step);

	// Update the LEDs
  copy_leds_2_disp_leds();
	write_display();
	
	// Delay for the FPS
  if (mode_cfg == 255)
    ms = 0;
	if (ms != 0)
		delay(ms);

	return (step == (mode_steps - 1));
}

void switchAnimation(uint8_t anim, uint8_t mode, uint8_t step) {
  (Animation_Switches[mode])(step);
}

void copy_leds_2_disp_leds() {
  // TODO update to pointer
  lp2_pixel pix;
  uint8_t i;
  pix.adr = 0; // color data addr
  pix.dc = 0;
  for (i = 0; i < NUM_LEDS; i++) {
      pix.grn = 0;
      pix.red = 0; 
      pix.blu = 0;
      // todo map leds[i].color (8-bit) to pixel.color (10-bit)
      //map(value, fromLow, fromHigh, toLow, toHigh)
      pix.grn = map(leds[i].green, 0, 255, 0, 1023);
      pix.red = map(leds[i].red, 0, 255, 0, 1023);
      pix.blu = map(leds[i].blue, 0, 255, 0, 1023);
      disp_leds[i] = pix;
  }
}

void write_display() {
/*
 * 0-0 [39] 1-2 [29] 2-4 [19] 3-6 [9]
 * 0-1 [38] 1-3 [28] 2-5 [18] 3-7 [8]
 * 0-2 [37] 1-4 [27] 2-6 [17] 4-0 [7]
 * 0-3 [36] 1-5 [26] 2-7 [16] 4-1 [6]
 * 0-4 [35] 1-6 [25] 3-0 [15] 4-2 [5]
 * 0-5 [34] 1-7 [24] 3-1 [14] 4-3 [4]
 * 0-6 [33] 2-0 [23] 3-2 [13] 4-4 [3]
 * 0-7 [32] 2-1 [22] 3-3 [12] 4-5 [2]
 * 1-0 [31] 2-2 [21] 3-3 [11] 4-6 [1]
 * 1-1 [30] 2-3 [20] 3-5 [10] 4-7 [0] 
 */
//lp2_pixel disp_leds[NUM_LEDS];

  lp2_pixel pixel;
  uint8_t i, j;
  uint32_t mask = 0x80000000;
  for (j = 0; j < NUM_LEDS; j++) {
    // pixel to write
    pixel = disp_leds[j];
    for(i = 0; i < 32; i++)
    {
      if((pixel.data)&mask)
      {
        // shift in a '1'
        digitalWrite(DATA, HIGH);
      }
      else
      {
        // shift in a '0'
        digitalWrite(DATA, LOW);
      }
    
      // clock in DATA
      digitalWrite(CLK, HIGH);
      digitalWrite(CLK, LOW);


      // shift the bits in copy
      pixel.data = pixel.data << 1;
    } // end for data shift
  } // end for each pixel

  /* LATCH */
  digitalWrite(LATCH, HIGH);;
  digitalWrite(LATCH, LOW);

} // end write_display
