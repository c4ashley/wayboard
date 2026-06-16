- stop using so many hardcoded values, especially if I want more keyboard layouts
- right shift doesn't work (maybe the other right modifiers also don't?)
- Function Keys row: Find a place for Scroll Lock, Pause, etc
- Alternate layout options
  - Smaller keyboards still need *some* way to use navigation keys
  - Nipple?
  - Stick to just the main keyboard
  - Simplified modifiers (like a mobile device will typically only have Shift)
  - Compact navigation (2-column), or no navigation section? Navigation keys are
    all available on the numpad anyway...
  - Maybe some experimental physical layouts like split keyboards, thumb clusters, much later on
  - Actual keychar layouts like AZERTY, etc, are definitely late-game. XKB can probably help. Can I query XKB for each key's char?
- Hold a key for other similar/related keys (hold 1 for '!' and possible fractions, etc)
- Floating/Docked
- Options for latching modes for modifier keys
  - Never latch
  - Always toggle latch
  - Latch until next key (default)
  - Double-click to latch (or tri-state?)
- Swype
- use submodules for freetype (?)
    ```
	[submodule "libs/freetype"]
	path = libs/freetype
	url = https://github.com/freetype/freetype
	shallow = true
	branch = master
    ```

- Key presses seem to be passed straight to the focused application, bypassing any global bindings.
  Investigate this to see if this is a quirk of the compositor (Niri), or if it's specified in the standard.
  See how other compositors handle virtual keyboard input, and see if it's worth proposing changes.
