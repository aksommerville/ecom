#ifndef NATIVE_INTERNAL_H
#define NATIVE_INTERNAL_H

#include "opt/hostio/hostio.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>

#define SCREENW 256
#define SCREENH 144

#define IMAGE_SIZE_LIMIT 1024

extern struct globals {
  int terminate;
  double next_time;
  double start_real;
  double start_cpu;
  int framec;
  int devid_keyboard;
  int pvinput;
} g;

struct image {
  void *v;
  int w,h,stride;
  int pixelsize;
};

// image.c
void image_del(struct image *image);
struct image *image_new_alloc(int pixelsize,int w,int h);
int image_force_rgba(struct image *image);

// png.c
struct image *png_decode(const void *src,int srcc);

// rom.c
int rom_init();
int rom_get_image(void *dstpp);
int rom_get_song(void *dstpp,const char *name,int namec);
int rom_get_stage(void *dstpp,int index);
int rom_get_script(void *dstpp);

// drivers.c
void drivers_quit();
int drivers_init();
int drivers_update();
struct hostio *drivers_get_hostio();
struct hostio_video *drivers_get_video();

// event.c, connected via drivers.c
void cb_connect(struct hostio_input *driver,int devid);
void cb_disconnect(struct hostio_input *driver,int devid);
void cb_button(struct hostio_input *driver,int devid,int btnid,int value);
void cb_close(struct hostio_video *driver);
void cb_focus(struct hostio_video *driver,int focus);
void cb_resize(struct hostio_video *driver,int w,int h);
int cb_key(struct hostio_video *driver,int keycode,int value);
void cb_text(struct hostio_video *driver,int codepoint);
void cb_mmotion(struct hostio_video *driver,int x,int y);
void cb_mbutton(struct hostio_video *driver,int btnid,int value);
void cb_mwheel(struct hostio_video *driver,int dx,int dy);
void cb_quit();
void cb_fullscreen();

// exec.c
int exec_init();
int exec_update();
void exec_send_key(const char *jsname,int value);
/* Our JS app looks for the following key names:
  ArrowLeft
  KeyA
  ArrowRight
  KeyD
  ArrowUp
  KeyW
  ArrowDown
  KeyS
  Space
  KeyZ
  Comma
  KeyX
  Enter
  ...also Escape, but we'll handle that one separately.
*/
void exec_remove_gamepad(int index);
void exec_add_gamepad(int index,int axisc,int btnc);
void exec_set_gamepad_axis(int index,int axisp,double v);
void exec_set_gamepad_button(int index,int btnp,int v);

// render.c
int render_init();
int render_begin();
int render_end();
void render_reset_gradient(int x,int y);
void render_set_gradient(int p,int rgb);
void render_fill_bg();
void render_copy_bg();
void render_fill_rect(int x,int y,int w,int h,int rgb);
void render_blit(int dsttexid,int dstx,int dsty,int srcx,int srcy,int w,int h,int flop);
void render_decint(int x,int y,int v,int digitc);

// clock.c
void clock_init();
void clock_update(); // may block
void clock_report();

// synth.c
int synth_init(int rate,int chanc);
void synth_update(int16_t *v,int c,struct hostio_audio *driver);
void synth_play_song(const void *serial,int serialc,int once);
void synth_play_sound(int id);

#endif
