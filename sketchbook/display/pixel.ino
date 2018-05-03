#include <avr/pgmspace.h>
#include "pixel.h"
#include "FastLED.h"

// ******************************************************************
// Color Palette Functionality
// ******************************************************************

// Palettes
const TProgmemPalette16 PurplePalette_p PROGMEM = {
  CRGB::Purple,CRGB::Purple,CRGB::Purple,CRGB::Purple,
  CRGB::Purple,CRGB::Purple,CRGB::Purple,CRGB::Purple,
  CRGB::Purple,CRGB::Purple,CRGB::Purple,CRGB::Purple,
  CRGB::Purple,CRGB::Purple,CRGB::Purple,CRGB::Purple
};

const TProgmemPalette16 RedPalette_p PROGMEM = {
  CRGB::Red,CRGB::Red,CRGB::Red,CRGB::Red,
  CRGB::Red,CRGB::Red,CRGB::Red,CRGB::Red,
  CRGB::Red,CRGB::Red,CRGB::Red,CRGB::Red,
  CRGB::Red,CRGB::Red,CRGB::Red,CRGB::Red
};

const TBlendType Blending[5] = {
  LINEARBLEND,
  LINEARBLEND,
  LINEARBLEND,
  NOBLEND,
  NOBLEND
};

CRGBPalette16 currentPalette;
TBlendType    currentBlending;

CRGBPalette16 backupPalette;
TBlendType    backupBlending;

void BackupPalette(void) {
  backupPalette = currentPalette;
  backupBlending = currentBlending;
}

void RestorePalette(void) {
  currentPalette = backupPalette;
  currentBlending = backupBlending;
}

void LoadPalette(uint8_t palette_id) {
  switch(palette_id) {
    case PALETTE_RAINBOW:
      currentPalette = RainbowColors_p;
      break;
    case PALETTE_PARTY:
      currentPalette = PartyColors_p;
      break;
    case PALETTE_HEAT:
      currentPalette = HeatColors_p;
      break;
    case PALETTE_PURPLE:
      currentPalette = PurplePalette_p;
      break;
    case PALETTE_RED:
      currentPalette = RedPalette_p;
      break;
  }
  currentBlending = Blending[palette_id];
}


// Set a pixel from a pixel # and color index
void setPixel(uint8_t p, uint8_t c, uint8_t brightness) {
  if (p < NUM_LEDS)
	  leds[p] = ColorFromPalette(currentPalette, c, brightness, currentBlending);
}

// Set a pixel from a pixel # and color index
void setPixelFinal(uint8_t p, uint8_t c, uint8_t brightness) {
  if (p < NUM_LEDS)
    final_leds[p] = ColorFromPalette(currentPalette, c, brightness, currentBlending);
}

// Format for Animations:
// Animation Header:
//	uint8_t frame_ct
//	uint8_t FPS
//	uint8_t palette
//	// Offset from start of animation data (LE 16 bit)
//	uint16_t frame_offsets[frame_ct]
//	frames[frame_ct]

// Frame format: 
//	uint8_t pixel_ct
//	pp_t pixels[pixel_ct]

// Packed Pixel from Matrix Editor
// If the pixel # is 0xFX, then it's a command instead
// typedef struct {
//	// Pixel index
//  	uint8_t pixel;
//	// Palette Color Index
//  	uint8_t color;
// } pp_t;

// Set the color of the companion wrist corsage
#define PP_CMD_SET_WRIST_COLOR	0

// Unpack array of pixels with palette into display array
// If undo is set, turn off the pixel instead of turning it on
void UnpackFrame(uint8_t frame, uint8_t *animation_data, bool undo, uint8_t color_offset) {
	uint8_t i, px, ct, pixel, color;

	uint16_t offset = pgm_read_word_near(animation_data + ((frame << 1) + 3));
	uint8_t size = pgm_read_byte_near(animation_data + offset);
	uint8_t *p = animation_data + offset + 1;

	// Unpack the frame
	for (i=0; i<2*size; i += 2) {
		pixel = pgm_read_byte_near(p + i);

		// It's a command
		if ((pixel & 0xF0) == 0xF0) {
			switch(pixel & 0xF) {
			// FIXME: Implement commands
			default:
				break;
			}
		// Otherwise, it's pixel data
		} else {
			// If undo is enabled, we turn off the LED
			if (undo) {
				leds[pixel] = CRGB::Black;
			// Colors are always maximum brightness...
			} else {
				color = pgm_read_byte_near(p + i + 1) + color_offset;
				leds[pixel] = ColorFromPalette(currentPalette, color, 0xFF, currentBlending);
			}
		}
	}
}

// Load animation frame from progmem
void LoadFrame(uint8_t frame_idx, uint8_t *animation_data, uint8_t color_offset) {
  uint8_t previous = frame_idx - 1;
	if (frame_idx == 0)
		previous = pgm_read_byte_near(animation_data) - 1;

	UnpackFrame(previous, animation_data, true, color_offset);
	UnpackFrame(frame_idx, animation_data, false, color_offset);
}
