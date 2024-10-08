#include <string.h>

/* Globals.
 */

extern const unsigned char _binary_mid_native_data_start[];
extern const unsigned char _binary_mid_native_data_end[];

// Arbitrary limits. Set to whatever there actually is, it's knowable at build time.
#define SONG_LIMIT 3
#define STAGE_LIMIT 13

static struct {
  const void *image; int imagec;
  const void *script; int scriptc;
  struct song {
    const char *name;
    const void *v;
    int namec,c;
  } songv[SONG_LIMIT];
  int songc;
  struct stage {
    const void *v;
    int c;
  } stagev[STAGE_LIMIT];
  int stagec;
} rom={0};
 
/* Init.
 */
 
int rom_init() {
  const unsigned char *src=_binary_mid_native_data_start;
  int srcc=_binary_mid_native_data_end-_binary_mid_native_data_start;
  int srcp=0;
  while (srcp<srcc) {
    unsigned char type=src[srcp++];
    if (!type) break;
    if (srcp>srcc-2) return -1;
    int len=(src[srcp]<<8)|src[srcp+1];
    srcp+=2;
    if (srcp>srcc-len) return -1;
    const void *v=src+srcp;
    srcp+=len;
    switch (type) {
      case 1: { // image
          if (rom.imagec) return -1;
          rom.image=v;
          rom.imagec=len;
        } break;
      case 2: { // song
          if (rom.songc>=SONG_LIMIT) return -1;
          struct song *song=rom.songv+rom.songc++;
          song->name=v;
          while ((song->namec<len)&&(song->name[song->namec]!=';')) song->namec++;
          if (song->namec>=len) return -1;
          song->v=song->name+song->namec+1;
          song->c=len-song->namec-1;
        } break;
      case 3: { // stage
          if (rom.stagec>STAGE_LIMIT) return -1;
          struct stage *stage=rom.stagev+rom.stagec++;
          stage->v=v;
          stage->c=len;
        } break;
      case 4: { // script
          if (rom.scriptc) return -1;
          if ((len<1)||((char*)v)[len-1]) return -1; // Scripts must be NUL-terminated for quickjs. Our compiler handles it.
          len--; // ...but don't count the NUL.
          rom.script=v;
          rom.scriptc=len;
        } break;
      default: return -1;
    }
  }
  return 0;
}

/* Public accessors.
 */
 
int rom_get_image(void *dstpp) {
  *(const void**)dstpp=rom.image;
  return rom.imagec;
}

int rom_get_song(void *dstpp,const char *name,int namec) {
  if (!name) namec=0; else if (namec<0) { namec=0; while (name[namec]) namec++; }
  const struct song *song=rom.songv;
  int i=rom.songc;
  for (;i-->0;song++) {
    if (song->namec!=namec) continue;
    if (memcmp(song->name,name,namec)) continue;
    *(const void**)dstpp=song->v;
    return song->c;
  }
  return 0;
}

int rom_get_stage(void *dstpp,int index) {
  if (index<0) return 0;
  if (index>=rom.stagec) return 0;
  *(const void**)dstpp=rom.stagev[index].v;
  return rom.stagev[index].c;
}

int rom_get_script(void *dstpp) {
  *(const void**)dstpp=rom.script;
  return rom.scriptc;
}
