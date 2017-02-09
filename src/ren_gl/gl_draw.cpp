extern "C"
{
#include "quakedef.h"

byte		*draw_chars;				// 8*8 graphic characters
qpic_t		*draw_disc;
qpic_t		*draw_backtile;
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