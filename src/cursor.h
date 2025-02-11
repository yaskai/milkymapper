#ifndef CURSOR_H_
#define CURSOR_H_

#include <sys/types.h>
#include "raylib.h"
#include "tilemap.h"

enum Tool : u_int8_t {
	PENCIL,
	SELECT,
	ERASER
};

#define CURSOR_ONUI 0x01
#define CURSOR_DRAG 0x02
#define C_CHAR_MODE 0x04
#define C_SHOW_CLIP 0x08

typedef struct {
	u_int8_t state_flags;
	char tile_ch;
	enum Tool tool;
	u_int16_t select_count;
	u_int16_t buf_count;
	bool on_ui;
	float ui_cooldown;
	Coords grid_pos;
	Vector2 position;
	char *clip_buf;
	Action *paste;
	Coords *select_box;
	Camera2D *cam;
	Tilemap *tilemap;
} Cursor;

Cursor MakeCursor(Tilemap *tilemap, Camera2D* cam);
void CursorUpdate(Cursor *cursor);
void CursorDraw(Cursor *cursor);
void CursorClose(Cursor *cursor);
void SetSelectionBox(Cursor *cursor);
void OnClick(Cursor *cursor);
void PaintSelection(Cursor *cursor, char ch);
void Copy(Cursor *cursor);
void Paste(Cursor *cursor);

#endif // !CURSOR_H_

