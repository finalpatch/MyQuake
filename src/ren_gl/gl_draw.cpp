#include <unordered_map>
#include "gl_renderpass.h"

extern "C"
{
#include "quakedef.h"

byte		*draw_chars;				// 8*8 graphic characters
qpic_t		*draw_disc;
qpic_t		*draw_backtile;
}

const static int kCharWidth = 8;
const static int kCharHeight = 8;
const static int kCharRows = 16;
const static int kCharCols = 16;

extern uint32_t vid_current_palette[256];

struct QpicTexture
{
    const qpic_t* ptr;
    std::unique_ptr<Texture> tex;
};

struct DrawChar
{
    int x;
    int y;
    int num;
    DrawChar(int _x, int _y, int _num) :
        x(_x), y(_y), num(_num)
    {}
};

std::unique_ptr<VertexArray> pictureVao;
std::unordered_map<std::string, QpicTexture> qpicTextureCache;
std::unordered_map<const qpic_t*, std::string> qpicTextureIndex;
std::unique_ptr<Texture> fontTex;
std::vector<DrawChar> charsToDraw;

const char conbackName[] = "gfx/conback.lmp";

void R_cachePicture(const char* name, const qpic_t* data)
{
    auto i = qpicTextureCache.find(name);
    if (i == qpicTextureCache.end())
    {
        std::vector<uint32_t> rgbtex(data->width * data->height);
        if (strncmp(conbackName, name, sizeof(conbackName)) == 0)
        {
            for (unsigned i = 0; i < rgbtex.size(); ++i)
            {
                auto clridx = data->data[i];
                rgbtex[i] = (vid_current_palette[clridx] & 0xffffff00) | 0xA0;
            }
        }
        else
        {
            for (unsigned i = 0; i < rgbtex.size(); ++i)
            {
                auto clridx = data->data[i];
                rgbtex[i] = (clridx == TRANSPARENT_COLOR) ? 0 : vid_current_palette[clridx];
            }
        }
        auto texture = std::make_unique<Texture>(GL_TEXTURE_2D, data->width, data->height, Texture::RGBA,
            GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST, rgbtex.data());
        auto& elem = qpicTextureCache[name];
        elem.ptr = data;
        elem.tex = std::move(texture);
        qpicTextureIndex[data] = name;
        Sys_Printf("cache picture %s\n", name);
    }
    else
    {
        auto& elem = qpicTextureCache[name];
        if (elem.ptr != data)
        {
            qpicTextureIndex.erase(elem.ptr);
            qpicTextureIndex[data] = name;
            elem.ptr = data;
        }
    }
}

void Draw_Init (void)
{
    static char conchars[] = "conchars";
    static char disc[] = "disc";
    static char backtile[] = "backtile";
    draw_chars = reinterpret_cast<byte*>(W_GetLumpName (conchars));
    draw_disc = reinterpret_cast<qpic_t*>(W_GetLumpName (disc));
    draw_backtile = reinterpret_cast<qpic_t*>(W_GetLumpName (backtile));

    // font texture
    std::vector<uint32_t> rgbtex(kCharWidth * kCharHeight * kCharRows * kCharCols);
    for (unsigned i = 0; i < rgbtex.size(); ++i)
    {
        auto clridx = draw_chars[i];
        rgbtex[i] = (clridx == 0) ? 0 : vid_current_palette[clridx];
    }
    fontTex = std::make_unique<Texture>(GL_TEXTURE_2D, kCharWidth * kCharCols, kCharHeight * kCharRows, Texture::RGBA,
        GL_CLAMP_TO_EDGE, GL_NEAREST, GL_NEAREST, rgbtex.data());

    pictureVao = std::make_unique<VertexArray>();
}
void Draw_Character (int x, int y, int num)
{
    num &= 0xff;

    int ch = num & 0x7f;

    // don't draw invisible chars
    if (ch == ' ' || ch == '\n' || ch == '\t' || ch == '\r' || ch == '\f')
        return;

    charsToDraw.emplace_back(x, y, num);
}
void Draw_DebugChar (char num)
{

}
void Draw_Pic (int x, int y, qpic_t *pic)
{
    auto i = qpicTextureIndex.find(pic);
    if (i == qpicTextureIndex.end())
        return;
    auto j = qpicTextureCache.find(i->second);
    if (j == qpicTextureCache.end())
        return;
    TextureBinding binding(*(j->second.tex), 0);
    PictureRenderPass::getInstance().setup(float(x)/vid.width, float(y)/vid.height,
        float(pic->width)/vid.width, float(pic->height)/vid.height);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}
void Draw_TransPic (int x, int y, qpic_t *pic)
{
    return Draw_Pic(x, y, pic);
}
void Draw_TransPicTranslate (int x, int y, qpic_t *pic, byte *translation)
{
    // FIXME
    return Draw_Pic(x, y, pic);
}
void Draw_ConsoleBackground (int lines)
{
    qpic_t* pic = Draw_CachePic((char*)conbackName);
    auto i = qpicTextureIndex.find(pic);
    if (i == qpicTextureIndex.end())
        return;
    auto j = qpicTextureCache.find(i->second);
    if (j == qpicTextureCache.end())
        return;
    TextureBinding binding(*(j->second.tex), 0);
    PictureRenderPass::getInstance().setup(0.0f, float(lines) / vid.height - 1.0f, 1.0f, 1.0f,
        0.0f, 0.0f, 1.0f, 1.0f);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
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
	while (*str)
	{
		Draw_Character (x, y, *str);
		str++;
		x += 8;
	}
}

void Draw_Commit()
{
    TextureBinding binding(*fontTex, 0);

    for (const auto& dc: charsToDraw)
    {
        int row = dc.num >> 4;
        int col = dc.num & 15;
        PictureRenderPass::getInstance().setup(
            float(dc.x)/vid.width, float(dc.y)/vid.height,
            float(kCharWidth)/vid.width, float(kCharHeight)/vid.height,
            float(col) / kCharCols, float(row) / kCharRows,
            1.0f / kCharCols, 1.0f / kCharRows);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    }
    charsToDraw.clear();
}
