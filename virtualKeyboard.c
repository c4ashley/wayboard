#define _VIRTUALKEYBOARD_C_
#ifndef _MAIN_C_
#include "main.c"
#endif
void keyboardKeymap(void *data,
		       [[maybe_unused]] struct wl_keyboard *wl_keyboard,
		       uint32_t format,
		       int32_t fd,
		       uint32_t size)
{
	struct AppContext* context = (struct AppContext*) data;

	if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1)
	{
		fprintf(stderr, "only xkbv1 format is supported\n");
		exit(1);
	}

	if (context->Xkb.Keymap)
	{
		xkb_state_unref(context->Xkb.State);
		xkb_keymap_unref(context->Xkb.Keymap);
		munmap(context->Xkb.Data, context->Xkb.cbData);
	}

	context->Xkb.Data = mmap(0, size, PROT_READ, MAP_PRIVATE, fd, 0);
	context->Xkb.cbData = size;
	context->Xkb.Keymap = xkb_keymap_new_from_string(context->Xkb.Context, context->Xkb.Data, format, 0);
	context->Xkb.State = xkb_state_new(context->Xkb.Keymap);

	zwp_virtual_keyboard_v1_keymap(context->VirtualKeyboard, format, fd, size);
};

void keyboardEnter([[maybe_unused]] void *data,
		           [[maybe_unused]] struct wl_keyboard *wl_keyboard,
		           [[maybe_unused]] uint32_t serial,
		           [[maybe_unused]] struct wl_surface *surface,
		           [[maybe_unused]] struct wl_array *keys) {}

void keyboardLeave([[maybe_unused]] void *data,
		           [[maybe_unused]] struct wl_keyboard *wl_keyboard,
		           [[maybe_unused]] uint32_t serial,
		           [[maybe_unused]] struct wl_surface *surface) {}

void keyboardKey([[maybe_unused]] void *data,
		         [[maybe_unused]] struct wl_keyboard *wl_keyboard,
		         [[maybe_unused]] uint32_t serial,
		         [[maybe_unused]] uint32_t time,
		         [[maybe_unused]] uint32_t key,
		         [[maybe_unused]] uint32_t state) {
printf("%s(%d, %d)\n", __func__, key, state);
};

void keyboardModifiers([[maybe_unused]] void *data,
			           [[maybe_unused]] struct wl_keyboard *wl_keyboard,
			           [[maybe_unused]] uint32_t serial,
			           [[maybe_unused]] uint32_t mods_depressed,
			           [[maybe_unused]] uint32_t mods_latched,
			           [[maybe_unused]] uint32_t mods_locked,
			           [[maybe_unused]] uint32_t group) {};

void keyboardRepeatInfo([[maybe_unused]] void *data,
			            [[maybe_unused]] struct wl_keyboard *wl_keyboard,
			            [[maybe_unused]] int32_t rate,
			            [[maybe_unused]] int32_t delay) {};

struct wl_keyboard_listener s_keyboardListener = {
	.keymap      = keyboardKeymap,
	.enter       = keyboardEnter,
	.leave       = keyboardLeave,
	.key         = keyboardKey,
	.modifiers   = keyboardModifiers,
	.repeat_info = keyboardRepeatInfo
};
