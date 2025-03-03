#ifndef SPRITESHEET_H_
#define SPRITESHEET_H_

#include <stdint.h>
#include "raylib.h"

typedef struct {
	uint8_t cols, rows;
	uint8_t frame_w, frame_h;
	uint8_t frame_count;
	Rectangle *frame_rec;
	Texture2D texture;
} Spritesheet;

Spritesheet MakeSpritesheet(uint8_t w, uint8_t h, Texture2D texture);
void SpritesheetClose(Spritesheet *spritesheet);
uint8_t GetFrame(Spritesheet *spritesheet, uint8_t c, uint8_t r);
void DrawSprite(Spritesheet *spritesheet, Vector2 position, Rectangle spr_rec, float alpha);

#endif // !SRITESHEET_H_
