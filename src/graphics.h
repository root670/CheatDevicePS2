/*
 * Graphics Manager
 * A simple abstraction to gsKit with helper functions.
 */

#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <tamtypes.h>
#include "gsKit.h"
#include "dmaKit.h"
#include "gsToolkit.h"

typedef enum { BLACK, WHITE, RED, GREEN, BLUE, YELLOW } graphicsColor_t;

int initGraphics();

// Draw text at (x,y) with a color
void graphicsDrawText(int x, int y, const char *txt, graphicsColor_t color);
// Draw centered text at y with a color
void graphicsDrawTextCentered(int y, const char *txt, graphicsColor_t color);
// Draw a single character at (x, y) with a color
void graphicsDrawChar(int x, int y, char c, graphicsColor_t color);
// Draw transparent box to go behind text for use in a popup menu (not used)
void graphicsDrawMiniMenuBox(int xsize, int ysize);
// Draw loading bar at (x,y). Progress must be within range [0.0, 1.0].
void graphicsDrawLoadingBar(int x, int y, float progress);
// Draw main menu graphics with activeItem being highlighted
void graphicsDrawMainMenu(int activeItem);
// Draw device menu with activeItem being highlighted
void graphicsDrawDeviceMenu(int activeItem);
// Draw about/credits screen
void graphicsDrawAboutPage();
// Draw selection pointer at (x,y)
void graphicsDrawPointer(int x, int y);
// Draw blue translucent rectangular box
void graphicsDrawPromptBox(int width, int height);
// Draw solid black rectangular box
void graphicsDrawPromptBoxBlack(int width, int height);
// Draw a quad
void graphicsDrawQuad(float x, float y, float xsize, float ysize, graphicsColor_t color);

// Get total width of characters in string
int graphicsGetWidth(const char *str);
// Get total width of first n characters in string
int graphicsGetWidthSubString(const char *str, int n);
// Get number of bytes representing the symbol starting at &str[index]
// An ASCII character is 1 byte, a special sequence is >1 bytes, etc.
// Ex: ("ABC{SQUARE}", 0) == 1 byte, ("ABC{SQUARE}", 3) == 8
int graphicsGetSymbolLength(const char *start, int index);
// Get number of bytes representing the symbol ending at &str[index]
// Ex: ("ABC{SQUARE}", 10) == 8
int graphicsGetSymbolLengthBackwards(const char *start, int index);

// Get display width in pixels
int graphicsGetDisplayWidth();
// Get display height in pixels
int graphicsGetDisplayHeight();

// Cover the entire screen with black
void graphicsClearScreen(int r, int g, int b);
// Draw the background graphic
void graphicsDrawBackground();
// Draw the last x rows of the background graphic
void graphicsDrawBackgroundBottom(unsigned int rows);

// Wait for the next vsync, then render.
void graphicsRenderNow();
// Render graphics only every 1/60 second (60 FPS)
void graphicsRender();


#endif
