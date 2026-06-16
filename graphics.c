#define _GRAPHICS_C_
#ifndef _MAIN_C_
#include "main.c"
#endif
#include <immintrin.h>

void CreateGraphicsFromBuffer(struct Buffer* buffer, struct AppContext* context, struct Graphics* result)
{
	result->Buffer   = buffer->Data;
	result->WlBuffer = buffer->Buffer;
	result->Width    = buffer->Width;
	result->Height   = buffer->Height;
	result->Stride   = buffer->Stride;
	result->Font     = context->Font_CharKeys;
};

static void UpdateNumericRow(struct AppContext* context)
{
	if (context->HeldModifiers & MOD_FN_MASK)
	{
		if (context->KeyChars[0] != TEXTCACHEINDEX(Esc))
		{
			memcpy(&context->KeyChars[0], &KEYCHARS[KEYCHARS_FKEYOFFSET], 16);
			memcpy(&context->KeyCodes[0], &KEYCODES[KEYCODES_FKEYOFFSET], 16);
		}
	}
	else if (context->HeldModifiers & MOD_SHIFTMASK)
	{
		if (context->KeyChars[0] != '~')
			memcpy(&context->KeyChars[0], &KEYCHARS[KEYCHARS_SHIFTOFFSET], 16);
		if (context->KeyCodes[0] != KEYCODES[0])
			memcpy(&context->KeyCodes[0], &KEYCODES[0], 16);
	}
	else
	{
		if (context->KeyChars[0] != '`')
			memcpy(&context->KeyChars[0], &KEYCHARS[0], 16);
		if (context->KeyCodes[0] != KEYCODES[0])
			memcpy(&context->KeyCodes[0], &KEYCODES[0], 16);
	}
}

static void UpdateLetterRows(struct AppContext* context)
{
	if (context->HeldModifiers & MOD_CAPSLOCK_MASK)
	{
		if (context->HeldModifiers & MOD_SHIFTMASK)
		{
			if (context->KeyChars[17] != 'q' || context->KeyChars[27] != '{')
				memcpy(&context->KeyChars[16], &KEYCHARS[KEYCHARS_SHIFTCAPSOFFSET + 16], 3*16);
		}
		else
		{
			if (context->KeyChars[17] != 'Q' || context->KeyChars[27] != '[')
				memcpy(&context->KeyChars[16], &KEYCHARS[KEYCHARS_CAPSOFFSET + 16], 3*16);
		}
	}
	else
	{
		if (context->HeldModifiers & MOD_SHIFTMASK)
		{
			if (context->KeyChars[17] != 'Q' || context->KeyChars[27] != '[')
				memcpy(&context->KeyChars[16], &KEYCHARS[KEYCHARS_SHIFTOFFSET + 16], 3*16);
		}
		else
		{
			if (context->KeyChars[17] != 'Q' || context->KeyChars[27] != '[')
				memcpy(&context->KeyChars[16], &KEYCHARS[16], 3*16);
		}
	}
}

void RefreshInvalidatedRegions(struct AppContext* context)
{
	struct Buffer* buffer = NULL;
	struct Graphics graphics;
	bool redrawing = false;

		struct timespec clock1, clock2;
		clock_gettime(CLOCK_MONOTONIC, &clock1);

	if (context->DirtyState)
	{
		if (!FindAvailableBuffer(context, false))
			return; // no buffers available, return without affecting the queue

		buffer = FindAvailableBuffer(context, true);
		if (buffer == NULL || buffer->Buffer == NULL)
			return;

		redrawing = true;

		CreateGraphicsFromBuffer(buffer, context, &graphics);
		wl_surface_attach(context->Surface, buffer->Buffer, 0, 0);

		switch (context->DirtyState)
		{
			case DS_LetterKeys:
				UpdateLetterRows(context);
				DrawRow(&graphics, context, 1);
				DrawRow(&graphics, context, 2);
				DrawRow(&graphics, context, 3);
				//DrawRow(&graphics, context, 4);
				wl_surface_damage_buffer(context->Surface, (int)context->StartX, (int)(context->StartY + context->RowHeight), (int)(14.9 * context->StandardKeyWidth), (int)(4 * context->RowHeight));
				break;
			case DS_NumericKeys:
				UpdateNumericRow(context);
				DrawRow(&graphics, context, 0);
				wl_surface_damage_buffer(context->Surface, (int)context->StartX, (int)context->StartY, (int)(14.9 * context->StandardKeyWidth), (int)(1 * context->RowHeight));
				break;
			case DS_WholeMainKeyboard:
				UpdateNumericRow(context);
				UpdateLetterRows(context);
				DrawRow(&graphics, context, 0);
				DrawRow(&graphics, context, 1);
				DrawRow(&graphics, context, 2);
				DrawRow(&graphics, context, 3);
				//DrawRow(&graphics, context, 4);
				wl_surface_damage_buffer(context->Surface, (int)context->StartX, (int)context->StartY, (int)(14.9 * context->StandardKeyWidth), (int)(5 * context->RowHeight));
				break;
			case DS_WholeSurface:
				UpdateNumericRow(context);
				UpdateLetterRows(context);
				// TODO: Refactor DrawKeyboard. DrawKeyboard will attempt to obtain a buffer, which it will be unable to do.
				DrawKeyboard(context);
				wl_surface_damage_buffer(context->Surface, 0, 0, buffer->Width, buffer->Height);
				break;
			default:
				break;
		}
		context->DirtyState = DS_None;
	}


	//printf("refreshing %d invalidated keys\n", Queue_Count(&context->DirtyKeys));
	if (!Queue_IsEmpty(&context->DirtyKeys))
	{

		if (!buffer)
		{
			if (!FindAvailableBuffer(context, false))
				return; // no buffers available, return without affecting the queue

			buffer = FindAvailableBuffer(context, true);
			if (buffer == NULL || buffer->Buffer == NULL)
				return;

			redrawing = true;

			CreateGraphicsFromBuffer(buffer, context, &graphics);
			wl_surface_attach(context->Surface, buffer->Buffer, 0, 0);
		}


		KeyIndex key;
		uint8_t state;
		while (Queue_Pop(&context->DirtyKeys, &key, &state))
		{
			DrawKey(context, &graphics, key, state);
		}

	}

	if (redrawing)
	{
		wl_surface_commit(context->Surface);

		clock_gettime(CLOCK_MONOTONIC, &clock2);

		unsigned long long dUsecs = (clock2.tv_sec - clock1.tv_sec) * 1'000'000 + (clock2.tv_nsec - clock1.tv_nsec) / 1000;
		//printf("Render: %.2fms\n", (double)dUsecs * 0.001);
	}
};

void Graphics_Clear(struct Graphics* graphics)
{
	Graphics_Fill(graphics, 0, 0, graphics->Width, graphics->Height);
}

void Graphics_Fill(struct Graphics* graphics, size_t x, size_t y, size_t width, size_t height)
{
	// 256 bits of 32-bit pixels means we fill 8 pixels per operation.
	const uint32_t val32 = graphics->Background;
	const __m256i  value = _mm256_set1_epi32(val32);
	char* row = ((char*)graphics->Buffer) + (y * graphics->Stride) + (x * 4);
	//printf("Filling %zu pixels wide means %zu AVX loops and %zu pixel loops\n", width, width / 8, width % 8);
	for (size_t y = 0; y < height; ++y, row += graphics->Stride)
	{
		__m256i* dst = (__m256i*) row;
		size_t x;
		for (x = 0; x < width - 7; x += 8, ++dst)
			_mm256_storeu_si256(dst, value);
		for (int i = 0; x < width; ++x)
			((uint32_t*)dst)[i++] = val32;
	}
}

void Graphics_SetFont(struct Graphics* graphics, FT_Face font)
{
	graphics->Font = font;
}

static inline float blend(float a, float b, float amount)
{
	return a + amount * (b - a);
}

#define UNPACK16_64(a, b, c, d) ((uint64_t)(a) | ((uint64_t)(b) << 16) | ((uint64_t)(c) << 32) | ((uint64_t)(d) << 48))
#define UNPACK8_64(a, b, c, d, e, f, g, h) ((uint64_t)(a) | ((uint64_t)(b) << 8) | ((uint64_t)(c) << 16) | ((uint64_t)(d) << 24) | ((uint64_t)(e) << 32) | ((uint64_t)(f) << 40) | ((uint64_t)(g) << 48) | ((uint64_t)(h) << 56))
#define BROADCAST16_64(a) UNPACK16_64(a, a, a, a)
#define SHUFFLE8_16_64(a) UNPACK8_64(a, 0x80, a, 0x80, a, 0x80, a, 0x80)

void DrawGlyph(struct Graphics* graphics, const FT_Bitmap* glyph, int startX, int startY)
{
	const FT_Int dx = glyph->width;
	const FT_Int dy = glyph->rows;
	const FT_Int glyphStride = glyph->pitch;
	uint8_t* pBuffer = (uint8_t*)graphics->Buffer + (startY * graphics->Stride) + (startX * 4);
	const uint8_t* pGlyph = (const uint8_t*) glyph->buffer;
	uint8_t* row = pBuffer;
	const uint8_t* rowGlyph = pGlyph;
	const __m256i ShuffleBrightness8_256 = _mm256_setr_epi8(0,0,0,0,  1,1,1,1,  0,0,0,0,  0,0,0,0,  2,2,2,2,  3,3,3,3,  0,0,0,0,  0,0,0,0);
	const __m256i Const258 = _mm256_set1_epi16(258);
	const __m256i Const516 = _mm256_set1_epi16(516);
	const __m256i Const255_8 = _mm256_set1_epi8(255u);
	const __m256i bg8  = _mm256_set1_epi32(graphics->Background);
	const __m256i bg16 = _mm256_cvtepu8_epi16(_mm256_castsi256_si128(bg8));
	const __m256i inv_a = _mm256_sub_epi8(Const255_8, bg8);
	const __m256i interleaved_1_1a = _mm256_and_si256(_mm256_srli_epi16(_mm256_unpacklo_epi8(Const255_8, inv_a), 1), _mm256_set1_epi8(0x7F));
	//printf("%d pixels wide means %d AVX2 loops + %d SSE loops\n", dx, dx / 4, dx % 4);
	int nSkippedRows = 0;
	if (startY < 0)
	{
		nSkippedRows = -startY;
		row += nSkippedRows * graphics->Stride;
		rowGlyph += nSkippedRows * glyphStride;
	}
	for (FT_Int iy = nSkippedRows; iy < dy; iy++)
	{
		uint8_t* pix = row;
		const uint8_t* pixGlyph = rowGlyph;
		FT_Int ix;
		for (ix = 0; ix < dx - 3; ix += 4, pix += 16, pixGlyph += 4)
		{
			__m256i t = _mm256_set1_epi32(*(uint32_t*)pixGlyph);                               // [latency 3]
			__m256i t8 = _mm256_shuffle_epi8(t, ShuffleBrightness8_256);                       // [latency 1] load and shuffle 32 bits (4 bytes) into 128 bits (16 bytes of [ x[0], x[0], x[0], x[0], x[1], x[1], x[1], x[1], x[2], x[2], x[2], x[2], x[3], x[3], x[3], x[3] ])
			//__m256i t16 = _mm256_cvtepu8_epi16(t8);                                          // [latency 3] 

			// lerp with 1: a + t(b - a) => a + t - ta

			//__m256i sum       = _mm256_add_epi16(bg16, t16);                                 // [latency 1]
			//__m256i ta        = _mm256_mulhi_epi16(_mm256_mullo_epi16(bg16, t16), Const258); // [mulhi: latency 5] [mullo: latency 5]
			//__m256i diff      = _mm256_sub_epi16(sum, ta);A                                  // [latency 1]
			//__m256i newColour = _mm256_packus_epi16(diff, diff);                             // [latency 1] packs into bytes as res[0:7],res[0:7],res[8:15],res[8:15]
			//        newColour = _mm256_permute4x64_epi64(newColour, 0xD8);                   // [latency 3] corrects the packing to res[0:7],res[8:15],res[0:7],res[8:15], though the higher 128 bits are unneeded

			// store the low 128 bits (res[0:15])
			//_mm_storeu_si128((__m128i*)pix, _mm256_castsi256_si128(newColour));              // [latency 5]

			// -------------
			//
			// If I want to make use of 'maddubs', I need to prepare the vectors so the horizontally summed pairs are what I need.
			// vec1 = interleave(a, t)
			// const vec2 = interleave(1, 1-a)
			// a(1) + t(1-a) => a + t - ta

			// 1-a  1      t   a
			// 6Dh 7Fh    00h 24h
			// 0.85 1      0  0.14
			// 0.85*0 + 1*0.14 = 0.14
			// after maddubs should be 0x2400, or 0x1200 with our loss of precision due to the signed `b` operand
			// We get 0x11DC
			// 0x11DC * 516 = 0x23FF70 => 0x23

			__m256i interleaved_a_t = _mm256_unpacklo_epi8(bg8, t8);                              // [latency 1]
			__m256i result = _mm256_maddubs_epi16(interleaved_a_t, interleaved_1_1a);             // [latency 5]
			        result = _mm256_mulhi_epu16(result, Const516);                                // [latency 5]
			        result = _mm256_permute4x64_epi64(_mm256_packus_epi16(result, result), 0xD8); // [permute: latency 3] [pack: latency 1]
			_mm_storeu_si128((__m128i*)pix, _mm256_castsi256_si128(result));                      // [latency 5]
			// result = addubs(vec1, vec2)
			// store(permute(pack(result)))
			//
		}

		for (; ix < dx; ++ix, pix += 4, ++pixGlyph)
		{
			__m128i t    = { *pixGlyph * 0x0001000100010001, 0 };
			__m128i bg   = _mm256_castsi256_si128(bg16);
			__m128i sum  =_mm_add_epi16(bg, t);
			__m128i ta   = _mm_mulhi_epi16(_mm_mullo_epi16(bg, t), _mm256_castsi256_si128(Const258));
			__m128i diff = _mm_sub_epi16(sum, ta);
			__m128i newColour = _mm_packus_epi16(diff, diff);
			_mm_storeu_si32((__m128i*)pix, newColour);
		}

		row += graphics->Stride;
		rowGlyph += glyphStride;
	}
}

// draws a glyph using a transparent background
void DrawGlyphOver(struct Graphics* graphics, const FT_Bitmap* glyph, int startX, int startY)
{
	const FT_Int dx = glyph->width;
	const FT_Int dy = glyph->rows;
	const FT_Int glyphStride = glyph->pitch;
	uint8_t* pBuffer = (uint8_t*)graphics->Buffer + (startY * graphics->Stride) + (startX * 4);
	const uint8_t* pGlyph = (const uint8_t*) glyph->buffer;
	uint8_t* row = pBuffer;
	const uint8_t* rowGlyph = pGlyph;
	const __m256i ShuffleBrightness16 = _mm256_setr_epi16(0x8000,0x8000,0x8000,0x8000,  0x8001,0x8001,0x8001,0x8001,  0x8002,0x8002,0x8002,0x8002,  0x8003,0x8003,0x8003,0x8003);
	const __m256i Const258 = _mm256_set1_epi16(258);
	//printf("%d pixels wide means %d AVX2 loops + %d SSE loops\n", dx, dx / 4, dx % 4);
	int nSkippedRows = 0;
	if (startY < 0)
	{
		nSkippedRows = -startY;
		row += nSkippedRows * graphics->Stride;
		rowGlyph += nSkippedRows * glyphStride;
	}
	for (FT_Int iy = nSkippedRows; iy < dy; iy++)
	{
		uint8_t* pix = row;
		const uint8_t* pixGlyph = rowGlyph;
		FT_Int ix;
		for (ix = 0; ix < dx - 3; ix += 4, pix += 16, pixGlyph += 4)
		{
			__m128i bg   = _mm_loadu_si128((__m128i*)pix);                                   // [latency 6]
			__m256i bg16 = _mm256_cvtepu8_epi16(bg);                                         // [latency 3]
			__m256i t    = _mm256_set1_epi32(*(uint32_t*)pixGlyph);                          // [load: latency 7] [broadcast: latency 3]
			__m256i t16  = _mm256_shuffle_epi8(t, ShuffleBrightness16);                      // [latency 1] load and shuffle 32 bits (4 bytes) into 128 bits (16 bytes of [ x[0], x[0], x[0], x[0], x[1], x[1], x[1], x[1], x[2], x[2], x[2], x[2], x[3], x[3], x[3], x[3] ])

			// lerp with 1: a + t(b - a) => a + t - ta

			__m256i sum       = _mm256_add_epi16(bg16, t16);                                 // [latency 1]
			__m256i ta        = _mm256_mulhi_epi16(_mm256_mullo_epi16(bg16, t16), Const258); // [mulhi: latency 5] [mullo: latency 5]
			__m256i diff      = _mm256_sub_epi16(sum, ta);                                   // [latency 1]
			__m256i newColour = _mm256_packus_epi16(diff, diff);                             // [latency 1] packs into bytes as res[0:7],res[0:7],res[8:15],res[8:15]
			        newColour = _mm256_permute4x64_epi64(newColour, 0xD8);                   // [latency 3] corrects the packing to res[0:7],res[8:15],res[0:7],res[8:15], though the higher 128 bits are unneeded

			// store the low 128 bits (res[0:15])
			//_mm_storeu_si128((__m128i*)pix, _mm256_castsi256_si128(newColour));              // [latency 5]


			_mm_storeu_si128((__m128i*)pix, _mm256_castsi256_si128(newColour));               // [latency 5]
			// result = addubs(vec1, vec2)
			// store(permute(pack(result)))
			//
		}

		for (; ix < dx; ++ix, pix += 4, ++pixGlyph)
		{
			// serial latency > 25
			__m128i bg = _mm_cvtepu8_epi16(_mm_loadu_si32(pix)); // [load: latency 7] [unpack: latency 1]
			__m128i t    = { *pixGlyph * 0x0001000100010001, 0 }; // latency unmeasured
			__m128i sum  = _mm_add_epi16(bg, t); // latency 1
			__m128i ta   = _mm_mulhi_epi16(_mm_mullo_epi16(bg, t), _mm256_castsi256_si128(Const258)); // mullo: latency 5, mulhi: latency 5
			__m128i diff = _mm_sub_epi16(sum, ta); // latency 1
			__m128i newColour = _mm_packus_epi16(diff, diff); // latency 1
			_mm_storeu_si32((__m128i*)pix, newColour); // latency 5
		}

		row += graphics->Stride;
		rowGlyph += glyphStride;
	}
}

void TryDrawCachedChar(struct Graphics* graphics, char c, FP266* x, FP266* y, [[maybe_unused]] FT_UInt* previousGlyphForKerning, const struct GlyphCache* glyphs)
{
	if ((c & 0x1F) >= 27)
		goto NoGlyph;
	const struct CachedGlyph* glyph = &glyphs->Glyphs[c & 0x1F];
	if (glyph->GlyphIndex == 0)
		goto NoGlyph;

	if (*x >= 0 && *y >= 0)
	{
		//printf("x=%6.1fpx ", *x/64.0f);
#if defined(USE_KERNING) && USE_KERNING
		bool isKerned = false;
		if (previousGlyphForKerning)
		{
			if (*previousGlyphForKerning)
			{
				FT_Vector kerning;
				FT_Get_Kerning(graphics->Font, *previousGlyphForKerning, glyph->GlyphIndex, FT_KERNING_DEFAULT, &kerning);
				*x += kerning.x;
				if (kerning.x < 0)
					isKerned = true;
				//printf("kern=% 3.1fpx ", kerning.x / 64.0f);
			}
			//else fputs("         px ", stdout);
			*previousGlyphForKerning = glyph->GlyphIndex;
		}
		//else fputs("         px ", stdout);
#endif
		//if (FT_Render_Glyph(graphics->Font->glyph, FT_RENDER_MODE_NORMAL) != 0)
		//	fprintf(stderr, "FreeType: Couldn't render glyph for char '%c' (%Xh)\n", c, c);
#if defined(USE_KERNING) && USE_KERNING
		const FT_Bitmap bmp = (FT_Bitmap) { .buffer = &glyphs->Data[glyph->Bitmap.Offset],
			                                .pitch  = glyph->Bitmap.Stride,
									        .rows   = glyph->Bitmap.Height,
									        .width  = glyph->Bitmap.Width };
		if (isKerned)
			DrawGlyphOver(graphics, &bmp, (*x>>6) + glyph->Bitmap.Left, (*y>>6) - glyph->Bitmap.Top);
		else
#endif
			DrawGlyph(graphics, &bmp, (*x>>6) + glyph->Bitmap.Left, (*y>>6) - glyph->Bitmap.Top);
	}
	//printf("advance=%4.1fpx width=%2dpx left=%2dpx\n", graphics->Font->glyph->advance.x/64.0f, graphics->Font->glyph->bitmap.width, graphics->Font->glyph->bitmap_left);
	*x += graphics->Font->glyph->advance.x;
	*y += graphics->Font->glyph->advance.y;
	return;

NoGlyph:
	DrawChar(graphics, c, x, y, previousGlyphForKerning);
}

void DrawChar(struct Graphics* graphics, char c, FP266* x, FP266* y, [[maybe_unused]] FT_UInt* previousGlyphForKerning)
{
	FT_UInt charIndex = FT_Get_Char_Index(graphics->Font, c);
	if (FT_Load_Glyph(graphics->Font, charIndex, FT_LOAD_RENDER) != 0)
		fprintf(stderr, "FreeType: Couldn't load glyph for char '%c' (%Xh)\n", c, c);
	//putchar(c); putchar(':'); putchar(' ');
	if (*x >= 0 && *y >= 0)
	{
		//printf("x=%6.1fpx ", *x/64.0f);
#if defined(USE_KERNING) && USE_KERNING
		bool isKerned = false;
		if (previousGlyphForKerning)
		{
			if (*previousGlyphForKerning)
			{
				FT_Vector kerning;
				FT_Get_Kerning(graphics->Font, *previousGlyphForKerning, charIndex, FT_KERNING_DEFAULT, &kerning);
				*x += kerning.x;
				if (kerning.x < 0)
					isKerned = true;
				//printf("kern=% 3.1fpx ", kerning.x / 64.0f);
			}
			//else fputs("         px ", stdout);
			*previousGlyphForKerning = charIndex;
		}
		//else fputs("         px ", stdout);
#endif
		//if (FT_Render_Glyph(graphics->Font->glyph, FT_RENDER_MODE_NORMAL) != 0)
		//	fprintf(stderr, "FreeType: Couldn't render glyph for char '%c' (%Xh)\n", c, c);
#if defined(USE_KERNING) && USE_KERNING
		if (isKerned)
			DrawGlyphOver(graphics, &graphics->Font->glyph->bitmap, (*x>>6) + graphics->Font->glyph->bitmap_left, (*y>>6) - graphics->Font->glyph->bitmap_top);
		else
#endif
			DrawGlyph(graphics, &graphics->Font->glyph->bitmap, (*x>>6) + graphics->Font->glyph->bitmap_left, (*y>>6) - graphics->Font->glyph->bitmap_top);
	}
	//printf("advance=%4.1fpx width=%2dpx left=%2dpx\n", graphics->Font->glyph->advance.x/64.0f, graphics->Font->glyph->bitmap.width, graphics->Font->glyph->bitmap_left);
	*x += graphics->Font->glyph->advance.x;
	*y += graphics->Font->glyph->advance.y;
}

void DrawKey(struct AppContext* context, struct Graphics* graphics, KeyIndex key, uint8_t state)
{
	float x = context->StartX + key.cluster * context->KeyboardClusterGap;
	float y = context->StartY + (key.row % 5) * context->RowHeight;
	float width = context->StandardKeyWidth;
	float height = context->RowHeight;
	char keyChar = context->KeyChars[CHARINDEXFORKEYINDEX(key)];
	//char keyChar = linearRow >= 4 ? KEYCHARS[4*16 + (linearRow - 4) * 4 + key.key] : KEYCHARS[linearRow * 16 + key.key];
	switch (key.cluster)
	{
		case 2:
		{
			x += 3 * context->StandardKeyWidth;
			if (key.row == 4)
			{
				if (key.key == 0)
					width *= 2;
				else
					x += context->StandardKeyWidth;
			}
			else if ((key.row == 1 && key.key == 3) || (key.row == 3 && key.key == 3))
			{
				height *= 2;
			}
		} goto AddMainKeyboardToX;
		case 1:
		{
			if (key.row == 3 && key.key == 0)
				x += context->StandardKeyWidth;
		AddMainKeyboardToX:
			x += 14.9 * context->StandardKeyWidth;
			x += key.key * context->StandardKeyWidth;
		} break;
		case 0:
		{
			const float* widths = &KEYWIDTHS[16 * key.row];
			for (size_t i = 0; i < key.key; ++i)
				x += widths[i] * context->StandardKeyWidth;
			width = widths[key.key] * context->StandardKeyWidth;
		} break;
	}
	width -= context->KeySpacing;
	height -= context->KeySpacing;

	SetBackgroundColourByState(state, graphics);
	DrawRectangle(graphics, (int)x, (int)y, (int)width, (int)height);
	SetTextColourByState(state, graphics);
	FP266 xf, yf;
	if (keyChar >= FKEY(0))
	{
		xf = context->KeyTextXPositions[key.key + XPOS_FKEYOFFSET];
		if (keyChar == FKEY(0))
		{
			keyChar = TEXTCACHEINDEX(Esc);
			goto DrawWord;
		}
		yf = (FP266)((y + 0.8f * (context->RowHeight - context->KeySpacing) - 0.8f * (context->FontSize_WordKeys)) * 64.0f);
		//yf += cache->Top << 6;
		Graphics_SetFont(graphics, context->Font_WordKeys);
#if defined(USE_KERNING) && USE_KERNING
		// Do any of the numbers or 'F' even do any kerning? I doubt...
		FT_UInt KERNGLYPH_ = 0, *KERNGLYPH = &KERNGLYPH_;
#else
		#define KERNGLYPH NULL
#endif
		TryDrawCachedChar(graphics, 'F', &xf, &yf, KERNGLYPH, &context->GlyphCache);
		const int fkey = keyChar - FKEY(1) + 1;
		if (fkey < 10)
		{
			DrawChar(graphics, fkey + '0', &xf, &yf, KERNGLYPH);
		}
		else
		{
			DrawChar(graphics, '1', &xf, &yf, KERNGLYPH);
			DrawChar(graphics, fkey - 10 + '0', &xf, &yf, KERNGLYPH);
		}
	}
	else if (keyChar > ' ')
	{
		xf = context->KeyTextXPositions[CHARINDEXFORKEYINDEX(key)];
		yf = (FP266)((y + 0.8f * (context->RowHeight - context->KeySpacing) - 5) * 64.0f);
		Graphics_SetFont(graphics, context->Font_CharKeys);
		DrawChar(graphics, keyChar, &xf, &yf, NULL);
	}
	else if (keyChar < ' ')
	{
		xf = context->KeyTextXPositions[CHARINDEXFORKEYINDEX(key)];
	DrawWord:
		const struct TextCache* cache = &((struct TextCache*)&context->TextCache)[(int)keyChar];
		//printf("Attempting to draw text for key %d.%d.%d (%Xh)\n", key.cluster, key.row, key.key, key.raw);
		Graphics_SetFont(graphics, context->Font_WordKeys);
		float fontHeight = (context->Font_WordKeys->ascender - context->Font_WordKeys->descender) / 64.0f;
		yf = (FP266)((y + 0.5f * (context->RowHeight - context->KeySpacing) - 0.8f * (fontHeight)) * 64.0f);
		yf += cache->Top << 6;
		//printf("Drawing %dx%dpx at (%.1f,%.1f)\n", cache->Bitmap.width, cache->Bitmap.rows, (float)xf / 64.0f, (float)yf / 64.0f);
		Graphics_SetFont(graphics, context->Font_CharKeys);
		DrawGlyph(graphics, &cache->Bitmap, xf >> 6, yf >> 6);
	}

	wl_surface_damage_buffer(context->Surface, (int)x, (int)y, (int)width, (int)height);
};

#if 0
void DrawCharCentred(struct Graphics* graphics, char c, FP266* x, FP266* y)
{
	FT_UInt charIndex = FT_Get_Char_Index(graphics->Font, c);
	if (FT_Load_Glyph(graphics->Font, charIndex, FT_LOAD_DEFAULT) != 0)
		fprintf(stderr, "FreeType: Couldn't load glyph for char '%c' (%Xh)\n", c, c);
	if (*x >= 0 && *y >= 0)
	{
		if (FT_Render_Glyph(graphics->Font->glyph, FT_RENDER_MODE_NORMAL) != 0)
			fprintf(stderr, "FreeType: Couldn't render glyph for char '%c' (%Xh)\n", c, c);
		
		DrawGlyph(graphics, (*x>>6) - graphics->Font->glyph->bitmap_left - (graphics->Font->glyph->bitmap.width/2), (*y>>6) - graphics->Font->glyph->bitmap_top);
	}
	*x += graphics->Font->glyph->advance.x;
	*y += graphics->Font->glyph->advance.y;
}
#endif
