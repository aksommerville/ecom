#include "native_internal.h"
#include "inmgr/inmgr.h"

/* Connect input device.
 */
 
static int cb_devcap(int btnid,int hidusage,int lo,int hi,int value,void *userdata) {
  inmgr_connect_more(*(int*)userdata,btnid,hidusage,lo,hi,value);
  return 0;
}
 
void cb_connect(struct hostio_input *driver,int devid) {
  int vid=0,pid=0,version=0;
  const char *name=0;
  if (driver->type->get_ids) name=driver->type->get_ids(&vid,&pid,&version,driver,devid);
  inmgr_connect_begin(devid,vid,pid,version,name,-1);
  if (driver->type->for_each_button) driver->type->for_each_button(driver,devid,cb_devcap,&devid);
  inmgr_connect_end(devid);
}

/* Disconnect input device.
 */
 
void cb_disconnect(struct hostio_input *driver,int devid) {
  inmgr_disconnect(devid);
}

/* Joystick event.
 */
 
void cb_button(struct hostio_input *driver,int devid,int btnid,int value) {
  inmgr_event(devid,btnid,value);
}

/* Minor WM events.
 */
 
void cb_close(struct hostio_video *driver) {
  g.terminate=1;
}
 
void cb_focus(struct hostio_video *driver,int focus) {
}
 
void cb_resize(struct hostio_video *driver,int w,int h) {
}

/* Keyboard.
 */
 
int cb_key(struct hostio_video *driver,int keycode,int value) {
  inmgr_event(g.devid_keyboard,keycode,value);
  return 1;
}

void cb_text(struct hostio_video *driver,int codepoint) {
}

/* Mouse.
 */
 
void cb_mmotion(struct hostio_video *driver,int x,int y) {
}

void cb_mbutton(struct hostio_video *driver,int btnid,int value) {
}

void cb_mwheel(struct hostio_video *driver,int dx,int dy) {
}

/* Inmgr signals.
 */
 
void cb_quit() {
  g.terminate=1;
}

void cb_fullscreen() {
  hostio_toggle_fullscreen(drivers_get_hostio());
}
