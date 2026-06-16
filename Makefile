PROJECT   := wayboard
SOURCES   += main.c pointer.c touch.c virtualKeyboard.c graphics.c common.h queue.c queue.h futex.h futex.c graphics.h
PROTOCOLS += virtual-keyboard-unstable-v1
PROTOCOLS += input-method-unstable-v2
PROTOCOLS += wayland-protocols/stable/xdg-shell/xdg-shell
PROTOCOLS += wlr-protocols/unstable/wlr-layer-shell-unstable-v1
DEFINES   += USE_KERNING=1

LIBS += wayland-client xkbcommon m 
FTLIBS = $$(pkg-config --libs-only-l $$(pkg-config --print-requires libs/freetype/build/freetype2.pc))
STATICLIBS += libs/freetype/build/libfreetype.a

FLAGS = -march=skylake
CFLAGS += $(addprefix -D,${DEFINES}) -I../freetype/include -std=gnu23 -Wall -Wextra -Werror=incompatible-pointer-types -funsigned-char
LDFLAGS = -flto


# flags to be passed to the local FreeType build (we disable as much as we can get away
# with, because it's statically linked, and we want as small a file as possible)
FTFLAGS += BUILD_SHARED_LIBS=OFF
FTFLAGS += FT_DISABLE_PNG=ON
FTFLAGS += FT_DISABLE_HARFBUZZ=ON
FTFLAGS += FT_DISABLE_BROTLI=ON

ifeq ($(DEBUG),1)
	FLAGS += -g3 -O0
	DEFINES += _DEBUG
else
	FLAGS += -O3
	LDFLAGS += -s
endif


# -----------------------------------------


FLATPROTOCOLS = $(addprefix protocols/,$(notdir $(PROTOCOLS)))

all: $(PROJECT) protocols libs/freetype/build/libfreetype.a

install: $(PROJECT)
	[ -w "/usr/local/bin" ] && install wayboard /usr/local/bin/wayboard || install wayboard ${HOME}/.local/bin/wayboard

uninstall:
	[ -w "/usr/local/bin" ] && rm /usr/local/bin/wayboard || rm ${HOME}/.local/bin/wayboard

libs/freetype:
	git clone --recursive --shallow-submodules --single-branch --depth 1 https://github.com/freetype/freetype libs/freetype

libs/freetype/build/include/freetype/config/ftoption.h: | libs/freetype
	mkdir -p libs/freetype/build && cd libs/freetype/build && cmake .. -D CMAKE_BUILD_TYPE=Release $(addprefix -D ,${FTFLAGS})
	sed -i '/define TT_CONFIG_OPTION_GPOS_KERNING/c\#define TT_CONFIG_OPTION_GPOS_KERNING' $@

libs/freetype/build/libfreetype.a: libs/freetype/build/include/freetype/config/ftoption.h
	cd libs/freetype/build && $(MAKE)
	#cp -L ../freetype/build/$@ ./
	#objcopy --remove-section=.note.gnu.gold-id --remove-section=.gnu.version_d $@

.SECONDEXPANSION:

PERCENT := %

protocols/:
	mkdir -p protocols

protocols/virtual-keyboard-unstable-v1.xml: | protocols/
	wget -N https://gitlab.freedesktop.org/wlroots/wlroots/-/raw/master/protocol/virtual-keyboard-unstable-v1.xml -O $@ -T 1 -t 1

protocols/input-method-unstable-v2.xml: | protocols/
	wget -N https://gitlab.freedesktop.org/wlroots/wlroots/-/raw/master/protocol/input-method-unstable-v2.xml -O $@ -T 1 -t 1

protocols/%.c: /usr/share/$$(filter $$(PERCENT)/$$*,$$(PROTOCOLS)).xml | protocols/
	wayland-scanner private-code $^ $@

protocols/%.h: /usr/share/$$(filter $$(PERCENT)/$$*,$$(PROTOCOLS)).xml | protocols/
	wayland-scanner client-header $^ $@

protocols/%.h: protocols/%.xml | protocols/
	wayland-scanner client-header $^ $@

protocols/%.c: protocols/%.xml | protocols/
	wayland-scanner private-code $^ $@

protocols: $(addsuffix .c,$(FLATPROTOCOLS)) $(addsuffix .h,$(FLATPROTOCOLS))

protocols/%.o: protocols/%.c protocols/%.h
	gcc $(CFLAGS) $(FLAGS) -c $< -o $@

$(PROJECT): $(SOURCES) $(addsuffix .o,$(FLATPROTOCOLS)) $(STATICLIBS)
	gcc -masm=intel $(CFLAGS) $(FLAGS) $< $(addsuffix .o,$(FLATPROTOCOLS)) -o $@ $(STATICLIBS) $(addprefix -l,$(LIBS)) $(FTLIBS) $(LDFLAGS) 

clean:
	rm -f wayboard protocols/*.o

simd-asm-test: simd-asm-test.c
	gcc -masm=intel $(CFLAGS) $(LDFLAGS) $(FLAGS) -save-temps $< -o $@
