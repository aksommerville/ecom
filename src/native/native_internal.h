#ifndef NATIVE_INTERNAL_H
#define NATIVE_INTERNAL_H

#include "opt/hostio/hostio.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>

extern struct globals {
  int terminate;
} g;

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

#endif
