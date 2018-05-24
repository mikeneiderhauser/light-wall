#ifndef __ANIMATIONS_T__
#define __ANIMATIONS_T__

#include "data_layout.h"

void initAnimation(uint8_t anim, uint8_t cfg);
void animAnimation(uint8_t anim, uint8_t step);
void switchAnimation(uint8_t anim, uint8_t step);
void write_display();
void write_display_reg(lp2_pixel leds[]);
void copy_leds_2_disp_leds();

#define ANIM_DISP_OFF  0 // mode 0, cfg x
#define ANIM_DISP_RAINBOW 1 // mode 1, cfg x
#define ANIM_DISP_FUSCIA 2 // mode 1, cfg x
#define ANIM_DISP_SWEEP_DOWN 3 // mode 2, cfg 0
#define ANIM_DISP_FADE_SWEEP_DOWN 4 // mode 2, cfg 1
#define ANIM_DISP_SPRIAL 5 // mode 3, cfg x
#define ANIM_DISP_SPACE_ODD 6 // mode 4
#define ANIM_COUNT 6  // ****SPACE ODD Disabled****

#define ANIM_MODE_OFF 0
#define ANIM_MODE_FADE 1
#define ANIM_MODE_SWEEP 2
#define ANIM_MODE_SPIRAL 3
#define ANIM_MODE_SPACE_ODD 4
#define ANIM_MODE_COUNT 5

const uint8_t layout[10][4] = {
  {39,29,19,9},
  {38,28,18,8},
  {37,27,17,7},
  {36,26,16,6},
  {35,25,15,5},
  {34,24,14,4},
  {33,23,13,3},
  {32,22,12,2},
  {31,21,11,1},
  {30,20,10,0}
};

#endif
