#define _MAIN_C_

#include "common.h"
#include "freetype/freetype.h"
#include "freetype/ftimage.h"
#include "protocols/wlr-layer-shell-unstable-v1.h"
#include "protocols/xdg-shell.h"
#include "protocols/input-method-unstable-v2.h"
#include <xkbcommon/xkbcommon-keysyms.h>
#include <xkbcommon/xkbcommon.h>
#include <stdarg.h>
#include <time.h>
#include <freetype/ftglyph.h>
#include <immintrin.h>
#include <poll.h>
#include <signal.h>

#include "graphics.h"

struct wl_registry_listener s_registryListener;
struct wl_output_listener s_outputListener;
struct wl_seat_listener s_seatListener;
struct wl_shm_listener s_shmListener;
struct zwlr_layer_surface_v1_listener s_layerSurfaceListener;
struct wl_buffer_listener s_bufferListener;
struct wl_pointer_listener s_pointerListener;
struct wl_touch_listener s_touchListener;
struct wl_keyboard_listener s_keyboardListener;
struct xdg_surface_listener s_xdgSurfaceListener;
struct xdg_toplevel_listener s_toplevelListener;
#if defined(KEYLISTENER) && KEYLISTENER
struct zwp_input_method_v2_listener s_inputMethodListener;
struct zwp_input_method_keyboard_grab_v2_listener s_inputGrabListener;
#endif

static void PrecalculateKeyGeometry(struct AppContext* context);
// @param touch: Increases the hitbox for touch input
static KeyIndex GetKeyAtLocation(float x, float y, const struct AppContext* context, bool touch);
static void UpdateSize(struct AppContext* context, uint32_t width, uint32_t height, uint32_t serial);
static uint32_t SetTextColourByKey(uint32_t stateField, int key, uint32_t currentState, struct Graphics* graphics);
static void SetTextColourByState(uint32_t state, struct Graphics* graphics);
static void SetBackgroundColourByState(uint32_t state, struct Graphics* graphics);
static uint32_t SetBackgroundColourByKey(uint32_t stateField, int key, uint32_t currentState, struct Graphics* graphics);
static void UpdateModifierUi(struct AppContext* context, xkb_mod_mask_t locked, xkb_mod_mask_t depressed);

// @param Modifiers: When used for KEYCHARS, Modifiers indicates the held modifier keys. For other lists, this should always be 0.
#define CHARINDEXFORKEY(Cluster, Row, Key) ({ \
			int offset = 0;\
			int row__ = (Cluster) * 5 + (Row), key__ = (Key);\
			if (row__ < 5)\
				offset = row__ * 16;\
			else\
				offset = row__ * 4 + 0x50 - 20;\
			offset += key__;\
			offset; })
#define CHARINDEXFORKEYINDEX(keyIndex) CHARINDEXFORKEY((keyIndex).cluster, (keyIndex).row, (keyIndex).key)
#define FKEY(number) (0xF0 + (number))
static const int KEYCHARS_SHIFTOFFSET = (16 * 5) + (4 * 10);
static const int KEYCHARS_CAPSOFFSET = (16 * 5) + (4 * 10) + (16 * 4);
static const int KEYCHARS_SHIFTCAPSOFFSET = (16 * 5) + (4 * 10) + (16 * 4) + (16 * 4);
static const int KEYCHARS_COMPACTOFFSET = (16 * 5) + (4 * 10) + (16 * 4) + (16 * 4) + (16 * 4);
static const int KEYCHARS_FKEYOFFSET = (16 * 5) + (4 * 10) + (16 * 4) + (16 * 4) + (16 * 4) + 16;
static const char KEYCHARS[] = {
	// --- main keyboard ---
	/* 0*/ '`','1','2','3','4','5','6','7','8','9','0','-','=',TEXTCACHEINDEX(Backspace),' ',' ',
	/*10*/ TEXTCACHEINDEX(Tab),
	/*  */     'q','w','e','r','t','y','u','i','o','p','[',']','\\',' ',' ',
	/*20*/ TEXTCACHEINDEX(CapsLock),
	/*  */     'a','s','d','f','g','h','j','k','l',';','\'',TEXTCACHEINDEX(Enter),' ',' ',' ',
	/*30*/ TEXTCACHEINDEX(Shift),
	/*  */     'z','x','c','v','b','n','m',',','.','/',TEXTCACHEINDEX(Shift),' ',' ',' ',' ',
	/*40*/ TEXTCACHEINDEX(Ctrl), TEXTCACHEINDEX(Fn), TEXTCACHEINDEX(Super), TEXTCACHEINDEX(Alt), ' ', TEXTCACHEINDEX(Alt), TEXTCACHEINDEX(Menu), TEXTCACHEINDEX(Super), TEXTCACHEINDEX(Ctrl), ' ',' ',' ',' ',' ',' ',' ',
	// --- navigation keys ---
	/*50*/ TEXTCACHEINDEX(Insert), TEXTCACHEINDEX(Home), TEXTCACHEINDEX(PageUp),  ' ',
	/*54*/ TEXTCACHEINDEX(Delete), TEXTCACHEINDEX(End),  TEXTCACHEINDEX(PageDown),' ',
	/*58*/ ' ',' ',' ',' ',
	/*5C*/ ' ',TEXTCACHEINDEX(Up),' ',' ',
	/*60*/ TEXTCACHEINDEX(Left), TEXTCACHEINDEX(Down),  TEXTCACHEINDEX(Right),' ',
	// --- numpad ---
	/*64*/ TEXTCACHEINDEX(NumLock),'/','*','-',
	/*68*/ '7','8','9','+',
	/*6C*/ '4','5','6',' ',
	/*70*/ '1','2','3',TEXTCACHEINDEX(Enter),
	/*74*/ '0','.',' ',' ',
	// --- main keyboard (shift) ---
    /*78*/ '~','!','@','#','$','%','^','&','*','(',')','_','+',TEXTCACHEINDEX(Backspace),' ',' ',
	/*88*/ TEXTCACHEINDEX(Tab),
	/*  */     'Q','W','E','R','T','Y','U','I','O','P','{','}','|',' ',' ',
	/*98*/ TEXTCACHEINDEX(CapsLock),
	           'A','S','D','F','G','H','J','K','L',':','"',TEXTCACHEINDEX(Enter),' ',' ',' ',
	/*A8*/ TEXTCACHEINDEX(Shift),
	           'Z','X','C','V','B','N','M','<','>','?',TEXTCACHEINDEX(Shift),' ',' ',' ',' ',
	// --- main keyboard (caps lock) ---
    /*B8*/ '`','1','2','3','4','5','6','7','8','9','0','-','=',TEXTCACHEINDEX(Backspace),' ',' ',
	/*C8*/ TEXTCACHEINDEX(Tab),
	           'Q','W','E','R','T','Y','U','I','O','P','[',']','\\',' ',' ',
	/*D8*/ TEXTCACHEINDEX(CapsLock),
	           'A','S','D','F','G','H','J','K','L',';','\'',TEXTCACHEINDEX(Enter),' ',' ',' ',
	/*E8*/ TEXTCACHEINDEX(Shift),
	           'Z','X','C','V','B','N','M',',','.','/',TEXTCACHEINDEX(Shift),' ',' ',' ',' ',
	// --- main keyboard (shift caps lock) ---
    /*78*/ '~','!','@','#','$','%','^','&','*','(',')','_','+',TEXTCACHEINDEX(Backspace),' ',' ',
	/*88*/ TEXTCACHEINDEX(Tab),
	/*  */     'q','w','e','r','t','y','u','i','o','p','{','}','|',' ',' ',
	/*98*/ TEXTCACHEINDEX(CapsLock),
	           'a','s','d','f','g','h','j','k','l',':','"',TEXTCACHEINDEX(Enter),' ',' ',' ',
	/*A8*/ TEXTCACHEINDEX(Shift),
	           'z','x','c','v','b','n','m','<','>','?',TEXTCACHEINDEX(Shift),' ',' ',' ',' ',
    // --- compact space row (arrows at end)
	/*B8*/ TEXTCACHEINDEX(Ctrl), TEXTCACHEINDEX(Fn), TEXTCACHEINDEX(Super), TEXTCACHEINDEX(Alt), ' ', TEXTCACHEINDEX(Left), TEXTCACHEINDEX(Down), TEXTCACHEINDEX(Up), TEXTCACHEINDEX(Right), ' ',' ',' ',' ',' ',' ',' ',
    // --- F-key row
	//     FKEY(0) is actually ESC, but we do some weird edge-case handling to translate it
	/*C8*/ FKEY(0), FKEY(1), FKEY(2),FKEY(3),FKEY(4),FKEY(5),FKEY(6),FKEY(7),FKEY(8),FKEY(9),FKEY(10),FKEY(11),FKEY(12),TEXTCACHEINDEX(Backspace),0,0,
};
static const uint8_t KEYSPERROW[] = {14, 14, 13, 12, 9,
	                             3, 3, 0, 2, 3,
								 4, 4, 3, 4, 2 };
static const float KEYWIDTHS[] = {   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1, 1.9, 0, 0, 
                                   1.5,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1, 1.4, 0, 0,
                                   1.8,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1, 2.1,   0, 0, 0,
                                   2.2,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1, 2.7,   0,   0, 0, 0,
                                   1.2,   1,   1,   1, 6.3, 1.1,   1,   1, 1.3,   0,   0,   0,   0,   0, 0, 0, };


static const uint8_t KEYCODES[] = {
KEY_GRAVE, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9, KEY_0, KEY_MINUS, KEY_EQUAL, KEY_BACKSPACE, 0, 0,
KEY_TAB, KEY_Q, KEY_W, KEY_E, KEY_R, KEY_T, KEY_Y, KEY_U, KEY_I, KEY_O, KEY_P, KEY_LEFTBRACE, KEY_RIGHTBRACE, KEY_BACKSLASH, 0, 0,
KEY_CAPSLOCK, KEY_A, KEY_S, KEY_D, KEY_F, KEY_G, KEY_H, KEY_J, KEY_K, KEY_L, KEY_SEMICOLON, KEY_APOSTROPHE, KEY_ENTER, 0, 0, 0,
KEY_LEFTSHIFT, KEY_Z, KEY_X, KEY_C, KEY_V, KEY_B, KEY_N, KEY_M, KEY_COMMA, KEY_DOT, KEY_SLASH, KEY_RIGHTSHIFT, 0, 0, 0, 0,
KEY_LEFTCTRL, KEY_RESERVED, KEY_LEFTMETA, KEY_LEFTALT, KEY_SPACE, KEY_RIGHTALT, KEY_MENU, KEY_RIGHTMETA, KEY_RIGHTCTRL, 0, 0, 0, 0, 0, 0, 0,

KEY_INSERT, KEY_HOME, KEY_PAGEUP, 0,
KEY_DELETE, KEY_END, KEY_PAGEDOWN, 0,
0, 0, 0, 0,
KEY_UP, KEY_UP, 0, 0,
KEY_LEFT, KEY_DOWN, KEY_RIGHT, 0,

KEY_NUMLOCK, KEY_KPSLASH, KEY_KPASTERISK, KEY_KPMINUS,
KEY_KP7, KEY_KP8, KEY_KP9, KEY_KPPLUS,
KEY_KP4, KEY_KP5, KEY_KP6, 0,
KEY_KP1, KEY_KP2, KEY_KP3, KEY_KPENTER,
KEY_KP0, KEY_KPDOT, KEY_KPDOT, 0,

// compact bottom row of main keyboard (arrows at end)
KEY_LEFTCTRL, KEY_RESERVED, KEY_LEFTMETA, KEY_LEFTALT, KEY_SPACE, KEY_LEFT, KEY_DOWN , KEY_UP, KEY_RIGHT, 0, 0, 0, 0, 0, 0, 0,

// F-key row
KEY_ESC, KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12, 0, 0, 0,
};

static const size_t KEYCODES_FKEYOFFSET    = (16 * 5) + (4 * 10) + 16;
static const size_t KEYCODES_COMPACTOFFSET = (16 * 5) + (4 * 10);
static const size_t XPOS_SHIFTOFFSET       = (16 * 5) + (4 * 10);
static const size_t XPOS_FKEYOFFSET        = (16 * 5) + (4 * 10) + (16 * 5);

static struct timespec s_clockResolution;


void SendKey(struct AppContext* context, KeyIndex key, int state, uint32_t time);
void SetHighlightFlag(struct AppContext* context, uint32_t flag, KeyIndex index, bool deferRedraw);
void ClearHighlightFlag(struct AppContext* context, uint32_t flag, KeyIndex index, bool deferRedraw);
void DrawKeyboard(struct AppContext* context);

void CreateBuffer(struct Buffer* buffer, int index, struct AppContext* context);
void DestroyBuffer(struct Buffer* buffer);
struct Buffer* FindAvailableBuffer(struct AppContext* context, bool makeUnavailable);

int NewShmFile();

static struct AppContext* s_context;

static void DestroyFreeType() { FT_Done_FreeType(s_context->FreeType); };
static void DestroyFonts()    { FT_Done_Face(s_context->Font_CharKeys); FT_Done_Face(s_context->Font_WordKeys);};

static void HandleSignal(int signo);
static void ReleaseAllKeys(struct AppContext* context);

int main(int argc, const char* argv[])
{
	struct AppContext context = {0};
	s_context = &context;
	context.KeySpacing = 5;
	context.ScaleFP266 = 1 << 6;
	context.ScaleFP248 = 1 << 8;
	if (clock_getres(CLOCK_MONOTONIC, &s_clockResolution) < 0)
		s_clockResolution = (struct timespec) {0};

	signal(SIGUSR1, HandleSignal);
	signal(SIGUSR2, HandleSignal);
	signal(SIGINT,  HandleSignal);

	memcpy(context.KeyCodes, KEYCODES, sizeof(context.KeyCodes));
	memcpy(context.KeyChars, KEYCHARS, sizeof(context.KeyChars));

	context.PointerHighlight = KEYINDEX_INVALID;
	context.PointerPressed = KEYINDEX_INVALID;
	context.Exclusive = true;
	for (int i = 0; i < argc; ++i)
	{
		if (strcmp(argv[i], "--toplevel") == 0)
		{
			context.IsToplevel = true;
		}
		else if (strcmp(argv[i], "--output") == 0)
		{
			++i;
			context.PreferredOutputName = argv[i];
		}
		else if (strcmp(argv[i], "--nonexclusive") == 0)
		{
			context.Exclusive = false;
		}
	}

	if (FT_Init_FreeType(&context.FreeType) != 0)
	{
		fprintf(stderr, "Failed to initialise FreeType\n");
		return -1;
	}
	atexit(DestroyFreeType);

	const char* fontPath;
	if (access("/usr/share/fonts/google-noto/NotoSans-Regular.ttf", R_OK) == 0)
		fontPath = "/usr/share/fonts/google-noto/NotoSans-Regular.ttf";
	else if (access("/usr/share/fonts/adwaita-sans-fonts/AdwaitaSans-Regular.ttf", R_OK) == 0)
		fontPath = "/usr/share/fonts/adwaita-sans-fonts/AdwaitaSans-Regular.ttf";
	else if (access("/usr/share/fonts/dejavu-sans-fonts/DejaVuSans.ttf", R_OK) == 0)
		fontPath = "/usr/share/fonts/dejavu-sans-fonts/DejaVuSans.ttf";
	else if (access("/usr/share/fonts/liberation-sans-fonts/LiberationSans-Regular.ttf", R_OK) == 0)
		fontPath = "/usr/share/fonts/liberation-sans-fonts/LiberationSans-Regular.ttf";
	else if (access("/usr/share/fonts/open-sans/OpenSans-Regular.ttf", R_OK) == 0)
		fontPath = "/usr/share/fonts/open-sans/OpenSans-Regular.ttf";
	else
	{
		fprintf(stderr, "Couldn't find any fonts.\n");
		return -1;
	}

	printf("Loading font %s\n", fontPath);
	FTAssertExit(FT_New_Face(context.FreeType, fontPath, 0, &context.Font_CharKeys), -1, "FreeType: Failed to create font\n");
	FTAssertExit(FT_New_Face(context.FreeType, fontPath, 0, &context.Font_WordKeys), -1, "FreeType: Failed to create font\n");
	atexit(DestroyFonts);

	// fixed point (64*int) + (frac)

	context.FontSize_CharKeys = 20;
	context.FontSize_WordKeys = 14;
	FTAssert(FT_Set_Char_Size(context.Font_CharKeys, 0, context.FontSize_CharKeys * context.ScaleFP266, 0, 0), "FreeType: Failed to set font size\n");
	FTAssert(FT_Set_Char_Size(context.Font_WordKeys, 0, context.FontSize_WordKeys * context.ScaleFP266, 0, 0), "FreeType: Failed to set font size\n");

	context.Display  = wl_display_connect(NULL);
	context.Registry = wl_display_get_registry(context.Display);
	if (wl_registry_add_listener(context.Registry, &s_registryListener, &context) != 0)
	{
		perror("Failed to add a listener to the registry object.");
		exit(1);
	}
	wl_display_roundtrip(context.Display);

	assert(context.Compositor != NULL);
	assert(context.Subcompositor != NULL);
	assert(context.SharedMemory != NULL);
	assert(context.Seat != NULL);
	assert(context.VirtualKeyboardManager != NULL);
	assert(context.Outputs[0].Handle != NULL);
	assert(context.Shell != NULL);
	assert(context.Xdg != NULL);
	assert(context.InputMethodManager != NULL);

	context.Xkb.Context = xkb_context_new(0);
	context.VirtualKeyboard = zwp_virtual_keyboard_manager_v1_create_virtual_keyboard(context.VirtualKeyboardManager, context.Seat);
#if defined(KEYLISTENER) && KEYLISTENER
	context.InputMethod = zwp_input_method_manager_v2_get_input_method(context.InputMethodManager, context.Seat);
	assert(context.InputMethod != NULL);
	context.Input = zwp_input_method_v2_grab_keyboard(context.InputMethod);
	assert(context.Input != NULL);
	zwp_input_method_keyboard_grab_v2_add_listener(context.Input, &s_inputGrabListener, &context);
#endif

	for (struct Monitor * pOutput = &context.Outputs[0]; pOutput < &context.Outputs[8] && pOutput->Handle; ++pOutput)
	{
		void* index = (void*)(intptr_t)(pOutput - &context.Outputs[0]);
		printf("assigning index %zd to output\n", (ssize_t)index);
		wl_output_add_listener(pOutput->Handle, &s_outputListener, pOutput);
	}
	wl_seat_add_listener(context.Seat,        &s_seatListener,   &context);
	wl_shm_add_listener(context.SharedMemory, &s_shmListener,    &context);
#if defined(KEYLISTENER) && KEYLISTENER
	zwp_input_method_v2_add_listener(context.InputMethod, &s_inputMethodListener, &context);
#endif

	wl_display_roundtrip(context.Display);

	assert(context.ShmValidFormatAvailable);

	context.Surface = wl_compositor_create_surface(context.Compositor);
	if (context.SeatCapabilities & WL_SEAT_CAPABILITY_POINTER)
	{
		context.Pointer = wl_seat_get_pointer(context.Seat);
		wl_pointer_add_listener(context.Pointer, &s_pointerListener, &context);
	}
	if (context.SeatCapabilities & WL_SEAT_CAPABILITY_TOUCH)
	{
		context.Touch   = wl_seat_get_touch(context.Seat);
		wl_touch_add_listener(context.Touch, &s_touchListener, &context);
	}
	if (context.SeatCapabilities & WL_SEAT_CAPABILITY_KEYBOARD)
	{
		context.Keyboard = wl_seat_get_keyboard(context.Seat);
		wl_keyboard_add_listener(context.Keyboard, &s_keyboardListener, &context);
	}
	assert(context.Surface != NULL);
	assert(context.Pointer != NULL);
	//assert(context.Touch != NULL);
	if (!context.IsToplevel)
	{
		context.Layer   = zwlr_layer_shell_v1_get_layer_surface(context.Shell, context.Surface, context.Outputs[context.TargetOutputIndex].Handle, ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY, "keyboard");
		assert(context.Layer != NULL);

		if (zwlr_layer_surface_v1_add_listener(context.Layer, &s_layerSurfaceListener, &context) != 0)
		{
			perror("failed to attach listener to WLR Shell Layer object");
			goto Cleanup;
		}
		zwlr_layer_surface_v1_set_size(context.Layer, 0, 380);
		zwlr_layer_surface_v1_set_anchor(context.Layer, ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM | ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT);
		if (context.Exclusive)
			zwlr_layer_surface_v1_set_exclusive_zone(context.Layer, 380);
#if LAYER_SHELL_VERSION > 4
		//zwlr_layer_surface_v1_set_exclusive_edge(context.Layer, ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM);
#endif
		zwlr_layer_surface_v1_set_keyboard_interactivity(context.Layer, 0);
	}
	else
	{
		context.XdgSurface = xdg_wm_base_get_xdg_surface(context.Xdg, context.Surface);
		context.Toplevel = xdg_surface_get_toplevel(context.XdgSurface);
		xdg_surface_add_listener(context.XdgSurface, &s_xdgSurfaceListener, &context);
		xdg_toplevel_add_listener(context.Toplevel, &s_toplevelListener, &context);

		xdg_toplevel_set_min_size(context.Toplevel, 1200, 240);
	}

	wl_surface_commit(context.Surface);

	context.Running = true;
	do {
		printf("Waiting for configure..\n");
		wl_display_roundtrip(context.Display);
	} while (context.WaitingForConfigure && context.Running);
	printf("Configured and ready.\n");
	//if (context.Buffers
	DrawKeyboard(&context);

	struct pollfd pfd = {
        .fd = wl_display_get_fd(context.Display),
        .events = POLLIN,
    };

	int err, protocolError;
	const struct wl_interface* errantInterface;
	while (context.Running)
	{
		while (wl_display_prepare_read(context.Display) != 0)
		{
			wl_display_dispatch_pending(context.Display);
		}

		wl_display_flush(context.Display);

		if (poll(&pfd, 1, -1))
			wl_display_read_events(context.Display);
		else
			wl_display_cancel_read(context.Display);

		wl_display_dispatch_pending(context.Display);

		RefreshInvalidatedRegions(&context);

		err = wl_display_get_error(context.Display);
		protocolError = wl_display_get_protocol_error(context.Display, &errantInterface, NULL);
		if (err != 0 || protocolError != 0)
			goto Fail;
		
	}
	err = wl_display_get_error(context.Display);
	protocolError = wl_display_get_protocol_error(context.Display, &errantInterface, NULL);
Fail:
	if (err != 0)
		fprintf(stderr, "Wayland error %d\n", err);
	if (protocolError != 0)
		fprintf(stderr, "Protocol error %d in interface %s\n", protocolError, errantInterface->name);

Cleanup:
	ReleaseAllKeys(&context);

	if (context.Xkb.Keymap)
	{
		xkb_state_unref(context.Xkb.State);
		xkb_keymap_unref(context.Xkb.Keymap);
		munmap(context.Xkb.Data, context.Xkb.cbData);
	}
	xkb_context_unref(context.Xkb.Context);
	zwp_virtual_keyboard_v1_destroy(context.VirtualKeyboard);
	zwp_virtual_keyboard_manager_v1_destroy(context.VirtualKeyboardManager);
	DestroyBuffer(&context.Buffers[0]);
	//DestroyBuffer(&context.Buffers[1]);
	if (context.Pool != NULL)
		wl_shm_pool_destroy(context.Pool);
	if (context.ShmMap != NULL)
		munmap(context.ShmMap, context.BufferSize * 2);
	if (context.Layer)
		zwlr_layer_surface_v1_destroy(context.Layer);
	if (context.Shell)
		zwlr_layer_shell_v1_destroy(context.Shell);
	if (context.Toplevel)
		xdg_toplevel_destroy(context.Toplevel);
	if (context.XdgSurface)
		xdg_surface_destroy(context.XdgSurface);
	xdg_wm_base_destroy(context.Xdg);
	wl_shm_release(context.SharedMemory);
	wl_seat_release(context.Seat);
	for (struct Monitor * pOutput = &context.Outputs[0]; pOutput < &context.Outputs[8] && pOutput->Handle; ++pOutput)
		wl_output_release(pOutput->Handle);
	wl_subcompositor_destroy(context.Subcompositor);
	wl_compositor_destroy(context.Compositor);
	wl_display_disconnect(context.Display);

	return 0;
}

void HandleSignal(int signo)
{
	switch (signo)
	{
		case SIGINT:
		{
			// Make sure any pressed keys get released, then we can safely close.
			// Force quit if we're stuck.
			if (s_context->CloseRequested)
				_exit(1);

			s_context->CloseRequested = true;
			s_context->Running = false;

			ReleaseAllKeys(s_context);
		} break;
		// TODO: Use SIGUSR signals to show/hide
		// TODO: Also use actual Wayland protocol messages for requesting the keyboard/IME
		case SIGUSR1:
		case SIGUSR2:
			break;
	}
};

void ReleaseAllKeys(struct AppContext* context)
{
	for (size_t i = 0; i < countof(context->KeyStates_Keyboard); ++i)
	{
		for (size_t j = 0, state = context->KeyStates_Keyboard[i]; state; ++j, state >>= 2)
			if (state & HIGHLIGHT_STATE_PRESSED)
				SendKey(context, (KeyIndex) {.cluster = 0, .row = i, .key = j}, WL_KEYBOARD_KEY_STATE_RELEASED, 0);
		context->KeyStates_Keyboard[i] &= ~(HIGHLIGHT_STATE_PRESSED * 0x55555555u);
	}

	for (size_t i = 0; i < countof(context->KeyStates_Navigation); ++i)
	{
		for (size_t j = 0, state = context->KeyStates_Navigation[i]; state; ++j, state >>= 2)
			if (state & HIGHLIGHT_STATE_PRESSED)
				SendKey(context, (KeyIndex) {.cluster = 1, .row = i, .key = j}, WL_KEYBOARD_KEY_STATE_RELEASED, 0);
		context->KeyStates_Navigation[i] &= ~(HIGHLIGHT_STATE_PRESSED * 0x55);
	}

	for (size_t i = 0; i < countof(context->KeyStates_NumPad); ++i)
	{
		for (size_t j = 0, state = context->KeyStates_NumPad[i]; state; ++j, state >>= 2)
			if (state & HIGHLIGHT_STATE_PRESSED)
				SendKey(context, (KeyIndex) {.cluster = 2, .row = i, .key = j}, WL_KEYBOARD_KEY_STATE_RELEASED, 0);
		context->KeyStates_NumPad[i] &= ~(HIGHLIGHT_STATE_PRESSED * 0x55);
	}
}

static inline void InvalidateKey(struct AppContext* context, KeyIndex key, uint8_t state)
{
	//printf("invalidating key %d.%d.%d\n", key.cluster, key.row, key.key);
	Queue_Push(&context->DirtyKeys, key, state);
};

// Draws a glyph to our cache buffer, which is grayscale. No SIMD optimisations are done, unless performed
// automatically by the compiler, because this function is intended for the smaller "Word" font, so glyphs
// are expected to be fairly small, and at 1 byte per pixel, the total data is no larger.
void DrawMonoGlyph(FT_Bitmap* dest, const FT_Bitmap* source, int x, int y)
{
	uint8_t* rowDest = &((uint8_t*)dest->buffer)[y * dest->pitch + x];
	const uint8_t* rowSrc = source->buffer;
	long ix;
	for (FT_UInt iy = 0; iy < source->rows; ++iy, rowDest += dest->pitch, rowSrc += source->pitch)
	{
		uint64_t* pDest = (uint64_t*)rowDest;
		const uint64_t* pSrc = (const uint64_t*)rowSrc;
		for (ix = 0; ix < (long)source->width - 7; ix += 8)
			*pDest++ = *pSrc++;

		uint8_t* pDest8 = (uint8_t*)pDest;
		const uint8_t* pSrc8 = (const uint8_t*)pSrc;
		for (; ix < source->width; ++ix)
			*pDest8++ = *pSrc8++;
	}
}

// Same as `DrawMonoGlyph`, but accounts for transparency, allowing a glyph to be safely drawn over the
// bounds of a previous glyph. Only necessary when "Advance" is less than a glyph's width, which is probably
// only the case when kerning.
void DrawMonoGlyphOver(FT_Bitmap* dest, const FT_Bitmap* source, int x, int y)
{
	const FT_UInt srcWidth = source->width;
	const FT_UInt srcHeight = source->rows;
	const FT_UInt srcStride = source->pitch;
	uint8_t* rowDest = &((uint8_t*)dest->buffer)[y * dest->pitch + x];
	const uint8_t* rowSrc = source->buffer;
	long ix;
	for (FT_UInt iy = 0; iy < srcHeight; ++iy, rowDest += dest->pitch, rowSrc += srcStride)
	{
		// In 99.9% of cases, we can be sure that overlapping glyphs will not have overlapping pixels.
		// The only exception I can think of would be on the antialiasing of ligatures, but I don't
		// even know how to create ligatures, nor am I intending to use any, so it's a non-issue and
		// we can safely use a naive ADD.
		uint8_t* pDest = rowDest;
		const uint8_t* pSrc = rowSrc;
		for (ix = 0; ix < (long)srcWidth - 7; ix += 8, pDest += 8, pSrc += 8)
		{
			// 17 cycles per 8 pixels
			__m128i vDest = _mm_loadu_si64(pDest);          // latency 7
			__m128i vSrc  = _mm_loadu_si64(pSrc);           // latency 7, but overlapped
			vDest         = _mm_cvtepu8_epi16(vDest);       // latency 1
			vSrc          = _mm_cvtepu8_epi16(vSrc);        // latency 1
			vDest         = _mm_add_epi16(vDest, vSrc);     // latency 1
			vDest         = _mm_packus_epi16(vDest, vDest); // latency 1
			_mm_storeu_si64(pDest, vDest);                 // latency 5
		}

		for (; ix < srcWidth - 1; ix += 2, pDest += 2, pSrc += 2)
		{
			// 11 cycles per 2 pixels
			__m128i vDest = _mm_loadu_si16(pDest);          // latency 3 (xor[1] then insert[2])
			__m128i vSrc  = _mm_loadu_si16(pSrc);           // latency 3, but overlapped
			vDest         = _mm_cvtepu8_epi16(vDest);       // latency 1
			vSrc          = _mm_cvtepu8_epi16(vSrc);        // latency 1
			vDest         = _mm_add_epi16(vDest, vSrc);     // latency 1
			vDest         = _mm_packus_epi16(vDest, vDest); // latency 1
			_mm_storeu_si16(pDest, vDest);                 // latency 3
		}

		if (ix < srcWidth)
		{
			if (*pDest != 0xFF)
			{
				if (*pDest + *pSrc >= 0xFF)
					*pDest = 0xFF;
				else
					*pDest += *pSrc;
			}
		}
	}
}

// returns the width, for using in geometry precalculations
FP266 PrecacheString([[maybe_unused]] FT_Face font, const struct GlyphCache* glyphs, struct TextCache* cache, const char* text)
{
	FP266 width = 0;
	int left;
	int bottom, top;
	const struct CachedGlyph* glyph;
#if defined(USE_KERNING) && USE_KERNING
	FT_UInt prevGlyph, glyphId;
	bool stringHasOverlapKerning = false;
	bool stringHasAnyKerning = false;
	if (FT_HAS_KERNING(font))
	{
		glyph = &glyphs->Glyphs[text[0]&0x1F];
		glyphId = glyph->GlyphIndex;
		left = glyph->Bitmap.Left;
		width += glyph->AdvanceX - (glyph->Bitmap.Left * 64);
		top = glyph->Bitmap.Top;
		bottom = glyph->Bitmap.Top + glyph->Bitmap.Height;
		prevGlyph = glyphId;
		for (const char* p = &text[1]; *p; ++p)
		{
			glyph = &glyphs->Glyphs[*p & 0x1F];
			glyphId = glyph->GlyphIndex;
			if (prevGlyph != 0)
			{
				FT_Vector kerning;
				FT_Get_Kerning(font, prevGlyph, glyphId, FT_KERNING_DEFAULT, &kerning);
				if (kerning.x != 0)
				{
					stringHasAnyKerning = true;
					if (kerning.x < 0)
						stringHasOverlapKerning = true;
				}
				width += kerning.x;
			}
			width += glyph->AdvanceX;
			if (glyph->Bitmap.Top < top)
				top = glyph->Bitmap.Top;
			if (glyph->Bitmap.Top + glyph->Bitmap.Height > bottom)
				bottom = glyph->Bitmap.Top + glyph->Bitmap.Height;
			prevGlyph = glyphId;
		}
	}
	else
#endif
	{
		glyph = &glyphs->Glyphs[text[0] & 0x1F];
		width += glyph->AdvanceX - (glyph->Bitmap.Left * 64);
		left = glyph->Bitmap.Left;
		top = glyph->Bitmap.Top;
		bottom = glyph->Bitmap.Top + glyph->Bitmap.Height;
		for (const char* p = &text[1]; *p; ++p)
		{
			glyph = &glyphs->Glyphs[*p & 0x1F];
			width += glyph->AdvanceX;
			if (glyph->Bitmap.Top < top)
				top = glyph->Bitmap.Top;
			if (glyph->Bitmap.Top + glyph->Bitmap.Height > bottom)
				bottom = glyph->Bitmap.Height + glyph->Bitmap.Height;
		}
	}

	int pixelWidth = (width + 63) >> 6;
	int stride = (pixelWidth + 31) & ~31;
	if (cache->Bitmap.buffer)
		free(cache->Bitmap.buffer);
	cache->Bitmap.buffer = aligned_alloc(16, (stride * (bottom-top) + 15) & ~16);
	cache->Bitmap.pitch = stride;
	cache->Bitmap.width  = pixelWidth;
	cache->Bitmap.rows = bottom - top;
	cache->Left   = left;
	cache->Top    = top;

	FP266 x = -left << 6, y = -top << 6;
	memset(cache->Bitmap.buffer, 0, stride * (bottom-top));

#if defined(USE_KERNING) && USE_KERNING
	if (stringHasAnyKerning)
	{
		prevGlyph = 0;
		for (const char* p = &text[0]; *p; ++p, prevGlyph = glyphId)
		{
			glyph = &glyphs->Glyphs[*p & 0x1F];
			glyphId = glyph->GlyphIndex;
			FT_Vector kerning;
			FT_Get_Kerning(font, prevGlyph, glyphId, FT_KERNING_DEFAULT, &kerning);
			x += kerning.x;
			FT_Bitmap bitmap = { .rows = glyph->Bitmap.Height, .width = glyph->Bitmap.Width, .pitch = glyph->Bitmap.Stride, .buffer = &glyphs->Data[glyph->Bitmap.Offset], 0 };
			if (kerning.x < 0)
				DrawMonoGlyph(&cache->Bitmap, &bitmap, (x >> 6) + glyph->Bitmap.Left, (y >> 6) + glyph->Bitmap.Top);
			else
				DrawMonoGlyphOver(&cache->Bitmap, &bitmap, (x >> 6) + glyph->Bitmap.Left, (y >> 6) + glyph->Bitmap.Top);
			x += glyph->AdvanceX;
		}
	}
	else
#endif
	{
		for (const char* p = &text[0]; *p; ++p)
		{
			glyph = &glyphs->Glyphs[*p & 0x1F];
			FT_Bitmap bitmap = { .rows = glyph->Bitmap.Height, .width = glyph->Bitmap.Width, .pitch = glyph->Bitmap.Stride, .buffer = &glyphs->Data[glyph->Bitmap.Offset], 0 };
			DrawMonoGlyph(&cache->Bitmap, &bitmap, (x >> 6) + glyph->Bitmap.Left, (y >> 6) + glyph->Bitmap.Top);
			x += glyph->AdvanceX;
		}
	}

	return width;
}

void PrecacheGlyphs(struct AppContext* context, FT_Face font)
{
	if (context->GlyphCache.Data == NULL)
	{
		context->GlyphCache.Data     = malloc(8192);
		context->GlyphCache.Capacity = 8192;
	}

	context->GlyphCache.Length   = 0;

	FT_UInt glyph = FT_Get_Char_Index(font, ' ');
	FT_Load_Glyph(font, glyph, FT_LOAD_DEFAULT);
	context->GlyphCache.Glyphs[0].GlyphIndex    = glyph;
	context->GlyphCache.Glyphs[0].AdvanceX      = font->glyph->advance.x;
	context->GlyphCache.Glyphs[0].Bitmap.Offset = 0;
	context->GlyphCache.Glyphs[0].Bitmap.Width  = 0;
	context->GlyphCache.Glyphs[0].Bitmap.Height = 0;
	context->GlyphCache.Glyphs[0].Bitmap.Stride = 0;
	context->GlyphCache.Glyphs[0].Bitmap.Left   = font->glyph->bitmap_left;
	context->GlyphCache.Glyphs[0].Bitmap.Top    = font->glyph->bitmap_top;

#warning "Please be aware that only the necessary char glyphs. Make sure to keep the glyph cache consistent if we change any strings."
	for (char c = 'A'; c <= 'W'; ++c)
	{
		if (c == 'J' || c == 'V')
			continue;

		glyph = FT_Get_Char_Index(font, c);
		FT_Load_Glyph(font, glyph, FT_LOAD_RENDER);
		context->GlyphCache.Glyphs[c&0x1F].GlyphIndex    = glyph;
		context->GlyphCache.Glyphs[c&0x1F].Bitmap.Offset = context->GlyphCache.Length;
		context->GlyphCache.Glyphs[c&0x1F].Bitmap.Stride = font->glyph->bitmap.pitch;
		context->GlyphCache.Glyphs[c&0x1F].Bitmap.Width  = font->glyph->bitmap.width;
		context->GlyphCache.Glyphs[c&0x1F].Bitmap.Height = font->glyph->bitmap.rows;
		context->GlyphCache.Glyphs[c&0x1F].Bitmap.Left   = font->glyph->bitmap_left;
		context->GlyphCache.Glyphs[c&0x1F].Bitmap.Top    = font->glyph->bitmap_top;
		context->GlyphCache.Glyphs[c&0x1F].AdvanceX      = font->glyph->advance.x;

		int size = font->glyph->bitmap.pitch * font->glyph->bitmap.rows;
		while (context->GlyphCache.Length + size > context->GlyphCache.Capacity)
		{
			context->GlyphCache.Data = realloc(context->GlyphCache.Data, context->GlyphCache.Capacity + 4096);
			context->GlyphCache.Capacity += 4096;
		}

		if (font->glyph->bitmap.buffer)
		{
			memcpy(&context->GlyphCache.Data[context->GlyphCache.Length], font->glyph->bitmap.buffer, size);
			context->GlyphCache.Length += size;
		}
		else
		{
			context->GlyphCache.Glyphs[c&0x1F].Bitmap.Stride = 0;
			context->GlyphCache.Glyphs[c&0x1F].Bitmap.Width  = 0;
			context->GlyphCache.Glyphs[c&0x1F].Bitmap.Height = 0;
		}
	}

	// Free up some memory pressure only if the used cache is a decent amount smaller than the allocated cache size
	if (context->GlyphCache.Length < context->GlyphCache.Capacity - 4096)
	{
		context->GlyphCache.Capacity = (context->GlyphCache.Length + 4095) & ~4095;
		context->GlyphCache.Data = realloc(context->GlyphCache.Data, context->GlyphCache.Capacity);
		// Hopefully `realloc` won't perform a move/copy, and instead it'll just mark the tail as free.
	}
}

void PrecacheStrings(struct AppContext* context, FP266* halfwidthsOut)
{
	halfwidthsOut[TEXTCACHEINDEX(Ctrl     )] = PrecacheString(context->Font_WordKeys, &context->GlyphCache, &context->TextCache.Ctrl,      "CTRL")      >> 1;
	halfwidthsOut[TEXTCACHEINDEX(Shift    )] = PrecacheString(context->Font_WordKeys, &context->GlyphCache, &context->TextCache.Shift,     "SHIFT")     >> 1;
	halfwidthsOut[TEXTCACHEINDEX(Super    )] = PrecacheString(context->Font_WordKeys, &context->GlyphCache, &context->TextCache.Super,     "SUPER")     >> 1;
	halfwidthsOut[TEXTCACHEINDEX(Enter    )] = PrecacheString(context->Font_WordKeys, &context->GlyphCache, &context->TextCache.Enter,     "ENTER")     >> 1;
	halfwidthsOut[TEXTCACHEINDEX(Tab      )] = PrecacheString(context->Font_WordKeys, &context->GlyphCache, &context->TextCache.Tab,       "TAB")       >> 1;
	halfwidthsOut[TEXTCACHEINDEX(Alt      )] = PrecacheString(context->Font_WordKeys, &context->GlyphCache, &context->TextCache.Alt,       "ALT")       >> 1;
	halfwidthsOut[TEXTCACHEINDEX(CapsLock )] = PrecacheString(context->Font_WordKeys, &context->GlyphCache, &context->TextCache.CapsLock,  "CAPS LOCK") >> 1;
	halfwidthsOut[TEXTCACHEINDEX(Menu     )] = PrecacheString(context->Font_WordKeys, &context->GlyphCache, &context->TextCache.Menu,      "MENU")      >> 1;
	halfwidthsOut[TEXTCACHEINDEX(Backspace)] = PrecacheString(context->Font_WordKeys, &context->GlyphCache, &context->TextCache.Backspace, "BACKSPACE") >> 1;
	halfwidthsOut[TEXTCACHEINDEX(Insert   )] = PrecacheString(context->Font_WordKeys, &context->GlyphCache, &context->TextCache.Insert   , "INSERT")    >> 1;
	halfwidthsOut[TEXTCACHEINDEX(Delete   )] = PrecacheString(context->Font_WordKeys, &context->GlyphCache, &context->TextCache.Delete   , "DELETE")    >> 1;
	halfwidthsOut[TEXTCACHEINDEX(Home     )] = PrecacheString(context->Font_WordKeys, &context->GlyphCache, &context->TextCache.Home     , "HOME")      >> 1;
	halfwidthsOut[TEXTCACHEINDEX(End      )] = PrecacheString(context->Font_WordKeys, &context->GlyphCache, &context->TextCache.End      , "END")       >> 1;
	halfwidthsOut[TEXTCACHEINDEX(PageUp   )] = PrecacheString(context->Font_WordKeys, &context->GlyphCache, &context->TextCache.PageUp   , "PG UP")     >> 1;
	halfwidthsOut[TEXTCACHEINDEX(PageDown )] = PrecacheString(context->Font_WordKeys, &context->GlyphCache, &context->TextCache.PageDown , "PG DN")     >> 1;
	halfwidthsOut[TEXTCACHEINDEX(NumLock  )] = PrecacheString(context->Font_WordKeys, &context->GlyphCache, &context->TextCache.NumLock  , "NUM LK")    >> 1;
	halfwidthsOut[TEXTCACHEINDEX(Left     )] = PrecacheString(context->Font_WordKeys, &context->GlyphCache, &context->TextCache.Left     , "LEFT")      >> 1;
	halfwidthsOut[TEXTCACHEINDEX(Right    )] = PrecacheString(context->Font_WordKeys, &context->GlyphCache, &context->TextCache.Right    , "RIGHT")     >> 1;
	halfwidthsOut[TEXTCACHEINDEX(Up       )] = PrecacheString(context->Font_WordKeys, &context->GlyphCache, &context->TextCache.Up       , "UP")        >> 1;
	halfwidthsOut[TEXTCACHEINDEX(Down     )] = PrecacheString(context->Font_WordKeys, &context->GlyphCache, &context->TextCache.Down     , "DOWN")      >> 1;
	halfwidthsOut[TEXTCACHEINDEX(Fn       )] = PrecacheString(context->Font_WordKeys, &context->GlyphCache, &context->TextCache.Fn       , "Fn")        >> 1;
	halfwidthsOut[TEXTCACHEINDEX(Esc      )] = PrecacheString(context->Font_WordKeys, &context->GlyphCache, &context->TextCache.Esc      , "ESC")       >> 1;
}


void PrecalculateKeyGeometry(struct AppContext* context)
{
		struct timespec clock1, clock2;
		clock_gettime(CLOCK_MONOTONIC, &clock1);

	size_t i;
	//float y = context->StartY;
	float keyWidth, textWidth;
	FP266 halfwidths[sizeof(context->TextCache)/sizeof(struct TextCache)];
	const float gap = context->KeySpacing;
	float x;

	float fontSize = (context->RowHeight - gap) * 0.6f;
	FT_Set_Char_Size(context->Font_CharKeys, 0, (FP266)(((context->ScaleFP266 / 64.0f) * fontSize) * 64), 0, 0);
	fontSize = (context->RowHeight - gap) * 0.26f;
	FT_Set_Char_Size(context->Font_WordKeys, 0, (FP266)(((context->ScaleFP266 / 64.0f) * fontSize) * 64), 0, 0);

	PrecacheGlyphs(context, context->Font_WordKeys);
	PrecacheStrings(context, halfwidths);

	const FP266 keyHalfWidth = (FP266) ((context->StandardKeyWidth - gap) * 32.0f);

	for (int row = 0; row < 5; ++row)
	{
		x = context->StartX;
		for (i = 0; i < KEYSPERROW[row]; x += KEYWIDTHS[row * 16 + i++] * context->StandardKeyWidth)
		{
			const char c = context->KeyChars[row * 16 + i];
			const float keyWidth = context->StandardKeyWidth * KEYWIDTHS[row * 16 + i] - gap;
			if (c < ' ')
			{
				context->KeyTextXPositions[row*16 + i] = (FP266)(x*64) + (FP266)(keyWidth * 32) - halfwidths[(int)c];
			}
			else if (c > ' ')
			{
				FT_Load_Char(context->Font_CharKeys, c, FT_LOAD_DEFAULT);
				textWidth = context->Font_CharKeys->glyph->metrics.width / 64.0f;
				context->KeyTextXPositions[row*16 + i] = (FP266)((x + (keyWidth - textWidth) * 0.5f) * 64.0);
			}
		}
	}

	keyWidth = context->StandardKeyWidth - gap;
	for (int row = 0; row < 9; ++row)
	{
		x = context->StartX + 14.9 * context->StandardKeyWidth + context->KeyboardClusterGap;
		if (row >= 5)
			x += 3 * context->StandardKeyWidth + context->KeyboardClusterGap;

		for (i = 0; i < KEYSPERROW[row+5]; x += context->StandardKeyWidth, ++i)
		{
			const char c = context->KeyChars[5*16 + row*4 + i];
			if (c < ' ')
			{
				context->KeyTextXPositions[5*16 + row*4 + i] = (FP266)(x*64) + keyHalfWidth - halfwidths[(int)c];
			}
			else if (c > ' ')
			{
				FT_Load_Char(context->Font_CharKeys, c, FT_LOAD_DEFAULT);
				textWidth = context->Font_CharKeys->glyph->metrics.width / 64.0f;
				context->KeyTextXPositions[5*16 + row*4 + i] = (FP266)((x + (keyWidth - textWidth) * 0.5f) * 64.0);
			}
		}
	}

	// UP key is correctly positioned according to position '1' of its row, but we use position '0' most of the time,
	// so copy the correct calculation from '1' to '0'
	context->KeyTextXPositions[5*16 + 3*4 + 0] = context->KeyTextXPositions[5*16 + 3*4 + 1];

	// '0' and '.' keys on the number pad are different and weird.
	x = context->StartX + 2 * context->KeyboardClusterGap + (14.9 + 3) * context->StandardKeyWidth;
	FT_Load_Char(context->Font_CharKeys, '0', FT_LOAD_DEFAULT);
	textWidth = context->Font_CharKeys->glyph->metrics.width / 64.0f;
	context->KeyTextXPositions[5*16 + 5*4 + 4*4 + 0] = (FP266)((x + (2 * context->StandardKeyWidth - gap - textWidth) * 0.5f) * 64.0f);

	x += 2 * context->StandardKeyWidth - gap;
	FT_Load_Char(context->Font_CharKeys, '.', FT_LOAD_DEFAULT);
	textWidth = context->Font_CharKeys->glyph->metrics.width / 64.0f;
	context->KeyTextXPositions[5*16 + 5*4 + 4*4 + 1] = (FP266)((x + (keyWidth - textWidth) * 0.5f) * 64.0f);
	context->KeyTextXPositions[5*16 + 5*4 + 4*4 + 2] = context->KeyTextXPositions[5*16 + 5*4 + 4*4 + 1];

	// F-Key row
	x = context->StartX;
	context->KeyTextXPositions[0 + XPOS_FKEYOFFSET] = (FP266)(x*64) + keyHalfWidth - halfwidths[TEXTCACHEINDEX(Esc)];
	x += context->StandardKeyWidth;

	FT_Load_Char(context->Font_WordKeys, 'F', FT_LOAD_DEFAULT);
	const FP266 advance_F = context->Font_WordKeys->glyph->metrics.horiAdvance;
	FT_Load_Char(context->Font_WordKeys, '1', FT_LOAD_DEFAULT);
	const FP266 width_1   = context->Font_WordKeys->glyph->metrics.width;
	const FP266 advance_1 = context->Font_WordKeys->glyph->metrics.horiAdvance;
	context->KeyTextXPositions[1 + XPOS_FKEYOFFSET] = (FP266)(x*64) + keyHalfWidth - ((advance_F + width_1) >> 1);
	x += context->StandardKeyWidth;

	for (int i = 2; i < 10; ++i, x += context->StandardKeyWidth)
	{
		FT_Load_Char(context->Font_WordKeys, '0' + i, FT_LOAD_DEFAULT);
		context->KeyTextXPositions[i + XPOS_FKEYOFFSET] = (FP266)(x*64) + keyHalfWidth - ((advance_F + context->Font_WordKeys->glyph->metrics.width) >> 1);
	}
	for (int i = 10; i <= 12; ++i, x += context->StandardKeyWidth)
	{
		FT_Load_Char(context->Font_WordKeys, '0' + i - 10, FT_LOAD_DEFAULT);
		context->KeyTextXPositions[i + XPOS_FKEYOFFSET] = (FP266)(x*64) + keyHalfWidth - ((advance_F + advance_1 + context->Font_WordKeys->glyph->metrics.width) >> 1);
	}

	// We'll keep Backspace on the F-key row
	context->KeyTextXPositions[13 + XPOS_FKEYOFFSET] = context->KeyTextXPositions[13];
	

		clock_gettime(CLOCK_MONOTONIC, &clock2);
		unsigned long long dUsecs = (clock2.tv_sec - clock1.tv_sec) * 1'000'000 + (clock2.tv_nsec - clock1.tv_nsec) / 1000;
		printf("Precalculate Geometry: %.1fms\n", (double)dUsecs * 0.001);
}

// @param chars An array of characters to print for each key. Spaces and zeroes will be ignored.
// @param widths An array of doubles representing the width factor for each key. A negative value will create a gap.
// @param nKeys The number of keys to print. `widths` must contain exactly this many elements, and `chars` must contain as many elements as required by positive values in `widths` (i.e., a negative width will not require a corresponding char, so an array of 10 widths with one negative element will only require an array of 9 chars)
void DrawRow(struct Graphics* graphics, struct AppContext* context, int row)
{
	float x = context->StartX;
	float y = context->StartY + context->RowHeight * (row % 5);
	uint32_t lastKeyState = UINT32_MAX;
	const float keyInterval = context->StandardKeyWidth;
	const float spacing = context->KeySpacing;
	const float rowInterval = context->RowHeight;
	const float keyWidth = keyInterval - spacing;
	const float keyHeight = rowInterval - spacing;

	uint32_t keyStates;
	if (row < 5)
		keyStates = context->KeyStates_Keyboard[row];
	else if (row < 10)
		keyStates = context->KeyStates_Navigation[row-5];
	else
		keyStates = context->KeyStates_NumPad[row-10];
	uint32_t state = keyStates;

	const char* const chars = &context->KeyChars[CHARINDEXFORKEY(row/5, row%5, 0)];
	const int nKeys = KEYSPERROW[row];
	if (row >= 5)
		goto ExtendedKeyboard;
	const float* keyWidths = &KEYWIDTHS[row * 16];

	for (int i = 0; i < nKeys; ++i)
	{
		float width = keyInterval * keyWidths[i];
		if (width < spacing)
		{
			if (width < 0)
			{
				x -= width;
			}
			continue;
		}
		uint32_t currentKeyState = state & 3;
		if (currentKeyState != lastKeyState)
		{
			SetBackgroundColourByState(currentKeyState, graphics);
			lastKeyState = currentKeyState;
		}
		DrawRectangle(graphics, x, y, width - spacing, keyHeight);
		x += width;
		state >>= 2;
	}

DrawKeyText:
	lastKeyState = UINT32_MAX;
	state = keyStates;
	//x = startX;
	//x += 0.5 * keyWidth;
	const float fontHeight = (context->Font_WordKeys->ascender - context->Font_WordKeys->descender) / 64.0f;
	const FP266 yChar = (FP266) ((y + 0.8f * keyHeight - 5) * 64.0f);
	const FP266 yWord = (FP266) ((y + 0.5f * keyHeight - 0.8f * fontHeight) * 64.0f);
	FP266 textX, textY;
	for (int i = 0; i < nKeys; ++i)
	{
		char keyChar = chars[i];
		if (keyChar >= FKEY(0))
		{
			textX = context->KeyTextXPositions[i + XPOS_FKEYOFFSET];
			if (keyChar == FKEY(0))
			{
				keyChar = TEXTCACHEINDEX(Esc);
				goto DrawWord;
			}
			textY = (FP266)((y + 0.8f * (context->RowHeight - context->KeySpacing) - 0.8f * (context->FontSize_WordKeys)) * 64.0f);
			Graphics_SetFont(graphics, context->Font_WordKeys);
			lastKeyState = SetTextColourByKey(state, i, lastKeyState, graphics);
			SetBackgroundColourByState(lastKeyState, graphics);
#if defined(USE_KERNING) && USE_KERNING
			// Do any of the numbers or 'F' even do any kerning? I doubt...
			FT_UInt KERNGLYPH_ = 0, *KERNGLYPH = &KERNGLYPH_;
#else
			#define KERNGLYPH NULL
#endif
			TryDrawCachedChar(graphics, 'F', &textX, &textY, KERNGLYPH, &context->GlyphCache);
			const int fkey = keyChar - FKEY(1) + 1;
			if (fkey < 10)
			{
				DrawChar(graphics, fkey + '0', &textX, &textY, KERNGLYPH);
			}
			else
			{
				DrawChar(graphics, '1', &textX, &textY, NULL);
				DrawChar(graphics, fkey - 10 + '0', &textX, &textY, KERNGLYPH);
			}
		}
		else if (keyChar > ' ')
		{
			textX = context->KeyTextXPositions[CHARINDEXFORKEY(row/5, row%5, i)];
			textY = yChar;
			Graphics_SetFont(graphics, context->Font_CharKeys);
			lastKeyState = SetTextColourByKey(state, i, lastKeyState, graphics);
			SetBackgroundColourByState(lastKeyState, graphics);
			DrawChar(graphics, keyChar, &textX, &textY, NULL);

		}
		else if (keyChar < ' ')
		{
			textX = context->KeyTextXPositions[CHARINDEXFORKEY(row/5, row%5, i)];
		DrawWord:
			const struct TextCache* const cache = &((struct TextCache*)&context->TextCache)[(int)keyChar];
			textY = yWord + (cache->Top << 6);
			//Graphics_SetFont(graphics, context->Font_WordKeys);
			lastKeyState = SetTextColourByKey(state, i, lastKeyState, graphics);
			SetBackgroundColourByState(lastKeyState, graphics);
			DrawGlyph(graphics, &cache->Bitmap, textX >> 6, textY >> 6);
		}
	}

	return;

ExtendedKeyboard:
	switch (row)
	{
		case 5: // ins/home/pgup
		case 6: // del/end/pgdn
		case 9: // left/down/right
			x += 14.9f * keyInterval + context->KeyboardClusterGap;
			for (int i = 0; i < 3; ++i)
			{
				state = SetBackgroundColourByKey(keyStates, i, state, graphics);
				DrawRectangle(graphics, x + i * keyInterval, y, keyWidth, keyHeight);
			}
			break;
		case 8: // up
			x += 14.9f * keyInterval + context->KeyboardClusterGap;
			SetBackgroundColourByKey(keyStates, 0, state, graphics);
			DrawRectangle(graphics, x + keyInterval, y, keyWidth, keyHeight);
			break;
		case 10:
			x += 14.9f * keyInterval + 3 * keyInterval + 2 * context->KeyboardClusterGap;
			for (int i = 0; i < 4; ++i)
			{
				state = SetBackgroundColourByKey(keyStates, i, state, graphics);
				DrawRectangle(graphics, x + i * keyInterval, y, keyWidth, keyHeight);
			}
			break;
		case 11:
		case 13:
			x += 14.9f * keyInterval + 3 * keyInterval + 2 * context->KeyboardClusterGap;
			for (int i = 0; i < 3; ++i)
			{
				state = SetBackgroundColourByKey(keyStates, i, state, graphics);
				DrawRectangle(graphics, x + i * keyInterval, y, keyWidth, keyHeight);
			}
			state = SetBackgroundColourByKey(keyStates, 3, state, graphics);
			DrawRectangle(graphics, x + 3 * keyInterval, y, keyWidth, 2 * rowInterval - spacing);
			break;
		case 12:
			x += 14.9f * keyInterval + 3 * keyInterval + 2 * context->KeyboardClusterGap;
			for (int i = 0; i < 3; ++i)
			{
				state = SetBackgroundColourByKey(keyStates, i, state, graphics);
				DrawRectangle(graphics, x + i * keyInterval, y, keyWidth, keyHeight);
			}
			break;
		case 14:
			x += 14.9f * keyInterval + 3 * keyInterval + 2 * context->KeyboardClusterGap;
			state = SetBackgroundColourByKey(keyStates, 0, state, graphics);
			DrawRectangle(graphics, x, y, 2 * keyInterval - spacing, keyHeight);
			state = SetBackgroundColourByKey(keyStates, 2, state, graphics);
			DrawRectangle(graphics, x + 2 * keyInterval, y, keyWidth, keyHeight);
			break;
	}

	goto DrawKeyText;
}

void DrawText(const struct AppContext* context, struct Graphics* graphics, const struct TextCache* text, int segment, int row, int key, uint32_t* keyState, [[maybe_unused]] bool multiline, double normalisedYOffset)
{
	if (keyState)
	{
		uint32_t newState;
		switch (segment)
		{
			case 0: newState = (context->KeyStates_Keyboard[row] >> (2 * key)) & 3; break;
			case 1: newState = (context->KeyStates_Navigation[row] >> (2 * key)) & 3; break;
			case 2: newState = (context->KeyStates_NumPad[row] >> (2 * key)) & 3; break;
			default: return;
		}
		if (newState != *keyState)
		{
			SetTextColourByState(newState, graphics);
			SetBackgroundColourByState(newState, graphics);
			*keyState = newState;
		}
	}

	double y = context->StartY + (row + normalisedYOffset + 0.5) * context->RowHeight;


	DrawGlyph(graphics, &text->Bitmap, (context->KeyTextXPositions[row * 16 + key] >> 6) + text->Left, y + text->Top);
}

void DrawKeyboard(struct AppContext* context)
{
	struct timespec clock1, clock2;
	clock_gettime(CLOCK_MONOTONIC, &clock1);

	if (context->Pool == NULL)
		return;

	struct Buffer* buffer = FindAvailableBuffer(context, true);
	if (buffer == NULL || buffer->Buffer == NULL)
		return;

	struct Graphics graphics;
	CreateGraphicsFromBuffer(buffer, context, &graphics);

	graphics.Background = RGBU32(61, 61, 61);
	Graphics_Clear(&graphics);
	Graphics_SetFont(&graphics, context->Font_CharKeys);

	DrawRow(&graphics, context, 0);
	DrawRow(&graphics, context, 1);
	DrawRow(&graphics, context, 2);
	DrawRow(&graphics, context, 3);
	DrawRow(&graphics, context, 4);

	if (context->KeyboardDisplayType > 0)
	{
		DrawRow(&graphics, context, 5);
		DrawRow(&graphics, context, 6);
		DrawRow(&graphics, context, 8);
		DrawRow(&graphics, context, 9);

		if (context->KeyboardDisplayType > 1)
		{
			DrawRow(&graphics, context, 10);
			DrawRow(&graphics, context, 11);
			DrawRow(&graphics, context, 12);
			DrawRow(&graphics, context, 13);
			DrawRow(&graphics, context, 14);
		}
	}


	clock_gettime(CLOCK_MONOTONIC, &clock2);

	unsigned long long dUsecs = (clock2.tv_sec - clock1.tv_sec) * 1'000'000 + (clock2.tv_nsec - clock1.tv_nsec) / 1000;
	printf("Render: %.1fms\n", (double)dUsecs * 0.001);

	wl_surface_attach(context->Surface, buffer->Buffer, 0, 0);
	struct wl_region* region = wl_compositor_create_region(context->Compositor);
	wl_region_add(region, 0, 0, context->Width, context->Height);
	wl_surface_damage(context->Surface, 0, 0, context->Width, context->Height);
	wl_surface_set_opaque_region(context->Surface, region);
	wl_surface_set_input_region(context->Surface, region);
	wl_region_destroy(region);
	wl_surface_commit(context->Surface);
}

void SetTextColourByState(uint32_t state, struct Graphics* graphics)
{
	switch (state)
	{
		case 0: graphics->Foreground = RGBU32(179, 179, 179); break;
		case 1: graphics->Foreground = RGBU32(214, 214, 214); break;
		case 2: 
		case 3: graphics->Foreground = RGBU32(255, 255, 255); break;
	}
}

void SetBackgroundColourByState(uint32_t state, struct Graphics* graphics)
{
	switch (state)
	{
		case 0: graphics->Background = RGBU32(36, 36, 36); break;
		case 1: graphics->Background = RGBU32(8, 8, 8); break;
		case 2: 
		case 3: graphics->Background = RGBU32(90, 90, 90); break;
	}
}

uint32_t SetTextColourByKey(uint32_t stateField, int key, [[maybe_unused]] uint32_t currentState, struct Graphics* graphics)
{
	stateField = (stateField >> (2 * key)) & 3;
	//if (stateField == currentState)
	//	return currentState;
	SetTextColourByState(stateField, graphics);
	return stateField;
}

uint32_t SetBackgroundColourByKey(uint32_t stateField, int key, [[maybe_unused]] uint32_t currentState, struct Graphics* graphics)
{
	stateField = (stateField >> (2 * key)) & 3;
	//if (stateField == currentState)
	//	return currentState;
	SetBackgroundColourByState(stateField, graphics);
	return stateField;
}


void registryGlobal(void *data,
		            struct wl_registry *registry,
		            uint32_t name,
		            const char *interface,
		            uint32_t version)
{
	printf("%s (version %u)\n", interface, version);
	struct AppContext* context = (struct AppContext*)data;

	if (strcmp(interface, zwp_virtual_keyboard_manager_v1_interface.name) == 0)
	{
		printf(" -- found virtual keyboard interface -- \n");
		context->VirtualKeyboardManager = (struct zwp_virtual_keyboard_manager_v1*) wl_registry_bind(registry, name, &zwp_virtual_keyboard_manager_v1_interface, 1);
	}
	else if (strcmp(interface, wl_compositor_interface.name) == 0)
		context->Compositor = (struct wl_compositor*) wl_registry_bind(registry, name, &wl_compositor_interface, 6);
	else if (strcmp(interface, wl_subcompositor_interface.name) == 0)
		context->Subcompositor = (struct wl_subcompositor*) wl_registry_bind(registry, name, &wl_subcompositor_interface, 1);
	else if (strcmp(interface, wl_seat_interface.name) == 0)
	{
		if (context->Seat != NULL)
			fprintf(stderr, "Seat has already been assigned.\n");
		else
			context->Seat = (struct wl_seat*) wl_registry_bind(registry, name, &wl_seat_interface, 5);
	}
	else if (strcmp(interface, wl_shm_interface.name) == 0)
		context->SharedMemory = (struct wl_shm*) wl_registry_bind(registry, name, &wl_shm_interface, 1);
	else if (strcmp(interface, wl_output_interface.name) == 0)
	{
		printf("binding output\n");
		for (struct Monitor * pOutput = &context->Outputs[0]; pOutput < &context->Outputs[8]; ++pOutput)
		{
			if (pOutput->Handle == NULL)
			{
				pOutput->App = context;
				pOutput->Index = (int) (pOutput - &context->Outputs[0]);
				pOutput->Handle = (struct wl_output*) wl_registry_bind(registry, name, &wl_output_interface, 4);
				return;
			}
		}
		fprintf(stderr, "Too many outputs detected; only the first 8 will be considered.\n");
	}
	else if (strcmp(interface, xdg_wm_base_interface.name) == 0)
		context->Xdg = (struct xdg_wm_base*) wl_registry_bind(registry, name, &xdg_wm_base_interface, 5);
	else if (strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0)
		context->Shell = (struct zwlr_layer_shell_v1*) wl_registry_bind(registry, name, &zwlr_layer_shell_v1_interface, LAYER_SHELL_VERSION);
	else if (strcmp(interface, zwp_input_method_manager_v2_interface.name) == 0)
		context->InputMethodManager = (struct zwp_input_method_manager_v2*) wl_registry_bind(registry, name, &zwp_input_method_manager_v2_interface, 1);
}

void registryRemove([[maybe_unused]] void *data,
			        [[maybe_unused]] struct wl_registry *wl_registry,
			        [[maybe_unused]] uint32_t name)
{
	printf("global %u removed\n", name);
}

void shmFormat([[maybe_unused]] void *data,
		       [[maybe_unused]] struct wl_shm *wl_shm,
		       [[maybe_unused]] uint32_t format)
{
	struct AppContext* context = (struct AppContext*) data;
	printf("pixel format %u\n", format);
	if (format == WL_SHM_FORMAT_ARGB8888)
		context->ShmValidFormatAvailable |= true;
}

void seatCapabilities([[maybe_unused]] void *data,
			          [[maybe_unused]] struct wl_seat *wl_seat,
			          [[maybe_unused]] uint32_t capabilities)
{
	struct AppContext* context = (struct AppContext*) data;
	context->SeatCapabilities = capabilities;
	printf("seat capabilities %x\n", capabilities);
}

void seatName([[maybe_unused]] void *data,
		      [[maybe_unused]] struct wl_seat *wl_seat,
		      [[maybe_unused]] const char *name)
{
	printf("seat name: %s\n", name);
}

void outputDescription([[maybe_unused]] void *data,
			           [[maybe_unused]] struct wl_output *wl_output,
			           [[maybe_unused]] const char *description)
{
	printf("output description: '%s'\n", description);
	struct Monitor* monitor = (struct Monitor*) data;
	struct AppContext* context = monitor->App;
	if (context->PreferredOutputName == NULL)
		return;

	printf("Checking again preferred output description '%s'\n", context->PreferredOutputName);
	if (strcmp(description, context->PreferredOutputName) == 0)
	{
		printf("Matched! Assigning output %d as target output.\n", monitor->Index);
		context->TargetOutputIndex = monitor->Index;
	}
}

void outputGeometry([[maybe_unused]] void *data,
		            [[maybe_unused]] struct wl_output *wl_output,
		            [[maybe_unused]] int32_t x,
		            [[maybe_unused]] int32_t y,
		            [[maybe_unused]] int32_t physical_width,
		            [[maybe_unused]] int32_t physical_height,
		            [[maybe_unused]] int32_t subpixel,
		            [[maybe_unused]] const char *make,
		            [[maybe_unused]] const char *model,
		            [[maybe_unused]] int32_t transform)
{
	printf("output geometry: %dx%d @ (%d, %d) - transform %d, subpixel %d\n", physical_width, physical_height, x, y, transform, subpixel);
	printf("                 make: %s\n", make);
	printf("                 model: %s\n", model);
}

void outputMode([[maybe_unused]] void *data,
		        [[maybe_unused]] struct wl_output *wl_output,
		        [[maybe_unused]] uint32_t flags,
		        [[maybe_unused]] int32_t width,
		        [[maybe_unused]] int32_t height,
		        [[maybe_unused]] int32_t refresh)
{
	printf("output mode: %dx%d@%d (%xh)\n", width, height, refresh, flags);
}

void outputDone([[maybe_unused]] void *data,
		        [[maybe_unused]] struct wl_output *wl_output)
{
	printf("output done\n");
}

void outputScale([[maybe_unused]] void *data,
		         [[maybe_unused]] struct wl_output *wl_output,
		         [[maybe_unused]] int32_t factor)
{
	printf("output scale: %d\n", factor);
}

void outputName([[maybe_unused]] void *data,
		        [[maybe_unused]] struct wl_output *wl_output,
		        [[maybe_unused]] const char *name)
{
	printf("output name: %s\n", name);
}


void layerConfigure([[maybe_unused]] void *data,
		            [[maybe_unused]] struct zwlr_layer_surface_v1 *surface,
		            [[maybe_unused]] uint32_t serial,
		            [[maybe_unused]] uint32_t width,
		            [[maybe_unused]] uint32_t height)
{
	printf("layerConfigure\n");
	UpdateSize( (struct AppContext*) data, width, height, serial);
}

void UpdateSize(struct AppContext* context, uint32_t width, uint32_t height, uint32_t serial)
{
	// Set the key size based on the height, then set the displayed keyboard type based on the width
	// e.g. width := 1000, height:=500
	// padding    := 8
	// keySpacing := 5
	// usableHeight = 500 - 2*8 = 484
	// usableWidth = 1000 - 2*8 = 984
	// rowSize = (484 + 5) / 5 = 97.8 
	// keySize = 97.8 * 1.1 = 107.58 
	// requiredWidth = 107.58 * 14.9 - 5 = 1597.942 
	// expectedKeys = (984 + 5) / 107.58 = 9.193
	// Can only fit 9.2 keys wide at this aspect ratio
	// This is not enough, so we set the key size based on the width
	// keySize = (984 + 5) / 14.9 = 66.376 
	// Should we keep the original 1.1 ratio keys?
	// rowSize = 66.376 / 1.1 = 60.34
	// usableHeight = 60.34 * 5 - 5 = 296.7
	// height = 296.7 + 2 * 8 = 312.7 (313)
	printf("UpdateSize(width = %u, height=%u)\n", width, height);
	context->Width = width;
	context->Height = height;
	context->Stride = ((width + 63) & ~63) * 4;

	double padding = 8;
	const double keySpacing = context->KeySpacing;
	double usableHeight = height - 2 * padding;
	double usableWidth  = width  - 2 * padding;
	double rowSize = (usableHeight + keySpacing) / 5.0;
	double keySize = rowSize * 1.1;
	double expectedKeys = (usableWidth + keySpacing) / keySize;

	printf("Total usable keyboard width is %.1fpx.\n", usableWidth);
	printf("Standard key size = %.1fpx\n", keySize);
	printf("Can fit %.2f standard keys (14.9 required for standard keyboard, 22.9 for full)\n", expectedKeys);
	if (expectedKeys < 14.9 * 0.9)
	{
		printf("Not enough space for current height. Resizing...\n");
		// available width is too small. make keys smaller to fit
		keySize = (usableWidth + keySpacing) / 14.9;
		rowSize = keySize / 1.1;
		usableHeight = rowSize * 5 - keySpacing;
		context->Height = ceil(usableHeight + 2 * padding);
		context->WaitingForConfigure = true;
		if (serial != 0)
		{
			zwlr_layer_surface_v1_set_size(context->Layer, context->Width, context->Height);
			zwlr_layer_surface_v1_set_exclusive_zone(context->Layer, context->Height);
			zwlr_layer_surface_v1_ack_configure(context->Layer, serial);
			wl_surface_commit(context->Surface);
		}
		return;
	}

	double requiredWidth = keySize * 14.9 - keySpacing;
	double extraClusterSpace = 0.0; // extra spacing between the keyboard and navigation keys, and between the navigation keys and numpad
	int displayType = 0; // 0 = keyboard only, 1 = no numpad, 2 = full
	if (expectedKeys > 14.9 * 1.1) // we have plenty of room left after fitting the regular keyboard
	{
		if (expectedKeys < 17.9) // but not enough room for navigation keys
			padding = (width - requiredWidth) * 0.5; // so just centre it
		else if (expectedKeys > 21.9) // plenty of space for the numpad and navigation keys
		{
			requiredWidth = usableWidth;
			displayType = 2;
			extraClusterSpace = (usableWidth - keySize * 21.9) / 2.0;
		}
		else
		{
			displayType = 1;
			extraClusterSpace = (usableWidth - keySize * 14.9);
		}
	}
	else
		keySize = (usableWidth + keySpacing) / 14.9;

	context->WaitingForConfigure = false;
	printf("Standard keyboard to consume %.1fpx\n", 14.9 * keySize);
	if (displayType > 0)
		printf("Navigation keys to consume %.1fpx\n", 2 * keySize);
	if (displayType > 1)
		printf("Numpad to consume %.1fpx\n", 4 * keySize);
	printf("%fpx of padding between clusters\n", extraClusterSpace);
	printf("Total width: %.1fpx\n", displayType == 0 ? (14.9 * keySize) : displayType == 1 ? (17.9 * keySize + extraClusterSpace) : (21.9 * keySize + 2 * extraClusterSpace));

	if (displayType == 0 && context->KeyboardDisplayType != 0)
		memcpy(&context->KeyCodes[4*16], &KEYCODES[KEYCODES_COMPACTOFFSET], 16);
	else if (displayType != 0 && context->KeyboardDisplayType == 0)
		memcpy(&context->KeyCodes[4*16], &KEYCODES[4*16], 16);

	context->KeyboardDisplayType = displayType;
	context->StandardKeyWidth    = keySize;
	context->KeyboardClusterGap  = extraClusterSpace;
	context->StartX              = padding;
	context->StartY              = 8;
	context->RowHeight           = rowSize;

	PrecalculateKeyGeometry(context);


	// recreate shm buffers
	DestroyBuffer(&context->Buffers[0]);
	//DestroyBuffer(&context->Buffers[1]);
	if (context->Pool != NULL)
		wl_shm_pool_destroy(context->Pool);
	if (context->ShmMap != NULL)
		munmap(context->ShmMap, context->BufferSize);

	context->Pool = NULL;
	context->ShmMap = NULL;

	int shmfd = NewShmFile();
	if (shmfd <= 0)
	{
		perror("failed to create shm file");
		return;
	}
	context->BufferSize =  context->Stride * context->Height;
	if (ftruncate(shmfd, context->BufferSize * 2) < 0)
	{
		perror("failed to set shm size");
		goto Cleanup;
	}
	context->ShmMap = mmap(0, context->BufferSize * 2, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
	if (context->ShmMap == MAP_FAILED)
	{
		perror("failed to mmap shm");
		goto Cleanup;
	}
	context->Pool = wl_shm_create_pool(context->SharedMemory, shmfd, context->BufferSize * 2);
	if (context->Pool == NULL)
	{
		perror("failed to create pool from shared memory");
		goto Cleanup;
	}
	CreateBuffer(&context->Buffers[0], 0, context);
	//CreateBuffer(&context->Buffers[1], 1, context);

	//cairo_font_extents_t extents;
	//cairo_font_extents(context->Buffers[0].Cairo, &extents);
	context->TextHeight = 18;
	close(shmfd);
	zwlr_layer_surface_v1_ack_configure(context->Layer, serial);
	return;

Cleanup:
	close(shmfd);
	return;
}

int NewShmFile()
{
	for (int retries = 10; retries > 0; --retries)
	{
		char name[64];
		snprintf(name, 64, "wl_shm_%d_%u", getpid(), rand());
		int fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 6<<6);
		if (fd >= 0)
		{
			shm_unlink(name);
			return fd;
		}
	}
	return -1;
}

void DestroyBuffer(struct Buffer* buffer)
{
	if (buffer->Buffer)
		wl_buffer_destroy(buffer->Buffer);
	//if (buffer->Image)
	//	cairo_surface_destroy(buffer->Image);
	//if (buffer->Cairo)
	//	cairo_destroy(buffer->Cairo);
	buffer->Buffer = NULL;
	//buffer->Image = NULL;
	//buffer->Cairo = NULL;
}

void CreateBuffer(struct Buffer* buffer, int index, struct AppContext* context)
{
	printf("Creating image buffer of (%d,%d) from shm pool\n", context->Width, context->Height);
	buffer->Buffer  = wl_shm_pool_create_buffer(context->Pool, index * context->BufferSize, context->Width, context->Height, context->Stride, WL_SHM_FORMAT_ARGB8888);
	buffer->Data    = (void*)(char*)context->ShmMap + (index * context->BufferSize);
	//buffer->Image   = cairo_image_surface_create_for_data(buffer->Data, CAIRO_FORMAT_ARGB32, context->Width, context->Height, context->Stride);
	//buffer->Cairo   = cairo_create(buffer->Image);
	buffer->Width   = context->Width;
	buffer->Height  = context->Height;
	buffer->Stride  = context->Stride;
	buffer->Size    = context->BufferSize;
	buffer->Busy    = false;
	buffer->Context = context;

	//cairo_font_options_t* options = cairo_font_options_create();
	//cairo_get_font_options(buffer->Cairo, options);
	//cairo_font_options_set_antialias(options, CAIRO_ANTIALIAS_FAST);
	//cairo_font_options_set_hint_style(options, CAIRO_HINT_STYLE_NONE);
	//cairo_font_options_set_hint_metrics(options, CAIRO_HINT_METRICS_OFF);
	//cairo_set_font_options(buffer->Cairo, options);
	//cairo_font_options_destroy(options);

	//cairo_set_font_size(buffer->Cairo, context->RowHeight * 0.4);

	wl_buffer_add_listener(buffer->Buffer, &s_bufferListener, buffer);
}

struct Buffer* FindAvailableBuffer(struct AppContext* context, bool makeUnavailable)
{
	for (int i = 0; i < 1; ++i)
	{
		if (context->Buffers[i].Pending)
			continue;

		if (makeUnavailable)
			context->Buffers[i].Busy = true;
		return &context->Buffers[i];
	}
	return NULL;
}

void CommitBuffer(struct Buffer* buffer)
{
	buffer->Pending = true;
}

void layerClosed([[maybe_unused]] void *data,
		         [[maybe_unused]] struct zwlr_layer_surface_v1 *zwlr_layer_surface_v1)
{
}

void bufferReleased([[maybe_unused]] void *data,
		            [[maybe_unused]] struct wl_buffer *wl_buffer)
{
	struct Buffer* buffer = (struct Buffer*) data;
	buffer->Busy = false;
	buffer->Pending = false;
}

static inline Modifiers_t GetModifierFlagForKey(KeyIndex key)
{
	if (key.cluster != 0)
		return 0;
	switch (key.raw & 0x7FF)
	{
		case MakeRawIndex(0, 2, 0):  return MOD_CAPSLOCK_PRESSED;
		case MakeRawIndex(0, 3, 0):  return MOD_LSHIFT;
		case MakeRawIndex(0, 3, 11): return MOD_RSHIFT;
		case MakeRawIndex(0, 4, 0):  return MOD_LCTRL;
		case MakeRawIndex(0, 4, 1):  return MOD_FN;
		case MakeRawIndex(0, 4, 2):  return MOD_LSUPER;
		case MakeRawIndex(0, 4, 3):  return MOD_LALT;
		case MakeRawIndex(0, 4, 5):  return MOD_RALT;
		case MakeRawIndex(0, 4, 6):  return MOD_RSUPER;
		case MakeRawIndex(0, 4, 8):  return MOD_RCTRL;
		case MakeRawIndex(2, 0, 0):  return MOD_NUMLOCK_PRESSED;
		default:                     return 0;
	}
};

static inline bool IsModifierKey(KeyIndex key)
{
	return GetModifierFlagForKey(key) != 0;
};

void ClearHighlightFlag(struct AppContext* context, uint32_t flag, KeyIndex index, bool deferRedraw)
{
	//printf("ClearHighlightFlag( (%d,%d,%d) -> %u )\n", index.cluster, index.row, index.key, flag);
	uint32_t newHighlight;
	const int keyShift = index.key << 1;
	const uint32_t bitTest = flag << keyShift;
	switch (index.cluster)
	{
		case 0: // regular keyboard
		{
			uint32_t value = context->KeyStates_Keyboard[index.row];
			if ((value & bitTest) == 0)
				return;
		
			value &= ~bitTest;
			newHighlight = value;
			context->KeyStates_Keyboard[index.row] = value;
			//wl_surface_damage_buffer(context->Surface, x, context->StartY + context->RowHeight * row, width, context->RowHeight - context->KeySpacing);
			//wl_surface_commit(context->Surface);
		
		} break;
		case 1:
		{
			if (index.key > 2)
				return;
			uint32_t value = context->KeyStates_Navigation[index.row];
			if ((value & bitTest) == 0)
				return;
			value &= ~bitTest;
			newHighlight = value;
			context->KeyStates_Navigation[index.row] = value;
		
		} break;
		case 2:
		{
			if (index.key > 3)
				return;
			uint32_t value = context->KeyStates_NumPad[index.row];
			if ((value & bitTest) == 0)
				return;
			value &= ~bitTest;
			newHighlight = value;
			context->KeyStates_NumPad[index.row] = value;
		} break;
		default:
		return;
	}

	if (!deferRedraw)
	{
		//RedrawKey(context, index);
		//DrawKeyboard(context);
		InvalidateKey(context, index, (newHighlight >> (index.key * 2)) & 3);
	}
};

void SetHighlightFlag(struct AppContext* context, uint32_t flag, KeyIndex index, bool deferRedraw)
{
	int keyShift = index.key << 1;
	uint32_t bitTest = flag << keyShift;
	uint32_t newHighlight;
	switch (index.cluster)
	{
		case 0: // regular keyboard
		{
			uint32_t value = context->KeyStates_Keyboard[index.row];
			if (value & bitTest)
				return;
		
			value |= bitTest;
			newHighlight = value;
			context->KeyStates_Keyboard[index.row] = value;
			//wl_surface_damage_buffer(context->Surface, x, context->StartY + context->RowHeight * row, width, context->RowHeight - context->KeySpacing);
			//wl_surface_commit(context->Surface);
		
		} break;
		case 1:
		{
			if (index.key > 2)
				return;
			uint32_t value = context->KeyStates_Navigation[index.row];
			if (value & bitTest)
				return;
			value |= bitTest;
			newHighlight = value;
			context->KeyStates_Navigation[index.row] = value;
		
		} break;
		case 2:
		{
			if (index.key > 3)
				return;
			uint32_t value = context->KeyStates_NumPad[index.row];
			if (value & bitTest)
				return;
			value |= bitTest;
			newHighlight = value;
			context->KeyStates_NumPad[index.row] = value;
		} break;
		default:
		return;
	}


	if (!deferRedraw)
	{
		//RedrawKey(context, index);
		//DrawKeyboard(context);
		InvalidateKey(context, index, (newHighlight >> (index.key * 2) & 3));
	}
};

void SendKey(struct AppContext* context, KeyIndex index, int state, uint32_t time)
{
	auto keycode = context->KeyCodes[CHARINDEXFORKEYINDEX(index)];
	//printf("Sending keycode %u for key %u.%u.%u (index %u)\n", keycode, index.cluster, index.row, index.key, CHARINDEXFORKEYINDEX(index));
	if (!keycode)
		return;

	zwp_virtual_keyboard_v1_key(context->VirtualKeyboard, time, keycode, state);
	switch (keycode)
	{
		case KEY_CAPSLOCK:
			//printf("caps lock\n");
			xkb_state_update_key(context->Xkb.State, KEY_CAPSLOCK+8, state == WL_KEYBOARD_KEY_STATE_PRESSED ? XKB_KEY_DOWN : XKB_KEY_UP);
			goto UpdateState;
		case KEY_LEFTSHIFT:
			//printf("left shift\n");
			xkb_state_update_key(context->Xkb.State, KEY_LEFTSHIFT+8, state == WL_KEYBOARD_KEY_STATE_PRESSED ? XKB_KEY_DOWN : XKB_KEY_UP);
			goto UpdateState;
		case KEY_LEFTCTRL:
			//printf("left control\n");
			xkb_state_update_key(context->Xkb.State, KEY_LEFTCTRL+8, state == WL_KEYBOARD_KEY_STATE_PRESSED ? XKB_KEY_DOWN : XKB_KEY_UP);
			goto UpdateState;
		case KEY_LEFTMETA:
			//printf("left meta\n");
			xkb_state_update_key(context->Xkb.State, KEY_LEFTMETA+8, state == WL_KEYBOARD_KEY_STATE_PRESSED ? XKB_KEY_DOWN : XKB_KEY_UP);
			goto UpdateState;
		case KEY_LEFTALT:
			xkb_state_update_key(context->Xkb.State, KEY_LEFTALT+8, state == WL_KEYBOARD_KEY_STATE_PRESSED ? XKB_KEY_DOWN : XKB_KEY_UP);
			goto UpdateState;
		case KEY_RIGHTALT:
			xkb_state_update_key(context->Xkb.State, KEY_RIGHTALT+8, state == WL_KEYBOARD_KEY_STATE_PRESSED ? XKB_KEY_DOWN : XKB_KEY_UP);
			goto UpdateState;
		case KEY_RIGHTMETA:
			xkb_state_update_key(context->Xkb.State, KEY_RIGHTMETA+8, state == WL_KEYBOARD_KEY_STATE_PRESSED ? XKB_KEY_DOWN : XKB_KEY_UP);
			goto UpdateState;
		case KEY_RIGHTCTRL:
			xkb_state_update_key(context->Xkb.State, KEY_RIGHTCTRL+8, state == WL_KEYBOARD_KEY_STATE_PRESSED ? XKB_KEY_DOWN : XKB_KEY_UP);
			goto UpdateState;
		case KEY_NUMLOCK:
			xkb_state_update_key(context->Xkb.State, KEY_NUMLOCK+8, state == WL_KEYBOARD_KEY_STATE_PRESSED ? XKB_KEY_DOWN : XKB_KEY_UP);
			goto UpdateState;
	}

	return;

UpdateState:
	{
		xkb_mod_mask_t latched, depressed, locked;
		xkb_layout_index_t group;
		latched   = xkb_state_serialize_mods(context->Xkb.State, XKB_STATE_MODS_LATCHED);
		depressed = xkb_state_serialize_mods(context->Xkb.State, XKB_STATE_MODS_DEPRESSED);
		locked    = xkb_state_serialize_mods(context->Xkb.State, XKB_STATE_MODS_LOCKED);
		group     = xkb_state_serialize_group(context->Xkb.State, XKB_STATE_EFFECTIVE);
		printf("Updating modifier state: latched=%x, depressed=%x, locked=%x, group=%x\n", latched, depressed, locked, group);
		zwp_virtual_keyboard_v1_modifiers(context->VirtualKeyboard, depressed, latched, locked, group);
	}
};


KeyIndex GetKeyAtLocation(float x, float y, const struct AppContext* context, bool touch)
{
	x -= context->StartX;
	y -= context->StartY;
	if (touch)
	{
		x += (float)context->KeySpacing / 2;
		y += (float)context->KeySpacing / 2;
	}
	if (x < 0 || y < 0)
		return KEYINDEX_INVALID;
	y /= context->RowHeight;

	const double ySpaceFactor = touch ? 0 : context->KeySpacing / context->RowHeight;
	const double xSpaceFactor = touch ? 0 : context->KeySpacing / context->StandardKeyWidth;
	if (y > 5 - ySpaceFactor)
		return KEYINDEX_INVALID;

	//printf("normalised x=%.2f, y=%.2f\n", x/context->StandardKeyWidth, y);
	
	int segment = 0;
	if (x >= 14.9 * context->StandardKeyWidth)
	{
		x -= 14.9 * context->StandardKeyWidth + context->KeyboardClusterGap;
		if (x < 0 || context->KeyboardDisplayType < 1)
			return KEYINDEX_INVALID;
		++segment;

		if (x >= 3 * context->StandardKeyWidth)
		{
			x -= 3 * context->StandardKeyWidth + context->KeyboardClusterGap;
			if (x < 0 || context->KeyboardDisplayType < 2)
				return KEYINDEX_INVALID;
			++segment;

			if (x >= 4 * context->StandardKeyWidth)
				return KEYINDEX_INVALID;
		}
	}

	x /= context->StandardKeyWidth;
	KeyIndex index = { .cluster = segment, .row = (int)y, .key = (int)x };
	switch (segment)
	{
		case 0:
		{
			y -= index.row;
			if (y < 0 || y > 1 - ySpaceFactor)
				return KEYINDEX_INVALID;
			const float* widths = &KEYWIDTHS[16 * index.row];
			const size_t nWidths = KEYSPERROW[index.row];

			for (size_t i = 0; i < nWidths; ++i)
			{
				if (x <= widths[i] - xSpaceFactor)
				{
					index.key = i;
					return index;
				}
				x -= widths[i];
				if (x < 0)
					return KEYINDEX_INVALID;
			}
		} break;
		case 1:
		{
			if (index.row == 2)
				return KEYINDEX_INVALID;
			if (index.row == 3)
			{
				if (x < 1 || x > 2 - xSpaceFactor)
					return KEYINDEX_INVALID;
				index.key = 1;
				return index;
			}
			x -= index.key;
			if (x < 0 || x > 1 - xSpaceFactor)
				return KEYINDEX_INVALID;
			y -= index.row;
			if (y < 0 || y > 1 - ySpaceFactor)
				return KEYINDEX_INVALID;
			return index;
		}
		case 2:
		{
			if (x >= 3)
			{
				if (x > 4 - xSpaceFactor)
					return KEYINDEX_INVALID;
				if (y >= 1 && y <= 3 - ySpaceFactor)
				{
					index.row = 1;
					return index;
				}
				else if (y >= 3 && y <= 5 - ySpaceFactor)
				{
					index.row = 3;
					return index;
				}
			}
			else if (y >= 4)
			{
				if (x >= 0 && x <= 2 - xSpaceFactor)
				{
					index.key = 0;
					return index;
				}
				if (x >= 2 && x <= 3 - xSpaceFactor)
				{
					index.key = 1;
					return index;
				}
			}
			x -= index.key;
			if (x < 0 || x > 1 - xSpaceFactor)
				return KEYINDEX_INVALID;
			y -= index.row;
			if (y < 0 || y > 1 - ySpaceFactor)
				return KEYINDEX_INVALID;
			return index;
		}
	}

	return KEYINDEX_INVALID;
}

#if defined(KEYLISTENER) && KEYLISTENER
void inputMethodActivate(       [[maybe_unused]] void *data,
		                        [[maybe_unused]] struct zwp_input_method_v2 *zwp_input_method_v2) {}
void inputMethodDeactivate(     [[maybe_unused]] void *data,
		                        [[maybe_unused]] struct zwp_input_method_v2 *zwp_input_method_v2){}
void inputMethodSurroundingText([[maybe_unused]] void *data,
			                    [[maybe_unused]] struct zwp_input_method_v2 *zwp_input_method_v2,
			                    [[maybe_unused]] const char *text,
			                    [[maybe_unused]] uint32_t cursor,
			                    [[maybe_unused]] uint32_t anchor){}
void inputMethodTextChangeCause([[maybe_unused]] void *data,
			                    [[maybe_unused]] struct zwp_input_method_v2 *zwp_input_method_v2,
			                    [[maybe_unused]] uint32_t cause){}
void inputMethodContentType(    [[maybe_unused]] void *data,
			                    [[maybe_unused]] struct zwp_input_method_v2 *zwp_input_method_v2,
			                    [[maybe_unused]] uint32_t hint,
			                    [[maybe_unused]] uint32_t purpose){}
void inputMethodDone(           [[maybe_unused]] void *data,
		                        [[maybe_unused]] struct zwp_input_method_v2 *zwp_input_method_v2){}
void inputMethodUnavailable(    [[maybe_unused]] void *data,
			                    [[maybe_unused]] struct zwp_input_method_v2 *zwp_input_method_v2){}
void inputGrabKeymap(    [[maybe_unused]] void *data,
		                 [[maybe_unused]] struct zwp_input_method_keyboard_grab_v2 *zwp_input_method_keyboard_grab_v2,
		                 [[maybe_unused]] uint32_t format,
		                 [[maybe_unused]] int32_t fd,
		                 [[maybe_unused]] uint32_t size)
{
	printf("inputGrabKeymap(format=%u, fd=%d, size=%u)\n", format, fd, size);
}
void inputGrabKey(       [[maybe_unused]] void *data,
		                 [[maybe_unused]] struct zwp_input_method_keyboard_grab_v2 *zwp_input_method_keyboard_grab_v2,
		                 [[maybe_unused]] uint32_t serial,
		                 [[maybe_unused]] uint32_t time,
		                 [[maybe_unused]] uint32_t key,
		                 [[maybe_unused]] uint32_t state)
{
	printf("inputGrabKey(key=%xh, state=%d)\n", key, state);
}
void inputGrabModifiers( [[maybe_unused]] void *data,
		                 [[maybe_unused]] struct zwp_input_method_keyboard_grab_v2 *zwp_input_method_keyboard_grab_v2,
		                 [[maybe_unused]] uint32_t serial,
		                 [[maybe_unused]] uint32_t mods_depressed,
		                 [[maybe_unused]] uint32_t mods_latched,
		                 [[maybe_unused]] uint32_t mods_locked,
		                 [[maybe_unused]] uint32_t group)
{
	printf("inputGrabModifiers(depressed=%xh, latched=%xh, locked=%xh, group=%xh)\n", mods_depressed, mods_latched, mods_locked, group);
}
void inputGrabRepeatInfo([[maybe_unused]] void *data,
			             [[maybe_unused]] struct zwp_input_method_keyboard_grab_v2 *zwp_input_method_keyboard_grab_v2,
			             [[maybe_unused]] int32_t rate,
			             [[maybe_unused]] int32_t delay){}
#endif

struct wl_registry_listener s_registryListener = {
	.global = registryGlobal,
	.global_remove = registryRemove
};

struct wl_output_listener s_outputListener = {
	.description = outputDescription,
	.done = outputDone,
	.geometry = outputGeometry,
	.mode = outputMode,
	.name = outputName,
	.scale = outputScale
};

struct wl_seat_listener s_seatListener = {
	.capabilities = seatCapabilities,
	.name = seatName
};

struct wl_shm_listener s_shmListener = {
	.format = shmFormat
};

struct zwlr_layer_surface_v1_listener s_layerSurfaceListener = {
	.closed = layerClosed,
	.configure = layerConfigure
};

struct wl_buffer_listener s_bufferListener = {
	.release = bufferReleased
};

void xdgSurfaceConfigure([[maybe_unused]] void *data,
		                 [[maybe_unused]] struct xdg_surface *xdg_surface,
		                 [[maybe_unused]] uint32_t serial)
{
	xdg_surface_ack_configure(xdg_surface, serial);
}

struct xdg_surface_listener s_xdgSurfaceListener = {
	.configure = xdgSurfaceConfigure
};

void toplevelConfigure([[maybe_unused]] void *data,
			           [[maybe_unused]] struct xdg_toplevel *xdg_toplevel,
			           [[maybe_unused]] int32_t width,
			           [[maybe_unused]] int32_t height,
			           [[maybe_unused]] struct wl_array *states)
{
	printf("toplevelConfigure\n");
	//struct AppContext* context = (struct AppContext*) data;
	//UpdateSize(context, width, height);
}

void toplevelClose([[maybe_unused]] void *data,
		           [[maybe_unused]] struct xdg_toplevel *xdg_toplevel)
{
	((struct AppContext*)data)->Running = false;
}

void toplevelConfigureBounds([[maybe_unused]] void *data,
				             [[maybe_unused]] struct xdg_toplevel *xdg_toplevel,
				             [[maybe_unused]] int32_t width,
				             [[maybe_unused]] int32_t height) 
{
	UpdateSize((struct AppContext*)data, width, 360, 0);
}

void toplevelWmCapabilities([[maybe_unused]] void *data,
				            [[maybe_unused]] struct xdg_toplevel *xdg_toplevel,
				            [[maybe_unused]] struct wl_array *capabilities) {}

struct xdg_toplevel_listener s_toplevelListener = {
	.configure = toplevelConfigure,
	.close = toplevelClose,
	.configure_bounds = toplevelConfigureBounds,
	.wm_capabilities = toplevelWmCapabilities
};

#if defined(KEYLISTENER) && KEYLISTENER
struct zwp_input_method_v2_listener s_inputMethodListener = {
	.activate           = inputMethodActivate,
	.deactivate         = inputMethodDeactivate,
	.surrounding_text   = inputMethodSurroundingText,
	.text_change_cause  = inputMethodTextChangeCause,
	.content_type       = inputMethodContentType,
	.done               = inputMethodDone,
	.unavailable        = inputMethodUnavailable
};

struct zwp_input_method_keyboard_grab_v2_listener s_inputGrabListener = {
	.keymap       = inputGrabKeymap,
	.key          = inputGrabKey,
	.modifiers    = inputGrabModifiers,
	.repeat_info  = inputGrabRepeatInfo 
};
#endif

#ifndef _VIRTUALKEYBOARD_C_
#include "virtualKeyboard.c"
#endif
#ifndef _POINTER_C_
#include "pointer.c"
#endif
#ifndef _TOUCH_C_
#include "touch.c"
#endif
#ifndef _GRAPHICS_C_
#include "graphics.c"
#endif
#ifndef _QUEUE_C_
#include "queue.c"
#endif
