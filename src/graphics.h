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

typedef enum {
    COLOR_BLACK,
    COLOR_WHITE,
    COLOR_RED,
    COLOR_GREEN,
    COLOR_BLUE,
    COLOR_YELLOW
} graphicsColor_t;

int initGraphics();

// Draw text at (x,y) with a color
void graphicsDrawText(float x, float y, graphicsColor_t color, const char *format, ...);
// Draw centered text at y with a color
void graphicsDrawTextCentered(float y, graphicsColor_t color, const char *format, ...);
// Draw text starting from (x,y) and growing to the left with a color
void graphicsDrawTextRightJustified(float x, float y, graphicsColor_t color, const char *format, ...);
// Draw a single character at (x, y) with a color
void graphicsDrawChar(float x, float y, char c, graphicsColor_t color);
// Draw loading bar at (x,y). Progress must be within range [0.0, 1.0].
void graphicsDrawLoadingBar(float x, float y, float progress);
// Draw main menu graphics with activeItem being highlighted
void graphicsDrawMainMenu(int activeItem);
// Draw device menu with activeItem being highlighted
void graphicsDrawDeviceMenu(int activeItem);
// Draw about/credits screen
void graphicsDrawAboutPage();
// Draw selection pointer at (x,y)
void graphicsDrawPointer(float x, float y);
// Draw hamburger icon at (x,y)
void graphicsDrawHamburger(float x, float y);
// Draw blue translucent rectangular box
void graphicsDrawPromptBox(float width, float height);
// Draw solid black rectangular box
void graphicsDrawPromptBoxBlack(float width, float height);
// Draw a quad
void graphicsDrawQuad(float x, float y, float xsize, float ysize, graphicsColor_t color);

// Get total width of characters in string
float graphicsGetWidth(const char *str);
// Get total width of first n characters in string
float graphicsGetWidthSubString(const char *str, int n);
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
void graphicsRender();


#endif
