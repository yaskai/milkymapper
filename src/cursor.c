#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "raylib.h"
#include "raymath.h"
#include "tilemap.h"
#include "cursor.h"

Tilemap *pTilemap = NULL;

bool DRAG = false;
Vector2 dragPos;

bool select_mode = false;
Coords select_strt;
Coords select_dest;
u_int16_t selection_w;
u_int16_t selection_h;
u_int16_t select_count;
int select_timer = 1;

u_int16_t buf_w, buf_h;

int mouse_down_timer = 1;

Coords prev_grid_pos;
Coords last_click_pos;

Cursor MakeCursor(Tilemap *tilemap, Camera2D *cam) {	
	pTilemap = tilemap;
	Action paste = MakeAction((Coords){1, 1}, (Coords){1, 1});

	return (Cursor) {
		.state_flags = 0,
		.select_count = 0,
		.buf_count = 0,
		.tilemap = tilemap,
		.cam = cam,
		.position = {0, 0},
		.grid_pos = {0, 0},
		.tool = SELECT,
		.tile_ch = TILE_BLOCK,
		.paste = &paste, 
		.select_box = (Coords*)malloc(sizeof(Coords) * tilemap->area),
		.clip_buf = (char*)malloc(sizeof(char) * tilemap->area),
		.ui_cooldown = 0.0f
	};
};

void CursorUpdate(Cursor *cursor) {
	if(cursor->tool == ERASER) cursor->tile_ch = TILE_EMPTY;
	
	// Position calculations	
	cursor->position = (Vector2) {
		GetMouseX() + cursor->cam->target.x - cursor->cam->offset.x,
		GetMouseY() + cursor->cam->target.y - cursor->cam->offset.y
	};

	if(cursor->position.x < 0) cursor->position.x = 0;
	if(cursor->position.y < 0) cursor->position.y = 0;

	prev_grid_pos = cursor->grid_pos;
	cursor->grid_pos = ScreenToGrid(pTilemap, (Vector2){cursor->position.x, cursor->position.y});

	if(GetMouseX() >= 1880) {
		cursor->ui_cooldown = 5.0f;
		cursor->on_ui = true;
		if(cursor->grid_pos.c == 0) cursor->grid_pos.c = pTilemap->width;
	}

	select_timer--;
	if(!cursor->on_ui && !(cursor->state_flags & CURSOR_ONUI)) cursor->ui_cooldown--;

	// Camera panning control
	if(IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
		if(!(cursor->state_flags & CURSOR_DRAG)) {
			dragPos = cursor->position;
			cursor->state_flags |= CURSOR_DRAG;
		} else if(cursor->state_flags & CURSOR_DRAG) {
			Vector2 distd = { 
				dragPos.x - cursor->position.x,
				dragPos.y - cursor->position.y
			};

			cursor->cam->target = Vector2Add(cursor->cam->target, distd);
		}
	} else if(IsMouseButtonReleased(MOUSE_BUTTON_RIGHT) && (cursor->state_flags & CURSOR_DRAG)) {
		Vector2 distr = {
			cursor->cam->target.x + (dragPos.x - cursor->position.x),
			cursor->cam->target.y + (dragPos.y - cursor->position.y)
		};

		cursor->cam->target = distr;
		cursor->state_flags &= ~CURSOR_DRAG;
	}

	if(InBounds(pTilemap, cursor->grid_pos) && !cursor->on_ui && cursor->ui_cooldown <= 0 && !(cursor->state_flags & C_SHOW_CLIP)) {
		if(cursor->tool == SELECT) {
			if(IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
				if(!select_mode) {
					select_strt = ScreenToGrid(pTilemap, (Vector2){cursor->position.x + 32, cursor->position.y + 32});
					select_mode = true;
				} else select_dest = ScreenToGrid(pTilemap, (Vector2){cursor->position.x + 32, cursor->position.y + 32});
			} else {
				if(select_mode && select_timer <= 0) {
					if(!CmpCoords(select_strt, select_dest)) {
						SetSelectionBox(cursor);
						select_timer = 2;
					} else select_count = 0;
				}

				select_mode = false;
			}
		} else if(cursor->tool == PENCIL || cursor->tool == ERASER) {
			if(IsMouseButtonDown(MOUSE_BUTTON_LEFT) &&
			   pTilemap->mapData[TileIndex(pTilemap, cursor->grid_pos.c, cursor->grid_pos.r)] != cursor->tile_ch &&
			   CheckCollisionPointRec(cursor->position, pTilemap->bounds))
				OnClick(cursor);	
		}
	} else if(cursor->state_flags & C_SHOW_CLIP) {
		if(IsMouseButtonDown(MOUSE_BUTTON_LEFT)) Paste(cursor);
	}
	
	if(IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) mouse_down_timer = 4;
	else mouse_down_timer--;
	if(mouse_down_timer < 0) mouse_down_timer = 0;

	if(!(cursor->state_flags & C_CHAR_MODE)) {
		// Cycle tools
		if(IsKeyPressed(KEY_T))	{
			cursor->tool++;
			if(cursor->tool > ERASER) cursor->tool = PENCIL;
			if(cursor->tool == PENCIL && cursor->tile_ch == '0') cursor->tile_ch = '1';
		}

		// Undo
		if(IsKeyPressed(KEY_Z) && pTilemap->action_index > 0 && cursor->ui_cooldown <= 0) {
			UndoAction(pTilemap, &pTilemap->actions[pTilemap->action_index]);
			last_click_pos = (Coords){pTilemap->width + 1, pTilemap->height + 1};
		}

		// Redo
		if(IsKeyPressed(KEY_R) && pTilemap->action_index < pTilemap->action_count) {
			RedoAction(pTilemap, &pTilemap->actions[pTilemap->action_index] + 1);
			last_click_pos = (Coords){pTilemap->width + 1, pTilemap->height + 1};
		}

		if(select_count > 0) {
			// Paint selected
			if(IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_F)) 
				PaintSelection(cursor, cursor->tile_ch);

			// Erase selected
			if(IsKeyPressed(KEY_BACKSPACE) || IsKeyPressed(KEY_D))
				PaintSelection(cursor, '0');
			
			// TO FIX...
			if(IsKeyPressed(KEY_C)) {
				Copy(cursor);
			}
		}

		if(cursor->buf_count > 0 && IsKeyPressed(KEY_V)) {
			//Paste(cursor);
			if(!(cursor->state_flags & C_SHOW_CLIP)) {
				cursor->state_flags |= C_SHOW_CLIP;	
			}
		}
	}

	if(cursor->state_flags & C_CHAR_MODE) {
		char key = GetCharPressed();
		if((key >= 'A' && key <= 'Z') || (key >= 'a' && key <= 'z')) {
			cursor->tile_ch = key;
			pTilemap->flags &= ~TM_CHARV;
			cursor->state_flags &= ~C_CHAR_MODE;
		} else if(key >= '0' && key <= '9') {
			cursor->tile_ch = key;
			pTilemap->flags &= ~TM_CHARV;
			cursor->state_flags &= ~C_CHAR_MODE;
		}
	}

	if(IsKeyPressed(KEY_GRAVE)) {
		if(cursor->state_flags & C_CHAR_MODE) {
			pTilemap->flags &= ~TM_CHARV;
			cursor->state_flags &= ~C_CHAR_MODE;
		} else if(!(cursor->state_flags & C_CHAR_MODE)) {
			pTilemap->flags |= TM_CHARV;
			cursor->state_flags |= C_CHAR_MODE;
		}
	}

}

void CursorDraw(Cursor *cursor) {
	if(select_mode && select_timer < 1) {
		Vector2 select_strt_v = CoordsToScreen(pTilemap, ClampCoords(pTilemap, select_strt)); 
		Vector2 select_dest_v = CoordsToScreen(pTilemap, ClampCoords(pTilemap, select_dest));
			
		Vector2 box_strt;
		Vector2 box_size;

		if(select_strt.c < select_dest.c) {
			box_strt.x = select_strt_v.x;
			box_size.x = select_dest_v.x - select_strt_v.x;
		} else if(select_strt.c >= select_dest.c) {
			box_strt.x = select_dest_v.x;
			box_size.x = select_strt_v.x - select_dest_v.x;
		} 

		if(select_strt.r < select_dest.r) { 
			box_strt.y = select_strt_v.y;
			box_size.y = select_dest_v.y - select_strt_v.y;
		} else if(select_strt.r >= select_dest.r) {
			box_strt.y = select_dest_v.y;
			box_size.y = select_strt_v.y - select_dest_v.y;
		}

		DrawRectangleV(box_strt, box_size, ColorAlpha(SKYBLUE, 0.5f));
	} else {
		if(!cursor->on_ui) {
			Coords draw_coords = cursor->grid_pos;
			if(draw_coords.c > pTilemap->width - 1) draw_coords.c = pTilemap->width - 1;
			if(draw_coords.r > pTilemap->height - 1) draw_coords.r = pTilemap->height - 1;
			ColorTile(pTilemap, ClampCoords(pTilemap, draw_coords), ColorAlpha(GREEN, 0.5f));
		}
	}

	if(cursor->select_box != NULL && mouse_down_timer <= 0) 
		for(int i = 0; i < select_count; i++) ColorTile(pTilemap, cursor->select_box[i], ColorAlpha(SKYBLUE, 0.25f));

	if(cursor->state_flags & C_SHOW_CLIP && cursor->buf_count > 0) {
		for(int i = 0; i < (cursor->buf_count); i++) {
			Coords draw_coords = (Coords) {
				cursor->grid_pos.c + (i % buf_w),
				cursor->grid_pos.r + (i / buf_w)
			};

			GetDrawTile(cursor->tilemap, cursor->clip_buf[i], draw_coords, 0);
			if(cursor->clip_buf[i] == '0') ColorTile(pTilemap, draw_coords, BLACK);
		}
	}
}

void CursorClose(Cursor *cursor) {
	free(cursor->select_box);
	free(cursor->clip_buf);
}

void OnClick(Cursor *cursor) {
	u_int16_t action_w, action_h;

	if(cursor->tool == PENCIL || cursor->tool == ERASER) {
		action_w = 1;
		action_h = 1;
	};
	
	u_int8_t tile_count = action_w * action_h;
	
	char *action_buffer_prev = (char*)malloc(sizeof(char) * tile_count); 
	char *action_buffer_next = (char*)malloc(sizeof(char) * tile_count); 
	 
	Action action = MakeAction(cursor->grid_pos, (Coords){action_w, action_h});

	for(u_int16_t i = 0; i < (action_w * action_h); i++) {
		u_int16_t c = i % action_w;
		u_int16_t r = i / action_w;

		action_buffer_prev[i] = FetchTile(pTilemap, cursor->grid_pos);	
		action_buffer_next[i] = cursor->tile_ch;
	}

	action.prev = action_buffer_prev;
	action.next = action_buffer_next;

	ApplyAction(pTilemap, &action);

	last_click_pos = cursor->grid_pos;
}

void SetSelectionBox(Cursor *cursor) {
	Coords min = cursor->grid_pos;
	Coords max = cursor->grid_pos;
	u_int16_t box_w = 0;
	u_int16_t box_h = 0;

	if(select_strt.c < select_dest.c) {
		min.c = select_strt.c;
		max.c = select_dest.c;
	} else if(select_strt.c > select_dest.c) {
		min.c = select_dest.c;
		max.c = select_strt.c;
	}

	if(select_strt.r < select_dest.r) {
		min.r = select_strt.r;
		max.r = select_dest.r;
	} else if(select_strt.r > select_dest.r) {
		min.r = select_dest.r;
		max.r = select_strt.r;
	}

	min = ClampCoords(pTilemap, min);
	max = ClampCoords(pTilemap, max);
	
	box_w = max.c - min.c;
	box_h = max.r - min.r;
	select_count = box_w * box_h;
	cursor->select_count = select_count;

	for(u_int16_t i = 0; i < select_count; i++) {
		u_int16_t box_c = i % box_w;
		u_int16_t box_r = i / box_w;
		cursor->select_box[i] = (Coords){box_c + min.c, box_r + min.r};
	}

	selection_w = box_w;
	selection_h = box_h;
}

void PaintSelection(Cursor *cursor, char ch) {
	Action paint = MakeAction(cursor->select_box[0], (Coords){selection_w, selection_h});
	
	char *paint_prev_buf = (char*)malloc(sizeof(char) * select_count);	
	char *paint_next_buf = (char*)malloc(sizeof(char) * select_count);
	
	for(u_int16_t i = 0; i < select_count; i++) {
		u_int16_t box_c = i % paint.w;
		u_int16_t box_r = i / paint.w;

		paint_prev_buf[i] = pTilemap->mapData[TileIndex(pTilemap, paint.c + box_c, paint.r + box_r)];
		paint_next_buf[i] = ch;
	}

	paint.prev = paint_prev_buf;
	paint.next = paint_next_buf;
	ApplyAction(pTilemap, &paint);
}

void Copy(Cursor *cursor) {
	if(cursor->state_flags & DEBUG_C) printf("copy\n");

	buf_w = selection_w;
	buf_h = selection_h;
	cursor->buf_count = select_count;

	Action copy = MakeAction(cursor->select_box[0], (Coords){selection_w, selection_h});
	
	cursor->paste->c = cursor->select_box[0].c;
	cursor->paste->r = cursor->select_box[0].r;
	cursor->paste->w = selection_w;
	cursor->paste->h = selection_h;
	cursor->clip_buf = realloc(cursor->clip_buf, sizeof(char) * cursor->buf_count);

	for(u_int16_t i = 0; i < cursor->select_count; i++) {
		copy.next[i] = FetchTile(pTilemap, cursor->select_box[i]);
		
		if(cursor->state_flags & DEBUG_B) {
			u_int16_t c = i % buf_w;
			printf("%c", copy.next[i]);
			if(c == selection_w - 1) printf("\n");
		}
	}

	cursor->clip_buf = copy.next;
}

void Paste(Cursor *cursor) {
	if(cursor->state_flags & DEBUG_C) printf("paste\n");

	Coords paste_coords = ClampCoords(pTilemap, cursor->grid_pos);
	
	u_int16_t paste_w = buf_w;
	u_int16_t paste_h = buf_h;

	if(paste_coords.c + paste_w > pTilemap->width)  paste_w = (pTilemap->width - paste_coords.c);
	if(paste_coords.r + paste_h > pTilemap->height) paste_h = (pTilemap->height - paste_coords.r);

	u_int16_t paste_count = paste_w * paste_h;
	
	char *paste_prev_buf = (char*)(malloc(sizeof(char) * paste_count));
	char *paste_next_buf = (char*)(malloc(sizeof(char) * paste_count));
	Action paste = MakeAction(paste_coords, (Coords){paste_w, paste_h});

	//TODO: 
	// Add check for left
	// and top of bounds
	
	for(u_int16_t r = 0; r < paste_h; r++) {
		for(u_int16_t c = 0; c < paste_w; c++) {
			u_int16_t paste_idx = c + r * paste_w;
			u_int16_t bufrr_idx = c + r * buf_w;

			paste_prev_buf[bufrr_idx] = FetchTile(pTilemap, (Coords){paste_coords.c + c, paste_coords.r + r}); 		
			paste_next_buf[paste_idx] = cursor->clip_buf[bufrr_idx];
		}
	}

	paste.prev = paste_prev_buf;
	paste.next = paste_next_buf;
	ApplyAction(pTilemap, &paste);
		
	cursor->state_flags &= ~C_SHOW_CLIP;
}

