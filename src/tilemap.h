#ifndef TILEMAP_H_
#define TILEMAP_H_

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include "raylib.h"
#include "spritesheet.h"

#define INIT_MAX_ACTIONS 128

// State and log flags
#define DEBUG_A  0x01 
#define DEBUG_B  0x02
#define DEBUG_C  0x04
#define TM_SAVED 0x08
#define TM_CHARV 0x10
#define TM_NDEF0 0x20
#define TM_NDEF1 0x40
#define TM_NDEF2 0x80

// Adjacency flags
#define TOP	0x01 
#define	LFT	0x02
#define	RGT	0x04
#define	BOT	0x08

// TODO...
// Define corner flags

enum TILE_CHARS : char {
	TILE_EMPTY	 = '0',
	TILE_BLOCK 	 = '1',
	TILE_PLAYER  = '2',
	TILE_ENEMY0  = '3',
	TILE_ENEMY1  = '4',
	TILE_DOOR	 = '5'
};

typedef struct {
	u_int16_t c, r;
} Coords;

typedef struct {
	u_int8_t id;
	u_int16_t c, r;
	u_int16_t w, h;
	char *prev;
	char *next;
} Action;

typedef struct {
	u_int8_t flags;
	u_int8_t action_count, action_index;
	u_int16_t width, height, area;
	u_int32_t action_max_count;
	Vector2 tileSize;
	Rectangle bounds;
	u_int8_t *spr_index;
	Spritesheet *spritesheet;
	Camera2D *cam;
	char *mapData;
	Action *actions;
} Tilemap;

void TilemapInit(Tilemap *tilemap, Camera2D *cam, Spritesheet *ss, u_int16_t width, u_int16_t height);
void TilemapUpdateSprites(Tilemap *tilemap);
void DrawTiles(Tilemap *tilemap);
void DrawTileGrid(Tilemap *tilemap);
void TilemapClose(Tilemap *tilemap);

void TilemapLoad(Tilemap *tilemap, char *path, u_int8_t flags);
void TilemapWrite(Tilemap *tilemap, char *path, u_int8_t flags);
void TilemapResize(Tilemap *tilemap, u_int16_t w, u_int16_t h, bool is_new);

Action MakeAction(Coords origin, Coords dimensions);
void ApplyAction(Tilemap *tilemap, Action *action);
void UndoAction(Tilemap *tilemap, Action *action);
void RedoAction(Tilemap *tilemap, Action *action);

Coords ScreenToGrid(Tilemap *tilemap, Vector2 position);
Vector2 CoordsToScreen(Tilemap *tilemap, Coords coords);
u_int16_t TileIndex(Tilemap *tilemap, u_int16_t c, uint16_t r);
Coords TileCoords(Tilemap *tilemap, u_int16_t index);
Coords ClampCoords(Tilemap *tilemap, Coords coords);
bool CmpCoords(Coords a, Coords b);
bool InBounds(Tilemap *tilemap, Coords coords);
bool InBoundsIndex(Tilemap *tilemap, u_int16_t index); 
char FetchTile(Tilemap *tilemap, Coords coords);
u_int8_t TileGetAdj(Tilemap *tilemap, Coords coords);

void ColorTile(Tilemap *tilemap, Coords coords, Color color);
void SetGridColor(Color color);
void DrawTileGroup(Tilemap *tilemap, char *tiles, Coords coords, Coords size);
void GetDrawTile(Tilemap *tilemap, char tile_ch, Coords coords, u_int16_t tile_index);

#endif // !TILEMAP_H_

