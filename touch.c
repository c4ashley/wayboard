#define _TOUCH_C_
#ifndef _MAIN_C_
#include "main.c"
#endif

void touchDown([[maybe_unused]] void *data,
		       [[maybe_unused]] struct wl_touch *wl_touch,
		       [[maybe_unused]] uint32_t serial,
		       [[maybe_unused]] uint32_t time,
		       [[maybe_unused]] struct wl_surface *surface,
		       [[maybe_unused]] int32_t id,
		       [[maybe_unused]] wl_fixed_t x,
		       [[maybe_unused]] wl_fixed_t y)
{
	printf("%s(%d, %d, %d)\n", __func__, id, x, y);
	if (id >= 10)
		return;
	struct AppContext* context = (struct AppContext*) data;
	double xd = wl_fixed_to_double(x);
	double yd = wl_fixed_to_double(y);
	KeyIndex index = GetKeyAtLocation(xd, yd, context, true);

	context->TouchState[id] = (struct Touch) {
		.IsDown   = true,
		.Initial  = { xd, yd },
		.Current  = { xd, yd },
		.Key      = index
	};

	if (KeyIndexIsValid(index))
	{
		SendKey(context, index, WL_KEYBOARD_KEY_STATE_PRESSED, 0u);
		SetHighlightFlag(context, HIGHLIGHT_STATE_PRESSED, index, false);

		Modifiers_t modifiers = GetModifierFlagForKey(index);
		if (modifiers)
		{
			context->HeldModifiers ^= modifiers;
			if (modifiers & MOD_LOCKMASK)
			{
				if (modifiers & MOD_CAPSLOCK_MASK)
					context->DirtyState = DS_WholeMainKeyboard;
			}
			else if (modifiers & MOD_SHIFTMASK)
				context->DirtyState = DS_WholeMainKeyboard;
		}
	}
};

void touchUp([[maybe_unused]] void *data,
		     [[maybe_unused]] struct wl_touch *wl_touch,
		     [[maybe_unused]] uint32_t serial,
		     [[maybe_unused]] uint32_t time,
		     [[maybe_unused]] int32_t id)
{
	printf("%s(%d)\n", __func__, id);
	if (id >= 10)
		return;
	struct AppContext* context = (struct AppContext*) data;
	if (context->TouchState[id].IsDown && KeyIndexIsValid(context->TouchState[id].Key))
	{
		SendKey(context, context->TouchState[id].Key, WL_KEYBOARD_KEY_STATE_RELEASED, 0u);
		ClearHighlightFlag(context, HIGHLIGHT_STATE_PRESSED, context->TouchState[id].Key, false);

		Modifiers_t modifiers = GetModifierFlagForKey(context->TouchState[id].Key);
		if (modifiers)
		{
			if (modifiers & MOD_LOCKMASK)
			{
				if ((context->HeldModifiers & (modifiers << 1)) && modifiers & MOD_CAPSLOCK_MASK)
					context->DirtyState = DS_WholeMainKeyboard;
				context->HeldModifiers ^= (modifiers << 1);
			}
			else if (modifiers & MOD_SHIFTMASK)
				context->DirtyState = DS_WholeMainKeyboard;

			context->HeldModifiers ^= modifiers;
		}
	}
	context->TouchState[id].IsDown = false;
};

void touchMotion([[maybe_unused]] void *data,
		         [[maybe_unused]] struct wl_touch *wl_touch,
		         [[maybe_unused]] uint32_t time,
		         [[maybe_unused]] int32_t id,
		         [[maybe_unused]] wl_fixed_t x,
		         [[maybe_unused]] wl_fixed_t y)
{ printf("%s(%d, %d, %d)\n", __func__, id, x, y); };

void touchFrame([[maybe_unused]] void *data,
		        [[maybe_unused]] struct wl_touch *wl_touch)
{ printf("%s()\n", __func__); };

void touchCancel([[maybe_unused]] void *data,
		         [[maybe_unused]] struct wl_touch *wl_touch)
{
	printf("%s()\n", __func__);
	struct AppContext* context = (struct AppContext*) data;
	for (int i = 0; i < 10; ++i)
		context->TouchState[i].IsDown = false;
};

void touchShape([[maybe_unused]] void *data,
		        [[maybe_unused]] struct wl_touch *wl_touch,
		        [[maybe_unused]] int32_t id,
		        [[maybe_unused]] wl_fixed_t major,
		        [[maybe_unused]] wl_fixed_t minor)
{ printf("%s(%d, %d, %d)\n", __func__, id, major, minor); };

void touchOrientation([[maybe_unused]] void *data,
			          [[maybe_unused]] struct wl_touch *wl_touch,
			          [[maybe_unused]] int32_t id,
			          [[maybe_unused]] wl_fixed_t orientation)
{ printf("%s(%d, %d)\n", __func__, id, orientation); };

struct wl_touch_listener s_touchListener = {
	.down =        touchDown,
	.up =          touchUp,
	.motion =      touchMotion,
	.frame =       touchFrame,
	.cancel =      touchCancel,
	.shape =       touchShape,
	.orientation = touchOrientation,
};
