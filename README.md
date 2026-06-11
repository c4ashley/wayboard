# wayboard

-----------------------

## About

***wayboard* is a simple, lightweight, responsive on-screen keyboard for Wayland.**

It presents up to a full US 104-key keybaord, including modifier keys (but not the
F-key row **yet**) for those who need more than just letters and basic punctuation.
The "completeness" of the keyboard is determined by the screen size: A 1080p screen
will show everything, a smaller screen might not include the number pad or navigation
section (arrows, Home/End/etc).

It is intended to be as lightweight and responsive as possible, using as little disk
space and RAM as possible, requiring as few dependencies as possible, prioritising
performance over appearance and features.

As such, it doesn't use *any* drawing library whatsoever (except for [FreeType] for
text), and it's hardcoded to make use of AVX2 extensions, with many optimisations
and assumptions to make rendering as quick and simple as possible.

Currently, it's just a standalone program with no auto-show or hide. It doesn't
automatically show itself when you focus a textbox, for example. So I recommend
creating a button in your Waybar or assigning a gesture or something. I use the
very simple `killall -SIGQUIT wayboard || wayboard` as the action for a button.

For the technically minded, it creates an exclusive-zone shell layer and uses the
[virtual_keyboard_unstable_v1] protocol to send key presses to the compositor.

Currently, it is hardcoded to imitate the US 104-key QWERTY layout. Alternate layouts
will be available at some point, but I'll have to think of some robust solution that
satisfies all the priorities of this project, so it'll be a bit later on.

#### Runtime dependencies:
- Wayland
- libxkbcommon
- A CPU that supports AVX2 extensions

#### Future Goals:
- Respond to Wayland IME requests to auto-show and hide keyboard
- Different keyboard "arrangements" *(whether to include the right-side modifiers,
or squish the navigation keys in with the rest like a laptop keyboard, etc)*
- Different keyboard layouts *(US, UK, AZERTY, etc)*
- Force an arrangement instead of infer from the screen size
- Undock and drag the keyboard
- More in [todo]

#### Important Notes:
- Keypresses may not trigger global bindings. This might vary by compositor, but in
  Niri, they definitely do not.
- Some compositors may have issues with retaining focus on a multi-monitor setup.
  I'm working on a [PR](https://github.com/niri-wm/niri/pull/3984) to have this
  fixed in Niri.

-----------------------

## Backstory

So I decided to finally install Linux on my Surface Pro 4. I toyed with the idea
some years ago, installing Fedora Workstation. It was beautiful, and seemed very
appropriate for the tablet, the way GNOME is designed, but it was atrocious. It
was so slow, animations everywhere, visual effects everywhere, my poor 0.9 GHz
dual-core CPU with 4GB RAM couldn't handle it, so I quickly aborted.

I tried again more recently, and this time I stuck with it. Instead of Fedora
Workstation, I opted for Fedora Plasma Mobile. I don't mind Plasma, and the KDE
team are pretty good, I figured their mobile variant would be pretty decent.

It was not. It's much lighter than GNOME, but many of the default apps are either
too basic, buggy, or effectively unusable. The shell is nice though. But crucially,
the on-screen keyboard is too limiting and "pretty." It prioritises looking good
over being fast and usable on a weak old garbage computer. The Surface being a
tablet, and me being a nerd, having a complete on-screen keyboard (with modifier
keys and arrows) was a priority. Do you have any idea it is to modify a .desktop
file without a `Ctrl` key or arrows?

Finding a usable on-screen keyboard was... a challenge. I might find one, but it's
similarly heavy. Or it might not be a full keyboard. Or it might require a bunch
of dependencies bumping up the total install size to 80MB. Or it might be obsolete
and I have to build it myself, but even they need a bunch of dependencies, and a
weird toolchain, and it's all just a huge confusing nightmare.

I just need a keyboard that's low on RAM usage, low on dependencies, simple on
graphics, is fast and responsive, works well with touch input, and offers a full
keyboard instead of just letters and numbers. Is that so much to ask?

So I've abandoned Plasma Mobile, and I'm using [Niri][niri] now, and I've been
developing this onscreen keyboard specifically for this use case. It literally
only requires [FreeType], [libxkbcommon], and of course, Wayland (and GCC, Make,
and unfortunately CMake is required for FreeType for now).

There's still a lot of work to do, but so far so good.

----------------------
## Building

#### Dependencies:
- gcc
- make
- cmake
- libxkbcommon
- wayland
- wayland-protocols
- wlr-protocols
- libfreetype (included)

```bash
make
make install      # to install locally to ~/.local/bin/
sudo make install # to install globally to /usr/local/bin/
```

> **NOTE:** *wayboard* is a "unity" build, which means you just compile and link one
> single source file, `main.c`. All the other source files are `#include`d directly
> inside of `main.c`. This is intended to reduce link time and compile time by only
> having one compilation unit to link, and only needing to `#include` system headers
> once. This does however cause havoc with LSPs, so for the sake of LSPs, there are
> some weird include-guards and unnecessary includes in each file.

> **NOTE:** *wayboard* requires `libfreetype`, but it expects a custom build that
> enables basic pairwise kerning, not complex contextual Harfbuzz kerning. I'm yet
> to work out how best to use git submodules, but in the meantime, a shallow clone
> and build is integrated into the `make` routine.

---------------------
## Using

```bash
./wayboard [--output NAME]
```

For now, *wayboard* just appears on the first monitor advertised to it, which is up
to the whimsy of the compositor. To specify a specific monitor, the `NAME` parameter
expects the `description` argument of the `wl_output::description` Wayland event,
which is sent to `stdout` when you run it. This will be improved, but it's what we've
got for now.

Note that there is no way to close or hide the keyboard from the keyboard's interface
as yet, nor does it automatically show when you focus a text field, etc, so I highly
recommend creating a button in your Waybar, or binding a gesture, to open and close
the keyboard. My waybar configuration has the following:

```json
// $HOME/.config/waybar/config.jsonc
{
  "modules-left": [ "custom/keyboard", ... ],
  ...
  "custom/keyboard": {
    "format": "Kbd",
    "on-click": "killall wayboard -SIGQUIT || wayboard"
  }
}
```
```css
/* $HOME/.config/waybar/style.css */
#custom-keyboard {
  border: 3px gry solid;
  padding: 0 6px;
  background: #222;
}
```

[niri]: https://niri-wm.github.io/niri/
[todo]: TODO.md
[FreeType]: https://freetype.org/
[virtual_keyboard_unstable_v1]: https://wayland.app/protocols/virtual-keyboard-unstable-v1
[libxkbcommon]: https://xkbcommon.org/doc/current/xkb-intro.html
