#define _POINTER_C_
#ifndef _MAIN_C_
#include "main.c"
#endif
void pointerEnter([[maybe_unused]] void *data,
		          [[maybe_unused]] struct wl_pointer *wl_pointer,
		          [[maybe_unused]] uint32_t serial,
		          [[maybe_unused]] struct wl_surface *surface,
		          [[maybe_unused]] wl_fixed_t surface_x,
		          [[maybe_unused]] wl_fixed_t surface_y)
{ printf("pointerEnter(%d, %d)\n", surface_x, surface_y); };

void pointerLeave([[maybe_unused]] void *data,
		          [[maybe_unused]] struct wl_pointer *wl_pointer,
		          [[maybe_unused]] uint32_t serial,
		          [[maybe_unused]] struct wl_surface *surface)
{
	struct AppContext* context = (struct AppContext*) data;
	if (KeyIndexIsValid(context->PointerHighlight))
	{
		ClearHighlightFlag(context, HIGHLIGHT_STATE_HOVER, context->PointerHighlight, false);
		context->PointerHighlight = KEYINDEX_INVALID;
	}
};

void pointerMotion([[maybe_unused]] void *data,
		           [[maybe_unused]] struct wl_pointer *wl_pointer,
		           [[maybe_unused]] uint32_t time,
		           [[maybe_unused]] wl_fixed_t surface_x,
		           [[maybe_unused]] wl_fixed_t surface_y)
{
	//printf("pointerMotion(%d, %d)\n", surface_x, surface_y);
	struct AppContext* context = (struct AppContext*) data;
	float x = surface_x / 256.0f;
	float y = surface_y / 256.0f;
	KeyIndex index = GetKeyAtLocation(x, y, context, false);
	if (!KeyIndexEquals(index, context->PointerHighlight))
	{
		/*printf("Clearing highlight from %d.%d.%d, setting highlight to %d.%d.%d\n",
				context->PointerHighlight.cluster,
				context->PointerHighlight.row,
				context->PointerHighlight.key,
				index.cluster,
				index.row,
				index.key);*/
		if (KeyIndexIsValid(context->PointerHighlight))
			ClearHighlightFlag(context, HIGHLIGHT_STATE_HOVER, context->PointerHighlight, false);
		context->PointerHighlight = index;
		if (KeyIndexIsValid(index))
			SetHighlightFlag(context, HIGHLIGHT_STATE_HOVER, index, false);
		//DrawKeyboard(context);
	}
};

void pointerAxis([[maybe_unused]] void *data,
		         [[maybe_unused]] struct wl_pointer *wl_pointer,
		         [[maybe_unused]] uint32_t time,
		         [[maybe_unused]] uint32_t axis,
		         [[maybe_unused]] wl_fixed_t value)
{ printf("pointerAxis(%u, %d)\n", axis, value); };
void pointerFrame([[maybe_unused]] void *data,
		          [[maybe_unused]] struct wl_pointer *wl_pointer)
{
	//printf("pointerFrame()\n");
};
void pointerAxisSource([[maybe_unused]] void *data,
			           [[maybe_unused]] struct wl_pointer *wl_pointer,
			           [[maybe_unused]] uint32_t axis_source)
{ printf("pointerAxisSource(%u)\n", axis_source); };
void pointerAxisStop([[maybe_unused]] void *data,
		             [[maybe_unused]] struct wl_pointer *wl_pointer,
		             [[maybe_unused]] uint32_t time,
		             [[maybe_unused]] uint32_t axis)
{ printf("pointerAxisStop(%u)\n", axis); };
void pointerAxisDiscrete([[maybe_unused]] void *data,
			             [[maybe_unused]] struct wl_pointer *wl_pointer,
			             [[maybe_unused]] uint32_t axis,
			             [[maybe_unused]] int32_t discrete)
{ printf("pointerAxisDiscrete(%u, %d)\n", axis, discrete); };
void pointerAxisValue120([[maybe_unused]] void *data,
			             [[maybe_unused]] struct wl_pointer *wl_pointer,
			             [[maybe_unused]] uint32_t axis,
			             [[maybe_unused]] int32_t value120)
{ printf("pointerAxisValue120(%u, %d)\n", axis, value120); };
void pointerAxisRelativeDirection([[maybe_unused]] void *data,
				                  [[maybe_unused]] struct wl_pointer *wl_pointer,
				                  [[maybe_unused]] uint32_t axis,
				                  [[maybe_unused]] uint32_t direction)
{ printf("pointerAxisRelativeDirection(%u, %u)\n", axis, direction); };

void pointerButton([[maybe_unused]] void *data,
		           [[maybe_unused]] struct wl_pointer *wl_pointer,
		           [[maybe_unused]] uint32_t serial,
		           [[maybe_unused]] uint32_t time,
		           [[maybe_unused]] uint32_t button,
		           [[maybe_unused]] uint32_t state)
{
	//printf("pointerButton(%xh, %u)\n", button, state);
	struct AppContext* context = (struct AppContext*) data;
	KeyIndex key = KEYINDEX_INVALID;
	if (button == BTN_LEFT)
	{
		if (state == WL_POINTER_BUTTON_STATE_PRESSED)
		{
			//printf("left button pressed (IsMouseDown: %d, PointerHighlight: %d, PointerPressed: %d\n", context->IsMouseDown, context->PointerHighlight.raw, context->PointerPressed.raw);
			if (!context->IsMouseDown && KeyIndexIsValid(context->PointerHighlight))
			{
				context->PointerPressed = context->PointerHighlight;
				context->IsMouseDown = true;
				key = context->PointerPressed;

				Modifiers_t modifierFlag = GetModifierFlagForKey(key);
				if (modifierFlag != 0)
				{
					context->HeldModifiers ^= modifierFlag;
					if (modifierFlag & MOD_LOCKMASK)
					{
						SendKey(context, key, WL_KEYBOARD_KEY_STATE_PRESSED, time);
						SetHighlightFlag(context, HIGHLIGHT_STATE_PRESSED, key, false);
						if (modifierFlag & MOD_CAPSLOCK_MASK)
							context->DirtyState = DS_WholeMainKeyboard;
					}
					else
					{
						if (context->HeldModifiers & modifierFlag)
						{
							SendKey(context, key, WL_KEYBOARD_KEY_STATE_PRESSED, time);
							SetHighlightFlag(context, HIGHLIGHT_STATE_PRESSED, key, false);
							if (modifierFlag & MOD_SHIFTMASK)
								context->DirtyState = DS_WholeMainKeyboard;
						}
						else
						{
							SendKey(context, key, WL_KEYBOARD_KEY_STATE_RELEASED, time);
							ClearHighlightFlag(context, HIGHLIGHT_STATE_PRESSED, key, false);
							if (modifierFlag & MOD_SHIFTMASK)
								context->DirtyState = DS_WholeMainKeyboard;
						}
					}
				}
				else
				{
					SendKey(context, key, WL_KEYBOARD_KEY_STATE_PRESSED, time);
					SetHighlightFlag(context, HIGHLIGHT_STATE_PRESSED, key, false);
				}
			}
		}
		else if (state == WL_POINTER_BUTTON_STATE_RELEASED)
		{
			//printf("left button released (IsMouseDown: %d, PointerHighlight: %d, PointerPressed: %d\n", context->IsMouseDown, context->PointerHighlight.raw, context->PointerPressed.raw);
			if (context->IsMouseDown && KeyIndexIsValid(context->PointerPressed))
			{
				key = context->PointerPressed;
				Modifiers_t modifierFlag = GetModifierFlagForKey(key);
				if (!modifierFlag)
				{
					SendKey(context, context->PointerPressed, WL_KEYBOARD_KEY_STATE_RELEASED, time);
					ClearHighlightFlag(context, HIGHLIGHT_STATE_PRESSED, context->PointerPressed, false);

					if (context->HeldModifiers)
					{
						if (context->HeldModifiers & MOD_SHIFTMASK)
						{
							context->DirtyState = DS_WholeMainKeyboard;
							if (context->HeldModifiers & MOD_LSHIFT)
							{
								ClearHighlightFlag(context, 3, (KeyIndex) {.cluster=0, .row = 3, .key = 0 }, false);
								SendKey(context, (KeyIndex) {.cluster=0, .row = 3, .key = 0 }, WL_KEYBOARD_KEY_STATE_RELEASED, time);
							}
							if (context->HeldModifiers & MOD_RSHIFT)
							{
								ClearHighlightFlag(context, 3, (KeyIndex) {.cluster=0, .row = 3, .key = 11 }, false);
								SendKey(context, (KeyIndex) {.cluster=0, .row = 3, .key = 11 }, WL_KEYBOARD_KEY_STATE_RELEASED, time);
							}
						}
						if (context->HeldModifiers & MOD_LCTRL)
						{
							ClearHighlightFlag(context, 3, (KeyIndex) {.cluster=0, .row = 4, .key = 0 }, false);
							SendKey(context, (KeyIndex) {.cluster=0, .row = 4, .key = 0 }, WL_KEYBOARD_KEY_STATE_RELEASED, time);
						}
						if (context->HeldModifiers & MOD_RCTRL)
						{
							ClearHighlightFlag(context, 3, (KeyIndex) {.cluster=0, .row = 4, .key = 7 }, false);
							SendKey(context, (KeyIndex) {.cluster=0, .row = 4, .key = 7 }, WL_KEYBOARD_KEY_STATE_RELEASED, time);
						}
						if (context->HeldModifiers & MOD_LALT)
						{
							ClearHighlightFlag(context, 3, (KeyIndex) {.cluster=0, .row = 4, .key = 2 }, false);
							SendKey(context, (KeyIndex) {.cluster=0, .row = 4, .key = 2 }, WL_KEYBOARD_KEY_STATE_RELEASED, time);
						}
						if (context->HeldModifiers & MOD_RALT)
						{
							ClearHighlightFlag(context, 3, (KeyIndex) {.cluster=0, .row = 4, .key = 4 }, false);
							SendKey(context, (KeyIndex) {.cluster=0, .row = 4, .key = 4 }, WL_KEYBOARD_KEY_STATE_RELEASED, time);
						}
						if (context->HeldModifiers & MOD_LSUPER)
						{
							ClearHighlightFlag(context, 3, (KeyIndex) {.cluster=0, .row = 4, .key = 1 }, false);
							SendKey(context, (KeyIndex) {.cluster=0, .row = 4, .key = 1 }, WL_KEYBOARD_KEY_STATE_RELEASED, time);
						}
						if (context->HeldModifiers & MOD_RSUPER)
						{
							ClearHighlightFlag(context, 3, (KeyIndex) {.cluster=0, .row = 4, .key = 5 }, false);
							SendKey(context, (KeyIndex) {.cluster=0, .row = 4, .key = 5 }, WL_KEYBOARD_KEY_STATE_RELEASED, time);
						}
						context->HeldModifiers &= MOD_LOCKMASK;
					}
				}
				else
				{
					if (modifierFlag & MOD_LOCKMASK)
					{
						SendKey(context, key, WL_KEYBOARD_KEY_STATE_RELEASED, time);
						if (context->HeldModifiers & (modifierFlag << 1))
						{
							ClearHighlightFlag(context, HIGHLIGHT_STATE_PRESSED, key, false);
							if (modifierFlag & MOD_CAPSLOCK_MASK)
								context->DirtyState = DS_WholeMainKeyboard;
						}
						context->HeldModifiers ^= (modifierFlag) | (modifierFlag << 1);
					}
				}
				context->PointerPressed = KEYINDEX_INVALID;
				context->IsMouseDown = false;
			}
		}
	}
	else if (button == BTN_RIGHT)
	{
		//context->ShowContextMenu = true;
		//context->ContextMenuLocation = {surface_x, surface_y};
	}
	return;

Modifier:
};

struct wl_pointer_listener s_pointerListener = {
	.enter                   = pointerEnter,
	.leave                   = pointerLeave,
	.motion                  = pointerMotion,
	.button                  = pointerButton,
	.axis                    = pointerAxis,
	.frame                   = pointerFrame,
	.axis_source             = pointerAxisSource,
	.axis_stop               = pointerAxisStop,
	.axis_discrete           = pointerAxisDiscrete,
	.axis_value120           = pointerAxisValue120,
	.axis_relative_direction = pointerAxisRelativeDirection,
};
