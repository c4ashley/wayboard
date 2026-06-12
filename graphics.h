#define _GRAPHICS_H_
#ifndef _COMMON_H_
#include "common.h"
#endif

struct Graphics
{
	uint32_t Foreground;
	uint32_t Background;
	void*    Buffer;
	struct wl_buffer * WlBuffer;
	size_t   Width, Height, Stride;
	FT_Face  Font;
};

void CreateGraphicsFromBuffer(struct Buffer* buffer, struct AppContext* context, struct Graphics* result);
void RefreshInvalidatedRegions(struct AppContext* context);
void Graphics_Clear(struct Graphics* graphics);
void Graphics_Fill(struct Graphics* graphics, size_t x, size_t y, size_t width, size_t height);
void Graphics_SetFont(struct Graphics* graphics, FT_Face font);
void DrawGlyph(struct Graphics* graphics, const FT_Bitmap* glyph, int startX, int startY);
void DrawGlyphOver(struct Graphics* graphics, const FT_Bitmap* glyph, int startX, int startY);
void DrawChar(struct Graphics* graphics, char c, FP266* x, FP266* y, [[maybe_unused]] FT_UInt* previousGlyphForKerning);
void DrawKey(struct AppContext* context, struct Graphics* graphics, KeyIndex key, uint8_t state);
