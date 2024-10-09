all:
.SILENT:
.SECONDARY:
PRECMD=echo "  $@" ; mkdir -p $(@D) ;

# Empty to skip the native build.
ifndef QJS_SDK
  QJS_SDK:=../thirdparty/quickjs/quickjs-2023-12-09
endif

# These are for both MIN and NATIVE; all the dependencies are only important to NATIVE.
NATIVE_OPT_ENABLE:=hostio xegl drmgx alsafd evdev
CC:=gcc -c -MMD -O3 -Isrc -Isrc/native -Werror -Wimplicit -Wno-pointer-sign -Wno-parentheses $(foreach U,$(NATIVE_OPT_ENABLE),-DUSE_$U=1) -I/usr/include/libdrm -I$(QJS_SDK)
LD:=gcc -z noexecstack
LDPOST:=-lz -lGLESv2 -lGL -lEGL $(QJS_SDK)/libquickjs.a -lm \
  $(if $(filter xegl,$(NATIVE_OPT_ENABLE)),-lX11) \
  $(if $(filter drmgx,$(NATIVE_OPT_ENABLE)),-ldrm -lgbm) \
  $(if $(filter pulse,$(NATIVE_OPT_ENABLE)),-lpulse-simple) \
  $(if $(filter asound,$(NATIVE_OPT_ENABLE)),-lasound)

INDEXHTML:=out/index.html
ZIP:=out/ecom.zip
all:$(INDEXHTML) $(ZIP)

SRCFILES:=$(shell find src -type f)

CFILES_MIN:=$(filter src/min/%.c,$(SRCFILES))
OFILES_MIN:=$(patsubst src/min/%.c,mid/min/%.o,$(CFILES_MIN))
-include $(OFILES_MIN:.o=.d)
mid/min/%.o:src/min/%.c;$(PRECMD) $(CC) -o$@ $<
EXE_MIN:=out/min
$(EXE_MIN):$(OFILES_MIN);$(PRECMD) $(LD) -o$@ $^ $(LDPOST)

SRCFILES_WWW:=$(filter src/www/%,$(SRCFILES))
$(INDEXHTML):$(SRCFILES_WWW) $(EXE_MIN);$(PRECMD) $(EXE_MIN) -o$@ src/www/index.html
$(ZIP):$(INDEXHTML);$(PRECMD) zip -jqX $@ $^ && echo "$@: $$(du -sb src/www | cut -f1) => $$(stat -c%s $(INDEXHTML)) => $$(stat -c%s $@)"

ifneq (,$(strip $(QJS_SDK)))
  ARCHIVE_NATIVE:=mid/native/data
  $(ARCHIVE_NATIVE):$(INDEXHTML) $(EXE_MIN);$(PRECMD) $(EXE_MIN) -o$@ $< --native
  ARCHO_NATIVE:=mid/native/data.o
  #TODO Can we ask the environment instead of assuming 'elf64-x86-64'?
  $(ARCHO_NATIVE):$(ARCHIVE_NATIVE);$(PRECMD) objcopy -Ibinary -Oelf64-x86-64 $< $@
  SRCFILES_NATIVE:=$(filter src/native/%,$(SRCFILES))
  CFILES_NATIVE:=$(filter %.c,$(SRCFILES_NATIVE))
  CFILES_NATIVE:= \
    $(filter-out src/native/opt/%,$(CFILES_NATIVE)) \
    $(filter $(addprefix src/native/opt/,$(addsuffix /%,$(NATIVE_OPT_ENABLE))),$(CFILES_NATIVE))
  OFILES_NATIVE:=$(patsubst src/native/%.c,mid/native/%.o,$(CFILES_NATIVE)) $(ARCHO_NATIVE)
  -include $(OFILES_NATIVE:.o=.d)
  mid/native/%.o:src/native/%.c;$(PRECMD) $(CC) -o$@ $<
  EXE_NATIVE:=out/ecom
  $(EXE_NATIVE):$(OFILES_NATIVE);$(PRECMD) $(LD) -o$@ $(OFILES_NATIVE) $(LDPOST)
  all:$(EXE_NATIVE)
  native-run:$(EXE_NATIVE);$(EXE_NATIVE)
endif

# 'make run' to serve the input files, prefer for high-frequency dev work.
run:;http-server src/www -c-1

# 'make run-final' to serve the minified single file. Definitely test this thoroughly before shipping!
# Unlike the loose input files, you can also just load this with `file:` protocol.
run-final:$(INDEXHTML);http-server out -c-1

edit:;node src/editor/main.js

clean:;rm -rf mid out
