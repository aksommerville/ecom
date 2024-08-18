all:
.SILENT:
.SECONDARY:
PRECMD=echo "  $@" ; mkdir -p $(@D) ;

CC:=gcc -c -MMD -O3 -Isrc -Werror -Wimplicit -Wno-pointer-sign -Wno-parentheses
LD:=gcc
LDPOST:=

INDEXHTML:=out/index.html
ZIP:=out/ecom.zip
all:$(INDEXHTML) $(ZIP)

SRCFILES:=$(shell find src -type f)

CFILES_MIN:=$(filter src/min/%.c,$(SRCFILES))
OFILES_MIN:=$(patsubst src/min/%.c,mid/min/%.o,$(CFILES_MIN))
-include $(OFILES_MIN:.o=.d)
mid/%.o:src/%.c;$(PRECMD) $(CC) -o$@ $<
EXE_MIN:=out/min
$(EXE_MIN):$(OFILES_MIN);$(PRECMD) $(LD) -o$@ $^ $(LDPOST)

SRCFILES_WWW:=$(filter src/www/%,$(SRCFILES))
$(INDEXHTML):$(SRCFILES_WWW) $(EXE_MIN);$(PRECMD) $(EXE_MIN) -o$@ src/www/index.html
$(ZIP):$(INDEXHTML);$(PRECMD) zip -jqX $@ $^ && echo "$@: $$(du -sb src/www | cut -f1) => $$(stat -c%s $(INDEXHTML)) => $$(stat -c%s $@)"

# 'make run' to serve the input files, prefer for high-frequency dev work.
run:;http-server src/www -c-1

# 'make run-final' to serve the minified single file. Definitely test this thoroughly before shipping!
# Unlike the loose input files, you can also just load this with `file:` protocol.
run-final:$(INDEXHTML);http-server out -c-1

clean:;rm -rf mid out
