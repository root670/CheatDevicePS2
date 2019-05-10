#include <stdio.h>
#include <string.h>

#ifdef __PS2__
    #include <time.h>
    #include <graph.h>
    #include <kernel.h>
    #include "gsKit.h"
    #include "dmaKit.h"
    #include "gsToolkit.h"
    #include "libraries/upng.h"
    #include "stb_font.h"
#elif __PS1__
    #include <psx.h>
#endif

#include "types.h"
#include "graphics.h"
#include "version.h"
#include "cheats.h"
#include "util.h"
#include "settings.h"
#include "menus.h"

#ifdef __PS2__
    typedef GSTEXTURE texture_t;
    typedef u64 color_t;

    #define COLOR_RGB(r, g, b) GS_SETREG_RGBAQ(r, g, b, 0x80, 0x80)
    #define ALPHA_ENABLE  gsKit_set_primalpha(gsGlobal, GS_SETREG_ALPHA(0,1,0,1,0), 0)
    #define ALPHA_DISABLE gsKit_set_primalpha(gsGlobal, GS_BLEND_BACK2FRONT, 0)

    static GSGLOBAL *gsGlobal;
    static int callbackId;
    static int VBlankStartSema;

    static stb_fontchar fontdata[STB_SOMEFONT_NUM_CHARS];
    extern u8  _background_png_start[];
    extern int _background_png_size;
    extern u8  _check_png_start[];
    extern int _check_png_size;
    extern u8  _hamburgerIcon_png_start[];
    extern int _hamburgerIcon_png_size;
    extern u8  _gamepad_png_start[];
    extern int _gamepad_png_size;
    extern u8  _cube_png_start[];
    extern int _cube_png_size;
    extern u8  _savemanager_png_start[];
    extern int _savemanager_png_size;
    extern u8  _flashdrive_png_start[];
    extern int _flashdrive_png_size;
    extern u8  _memorycard1_png_start[];
    extern int _memorycard1_png_size;
    extern u8  _memorycard2_png_start[];
    extern int _memorycard2_png_size;
    extern u8  _buttonCross_png_start[];
    extern int _buttonCross_png_size;
    extern u8  _buttonCircle_png_start[];
    extern int _buttonCircle_png_size;
    extern u8  _buttonTriangle_png_start[];
    extern int _buttonTriangle_png_size;
    extern u8  _buttonSquare_png_start[];
    extern int _buttonSquare_png_size;
    extern u8  _buttonStart_png_start[];
    extern int _buttonStart_png_size;
    extern u8  _buttonSelect_png_start[];
    extern int _buttonSelect_png_size;
    extern u8  _buttonL1_png_start[];
    extern int _buttonL1_png_size;
    extern u8  _buttonL2_png_start[];
    extern int _buttonL2_png_size;
    extern u8  _buttonL3_png_start[];
    extern int _buttonL3_png_size;
    extern u8  _buttonR1_png_start[];
    extern int _buttonR1_png_size;
    extern u8  _buttonR2_png_start[];
    extern int _buttonR2_png_size;
    extern u8  _buttonR3_png_start[];
    extern int _buttonR3_png_size;

    static texture_t bg;
    static texture_t check;
    static texture_t hamburgerIcon;
    static texture_t font;
    static texture_t gamepad;
    static texture_t cube;
    static texture_t savemanager;
    static texture_t flashdrive;
    static texture_t memorycard1;
    static texture_t memorycard2;
    static texture_t buttonCross;
    static texture_t buttonCircle;
    static texture_t buttonTriangle;
    static texture_t buttonSquare;
    static texture_t buttonStart;
    static texture_t buttonSelect;
    static texture_t buttonL1;
    static texture_t buttonL2;
    static texture_t buttonL3;
    static texture_t buttonR1;
    static texture_t buttonR2;
    static texture_t buttonR3;

    static int vsync_callback()
    {
        // Unblock graphicsRender()
        ee_sema_t sema;
        iReferSemaStatus(VBlankStartSema, &sema);
        if(sema.count<sema.max_count)
            iSignalSema(VBlankStartSema);

        ExitHandler();
        return 0;
    }

    static void loadPNG(texture_t *tex, u8 *data, int len, int linear_filtering)
    {
        upng_t* pngTexture = upng_new_from_bytes(data, len);
        upng_header(pngTexture);
        upng_decode(pngTexture);

        tex->VramClut = 0;
        tex->Clut = NULL;
        tex->Width = upng_get_width(pngTexture);
        tex->Height = upng_get_height(pngTexture);
        u8 *imageBuffer = (u8 *)upng_get_buffer(pngTexture);
        
        if(upng_get_format(pngTexture) == UPNG_RGB8)
        {
            tex->PSM = GS_PSM_CT24;
        }
        else if(upng_get_format(pngTexture) == UPNG_RGBA8)
        {
            tex->PSM = GS_PSM_CT32;

            // Convert alpha value range to [0, 80]
            int i;
            for(i = 0; i < tex->Width * tex->Height; i++)
            {
                u8 alpha = imageBuffer[i * 4 + 3];

                if(alpha == 0xFF)
                {
                    alpha = 0x80;
                }
                else
                {
                    alpha = alpha >> 1;
                }

                imageBuffer[i * 4 + 3] = alpha;
            }
        }

        tex->Mem = memalign(128, gsKit_texture_size_ee(tex->Width, tex->Height, tex->PSM));
        memcpy(tex->Mem, imageBuffer, gsKit_texture_size_ee(tex->Width, tex->Height, tex->PSM));
        tex->Vram = gsKit_vram_alloc(gsGlobal, gsKit_texture_size(tex->Width, tex->Height, tex->PSM), GSKIT_ALLOC_USERBUFFER);
        tex->Filter = (linear_filtering) ? GS_FILTER_LINEAR : GS_FILTER_NEAREST;
        gsKit_texture_upload(gsGlobal, tex);
        
        upng_free(pngTexture);
    }

#elif __PS1__
    typedef GsImage texture_t;
    typedef uint32_t color_t;

    #define COLOR_RGB(r, g, b) (b&0xff) | ((g&0xff) << 8) | ((r&0xff) << 16)
    #define GET_RGB_R(color)   ((color&0xFF0000) >> 16)
    #define GET_RGB_G(color)   ((color&0xFF00) >> 8)
    #define GET_RGB_B(color)   (color&0xFF)

    #define ALPHA_ENABLE 
    #define ALPHA_DISABLE

    extern int fb_font_x;
    extern int fb_font_y;
    extern int fb_font_cx;
    extern int fb_font_cy;

    static unsigned int primList[0x4000];

    static volatile int vsync = 1;
    static void vblank_handler()
    {
        vsync = 1;
    }
#endif

typedef struct menuIcon {
    char *label;
    texture_t *tex;
} menuIcon_t;

static int initialized = 0;

static color_t graphicsColorTable[] = {
    COLOR_RGB(0x00,0x00,0x00), // COLOR_BLACK
    COLOR_RGB(0xF0,0xF0,0xF0), // COLOR_WHITE
    COLOR_RGB(0xF0,0x00,0x00), // COLOR_RED
    COLOR_RGB(0x00,0xF0,0x00), // COLOR_GREEN
    COLOR_RGB(0x20,0x20,0xA0), // COLOR_BLUE
    COLOR_RGB(0xF0,0xB0,0x00)  // COLOR_YELLOW
};

static void initFont()
{
#ifdef __PS2__
    font.Width = STB_SOMEFONT_BITMAP_WIDTH;
    font.Height = STB_SOMEFONT_BITMAP_HEIGHT;
    font.PSM = GS_PSM_T8;
    font.ClutPSM = GS_PSM_CT32;
    font.Mem = memalign(128, gsKit_texture_size_ee(font.Width, font.Height, font.PSM));
    font.Clut = memalign(128, gsKit_texture_size_ee(16, 16, font.ClutPSM));
    font.VramClut = gsKit_vram_alloc(gsGlobal, gsKit_texture_size(16, 16, font.ClutPSM), GSKIT_ALLOC_USERBUFFER);
    font.Vram = gsKit_vram_alloc(gsGlobal, gsKit_texture_size(font.Width, font.Height, font.PSM), GSKIT_ALLOC_USERBUFFER);
    font.Filter = GS_FILTER_NEAREST;

    /* Generate palette */
    unsigned int i;
    for(i = 0; i < 256; ++i)
    {
        u8 alpha = (i * 128) / 255;
        font.Clut[i] = GS_SETREG_RGBA(0xFF, 0xFF, 0xFF, alpha);
    }

    STB_SOMEFONT_CREATE(fontdata, (unsigned char(*)[])font.Mem, STB_SOMEFONT_BITMAP_HEIGHT);
    gsKit_texture_upload(gsGlobal, &font);

#elif __PS1__
    printf("Loading font\n");
    GsLoadFont(768, 0, 768, 256);
#endif
}

static void loadTextures()
{
#ifdef __PS2__
    loadPNG(&check, _check_png_start, _check_png_size, 0);
    loadPNG(&gamepad, _gamepad_png_start, _gamepad_png_size, 1);
    loadPNG(&cube, _cube_png_start, _cube_png_size, 1);
    loadPNG(&hamburgerIcon, _hamburgerIcon_png_start, _hamburgerIcon_png_size, 0);
    loadPNG(&savemanager, _savemanager_png_start, _savemanager_png_size, 1);
    loadPNG(&flashdrive, _flashdrive_png_start, _flashdrive_png_size, 1);
    loadPNG(&memorycard1, _memorycard1_png_start, _memorycard1_png_size, 1);
    loadPNG(&memorycard2, _memorycard2_png_start, _memorycard2_png_size, 1);
    loadPNG(&buttonCross, _buttonCross_png_start, _buttonCross_png_size, 0);
    loadPNG(&buttonCircle, _buttonCircle_png_start, _buttonCircle_png_size, 0);
    loadPNG(&buttonTriangle, _buttonTriangle_png_start, _buttonTriangle_png_size, 0);
    loadPNG(&buttonSquare, _buttonSquare_png_start, _buttonSquare_png_size, 0);
    loadPNG(&buttonStart, _buttonStart_png_start, _buttonStart_png_size, 0);
    loadPNG(&buttonSelect, _buttonSelect_png_start, _buttonSelect_png_size, 0);
    loadPNG(&buttonL1, _buttonL1_png_start, _buttonL1_png_size, 0);
    loadPNG(&buttonL2, _buttonL2_png_start, _buttonL2_png_size, 0);
    loadPNG(&buttonL3, _buttonL3_png_start, _buttonL3_png_size, 0);
    loadPNG(&buttonR1, _buttonR1_png_start, _buttonR1_png_size, 0);
    loadPNG(&buttonR2, _buttonR2_png_start, _buttonR2_png_size, 0);
    loadPNG(&buttonR3, _buttonR3_png_start, _buttonR3_png_size, 0);
#endif // __PS2__
}

static void loadBackgroundTexture()
{
#ifdef __PS2__
    loadPNG(&bg, _background_png_start, _background_png_size, 0);
#endif // __PS2__
}

static void platformInit()
{
#ifdef __PS2__
    dmaKit_init(D_CTRL_RELE_OFF, D_CTRL_MFD_OFF, D_CTRL_STS_UNSPEC,
                D_CTRL_STD_OFF, D_CTRL_RCYC_8, 1 << DMA_CHANNEL_GIF);

    // Initialize the DMAC
    dmaKit_chan_init(DMA_CHANNEL_GIF);

    // Initialize the GS
    gsGlobal = gsKit_init_global();
    gsGlobal->PrimAAEnable = GS_SETTING_ON;
    gsGlobal->PrimAlphaEnable = GS_SETTING_ON;
    gsGlobal->DoubleBuffering = GS_SETTING_ON; 
    gsGlobal->ZBuffering = GS_SETTING_OFF;
    
    ee_sema_t sema;
    sema.init_count=0;
    sema.max_count=1;
    sema.attr=sema.option=0;
    VBlankStartSema=CreateSema(&sema);

    callbackId = gsKit_add_vsync_handler(&vsync_callback);
    gsKit_init_screen( gsGlobal );
    gsKit_mode_switch( gsGlobal, GS_ONESHOT );

    // Clear the screen right away to prevent the old framebuffer from being dumped to screen
    gsKit_set_test(gsGlobal, GS_ZTEST_OFF);
    gsKit_set_primalpha(gsGlobal, GS_SETREG_ALPHA(0, 1, 0, 1, 0), 0);
    gsKit_clear(gsGlobal, GS_SETREG_RGBAQ(0x00,0x00,0x00,0x00,0x00));
    gsKit_sync_flip( gsGlobal );

#elif __PS1__
    GsInit();
    GsSetList(primList);
    GsClearMem();
    GsSetVideoMode(graphicsGetDisplayWidth(), graphicsGetDisplayHeight(), VMODE_NTSC);
    GsLoadFont(768, 0, 768, 256);

    SetVBlankHandler(vblank_handler);
    GsSetDrawEnvSimple(0, 0, graphicsGetDisplayWidth(), graphicsGetDisplayHeight());
    GsSetDispEnvSimple(0, 0);
    GsSetAutoWait();

#endif
}

int initGraphics()
{
    if(initialized)
        return 0;

    printf("\n ** Initializing Graphics **\n");

    platformInit();
    initFont();
    loadBackgroundTexture();

    graphicsDrawBackground();
    graphicsDrawText(450, 400, COLOR_WHITE, "Please wait...");
    graphicsRender();

    loadTextures();

    return 1;
}

/*
Get special texture from text sequence.
str should be a char string starting with '{' and containing a matching
'}' somewhere in it. The first curly-brace enclosed substring (ex: {CIRCLE})
will be considered. If a valid sequence is found, the pointer to the position
after the matching '}' is returned and texture points to the associated texture
object. Otherwise NULL is returned.
*/
static char* getSpecialTexture(const char *str, texture_t** texture)
{
#ifdef __PS2__
    char const *cptr = str;
    char special[17];
    int i = 0;
    texture_t *specialTexture = NULL;
    special[0] = '\0';

    if(!cptr)
        return NULL;

    if(cptr[0] != '{')
        return NULL;
    else
        cptr++;

    while(*cptr && *cptr != '}' && i < 16)
    {
        special[i] = *cptr;
        special[i + 1] = '\0';
        cptr++;
        i++;
    }

    if(*cptr == '}')
    {
        // Skip over ending character
        cptr++;
    }
    else
    {
        // Missing ending character
        return NULL;
    }

    if(strncmp(special, "CROSS", 16) == 0)
    {
        specialTexture = &buttonCross;
    }
    else if(strncmp(special, "CIRCLE", 16) == 0)
    {
        specialTexture = &buttonCircle;
    }
    else if(strncmp(special, "TRIANGLE", 16) == 0)
    {
        specialTexture = &buttonTriangle;
    }
    else if(strncmp(special, "SQUARE", 16) == 0)
    {
        specialTexture = &buttonSquare;
    }
    else if(strncmp(special, "START", 16) == 0)
    {
        specialTexture = &buttonStart;
    }
    else if(strncmp(special, "SELECT", 16) == 0)
    {
        specialTexture = &buttonSelect;
    }
    else if(strncmp(special, "L1", 16) == 0)
    {
        specialTexture = &buttonL1;
    }
    else if(strncmp(special, "L2", 16) == 0)
    {
        specialTexture = &buttonL2;
    }
    else if(strncmp(special, "L3", 16) == 0)
    {
        specialTexture = &buttonL3;
    }
    else if(strncmp(special, "R1", 16) == 0)
    {
        specialTexture = &buttonR1;
    }
    else if(strncmp(special, "R2", 16) == 0)
    {
        specialTexture = &buttonR2;
    }
    else if(strncmp(special, "R3", 16) == 0)
    {
        specialTexture = &buttonR3;
    }
    else
    {
        // Unrecognized identifier.
        return NULL;
    }

    if(texture)
        *texture = specialTexture;
    return cptr;

#elif __PS1__
    return NULL;
#endif
}

int graphicsGetSymbolLength(const char *start, int index)
{
    if(!start)
        return 1;

    // Check if the character at index is enclosed by curley braces,
    // in which case the character might be part of a special symbol.
    
    // Start by looking ahead for a closing brace
    int endIndex = strlen(start);
    int closingIndex = index;
    while(start[closingIndex] && start[closingIndex] != '}')
        closingIndex++;
    
    // Now step backwards to find an opening brace
    int openingIndex = closingIndex;
    while(openingIndex > 0 && start[openingIndex] != '{')
        openingIndex--;

    if(openingIndex > index ||
       openingIndex == closingIndex ||
       closingIndex == endIndex)
       return 1; // Character is not surrounded by curley braces

    // Might be in a special symbol. Check if it's valid.
    char *end = getSpecialTexture(&start[index], NULL);

    if(end)
        return end - &start[index];
    else
        return 1;
}

int graphicsGetSymbolLengthBackwards(const char *start, int index)
{
    if(!start || start[index] != '}')
        return 1;

    while(index > 0 && start[index] != '{')
        index--;

    char *end = getSpecialTexture(&start[index], NULL);

    if(end)
        return end - &start[index];
    else
        return 1;
}

static void graphicsPrintText(float x, float y, const char *txt, color_t color)
{
    char const *cptr = txt;
    char special[17];
    #ifdef __PS2__
    float cx = x;
    float cy = y;
    #elif __PS1__
    int cx = x;
    int cy = y;
    #endif

    memset(special, '\0', 17);

    ALPHA_ENABLE;

    while(*cptr)
    {
        if(*cptr == '\n')
        {
            cy += 22;
            cx = x;
            cptr++;
            continue;
        }
        else if(*cptr == '{')
        {
            // Read special sequence. A special sequence is surrounded by curly
            // braces and will be replaced by a texture when drawn.
            // For example, {CROSS} will be drawn as a cross button symbol

            char *ret;
            texture_t *specialTexture;
            ret = getSpecialTexture(cptr, &specialTexture);

            if(ret != NULL)
            {
                color_t graphicColor;
                if(color == graphicsColorTable[COLOR_YELLOW])
                {
                    graphicColor = color;
                }
                else
                {
                    graphicColor = COLOR_RGB(0x80, 0x80, 0x80);
                }
#ifdef __PS2__
                gsKit_prim_sprite_texture(gsGlobal, specialTexture,
                                                    cx,
                                                    cy + 4,
                                                    0,
                                                    0,
                                                    cx + specialTexture->Width,
                                                    cy + 4 + specialTexture->Height,
                                                    specialTexture->Width,
                                                    specialTexture->Height,
                                                    1,
                                                    graphicColor);
                cx += specialTexture->Width;
#elif __PS1__
                if(graphicColor) {}
#endif
                cptr = ret;
            }
            else
            {
                goto regularPrint;
            }
        }
        else
        {
regularPrint: ;
            int char_codepoint = *cptr++;
            #ifdef __PS2__
            stb_fontchar *cdata = &fontdata[char_codepoint - STB_SOMEFONT_FIRST_CHAR];
            #endif
            
            if(cx < graphicsGetDisplayWidth() + 16 &&
               cx > -16 &&
               char_codepoint != ' ')
            {
                #ifdef __PS2__
                gsKit_prim_sprite_texture(gsGlobal, &font, cx + cdata->x0f, cy + cdata->y0f, STB_SOMEFONT_BITMAP_WIDTH*cdata->s0f, cdata->t0f*STB_SOMEFONT_BITMAP_HEIGHT,
                                                       cx + cdata->x1f, cy + cdata->y1f, STB_SOMEFONT_BITMAP_WIDTH*cdata->s1f, cdata->t1f*STB_SOMEFONT_BITMAP_HEIGHT,
                                                       1, color);
                #elif __PS1__
                GsSprite spr;

                spr.x = cx;
                spr.y = cy;
                spr.r = GET_RGB_R(color);
                spr.g = GET_RGB_G(color);
                spr.b = GET_RGB_B(color);
                spr.attribute = 0;
                spr.cx = fb_font_cx;
                spr.cy = fb_font_cy;
                spr.tpage = (fb_font_x / 64) + ((fb_font_y / 256) * 16);
                spr.w = spr.h = 8;
                spr.scalex = spr.scaley = 0;
                spr.u = ((fb_font_x & 0x3f)*4)+((char_codepoint & 7) << 3);
			    spr.v = (fb_font_y & 0xff)+(char_codepoint & 0xf8);

                GsSortSimpleSprite(&spr);
                #endif

            }
            #ifdef __PS2__
            cx += cdata->advance;
            #elif __PS1__
            cx += 8;
            #endif
        }
    }

    ALPHA_DISABLE;
}

void graphicsDrawChar(float x, float y, char c, graphicsColor_t color)
{
    color_t colorValue = graphicsColorTable[color];
#ifdef __PS2__
    stb_fontchar *cdata = &fontdata[c - STB_SOMEFONT_FIRST_CHAR];

    ALPHA_ENABLE;
    gsKit_prim_sprite_texture(gsGlobal, &font, x + cdata->x0f, y + cdata->y0f, STB_SOMEFONT_BITMAP_WIDTH*cdata->s0f, cdata->t0f*STB_SOMEFONT_BITMAP_HEIGHT,
                                               x + cdata->x1f, y + cdata->y1f, STB_SOMEFONT_BITMAP_WIDTH*cdata->s1f, cdata->t1f*STB_SOMEFONT_BITMAP_HEIGHT,
                                               1, colorValue);
    ALPHA_DISABLE;

#elif __PS1__
    GsSprite spr;

    spr.x = x;
    spr.y = y;
    spr.r = GET_RGB_R(colorValue);
    spr.g = GET_RGB_G(colorValue);
    spr.b = GET_RGB_B(colorValue);
    spr.attribute = 0;
    spr.cx = fb_font_cx;
    spr.cy = fb_font_cy;
    spr.tpage = (fb_font_x / 64) + ((fb_font_y / 256) * 16);
    spr.w = spr.h = 8;
    spr.scalex = spr.scaley = 0;
    spr.u = ((fb_font_x & 0x3f)*4)+((c & 0x07) << 3);
    spr.v =  (fb_font_y & 0xff   )+ (c & 0xf8);

    GsSortSimpleSprite(&spr);
#endif
}

void graphicsDrawText(float x, float y, graphicsColor_t color, const char *format, ...)
{
    va_list args;
    char buffer[4096];

    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    graphicsPrintText(x, y, buffer, graphicsColorTable[color]);
    va_end(args);
}

void graphicsDrawTextCentered(float y, graphicsColor_t color, const char *format, ...)
{
    va_list args;
    char buffer[4096];
    
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    char *cptr = buffer;
    char *start = buffer;
    #ifdef __PS2__
    float lineWidth = 0;
    #elif __PS1__
    int lineWidth = 0;
    #endif
    
    while(*cptr)
    {
        if(*cptr == '\n')
        {
            *cptr = '\0';
            graphicsPrintText((graphicsGetDisplayWidth() - lineWidth)/2.0, y, start, graphicsColorTable[color]);
            *cptr = '\n';
            cptr++;
            lineWidth = 0;
            start = cptr;
            y += 22;
            continue;
        }
        else if(*cptr == '{')
        {
            // Read special sequence
            char *ret;
            texture_t *specialTexture;
            ret = getSpecialTexture(cptr, &specialTexture);

            if(ret != NULL)
            {
                #ifdef __PS2__
                lineWidth += specialTexture->Width;
                #endif
                cptr = ret;
                continue;
            }
        }

        int char_codepoint = *cptr++;
        #ifdef __PS2__
        stb_fontchar *cdata = &fontdata[char_codepoint - STB_SOMEFONT_FIRST_CHAR];
        lineWidth += cdata->advance;
        #elif __PS1__
        if(char_codepoint) {}
        lineWidth += 8;
        #endif
    }
    
    graphicsPrintText((graphicsGetDisplayWidth() - lineWidth)/2.0, y, start, graphicsColorTable[color]); // last line
}

void graphicsDrawTextRightJustified(float x, float y, graphicsColor_t color, const char *format, ...)
{
    va_list args;
    char buffer[4096];
    
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    char *cptr = buffer;
    char *start = buffer;
    #ifdef __PS2__
    float lineWidth = 0;
    #elif __PS1__
    int lineWidth = 0;
    #endif
    
    while(*cptr)
    {
        if(*cptr == '\n')
        {
            *cptr = '\0';
            graphicsPrintText(x - lineWidth, y, start, graphicsColorTable[color]);
            *cptr = '\n';
            cptr++;
            lineWidth = 0;
            start = cptr;
            y += 22;
            continue;
        }
        else if(*cptr == '{')
        {
            // Read special sequence
            char *ret;
            texture_t *specialTexture;
            ret = getSpecialTexture(cptr, &specialTexture);

            if(ret != NULL)
            {
                #ifdef __PS2__
                lineWidth += specialTexture->Width;
                #endif
                cptr = ret;
                continue;
            }
        }

        #ifdef __PS2__
        int char_codepoint = *cptr++;
        stb_fontchar *cdata = &fontdata[char_codepoint - STB_SOMEFONT_FIRST_CHAR];
        lineWidth += cdata->advance;
        #elif __PS1__
        lineWidth += 8;
        #endif
    }
    
    graphicsPrintText(x - lineWidth, y, start, graphicsColorTable[color]); // last line
}

void graphicsDrawLoadingBar(float x, float y, float progress)
{
    #ifdef __PS2__
    float height = 10.0f;
    float width = gsGlobal->Width - 2.0f*x;
    color_t color = graphicsColorTable[COLOR_BLUE];
    color_t outline = graphicsColorTable[COLOR_BLACK];

    if(progress < 0.0f)
        progress = 0.0f;
    if(progress > 1.0f)
        progress = 1.0f;

    ALPHA_ENABLE;
    // outline
    gsKit_prim_sprite(gsGlobal, x - 5.0f, y - 5.0f, x + 5.0f + width, y + 5.0f + height, 1, outline);
    // progress bar
    gsKit_prim_sprite(gsGlobal, x, y, x + (progress * width), y + height, 1, color);
    ALPHA_DISABLE;
    #endif
}

void graphicsDrawQuad(float x, float y, float xsize, float ysize, graphicsColor_t color)
{
#ifdef __PS2__
    ALPHA_ENABLE;
    gsKit_prim_sprite(gsGlobal, x, y, x + xsize, y + ysize, 1, graphicsColorTable[color]);
    ALPHA_DISABLE;

#elif __PS1__
    
    GsRectangle rect;
    rect.x = x;
    rect.y = y;
    rect.w = xsize;
    rect.h = ysize;

    uint32_t rawColor = graphicsColorTable[color];
    rect.r = GET_RGB_R(rawColor);
    rect.g = GET_RGB_G(rawColor);
    rect.b = GET_RGB_B(rawColor);

    rect.attribute = 0;

    GsSortRectangle(&rect);
#endif
}

static void drawPromptBox(float width, float height, color_t color)
{
    const float x0 = (graphicsGetDisplayWidth()/2.0) - (width/2.0);
    const float y0 = (graphicsGetDisplayHeight()/2.0) - (height/2.0);

#ifdef __PS2__
    const float x1 = (graphicsGetDisplayWidth()/2.0) + (width/2.0);
    const float y1 = (graphicsGetDisplayHeight()/2.0) + (height/2.0);
    ALPHA_ENABLE;
    gsKit_prim_sprite(gsGlobal, x0, y0, x1, y1, 1, color);
    ALPHA_DISABLE;

#elif __PS1__
    GsRectangle rect;
    rect.x = x0;
    rect.y = y0;
    rect.w = width;
    rect.h = height;

    rect.r = GET_RGB_R(color);
    rect.g = GET_RGB_G(color);
    rect.b = GET_RGB_B(color);
    rect.attribute = 0;

    GsSortRectangle(&rect);
#endif
}

void graphicsDrawPromptBox(float width, float height)
{
    #ifdef __PS2__
    drawPromptBox(width, height, GS_SETREG_RGBAQ(0x22, 0x22, 0xEE, 0x60, 0x80));
    #elif __PS1__
    drawPromptBox(width, height, COLOR_RGB(0x22, 0x22, 0xEE));
    #endif
}

void graphicsDrawPromptBoxBlack(float width, float height)
{
    drawPromptBox(width, height, graphicsColorTable[COLOR_BLACK]);
}

#ifdef __PS2__
static void drawMenu(const menuIcon_t icons[], int numIcons, int activeItem)
{
    int i;
    const color_t unselected = GS_SETREG_RGBAQ(0x50, 0x50, 0x50, 0x20, 0x80);
    const color_t selected = GS_SETREG_RGBAQ(0x50, 0x50, 0x50, 0x80, 0x80);
    
    graphicsDrawPromptBox(350, 150);
    
    for(i = 0; i < numIcons; i++)
    {
        float x = (gsGlobal->Width / 2.0f) - ((75.0f * numIcons) / 2.0f) + (75.0f * i);
        ALPHA_ENABLE;
        gsKit_prim_sprite_texture(gsGlobal, icons[i].tex,
                                            x,
                                            192,
                                            0,
                                            0,
                                            x + (icons[i].tex)->Width,
                                            192 + (icons[i].tex)->Height,
                                            (icons[i].tex)->Width,
                                            (icons[i].tex)->Height,
                                            1,
                                            (activeItem == i) ? selected : unselected);
        if (activeItem == i) graphicsDrawTextCentered(265, COLOR_WHITE, icons[i].label);
        ALPHA_DISABLE;
    }
}
#endif

void graphicsDrawMainMenu(int activeItem)
{
    #ifdef __PS2__
    static const menuIcon_t icons[] = {
        {"Start Game", &gamepad},
        {"Game List", &cube},
        {"Save Manager", &savemanager}
    };
    
    drawMenu(icons, 3, activeItem);
    #endif
}

void graphicsDrawDeviceMenu(int activeItem)
{
    #ifdef __PS2__
    static const menuIcon_t icons[] = {
        {"Memory Card (Slot 1)", &memorycard1},
        {"Memory Card (Slot 2)", &memorycard2},
        {"Flash Drive", &flashdrive}
    };
    
    drawMenu(icons, 3, activeItem);
    #endif
}

void graphicsClearScreen(int r, int g, int b)
{
#ifdef __PS2__
    gsKit_clear(gsGlobal, GS_SETREG_RGBAQ(r, g, b, 0x00, 0x00));
#elif __PS1__
    GsSortCls(r, g, b);
#endif
}

void graphicsDrawBackground()
{
#ifdef __PS2__
    ALPHA_ENABLE;
    gsKit_prim_sprite_texture(gsGlobal, &bg,
                                        0,         // X1
                                        0,         // Y1
                                        0,         // U1
                                        0,         // V1
                                        bg.Width,  // X2
                                        bg.Height, // Y2
                                        bg.Width,  // U2
                                        bg.Height, // V2
                                        1,         // Z
                                        0x80808080 // RGBAQ
                                        );
    ALPHA_DISABLE;

#elif __PS1__
    GsGPoly4 poly;
    poly.x[0] = poly.x[1] = 0;
    poly.x[2] = poly.x[3] = graphicsGetDisplayWidth();
    poly.y[0] = poly.y[2] = 0;
    poly.y[1] = poly.y[3] = graphicsGetDisplayHeight();
    
    // Top colors
    poly.r[0] = poly.r[2] = 128;
    poly.g[0] = poly.g[2] = 0;
    poly.b[0] = poly.b[2] = 0;
    
    // Bottom colors
    poly.r[1] = poly.r[3] = 0;
    poly.g[1] = poly.g[3] = 0;
    poly.b[1] = poly.b[3] = 128;
    poly.attribute = 0;
    
    GsSortGPoly4(&poly);
#endif
}

void graphicsDrawBackgroundBottom(unsigned int rows)
{
#ifdef __PS2__
    ALPHA_ENABLE;
    gsKit_prim_sprite_texture(gsGlobal, &bg,
                                        0,                // X1
                                        bg.Height - rows, // Y1
                                        0,                // U1
                                        bg.Height - rows, // V1
                                        bg.Width,         // X2
                                        bg.Height,        // Y2
                                        bg.Width,         // U2
                                        bg.Height,        // V2
                                        1,                // Z
                                        0x80808080        // RGBAQ
                                        );
    ALPHA_DISABLE;

#elif __PS1__
    GsGPoly4 poly;
    poly.x[0] = poly.x[1] = 0;
    poly.x[2] = poly.x[3] = graphicsGetDisplayWidth();
    poly.y[0] = poly.y[2] = graphicsGetDisplayHeight() - rows;
    poly.y[1] = poly.y[3] = graphicsGetDisplayHeight();

    // Top colors
    poly.r[0] = poly.r[2] = 128;
    poly.g[0] = poly.g[2] = 128;
    poly.b[0] = poly.b[2] = 255;

    // Bottom colors
    poly.r[1] = poly.r[3] = 255;
    poly.g[1] = poly.g[3] = 255;
    poly.b[1] = poly.b[3] = 255;
    poly.attribute = 0;

    GsSortGPoly4(&poly);
#endif
}

void graphicsDrawPointer(float x, float y)
{
#ifdef __PS2__
    ALPHA_ENABLE;
    gsKit_prim_sprite_texture(gsGlobal, &check,
                                        x,
                                        y,
                                        0,
                                        0,
                                        x + check.Width,
                                        y + check.Height,
                                        check.Width,
                                        check.Height,
                                        1,
                                        0x80808080);
    ALPHA_DISABLE;

#elif __PS1__
    GsPoly3 poly;

    poly.x[0] = x;
    poly.x[1] = x + 10;
    poly.x[2] = x;
    poly.y[0] = y;
    poly.y[1] = y + 5;
    poly.y[2] = y + 10;

    poly.r = 64;
    poly.g = 64;
    poly.b = 64;

    GsSortPoly3(&poly);
#endif
}

void graphicsDrawHamburger(float x, float y)
{
#ifdef __PS2__
    ALPHA_ENABLE;
    gsKit_prim_sprite_texture(gsGlobal, &hamburgerIcon,
                                        x,
                                        y,
                                        0,
                                        0,
                                        x + hamburgerIcon.Width,
                                        y + hamburgerIcon.Height,
                                        hamburgerIcon.Width,
                                        hamburgerIcon.Height,
                                        1,
                                        0x80808080);
    ALPHA_DISABLE;
#endif
}

float graphicsGetWidthSubString(const char *str, int n)
{
    if(!str)
        return 0;

    char const *cptr = str;
    float maxWidth = 0;
    float lineWidth = 0;
    
    while(*cptr && n-- > 0)
    {
        if(*cptr == '\n')
        {
            if(lineWidth > maxWidth)
                maxWidth = lineWidth;
            cptr++;
            lineWidth = 0;
        }
        else if(*cptr == '{')
        {
            // Read special sequence
            const char *ret;
            texture_t *specialTexture;
            ret = getSpecialTexture(cptr, &specialTexture);

            if(ret != NULL)
            {
                #ifdef __PS2__
                lineWidth += specialTexture->Width;
                #endif
                n -= (ret - cptr) - 1;
                cptr = ret;
                continue;
            }
        }

        int char_codepoint = *cptr++;
        #ifdef __PS2__
        stb_fontchar *cdata = &fontdata[char_codepoint - STB_SOMEFONT_FIRST_CHAR];
        lineWidth += cdata->advance;
        #elif __PS1__
        if(char_codepoint) {}
        lineWidth += 8;

        #endif
    }

    if(lineWidth > maxWidth)
        maxWidth = lineWidth;

    return maxWidth;
}

float graphicsGetWidth(const char *str)
{
    if(!str)
        return 0;

    int len = strlen(str);
    return graphicsGetWidthSubString(str, len);
}

int graphicsGetDisplayWidth()
{
#ifdef __PS2__
    return gsGlobal->Width;
#elif __PS1__
    return 512;
#endif
}

int graphicsGetDisplayHeight()
{
#ifdef __PS2__
    return gsGlobal->Height;
#elif __PS1__
    return 480;
#endif
}

void graphicsDrawAboutPage()
{
    char msg[256];

    const char *readOnlyPath = settingsGetReadOnlyDatabasePath();
    const char *readWritePath = settingsGetReadWriteDatabasePath();

    snprintf(msg, sizeof(msg),
        "Cheat Device " GIT_VERSION "\n"
        "Compiled " __DATE__ " " __TIME__ "\n"
        "\n"
        "Total Games: %d\n"
        "Read-Only Database: %s\n"
        "Read/Write Database: %s",
        cheatsGetNumGames(),
        readOnlyPath ? readOnlyPath : "(none)",
        readWritePath ? readWritePath : "(none)"
    );

    displayError(msg);
}

void graphicsRender()
{
#ifdef __PS2__
    gsKit_set_finish(gsGlobal);
    gsKit_queue_exec(gsGlobal);
    gsKit_finish();

    if(!gsGlobal->FirstFrame)
    {
        // Wait for vsync...
        PollSema(VBlankStartSema);
	    WaitSema(VBlankStartSema);

        if (gsGlobal->DoubleBuffering == GS_SETTING_ON)
        {
            GS_SET_DISPFB2(gsGlobal->ScreenBuffer[gsGlobal->ActiveBuffer & 1] / 8192,
                            gsGlobal->Width / 64, gsGlobal->PSM, 0, 0);

            gsGlobal->ActiveBuffer ^= 1;
        }
    }

    gsKit_setactive(gsGlobal);

#elif __PS1__
    while(!vsync) { };
    
    GsDrawList();
    //while(GsIsDrawing());
    
    GsSetDrawEnvSimple(0, 0, graphicsGetDisplayWidth(), graphicsGetDisplayHeight());
    GsSetDispEnvSimple(0, 0);
    vsync = 0;
#endif

    // Draw common elements
    graphicsDrawBackground();
    menuRender();
}
