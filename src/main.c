#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#include "raylib.h"
#include "include/resource_dir.h"
#include "spritesheet.h"

#define RAYGUI_IMPLEMENTATION
#include "include/raygui.h"
#undef RAYGUI_IMPLEMENTATION

#define GUI_WINDOW_FILE_DIALOG_IMPLEMENTATION
#include "gui_window_file_dialog.h"

#define GUI_PROPERTY_LIST_IMPLEMENTATION
#include "dm_property_list.h"

#define SIZEOF(A) (sizeof(A)/sizeof(A[0]))

#include "include/styles/style_cyber.h"             // raygui style: cyber
#include "include/styles/style_jungle.h"            // raygui style: jungle
#include "include/styles/style_lavanda.h"           // raygui style: lavanda
#include "include/styles/style_dark.h"              // raygui style: dark
#include "include/styles/style_bluish.h"            // raygui style: bluish
#include "include/styles/style_terminal.h"          // raygui style: terminal
#include "include/styles/style_cherry.h" 			// raygui style: cherry

#include "tilemap.h"
#include "cursor.h"

enum EDITOR_STATE : u_int8_t {
	HOME,
	MAIN
};
u_int8_t _editor_state = HOME;

#define FLAG_A 0x1 
#define FLAG_B 0x2
#define FLAG_C 0x4

#define WINDOW_FLAG_A 0x01
#define WINDOW_FLAG_B 0x02
#define WINDOW_FLAG_C 0x04

u_int8_t flags = (FLAG_A);
u_int8_t window_flags = (WINDOW_FLAG_B); 

enum NewFileInput {
	COLS,
	ROWS,
	NAME
};
u_int8_t new_input = COLS;

void DrawHelpText(int x, int y);

int main () {
	SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI | FLAG_FULLSCREEN_MODE);
	
	//printf("Monitors: %d\n", GetMonitorCount());
	int monitor = GetCurrentMonitor();
	int ww = 1920;
	int wh = 1080;
	int fps = 60;
	
	if(window_flags & WINDOW_FLAG_A) {
		ww = 1920;
		wh = 1080;
		fps = 100;
	} else if(window_flags & WINDOW_FLAG_B) {
		ww = 1920;
		wh = 1200;
		fps = 60;
	} else if(window_flags & WINDOW_FLAG_C) {
		SetTargetFPS(monitor);
		ww = GetMonitorWidth(monitor);
		wh = GetMonitorHeight(monitor);
	}

	InitWindow(ww, wh, "Milky Mapper");
	SetTargetFPS(fps);	
	HideCursor();
	SetExitKey(KEY_F4);

	SearchAndSetResourceDir("resources");
	GuiLoadStyleLavanda();
	//GuiLoadStyleJungle();

	bool exit_window = false;
	bool exit_requested = false;

	Spritesheet tile_sheet = MakeSpritesheet(64, 64, LoadTexture("tileset00.png"));

	Camera2D cam = {
		.target = {-ww * 0.25f, -wh * 0.25f},
		//.offset = {1920 * 0.5f, 1200 * 0.5f},
		.offset = {0, 0},
		.rotation = 0.0f,
		.zoom = 1.0f
	};
	
	// Init
	Tilemap tilemap;
	TilemapInit(&tilemap, &cam, &tile_sheet, 160, 16);
	Tilemap *pTilemap = &tilemap;

	Cursor cursor = MakeCursor(&tilemap, &cam);
	Cursor *pCursor = &cursor;

	bool NEW_FILE = false;
	int new_w;
	int new_h;

	bool RESIZE = false;
	
	GuiWindowFileDialogState fileDialogState = InitGuiWindowFileDialog(GetWorkingDirectory());
    char fileNameToLoad[512] = { 0 };
	char fileNameToSave[512] = { 0 };
	fileDialogState.windowBounds = (Rectangle){ww * 0.25f, wh * 0.25f, 1000, 700};

	// Button recs
	Rectangle TOOL_PENCIL_REC = { ww - 60, 100, 40, 40 };
	Rectangle TOOL_SELECT_REC = { ww - 60, 150, 40, 40 };
	Rectangle TOOL_ERASER_REC = { ww - 60, 200, 40, 40 };
	Rectangle FILE_OPTION_REC = { 0, 0, 100, 30 };
	Rectangle EDIT_OPTION_REC = { 100, 0, 100, 30 };

	Rectangle button_recs[3] = {
		TOOL_PENCIL_REC,
		TOOL_SELECT_REC,
		TOOL_ERASER_REC
	};
	
	// Dropdown menus
	int TAB_FILE_ACTIVE = 0;
	bool FILE_TAB_EDIT = false;

	int TAB_EDIT_ACTIVE = 0;
	bool EDIT_TAB_EDIT = false;
	
	int dm_open_active = 0;
	bool dm_open_edit = false;	

	int dm_save_active = 0;
	bool dm_save_edit = false;

	Rectangle fnew_opt = (Rectangle){FILE_OPTION_REC.x, 
		FILE_OPTION_REC.y + (FILE_OPTION_REC.height * 1),
		FILE_OPTION_REC.width, FILE_OPTION_REC.height};
	
	Rectangle open_opt = (Rectangle){FILE_OPTION_REC.x, 
		FILE_OPTION_REC.y + (FILE_OPTION_REC.height * 2),
		FILE_OPTION_REC.width, FILE_OPTION_REC.height};
	
	Rectangle save_opt = (Rectangle){FILE_OPTION_REC.x, 
		FILE_OPTION_REC.y + (FILE_OPTION_REC.height * 3),
		FILE_OPTION_REC.width, FILE_OPTION_REC.height};

	Rectangle exit_opt = (Rectangle){FILE_OPTION_REC.x, 
		FILE_OPTION_REC.y + (FILE_OPTION_REC.height * 4),
		FILE_OPTION_REC.width, FILE_OPTION_REC.height};
	
	Rectangle resize_opt = (Rectangle){EDIT_OPTION_REC.x, 
		EDIT_OPTION_REC.y + (EDIT_OPTION_REC.height * 1),
		EDIT_OPTION_REC.width, EDIT_OPTION_REC.height};

	Rectangle undo_opt = (Rectangle){EDIT_OPTION_REC.x, 
		EDIT_OPTION_REC.y + (EDIT_OPTION_REC.height * 2),
		EDIT_OPTION_REC.width, EDIT_OPTION_REC.height};

	Rectangle redo_opt = (Rectangle){EDIT_OPTION_REC.x, 
		EDIT_OPTION_REC.y + (EDIT_OPTION_REC.height * 3),
		EDIT_OPTION_REC.width, EDIT_OPTION_REC.height};

	int toggle_group_active = 0;

	bool SAVE_PROMPT = false;

	GuiDMProperty new_file_props[] = {
		PINT("WIDTH", 2048, 16),
		PINT("HEIGHT", 2048, 16),
		PBOOL("CONFIRM?", true, 0)
	};

	int new_focus = 0;
	int new_scroll = 0;
	new_file_props[2].value.vbool = true;

	SetGridColor(GetColor(GuiGetStyle(DEFAULT, TEXT_COLOR_DISABLED)));

	while(!WindowShouldClose()) {
		cursor.on_ui = false;
		if(fileDialogState.itemFocused && fileDialogState.windowActive) cursor.on_ui = true;
       
		if(fileDialogState.SelectFilePressed) {
            // Load level file(if supported extension)
            if(IsFileExtension(fileDialogState.fileNameText, ".mlf")) {
                strcpy(fileNameToLoad, TextFormat("%s" PATH_SEPERATOR "%s", fileDialogState.dirPathText, fileDialogState.fileNameText));
				strcpy(fileNameToSave, fileNameToLoad);
				
				TilemapLoad(&tilemap, fileNameToLoad, flags);
				_editor_state = MAIN;
			}

            fileDialogState.SelectFilePressed = false;
        }

		if(_editor_state == MAIN) {
			// Check if hovering tool buttons
			for(int i = 0; i < 4; i++) {
				if(CheckCollisionPointRec(GetMousePosition(), button_recs[i])) {
					cursor.on_ui = true;
					cursor.ui_cooldown = 10;
				}
			}

			if(IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_R)) {
				new_file_props[2].value.vbool = true;
				RESIZE = true;	
			}
		}
		
		// Draw and GUI
		BeginDrawing();
			ClearBackground(BLACK);

			switch(_editor_state) {
				case HOME:
					DrawHelpText(100, 100);
					break;

				case MAIN:
					BeginMode2D(cam);
						DrawTiles(&tilemap);
						CursorDraw(&cursor);
						DrawTileGrid(&tilemap);
					EndMode2D();

					if(GuiButton(TOOL_PENCIL_REC, GuiIconText(ICON_PENCIL_BIG, ""))) {
						pCursor->tool = PENCIL;
						pCursor->tile_ch = TILE_BLOCK;
						pCursor->ui_cooldown = 10;
						pCursor->on_ui = true;
						pCursor->select_count = 0;
					}
					
					if(GuiButton(TOOL_SELECT_REC, GuiIconText(ICON_BOX_DOTS_BIG, ""))) {
						pCursor->tool = SELECT;
						pCursor->tile_ch = TILE_BLOCK;
						pCursor->ui_cooldown = 10;
						pCursor->on_ui = true;
						pCursor->select_count = 0;
					}

					if(GuiButton(TOOL_ERASER_REC, GuiIconText(ICON_RUBBER, ""))) {
						pCursor->tool = ERASER;
						pCursor->tile_ch = TILE_EMPTY;
						pCursor->ui_cooldown = 10;
						pCursor->on_ui = true;
						pCursor->select_count = 0;
					}
					break;
			}
		
		// Current file Label
		GuiSetStyle(DEFAULT, TEXT_SIZE, 18);
		DrawRectangleV((Vector2){0, wh - 40}, (Vector2){ww, 40}, ColorAlpha(BLACK, 0.5f));
		if(pTilemap->flags & TM_SAVED) GuiLabel((Rectangle){4, wh - 40, 500, 40}, fileNameToSave);
		else GuiLabel((Rectangle){4, wh - 40, 500, 40}, TextFormat("%s*", fileNameToSave));
		GuiSetStyle(DEFAULT, TEXT_SIZE, 15);

		if(fileDialogState.windowActive) {
			GuiLock();
			cursor.on_ui = true;	
		}
		 
		if(IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_O)) fileDialogState.windowActive = true;
	
		if(_editor_state == MAIN && !(cursor.state_flags & C_CHAR_MODE)) {	
			if(IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_S)) SAVE_PROMPT = true;
			else if(IsKeyUp(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_W)) {
				if(strcmp(fileNameToSave, "")) {
					TilemapWrite(&tilemap, fileNameToSave, 0);	
				} else SAVE_PROMPT = true;
			}
		}
		
		GuiUnlock();
		GuiWindowFileDialog(&fileDialogState);
		
		if(NEW_FILE) {
            GuiDMPropertyList((Rectangle){(ww - 180)/2, (wh - 280)/2, 180, 280}, new_file_props, SIZEOF(new_file_props), &new_focus, &new_scroll);

			if(new_file_props[2].value.vbool != true) {
				new_w = new_file_props[0].value.vint.val;	
				new_h = new_file_props[1].value.vint.val;	
				
				TilemapResize(&tilemap, new_w, new_h, true);
				NEW_FILE = false;
				_editor_state = MAIN;

				new_file_props[2].value.vbool = 0;

				strcpy(fileNameToSave, "");
			}

			if(IsKeyDown(KEY_ESCAPE)) {
				NEW_FILE = false;	
				new_file_props[2].value.vbool = 0;
			}
		}

		if(RESIZE) {
            GuiDMPropertyList((Rectangle){(ww - 180)/2, (wh - 280)/2, 180, 280}, new_file_props, SIZEOF(new_file_props), &new_focus, &new_scroll);
			
			if(new_file_props[2].value.vbool != true) {
				u_int16_t w = new_file_props[0].value.vint.val;
				u_int16_t h = new_file_props[1].value.vint.val;
				
				TilemapResize(&tilemap, w, h, false);

				new_file_props[2].value.vbool = 0;

				RESIZE = false;
			}
		}

		if(SAVE_PROMPT) {
			cursor.on_ui = true;
			cursor.ui_cooldown = 10;

            int result = GuiTextInputBox((Rectangle){ (float)GetScreenWidth()/2 - 240,
				(float)GetScreenHeight()/2 - 80, 480, 200 },
				 GuiIconText(ICON_FILE_SAVE, "Save file as..."),
				 "Introduce output file name:", "Ok;Cancel", fileNameToSave, 255, NULL);
				
                if(result == 1) {
                    // TODO: Validate textInput value and save
					TilemapWrite(pTilemap, fileNameToSave, flags);
					SAVE_PROMPT = false;
                }

                if((result == 0) || (result == 1) || (result == 2)) {
                    SAVE_PROMPT = false;
                }
		}
		
		// Dropdown menus
		GuiUnlock();
		GuiSetStyle(BUTTON, TEXT_PADDING, 4);
		GuiSetStyle(BUTTON, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
		
		// FILE...
		if(GuiButton(FILE_OPTION_REC, "#01#FILE")) {
			FILE_TAB_EDIT = ! FILE_TAB_EDIT;
		}

		if(FILE_TAB_EDIT) {
			cursor.ui_cooldown = 10;
			cursor.on_ui = true;

			if(GuiButton(open_opt, "#03#OPEN")) {
				fileDialogState.windowActive = true;
				FILE_TAB_EDIT = false;
			}

			if(GuiButton(fnew_opt, "#08#NEW")) {
				NEW_FILE = true;
				new_input = 0;
				new_file_props[2].value.vbool = true;
				FILE_TAB_EDIT = false;
			}

			if(GuiButton(save_opt, "#02#SAVE")) {
				SAVE_PROMPT = true;
				FILE_TAB_EDIT = false;
			}

			if(GuiButton(exit_opt, "#159#EXIT")) {
				break;
			}

			if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
				if(!CheckCollisionPointRec(GetMousePosition(),
					(Rectangle){FILE_OPTION_REC.x, FILE_OPTION_REC.y, FILE_OPTION_REC.width, FILE_OPTION_REC.height * 5})) {
					FILE_TAB_EDIT = false;
				}
			}
		}

		// EDIT...
		if(GuiButton(EDIT_OPTION_REC, "EDIT")) {
			EDIT_TAB_EDIT = ! EDIT_TAB_EDIT;
		}

		if(EDIT_TAB_EDIT) {
			cursor.ui_cooldown = 10;
			cursor.on_ui = true;

			if(GuiButton(resize_opt, "#33#RESIZE")) {
				if(_editor_state == MAIN) {
					new_file_props[2].value.vbool = true;
					RESIZE = true;
				}

				EDIT_TAB_EDIT = false;	
			}

			if(GuiButton(undo_opt, "#56#UNDO")) {
				if(pTilemap->action_index > 0) UndoAction(pTilemap, &pTilemap->actions[pTilemap->action_index]);
				//EDIT_TAB_EDIT = false;	
			}

			if(GuiButton(redo_opt, "#57#REDO")) {
				if(pTilemap->action_index < pTilemap->action_count) RedoAction(pTilemap, &pTilemap->actions[pTilemap->action_index] + 1);
				if(pTilemap->action_index == pTilemap->action_count) EDIT_TAB_EDIT = false;	
			}
			
			if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
				if(!CheckCollisionPointRec(GetMousePosition(),
					(Rectangle){EDIT_OPTION_REC.x, EDIT_OPTION_REC.y, EDIT_OPTION_REC.width, EDIT_OPTION_REC.height * 4})) {
					EDIT_TAB_EDIT = false;
				}
			}
		}

		GuiSetStyle(BUTTON, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
		GuiSetStyle(BUTTON, TEXT_PADDING, 0);

		if(CheckCollisionPointRec(GetMousePosition(), FILE_OPTION_REC) || CheckCollisionPointRec(GetMousePosition(), EDIT_OPTION_REC)) {
			cursor.on_ui = true;
			cursor.ui_cooldown = 10;
		};
		
		// Draw cursor icon
		if(cursor.state_flags & CURSOR_DRAG) {
			GuiDrawIcon(ICON_CURSOR_HAND, GetMouseX(), GetMouseY(), 2, GetColor(GuiGetStyle(DEFAULT, TEXT_COLOR_NORMAL)));
		} else {
			if(!cursor.on_ui) {
				switch(cursor.tool) {
					case PENCIL:
						GuiDrawIcon(ICON_PENCIL_BIG, GetMouseX(), GetMouseY(), 2, GetColor(GuiGetStyle(DEFAULT, TEXT_COLOR_NORMAL)));
						break;

					case SELECT:
						GuiDrawIcon(ICON_CURSOR_CLASSIC, GetMouseX(), GetMouseY(), 2, GetColor(GuiGetStyle(DEFAULT, TEXT_COLOR_NORMAL)));
						break;

					case ERASER:
						GuiDrawIcon(ICON_RUBBER, GetMouseX(), GetMouseY(), 2, GetColor(GuiGetStyle(DEFAULT, TEXT_COLOR_NORMAL)));
						break;
				}
			} else {
				GuiDrawIcon(ICON_CURSOR_CLASSIC, GetMouseX(), GetMouseY(), 2, GetColor(GuiGetStyle(DEFAULT, TEXT_COLOR_NORMAL)));
			}
		}

		EndDrawing();
		
		CursorUpdate(&cursor);
	}

	// Unload all
	TilemapClose(&tilemap);
	CursorClose(&cursor);
	SpritesheetClose(&tile_sheet);
	
	CloseWindow();
	return 0;
}

void DrawHelpText(int x, int y) {
	GuiSetStyle(DEFAULT, TEXT_SIZE, 30);

	DrawRectangleLines(x - 4, y - 4, 1008, 1008, WHITE);
	DrawRectangle(x, y,  1000, 1000, BLACK);

	GuiDrawText("COMMANDS:", (Rectangle){x, y, 500, 32}, TEXT_ALIGN_CENTER, WHITE);
	GuiDrawText("W,	SAVE", (Rectangle){x, y + (32 * 1), 500, 32}, 0, WHITE);
	GuiDrawText("T, CYCLE TOOLS", (Rectangle){x, y + (32 * 2), 500, 32}, 0, WHITE);
	GuiDrawText("C, COPY", (Rectangle){x, y + (32 * 3), 500, 32}, 0, WHITE);
	GuiDrawText("V, PASTE", (Rectangle){x, y + (32 * 4), 500, 32}, 0, WHITE);
	GuiDrawText("F, FILL SELECTION", (Rectangle){x, y + (32 * 5), 500, 32}, 0, WHITE);
	GuiDrawText("D, DELETE SELECTION", (Rectangle){x, y + (32 * 6), 500, 32}, 0, WHITE);
	GuiDrawText("Z, UNDO", (Rectangle){x, y + (32 * 7), 500, 32}, 0, WHITE);
	GuiDrawText("R, REDO", (Rectangle){x, y + (32 * 8), 500, 32}, 0, WHITE);
	GuiDrawText("~, CHAR MODE ON/OFF", (Rectangle){x, y + (32 * 9), 500, 32}, 0, WHITE);

	GuiDrawText("CTRL + N, NEW", (Rectangle){x + 500, y + (32 * 1), 500, 32}, 0, WHITE);
	GuiDrawText("CTRL + O, OPEN", (Rectangle){x + 500, y + (32 * 2), 500, 32}, 0, WHITE);
	GuiDrawText("CTRL + S, SAVE AS", (Rectangle){x + 500, y + (32 * 3), 500, 32}, 0, WHITE);
	GuiDrawText("CTRL + R, RESIZE", (Rectangle){x + 500, y + (32 * 4), 500, 32}, 0, WHITE);
	GuiDrawText("F4, QUIT", (Rectangle){x + 500, y + (32 * 5), 500, 32}, 0, WHITE);
	
	GuiSetStyle(DEFAULT, TEXT_SIZE, 15);
}

