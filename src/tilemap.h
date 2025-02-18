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
	TILE_DOOR	 = '5',
	TILE_FLOWER  = 'f'
};

typedef struct {
	uint16_t c, r;
} Coords;

typedef struct {
	uint8_t id;
	uint16_t c, r;
	uint16_t w, h;
	char *prev;
	char *next;
} Action;

typedef struct {
	uint8_t flags;
	uint8_t action_count, action_index;
	uint16_t width, height, area;
	uint32_t action_max_count;
	Vector2 tileSize;
	Rectangle bounds;
	uint8_t *spr_index;
	Spritesheet *spritesheet;
	Camera2D *cam;
	char *mapData;
	Action *actions;
} Tilemap;

void TilemapInit(Tilemap *tilemap, Camera2D *cam, Spritesheet *ss, uint16_t width, uint16_t height);
void TilemapUpdateSprites(Tilemap *tilemap);
void DrawTiles(Tilemap *tilemap);
void DrawTileGrid(Tilemap *tilemap);
void TilemapClose(Tilemap *tilemap);

void TilemapLoad(Tilemap *tilemap, char *path, uint8_t flags);
void TilemapWrite(Tilemap *tilemap, char *path, uint8_t flags);
void TilemapResize(Tilemap *tilemap, uint16_t w, uint16_t h, bool is_new);

Action MakeAction(Coords origin, Coords dimensions);
void ApplyAction(Tilemap *tilemap, Action *action);
void UndoAction(Tilemap *tilemap, Action *action);
void RedoAction(Tilemap *tilemap, Action *action);

Coords ScreenToGrid(Tilemap *tilemap, Vector2 position);
Vector2 CoordsToScreen(Tilemap *tilemap, Coords coords);
uint16_t TileIndex(Tilemap *tilemap, uint16_t c, uint16_t r);
Coords TileCoords(Tilemap *tilemap, uint16_t index);
Coords ClampCoords(Tilemap *tilemap, Coords coords);
bool CmpCoords(Coords a, Coords b);
bool InBounds(Tilemap *tilemap, Coords coords);
bool InBoundsIndex(Tilemap *tilemap, uint16_t index); 
char FetchTile(Tilemap *tilemap, Coords coords);
uint8_t TileGetAdj(Tilemap *tilemap, Coords coords);

void ColorTile(Tilemap *tilemap, Coords coords, Color color);
void SetGridColor(Color color);
void DrawTileGroup(Tilemap *tilemap, char *tiles, Coords coords, Coords size);
void GetDrawTile(Tilemap *tilemap, char tile_ch, Coords coords, uint16_t tile_index);

#endif // !TILEMAP_H_

