#ifndef SPRITESHEET_H_
#define SPRITESHEET_H_

#include <sys/types.h>
#include "raylib.h"

typedef struct {
	u_int8_t cols, rows;
	u_int8_t frame_w, frame_h;
	u_int8_t frame_count;
	Rectangle *frame_rec;
	Texture2D texture;
} Spritesheet;

Spritesheet MakeSpritesheet(u_int8_t w, u_int8_t h, Texture2D texture);
void SpritesheetClose(Spritesheet *spritesheet);
u_int8_t GetFrame(Spritesheet *spritesheet, u_int8_t c, u_int8_t r);
void DrawSprite(Spritesheet *spritesheet, Vector2 position, Rectangle spr_rec, float alpha);

#endif // !SRITESHEET_H_
