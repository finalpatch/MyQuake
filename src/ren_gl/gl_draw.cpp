#include <unordered_map>
#include "gl_helpers.h"

extern "C"
{
#include "quakedef.h"

byte		*draw_chars;				// 8*8 graphic characters
qpic_t		*draw_disc;
qpic_t		*draw_backtile;
}

extern uint32_t vid_current_palette[256];

std::unordered_map<std::string, std::unique_ptr<Texture>> qpicTextureCache;

void R_cachePicture(const char* name, const qpic_t* data)
{
    std::vector<uint32_t> rgbtex(data->width * data->height);
    for (unsigned i = 0; i < rgbtex.size(); ++i)
        rgbtex[i] = vid_current_palette[data->data[i]];
    auto texture = std::make_unique<Texture>(GL_TEXTURE_2D, data->width, data->height, Texture::RGBA,
        GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST, rgbtex.data());
    qpicTextureCache.emplace(name, std::move(texture));
    Sys_Printf("cache picture %s\n", name);
}

void Draw_Init (void)
{
    static char conchars[] = "conchars";
    static char disc[] = "disc";
    static char backtile[] = "backtile";
    draw_chars = reinterpret_cast<byte*>(W_GetLumpName (conchars));
    draw_disc = reinterpret_cast<qpic_t*>(W_GetLumpName (disc));
    draw_backtile = reinterpret_cast<qpic_t*>(W_GetLumpName (backtile));
}
void Draw_Character (int x, int y, int num)
{

}
void Draw_DebugChar (char num)
{

}
void Draw_Pic (int x, int y, qpic_t *pic)
{

}
void Draw_TransPic (int x, int y, qpic_t *pic)
{

}
void Draw_TransPicTranslate (int x, int y, qpic_t *pic, byte *translation)
{

}
void Draw_ConsoleBackground (int lines)
{

}
void Draw_BeginDisc (void)
{

}
void Draw_EndDisc (void)
{

}
void Draw_TileClear (int x, int y, int w, int h)
{

}
void Draw_Fill (int x, int y, int w, int h, int c)
{

}
void Draw_FadeScreen (void)
{

}
void Draw_String (int x, int y, char *str)
{

}
