#include "native_internal.h"
#include <math.h>

/* From weebpoc/README.md, which i cleverly forgot to document in this repo:
static const char *alpha_opcode = "'()*+,-./01234567"; // delay,ch0,...,ch15
static const char *alpha_data = "?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~"; // 0..63

Compiled songs are plain text.
Whitespace is unused, you must strip it before feeding to the synthesizer.

0x27..0x37 `'()*+,-./01234567` are opcodes.
0x3f..0x7e are data bytes (0..63).

Starts with one data byte for the tempo: N+1 is ms per tick, 1..64.

Apostrophe followed by a data byte is a delay, for N+1 ticks.

Other opcode bytes correspond to one MIDI channel, and indicate one note on that channel.
`OPCODE NOTEID DURATION` where NOTEID and DURATION are each one data byte.
NOTEID, add 0x23 for the MIDI note id. Range is B1..D7. (0x23 happens to be the start of GM drums, in case that ever comes up).
*/

/* Private globals.
 */
 
#define VOICE_LIMIT 16
#define WAVE_LENGTH_BITS 10
#define WAVE_LENGTH_SAMPLES (1<<WAVE_LENGTH_BITS)
#define WAVE_SHIFT (32-WAVE_LENGTH_BITS)
 
static struct {
  int rate,chanc;
  struct voice {
    int inuse;
    uint32_t p;
    uint32_t dp;
    uint32_t ddp;
    float level;
    float dlevel;
    float targetlevel;
    int ttl;
  } voicev[VOICE_LIMIT];
  int voicec;
  uint32_t dp_by_noteid[64];
  int tempo;
  const uint8_t *song; // with tempo trimmed
  int songc;
  int songp;
  int songdelay; // frames
  int songrepeat;
  float wave[WAVE_LENGTH_SAMPLES];
} synth={0};

/* Init.
 */
 
int synth_init(int rate,int chanc) {
  synth.rate=rate;
  synth.chanc=chanc;
  
  double hz=440.0;
  int noteid=0x22;
  int i=12;
  for (;i-->0;noteid++) {
    synth.dp_by_noteid[noteid]=(uint32_t)((hz*4294967296.0)/(double)rate);
    hz*=1.0594630943592953;
  }
  for (;noteid<0x40;noteid++) {
    synth.dp_by_noteid[noteid]=synth.dp_by_noteid[noteid-12]<<1;
  }
  for (noteid=0x22;noteid-->0;) {
    synth.dp_by_noteid[noteid]=synth.dp_by_noteid[noteid+12]>>1;
  }
  
  float *wavep=synth.wave;
  float t=0.0f,dt=(M_PI*2.0f)/WAVE_LENGTH_SAMPLES;
  for (i=WAVE_LENGTH_SAMPLES;i-->0;wavep++,t+=dt) {
    *wavep=sinf(t);
  }
  
  return 0;
}

/* Find an unused voice or hijack one.
 */
 
static struct voice *synth_voice_new() {
  if (synth.voicec<VOICE_LIMIT) return synth.voicev+synth.voicec++;
  struct voice *voice=synth.voicev;
  struct voice *oldest=voice;
  int i=VOICE_LIMIT;
  for (;i-->0;voice++) {
    if (!voice->inuse) return voice;
    if (voice->ttl<oldest->ttl) oldest=voice;
  }
  return oldest;
}

/* Play note.
 */
 
static struct voice *synth_play_note(int chid,int noteid,int durms) {
  if ((chid<0)||(chid>15)) return 0;
  if ((noteid<0)||(noteid>63)) return 0;
  struct voice *voice=synth_voice_new();
  voice->p=0;
  voice->dp=synth.dp_by_noteid[noteid];
  voice->ddp=0;
  voice->level=0.0f;
  voice->targetlevel=0.200f;
  voice->dlevel=voice->targetlevel/(synth.rate*0.015f);
  voice->ttl=(durms*synth.rate)/1000;
  voice->inuse=1;
  return voice;
}

/* Update voice.
 */
 
static void voice_update(int16_t *v,int c,struct voice *voice) {
  if (!voice->inuse) return;
  voice->ttl-=c;
  for (;c-->0;v++) {
    
    /* Sine wave from a table with envelope. */
    float sample=synth.wave[voice->p>>WAVE_SHIFT];
    if (voice->dlevel>0.0f) {
      if ((voice->level+=voice->dlevel)>=voice->targetlevel) {
        voice->level=voice->targetlevel;
        voice->dlevel=0.0f;
      }
    } else if (voice->dlevel<0.0f) {
      if ((voice->level+=voice->dlevel)<=0.0f) {
        voice->level=0.0f;
        voice->dlevel=0.0f;
        voice->inuse=0;
        break;
      }
    } else if (voice->ttl<=0) {
      voice->dlevel=voice->level/(synth.rate*-0.200f);
    }
    sample*=voice->level;
    int isample=(int)(sample*20000.0f);
    if (isample>32767) isample=32767;
    else if (isample<-32768) isample=-32768;
    (*v)+=isample;
    /**/
    
    voice->p+=voice->dp;
    voice->dp+=voice->ddp;
  }
}

static void synth_update_voices(int16_t *v,int framec) {
  struct voice *voice=synth.voicev;
  int i=synth.voicec;
  for (;i-->0;voice++) {
    voice_update(v,framec,voice);
  }
}

/* Update song.
 * Give us the maximum frame count.
 * Returns the frame count to update, before we need to be called again.
 */
 
static int synth_update_song(int limit) {
  #define PANIC { synth.song=0; return limit; }
  for (;;) {
    if (!synth.song) return limit;
    if (synth.songdelay>0) {
      if (synth.songdelay>=limit) {
        synth.songdelay-=limit;
        return limit;
      }
      int result=synth.songdelay;
      synth.songdelay=0;
      return result;
    }
    if (synth.songp>=synth.songc) {
      if (synth.songrepeat) {
        synth.songp=0;
        return 1;
      } else {
        synth.song=0;
        return limit;
      }
    }
    int opcode=synth.song[synth.songp++]-0x27;
    if ((opcode<0)||(opcode>16)) PANIC
    if (opcode==0) { // Delay
      if (synth.songp>=synth.songc) PANIC
      int len=synth.song[synth.songp++]-0x3e;
      if ((len<1)||(len>0x40)) PANIC
      len*=synth.tempo;
      synth.songdelay=(len*synth.rate)/1000;
    } else { // Note
      int chid=opcode-1;
      if (synth.songp>synth.songc-2) PANIC
      int noteid=synth.song[synth.songp++]-0x3f;
      int durms=synth.song[synth.songp++]-0x3f;
      if ((noteid<0)||(noteid>0x3f)||(durms<0)||(durms>0x3f)) PANIC
      durms*=synth.tempo;
      synth_play_note(chid,noteid,durms);
    }
  }
  #undef PANIC
}

/* Update.
 */
 
static void synth_update_mono(int16_t *v,int c) {
  while (c>0) {
    int updc=synth_update_song(c);
    if (updc<1) return;
    synth_update_voices(v,updc);
    v+=updc;
    c-=updc;
  }
}

static void synth_update_stereo(int16_t *v,int framec) {
  synth_update_mono(v,framec);
  int16_t *dst=v+(framec<<1);
  int16_t *src=v+framec;
  int i=framec;
  while (i-->0) {
    src--;
    *(--dst)=*src;
    *(--dst)=*src;
  }
}

static void synth_update_multi(int16_t *v,int framec) {
  synth_update_mono(v,framec);
  int extra=synth.chanc-2;
  int16_t *dst=v+(framec<<1);
  int16_t *src=v+framec;
  int i=framec;
  while (i-->0) {
    src--;
    dst-=extra;
    *(--dst)=*src;
    *(--dst)=*src;
  }
}
 
void synth_update(int16_t *v,int c,struct hostio_audio *driver) {
  memset(v,0,c<<1);
  int framec=c/synth.chanc;
  switch (synth.chanc) {
    case 1: synth_update_mono(v,c); break;
    case 2: synth_update_stereo(v,framec); break;
    default: synth_update_multi(v,framec); break;
  }
  while (synth.voicec&&(!synth.voicev[synth.voicec-1].inuse)) synth.voicec--;
}

/* Play song.
 */
 
void synth_play_song(const void *src,int srcc,int once) {
  if (srcc<2) return;
  synth.song=(uint8_t*)src+1;
  synth.songc=srcc-1;
  synth.songp=0;
  synth.songdelay=0;
  synth.songrepeat=!once;
  if ((synth.tempo=((uint8_t*)src)[0]-0x3e)<1) synth.tempo=1;
}

/* Play sound.
 */
 
void synth_play_sound(int id) {
  switch (id) {
    case 0: { // jump: [0.700, "sine", 300, 1000],
        struct voice *voice=synth_play_note(0,30,300);
        if (voice) {
          uint32_t targetdp=synth.dp_by_noteid[50];
          voice->ddp=(targetdp-voice->dp)/(synth.rate);
        }
      } break;
    case 1: { // die: [0.200, "sawtooth", 100],
        struct voice *voice=synth_play_note(0,20,400);
        if (voice) {
        }
      } break;
  }
}
