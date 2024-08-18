#include "min_internal.h"
#include "midi.h"

static const char *alpha_opcode = "'()*+,-./01234567"; // delay,ch0,...,ch15
static const char *alpha_data = "?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~"; // 0..63

struct held {
  uint8_t chid;
  uint8_t noteid;
  int durp;
  int when;
};

#define HELD_LIMIT 64

/* Convert MIDI to our format and append to output.
 */
 
int minify_song(const void *src,int srcc,const char *refname) {
  const int tempo=10; // One is free to change this. I first thought I'd find some best-fit for the notes' timing, but that's too complicated.
  struct held heldv[HELD_LIMIT];
  int heldc=0;
  if (sr_encode_u8(&min.dst,alpha_data[tempo-1])<0) return -1;
  struct midi_file *file=midi_file_new(src,srcc,1000);
  if (!file) {
    fprintf(stderr,"%s: Failed to decode MIDI file\n",refname);
    return -2;
  }
  int delay=0;
  int when=0;
  #define FLUSHDELAY { \
    int tickc=delay/tempo; \
    delay-=tickc*tempo; \
    while (tickc>0x40) { \
      sr_encode_u8(&min.dst,'\''); \
      sr_encode_u8(&min.dst,alpha_data[0x3f]); \
      tickc-=0x40; \
    } \
    if (tickc>0) { \
      sr_encode_u8(&min.dst,'\''); \
      sr_encode_u8(&min.dst,alpha_data[tickc-1]); \
    } \
  }
  for (;;) {
    struct midi_event event={0};
    int err=midi_file_next(&event,file);
    if (err<0) {
      if (midi_file_is_finished(file)) break;
      fprintf(stderr,"%s: Error streaming from MIDI file\n",refname);
      midi_file_del(file);
      return -2;
    }
    if (err>0) {
      delay+=err;
      when+=err;
      if (midi_file_advance(file,err)<0) {
        midi_file_del(file);
        return -1;
      }
    } else {
      switch (event.opcode) {
      
        case MIDI_OPCODE_NOTE_OFF: {
            int i=0;
            struct held *held=heldv;
            for (;i<heldc;i++,held++) {
              if (held->chid!=event.chid) continue;
              if (held->noteid!=event.a) continue;
              break;
            }
            if (i>=heldc) break;
            int dur=(when-held->when)/tempo;
            if (dur<0) dur=0;
            else if (dur>0x3f) dur=0x3f;
            ((uint8_t*)min.dst.v)[held->durp]=alpha_data[dur];
            heldc--;
            memmove(held,held+1,sizeof(struct held)*(heldc-i));
          } break;
          
        case MIDI_OPCODE_NOTE_ON: {
            int noteid=event.a-0x23;
            if ((noteid<0)||(noteid>=0x40)) {
              fprintf(stderr,"%s: Ignoring OOB note 0x%02x on track %d, channel %d.\n",refname,event.a,event.trackid,event.chid);
              continue;
            }
            FLUSHDELAY
            sr_encode_u8(&min.dst,alpha_opcode[1+event.chid]);
            sr_encode_u8(&min.dst,alpha_data[noteid]);
            int durp=min.dst.c;
            sr_encode_u8(&min.dst,alpha_data[0]); // placeholder for duration
            if (heldc<HELD_LIMIT) { // 64 notes at once is insane, but if it happens, we stop sustaining, not the end of the world.
              struct held *held=heldv+heldc++;
              held->chid=event.chid;
              held->noteid=event.a;
              held->durp=durp;
              held->when=when;
            }
          } break;
          
      }
    }
  }
  FLUSHDELAY
  #undef FLUSHDELAY
  midi_file_del(file);
  if (sr_encode_u8(&min.dst,0x0a)<0) return -1;
  return 0;
}
