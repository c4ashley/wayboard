#ifndef _COMMON_H_
#define _COMMON_H_

#include "freetype/fttypes.h"
#define LAYER_SHELL_VERSION 4

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>
#include <math.h>
#include <linux/input-event-codes.h>
#include <xkbcommon/xkbcommon.h>
#include "protocols/virtual-keyboard-unstable-v1.h"
#include "protocols/xdg-shell.h"
#include "protocols/wlr-layer-shell-unstable-v1.h"
#include <ft2build.h>
#include <freetype/freetype.h>

#define countof(a) (sizeof(a) / sizeof(*(a)))

#define HIGHLIGHT_STATE_HOVER    1
#define HIGHLIGHT_STATE_PRESSED  2

#ifdef _DEBUG
#define FTAssert(expr, ...) do { if ((expr) != 0) { fprintf(stderr, __VA_ARGS__); } } while(0)
#else
#define FTAssert(expr, ...) do { (expr); } while (0)
#endif
#define FTAssertExit(expr, code, ...) do { if ((expr) != 0) { fprintf(stderr, __VA_ARGS__); exit((code)); } } while(0)

typedef long FP266;
typedef wl_fixed_t FP248;

enum HighlightFlag
{
	HF_Hover   = 1,
	HF_Pressed = 2,
	HF_Latched = 4
};

enum DirtyStateEnum
{
	DS_None = 0,
	DS_LetterKeys,
	DS_NumericKeys,
	DS_WholeMainKeyboard,
	DS_WholeSurface

};

struct Buffer
{
	int                Width, Height, Stride, Size;
	bool               Busy;
	bool               Pending;
	struct wl_buffer * Buffer;
	void             * Data;
	struct AppContext* Context;
};

typedef union
{
	struct
	{
		uint16_t key : 6;
		uint16_t row : 3;
		uint16_t cluster : 2;
	};
	uint16_t raw;
} KeyIndex;
#define MakeRawIndex(Cluster, Row, Key) (((Key) & 63) | (((Row) & 7) << 6) | (((Cluster) & 3) << 3))
#define MakeIndex(Cluster, Row, Key) ((KeyIndex) {.cluster=(Cluster), .row=(Row), .key=(Key)})
#define KEYINDEX_INVALID ((KeyIndex){.raw=UINT16_MAX})
#define KEYINDEX_MASK (0x7FFu)

#define RGBU32(r, g, b) (((r) & 0xFFu) | (((g) & 0xFFu) << 8) | (((b) & 0xFFu) << 16) | (0xFFu << 24))

#ifndef _QUEUE_H_
#include "queue.h"
#endif

struct Touch
{
	bool IsDown;
	struct
	{
		double X, Y;
	} Current, Initial;
	KeyIndex Key;
};

struct Monitor
{
	struct AppContext* App;
	int Index;
	struct wl_output* Handle;
};

struct BitmapCache
{
	int Offset;
	int Stride;
	unsigned short Width;
	unsigned short Height;
	short Left;
	short Top;
};

struct CachedGlyph
{
	FT_UInt GlyphIndex;
	FP266 AdvanceX;
	struct BitmapCache Bitmap;
};

struct GlyphCache
{
	uint8_t* Data;
	size_t Capacity;
	size_t Length;
	struct CachedGlyph Glyphs[27];
};

typedef enum Modifiers
{
	MOD_LSHIFT = 1<<0,
	MOD_RSHIFT = 1<<1,
	MOD_SHIFTMASK = MOD_LSHIFT | MOD_RSHIFT,

	MOD_LCTRL = 1<<2,
	MOD_RCTRL = 1<<3,
	MOD_CTRLMASK = MOD_LCTRL | MOD_RCTRL,

	MOD_LALT = 1<<4,
	MOD_RALT = 1<<5,
	MOD_ALTMASK = MOD_LALT | MOD_RALT,

	MOD_LSUPER = 1<<6,
	MOD_RSUPER = 1<<7,
	MOD_SUPERMASK = MOD_LSUPER | MOD_RSUPER,

	MOD_CAPSLOCK_PRESSED   = 1<<8,
	MOD_CAPSLOCK_LATCHED   = 1<<9,
	MOD_CAPSLOCK_MASK      = MOD_CAPSLOCK_PRESSED | MOD_CAPSLOCK_LATCHED,
	MOD_NUMLOCK_PRESSED    = 1<<10,
	MOD_NUMLOCK_LATCHED    = 1<<11,
	MOD_NUMLOCK_MASK       = MOD_NUMLOCK_PRESSED | MOD_NUMLOCK_LATCHED,
	MOD_SCROLLLOCK_PRESSED = 1<<12,
	MOD_SCROLLLOCK_LATCHED = 1<<13,
	MOD_SCROLLLOCK_MASK    = MOD_SCROLLLOCK_PRESSED | MOD_SCROLLLOCK_LATCHED,

	MOD_LOCKMASK = MOD_CAPSLOCK_MASK | MOD_NUMLOCK_MASK | MOD_SCROLLLOCK_MASK,
	MOD_LOCKMASK_LATCH = MOD_CAPSLOCK_LATCHED | MOD_NUMLOCK_LATCHED | MOD_SCROLLLOCK_LATCHED,
} Modifiers_t;

struct AppContext
{
	struct wl_display                     * Display;
	struct wl_registry                    * Registry;
	struct wl_compositor                  * Compositor;
	struct wl_subcompositor               * Subcompositor;
	struct wl_shm                         * SharedMemory;
	struct wl_seat                        * Seat;
	struct Monitor                          Outputs[8];
	struct zwp_virtual_keyboard_manager_v1* VirtualKeyboardManager;
	struct zwlr_layer_shell_v1            * Shell;
	struct xdg_wm_base                    * Xdg;
	struct zwp_virtual_keyboard_v1        * VirtualKeyboard;
	struct zwp_input_method_manager_v2    * InputMethodManager;
	struct zwp_input_method_v2            * InputMethod;
	struct zwp_input_method_keyboard_grab_v2 * Input;

	struct wl_surface            * Surface;
	struct wl_touch              * Touch;
	struct wl_pointer            * Pointer;
	struct wl_keyboard           * Keyboard;
	struct zwlr_layer_surface_v1 * Layer;
	struct wl_shm_pool           * Pool;
	struct xdg_surface           * XdgSurface;
	struct xdg_toplevel          * Toplevel;
	void                         * ShmMap;
	struct
	{
		struct xkb_context * Context;
		struct xkb_keymap  * Keymap;
		struct xkb_state   * State;
		void               * Data;
		size_t               cbData;
	} Xkb;

	FT_Library FreeType;
	FT_Face    Font_CharKeys;
	FT_Face    Font_WordKeys;
	int        FontSize_CharKeys;
	int        FontSize_WordKeys;
	FP248      ScaleFP248; /* 24.8 fixed-point representation of the fractional scale */
	FT_Fixed   ScaleFP266; /* 26.6 fixed-point representation of the fractional scale */

	bool   CloseRequested;
	struct KeyQueue DirtyKeys;
	enum   DirtyStateEnum DirtyState;
	bool   Visible;

	struct 
	{
		struct TextCache
		{
			/*void* Buffer;
			int   Width;
			int   Height;
			int   Stride;*/
			FT_Bitmap Bitmap;
			int   Left;
			int   Top;
		}
		Esc, CapsLock, Shift, Ctrl, Super, Menu, Alt, Enter, Backspace, Tab, Insert, Delete, Home, End, PageUp, PageDown, Up, Down, Left, Right, NumLock;
	} TextCache;
	struct GlyphCache GlyphCache;

	bool WaitingForConfigure;
	const char* PreferredOutputName;
	int TargetOutputIndex;
	int Width, Height, Stride, BufferSize;
	int KeyboardDisplayType;
	int KeySpacing;
	float StandardKeyWidth, RowHeight;
	float KeyboardClusterGap;
	float StartX, StartY;
	float TextHeight;
	struct Touch TouchState[10];

	uint32_t SeatCapabilities;
	Modifiers_t HeldModifiers;
	bool IsShiftDown;
	bool ShmValidFormatAvailable;
	bool Running;
	bool IsMouseDown;
	bool IsToplevel;

	struct Buffer Buffers[2];

	KeyIndex PointerHighlight;
	KeyIndex PointerPressed;
	uint32_t KeyStates_Keyboard[5];
	uint8_t KeyStates_Navigation[5];
	uint8_t KeyStates_NumPad[5];
	FP266 KeyTextXPositions[5 * 16 + 10 * 4]; // 5 rows of 16 for main keyboard, 10 rows of 4 for supplementary
};

static inline bool KeyIndexEquals(KeyIndex a, KeyIndex b)
{
	return (a.raw & 0x7FF) == (b.raw & 0x7FF);
};

static inline bool KeyIndexIsValid(KeyIndex a)
{
	return a.raw != UINT16_MAX;
};

#define TEXTCACHEINDEX(text) ((offsetof(struct AppContext, TextCache.text) - offsetof(struct AppContext, TextCache)) / sizeof(struct TextCache))

#endif
