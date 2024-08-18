/* midi.h
 */
 
#ifndef MIDI_H
#define MIDI_H

#include <stdint.h>

struct midi_event {
  uint8_t chid;
  uint8_t opcode;
  uint8_t a,b;
  const void *v;
  int c;
  int trackid; // Reading from files, the zero-based index of the MTrk, not that you need to care.
};

/* File reader.
 *********************************************************************/
 
struct midi_file {
  int format;
  int division;
  int track_count; // as reported in header
  int rate;
  int usperqnote;
  double framespertick;
  double elapsed_ticks; // 0..1, collects roundoff error
  struct midi_track {
    const uint8_t *v;
    int c;
    int p;
    uint8_t status;
    int delay; // Pending delay in ticks, <0 if unset.
  } *trackv;
  int trackc,tracka;
};

void midi_file_del(struct midi_file *file);

/* We borrow (src) weakly.
 * Caller must keep it valid and constant as long as this file lives.
 * Typically, either you are reading the entire file synchronously to convert it,
 * or you've got the file from some global read-only archive.
 * If you need more dynamic ownership, you must take care of that on your own externally.
 *
 * (rate) is frames per second.
 * If zero, we'll report timing in natural ticks and disregard tempo.
 * That's not a normal thing to want.
 */
struct midi_file *midi_file_new(const void *src,int srcc,int rate);

/* If an event is ready, populate (event) and return 0.
 * Otherwise return the delay in frames until the next event.
 * Or <0 for real errors, eg EOF.
 */
int midi_file_next(struct midi_event *event,struct midi_file *file);

/* Advance the file reader's clock by (framec) which should be
 * no more than the last thing returned by midi_file_next().
 */
int midi_file_advance(struct midi_file *file,int framec);

/* You can call this after midi_file_next() returns <0, to distinguish EOF from encoding errors.
 */
int midi_file_is_finished(const struct midi_file *file);

/* Constants.
 *********************************************************************/

/* Opcodes.
 * Note that we strip chid from the Channel Voice commands.
 */
#define MIDI_OPCODE_NOTE_OFF       0x80
#define MIDI_OPCODE_NOTE_ON        0x90
#define MIDI_OPCODE_NOTE_ADJUST    0xa0
#define MIDI_OPCODE_CONTROL        0xb0
#define MIDI_OPCODE_PROGRAM        0xc0
#define MIDI_OPCODE_PRESSURE       0xd0
#define MIDI_OPCODE_WHEEL          0xe0 /* (a|(b<<7))-0x2000 */
#define MIDI_OPCODE_SYSEX_MORE     0xf0
#define MIDI_OPCODE_SYSEX_FINAL    0xf7
#define MIDI_OPCODE_RESET          0xff
#define MIDI_OPCODE_META           0x01

/* Control keys.
 * Many more are defined, just interesting ones here.
 */
#define MIDI_CONTROL_BANK_MSB       0x00
#define MIDI_CONTROL_MOD_MSB        0x01
#define MIDI_CONTROL_BREATH_MSB     0x02
#define MIDI_CONTROL_FOOT_MSB       0x04
#define MIDI_CONTROL_VOLUME_MSB     0x07
#define MIDI_CONTROL_BALANCE_MSB    0x08
#define MIDI_CONTROL_PAN_MSB        0x0a
#define MIDI_CONTROL_EXPR_MSB       0x0b
#define MIDI_CONTROL_BANK_LSB       0x20
#define MIDI_CONTROL_SUSTAIN        0x40
#define MIDI_CONTROL_SOUND_OFF      0x78
#define MIDI_CONTROL_RESET          0x79
#define MIDI_CONTROL_LOCAL_SWITCH   0x7a
#define MIDI_CONTROL_NOTES_OFF      0x7b
#define MIDI_CONTROL_OMNI_OFF       0x7c
#define MIDI_CONTROL_OMNI_ON        0x7d
#define MIDI_CONTROL_POLY_SWITCH    0x7e
#define MIDI_CONTROL_POLY_ON        0x7f

/* Meta keys.
 * 0x01..0x0f are text; key isn't so interesting.
 * Many others are defined. I don't really expect to use any but SET_TEMPO.
 */
#define MIDI_META_CHANNEL_PREFIX 0x20
#define MIDI_META_EOT            0x2f
#define MIDI_META_SET_TEMPO      0x51

#endif
