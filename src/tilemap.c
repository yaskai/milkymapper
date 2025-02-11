#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdio.h>
#include "raylib.h"
#include "spritesheet.h"
#include "tilemap.h"

const int TILE_SIZE = 64;
float scaled_tile_size = TILE_SIZE;

Color grid_color = RAYWHITE;

u_int16_t tile_lft;
u_int16_t tile_rgt;
u_int16_t tile_top;
u_int16_t tile_bot;

void TilemapInit(Tilemap *tilemap, Camera2D *cam, Spritesheet *ss, u_int16_t width, u_int16_t height) {
	tilemap->cam = cam;
	tilemap->spritesheet = ss;
	
	tilemap->width = width;
	tilemap->height = height;
	tilemap->area = width * height;
	tilemap->tileSize = (Vector2){TILE_SIZE, TILE_SIZE};
	
	tilemap->mapData = (char*)malloc(sizeof(char));
	tilemap->spr_index = (u_int8_t*)malloc(sizeof(u_int8_t));
	
	tilemap->action_count = 0;
	tilemap->action_index = 0;
	tilemap->action_max_count = INIT_MAX_ACTIONS;
	tilemap->actions = (Action*)malloc(sizeof(Action) * INIT_MAX_ACTIONS);

	tilemap->flags = (DEBUG_A);
}

void TilemapUpdateSprites(Tilemap *tilemap) {
	for(uint16_t i = 0; i < tilemap->area; i++) {
		u_int16_t c = i % tilemap->width;
		u_int16_t r = i / tilemap->width;
		tilemap->spr_index[i] = TileGetAdj(tilemap, (Coords){c, r});
	}
}

void DrawTiles(Tilemap *tilemap) {
	u_int16_t frame_w = (1920 / scaled_tile_size) + 4;
	u_int16_t frame_h = (1080 / scaled_tile_size) + 4;
	
	tile_lft = ScreenToGrid(tilemap, (Vector2){tilemap->cam->target.x, tilemap->cam->target.y}).c - 2;
	tile_rgt = tile_lft + frame_w;
	tile_top = ScreenToGrid(tilemap, (Vector2){tilemap->cam->target.x, tilemap->cam->target.y}).r - 2;
	tile_bot = tile_top + frame_h;

	if(tile_rgt - tile_lft < frame_w) {
		tile_lft = 0;
		tile_rgt = frame_w;
	}

	if(tile_bot - tile_top < frame_h) {
		tile_top = 0;
		tile_bot = frame_h;
	}

	if(tile_lft < 0) tile_lft = 0;
	if(tile_rgt > tilemap->width) tile_rgt = tilemap->width;
	if(tile_top < 0) tile_top = 0;
	if(tile_bot > tilemap->height) tile_bot = tilemap->height;

	for(u_int16_t r = tile_top; r < tile_bot; r++) {
		for(u_int16_t c = tile_lft; c < tile_rgt; c++) {
			GetDrawTile(tilemap, FetchTile(tilemap, (Coords){c, r}), (Coords){c, r}, TileIndex(tilemap, c, r));
		}
	}
}

void DrawTileGrid(Tilemap *tilemap) {
	for(u_int16_t i = 0; i < tilemap->area; i++) {
		u_int16_t c = i % tilemap->width;
		u_int16_t r = i / tilemap->width;

		DrawRectangleLines(c * 64, r * 64,  64, 64, grid_color);
		
		if(tilemap->flags & TM_CHARV) {
			DrawText(TextFormat("%c", tilemap->mapData[TileIndex(tilemap, c, r)]),
			c * scaled_tile_size, r * scaled_tile_size, 20, GREEN);
		}
	}

	if(tilemap->flags & DEBUG_C) {
		// Visual debug info
		for(u_int16_t i = 0; i < tilemap->action_count; i++) {
			Action current = tilemap->actions[i];
			Vector2 draw_pos = CoordsToScreen(tilemap, (Coords){current.c, current.r});
			
			DrawText(TextFormat("%d", current.id), draw_pos.x, draw_pos.y, 20, WHITE);
			DrawText(TextFormat("%c", current.prev[0]), draw_pos.x, draw_pos.y + 20, 14, RED);
			DrawText(TextFormat("%c", current.next[0]), draw_pos.x, draw_pos.y + 40, 14, GREEN);
		}
	}
}


void TilemapClose(Tilemap *tilemap) {
	free(tilemap->mapData);
	free(tilemap->actions);
}

void ColorTile(Tilemap *tilemap, Coords coords, Color color) {
	// Paint a grid box with a color
	DrawRectangleV(CoordsToScreen(tilemap, coords), (Vector2){scaled_tile_size, scaled_tile_size}, color);
}

void SetGridColor(Color color) {
	grid_color = color;	
}

Coords ScreenToGrid(Tilemap *tilemap, Vector2 position) {
	return (Coords) {
		position.x / scaled_tile_size,
		position.y / scaled_tile_size 
	};
}

Vector2 CoordsToScreen(Tilemap *tilemap, Coords coords) {
	return (Vector2) {
		coords.c * scaled_tile_size,
		coords.r * scaled_tile_size 
	};
}

u_int16_t TileIndex(Tilemap *tilemap, u_int16_t c, u_int16_t r) {
	return c + r * tilemap->width;
}

Coords TileCoords(Tilemap *tilemap, u_int16_t index) {
	return (Coords) {
		index % tilemap->width,
		index / tilemap->width
	};
}

bool CmpCoords(Coords a, Coords b) {
	if(a.c - b.c == 0 && a.r - b.r == 0) return true;
	else return false;
}

Coords ClampCoords(Tilemap *tilemap, Coords coords) {
	if(coords.c < 1) coords.c = 0;
	else if(coords.c > tilemap->width) coords.c = tilemap->width;
	if(coords.r < 1) coords.r = 0;
	else if(coords.r > tilemap->height) coords.r = tilemap->height;
	return coords;
}

bool InBounds(Tilemap *tilemap, Coords coords) {
	if( coords.c >= 0 &&
	    coords.r >= 0 && 
	    coords.c <= tilemap->width &&
	    coords.r <= tilemap->height) {
		return true;
	} else return false;
}

bool InBoundsIndex(Tilemap *tilemap, u_int16_t index) {
	u_int16_t c = index % tilemap->width;
	u_int16_t r = index / tilemap->width;
	if(InBounds(tilemap, (Coords){c, r})) return true;
	else return false;
}

char FetchTile(Tilemap *tilemap, Coords coords) {
	char tile_ch = '0';
	if(InBounds(tilemap, coords))
		tile_ch = tilemap->mapData[TileIndex(tilemap, coords.c, coords.r)];
	
	return tile_ch;
}

uint8_t TileGetAdj(Tilemap *tilemap, Coords pos) {
	u_int8_t adj = 0;

	if(FetchTile(tilemap, (Coords){pos.c, pos.r - 1}) == '1' || pos.r - 1 < 0) 				 		adj |= TOP; 
	if(FetchTile(tilemap, (Coords){pos.c, pos.r + 1}) == '1' || pos.r + 1 > tilemap->height - 1)    adj |= BOT; 
	if(FetchTile(tilemap, (Coords){pos.c - 1, pos.r}) == '1' || pos.c - 1 < 0) 				 		adj |= LFT; 
	if(FetchTile(tilemap, (Coords){pos.c + 1, pos.r}) == '1' || pos.c + 1 > tilemap->width - 1)     adj |= RGT;

	// TODO:
	// Add, corner checks... 
	/*
	if(FetchTile(tilemap, (Coords){pos.c - 1, pos.r - 1}) == '1') adj |= T_L;
	if(FetchTile(tilemap, (Coords){pos.c + 1, pos.r - 1}) == '1') adj |= T_R;
	if(FetchTile(tilemap, (Coords){pos.c - 1, pos.r + 1}) == '1') adj |= B_L;
	if(FetchTile(tilemap, (Coords){pos.c + 1, pos.r + 1}) == '1') adj |= B_R;
	*/

	return adj;
}

void GetDrawTile(Tilemap *tilemap, char tile_ch, Coords coords, u_int16_t tile_index) {
	switch(tile_ch) {
		case TILE_BLOCK:
			DrawSprite(tilemap->spritesheet, CoordsToScreen(tilemap, coords),
				tilemap->spritesheet->frame_rec[tilemap->spr_index[tile_index]], 1.0f);
			break;

		case TILE_PLAYER:
			ColorTile(tilemap, coords, GREEN);
			break;
		
		case TILE_ENEMY0:
			ColorTile(tilemap, coords, RED);
			break;

		case TILE_ENEMY1:
			ColorTile(tilemap, coords, ORANGE);
			break;

		case TILE_DOOR:
			ColorTile(tilemap, coords, YELLOW);
			break;
	}
}

void TilemapLoad(Tilemap *tilemap, char *path, u_int8_t flags) {
	FILE *pF = fopen(path, "r");

	if(pF != NULL) {
		// Set dimensions
		int map_w, map_h;
		fscanf(pF, "%d", &map_w);		
		fscanf(pF, "%d", &map_h);	
		
		tilemap->width  = map_w;
		tilemap->height = map_h;
		tilemap->area   = map_w * map_h;
		
		// Resize arrays
		tilemap->mapData = realloc(tilemap->mapData, sizeof(char) * tilemap->area);
		tilemap->spr_index = realloc(tilemap->spr_index, sizeof(u_int8_t) * tilemap->area);
		
		// Set data
		for(u_int16_t r = 0; r < tilemap->height; r++) {
			char line[tilemap->width];			
			fscanf(pF, "%s", line);

			for(u_int16_t c = 0; c < tilemap->width; c++) 
				tilemap->mapData[TileIndex(tilemap, c, r)] = line[c];
		}
		
		// Print debug info
		if(flags & DEBUG_A) {
			printf("Opened %s\n", path);
			printf("w: %d h: %d\n", tilemap->width, tilemap->height);
			
			for(u_int16_t i = 0; i < tilemap->area; i++) {
				printf("%c", tilemap->mapData[i]);
				if(i % tilemap->width == tilemap->width - 1) printf("\n");
			}

			fseek(pF, 0, SEEK_END);
			size_t file_size = ftell(pF);
			fseek(pF, 0, SEEK_SET);
			printf("size: %zu bytes\n", file_size);
		}
		
		fclose(pF);

		tilemap->bounds = (Rectangle){0, 0, tilemap->width * TILE_SIZE, tilemap->height * TILE_SIZE};
		TilemapUpdateSprites(tilemap);
		
		tilemap->flags |= TM_SAVED;
		
	} else if(flags & DEBUG_A) printf("Unable to read file %s\n", path);
}

void TilemapWrite(Tilemap *tilemap, char *path, u_int8_t flags) {
	if(FileExists(path)) remove(path);
	FILE *pF = fopen(path, "w");

	char w_str[4];
	char h_str[4];
	sprintf(w_str, "%d", tilemap->width);
	sprintf(h_str, "%d", tilemap->height);
	fprintf(pF, "%s\n", w_str);
	fprintf(pF, "%s\n", h_str);

	for(u_int16_t i = 0; i < tilemap->area; i++) {
		int c = i % tilemap->width;
		fprintf(pF, "%c", tilemap->mapData[i]);
		if(c == tilemap->width - 1) fprintf(pF, "\n");
	}

	fclose(pF);

	if(tilemap->flags & DEBUG_B) {
		printf("file saved!\n");
		printf("%s\n", path);
	}

	tilemap->flags |= TM_SAVED;
}

void TilemapResize(Tilemap *tilemap, u_int16_t w, u_int16_t h, bool is_new) {
	u_int16_t new_w = w;
	u_int16_t new_h = h;
	u_int16_t new_a = w * h;
	
	if(is_new) {
		tilemap->width = new_w;
		tilemap->height = new_h;
		tilemap->area = new_a;

		tilemap->mapData = realloc(tilemap->mapData, sizeof(char) * tilemap->area);
		for(u_int16_t i = 0; i < tilemap->area; i++) tilemap->mapData[i] = '0';
		tilemap->action_count = 0;
		tilemap->action_index = 0;

		tilemap->spr_index = realloc(tilemap->spr_index, sizeof(u_int8_t) * tilemap->area);
		tilemap->bounds = (Rectangle){0, 0, tilemap->width * TILE_SIZE, tilemap->height * TILE_SIZE};
	} else {
		u_int16_t buf_w = tilemap->width;
		u_int16_t buf_h = tilemap->height;

		char *buf = malloc(sizeof(char) * tilemap->area);
		
		for(u_int16_t i = 0; i < tilemap->area; i++) {
			u_int16_t c = i % tilemap->width;
			u_int16_t r = i / tilemap->width;

			buf[i] = FetchTile(tilemap, (Coords){c, r});
		}

		tilemap->mapData = realloc(tilemap->mapData, sizeof(char) * new_a);
		tilemap->spr_index = realloc(tilemap->spr_index, sizeof(u_int8_t) * new_a);
		tilemap->bounds = (Rectangle){0, 0, new_w * TILE_SIZE, new_h * TILE_SIZE};
		
		tilemap->width = new_w;
		tilemap->height = new_h;
		tilemap->area = new_a;

		for(u_int16_t r = 0; r < tilemap->height; r++) {
			for(u_int16_t c = 0; c < tilemap->width; c++) {
				u_int16_t old_idx = c + r * buf_w;
				u_int16_t new_idx = c + r * tilemap->width;

				if(c < buf_w && r < buf_h) tilemap->mapData[new_idx] = buf[old_idx];
				else tilemap->mapData[new_idx] = '0';
			}
		}

		free(buf);
	}

	TilemapUpdateSprites(tilemap);
}

Action MakeAction(Coords origin, Coords dimensions) {
	return (Action) {
		.c = origin.c,
		.r = origin.r,
		.w = dimensions.c,
		.h = dimensions.r,
		.prev = (char*)malloc(sizeof(char) * (dimensions.c * dimensions.r)),
		.next = (char*)malloc(sizeof(char) * (dimensions.c * dimensions.r))
	};
}

void ApplyAction(Tilemap *tilemap, Action *action) {
	action->id = tilemap->action_index;

	tilemap->action_count++;
	tilemap->action_index++;

	// Grow array size if needed
	if(tilemap->action_count == tilemap->action_max_count) {
		tilemap->action_max_count *= 2;
		
		Action *new_actions_buffer = (Action*)malloc(sizeof(Action) * tilemap->action_max_count);
		for(u_int32_t i = 0; i < tilemap->action_max_count / 2; i++) new_actions_buffer[i] = tilemap->actions[i];
		
		free(tilemap->actions);
		tilemap->actions = new_actions_buffer;

		if(tilemap->flags & DEBUG_C) {
			printf("resizing actions array..\n");
			printf("%i -> %i", tilemap->action_max_count / 2, tilemap->action_max_count);
		}
	}

	// Set data
	for(u_int16_t i = 0; i < (action->w * action->h); i++) {
		u_int16_t c = i % action->w;
		u_int16_t r = i / action->w;

		u_int16_t tile_index = TileIndex(tilemap, c + action->c, r + action->r);
		if(InBounds(tilemap, (Coords){c, r})) tilemap->mapData[tile_index] = action->next[i];
	}
	
	if(tilemap->action_index < tilemap->action_count) {
		for(int i = tilemap->action_index; i < tilemap->action_count; i++) {
			free(tilemap->actions[i].prev);
			free(tilemap->actions[i].next);
		}
			
		tilemap->action_count = tilemap->action_index;
	}
	
	tilemap->actions[tilemap->action_count] = *action;
	
	if(tilemap->flags & DEBUG_B) {
		printf("applied action...\n");
		printf("coords: %d, %d\n", action->c, action->r);
		printf("prev: %c\n", action->prev[0]);
		printf("next: %c\n", action->next[0]);
	}

	TilemapUpdateSprites(tilemap);
	tilemap->flags &= ~TM_SAVED;
}

void UndoAction(Tilemap *tilemap, Action *action) {
	for(u_int16_t i = 0; i < (action->w * action->h); i++) {
		u_int16_t c = i % action->w;
		u_int16_t r = i / action->w;

		action->next[i] = tilemap->mapData[TileIndex(tilemap, action->c + c, action->r + r)];
		tilemap->mapData[TileIndex(tilemap, action->c + c, action->r + r)] = action->prev[i];
	}

	if(tilemap->flags & DEBUG_B) {
		printf("action undo <-...\n");
		printf("coords: %d, %d\n", action->c, action->r);
		printf("prev: %c\n", action->prev[0]);
		printf("next: %c\n", action->next[0]);
	}
	
	TilemapUpdateSprites(tilemap);
	tilemap->action_index--;
	tilemap->flags &= ~TM_SAVED;
}

void RedoAction(Tilemap *tilemap, Action *action) {
	for(u_int16_t i = 0; i < (action->w * action->h); i++) {
		u_int16_t c = i % action->w;
		u_int16_t r = i / action->w;
	
		action->prev[i] = tilemap->mapData[TileIndex(tilemap, action->c + c, action->r + r)];
		tilemap->mapData[TileIndex(tilemap, action->c + c, action->r + r)] = action->next[i];
	}
	
	if(tilemap->flags & DEBUG_B) {
		printf("action redo ->...\n");
		printf("coords: %d, %d\n", action->c, action->r);
		printf("prev: %c\n", action->prev[0]);
		printf("next: %c\n", action->next[0]);
	}

	TilemapUpdateSprites(tilemap);
	tilemap->action_index++;
	tilemap->flags &= ~TM_SAVED;
}

