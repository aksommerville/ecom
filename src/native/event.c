#include "native_internal.h"

/* Connect input device.
 */
 
void cb_connect(struct hostio_input *driver,int devid) {
  fprintf(stderr,"%s %d\n",__func__,devid);
}

/* Disconnect input device.
 */
 
void cb_disconnect(struct hostio_input *driver,int devid) {
  fprintf(stderr,"%s %d\n",__func__,devid);
}

/* Joystick event.
 */
 
void cb_button(struct hostio_input *driver,int devid,int btnid,int value) {
  fprintf(stderr,"%s %d.%d=%d\n",__func__,devid,btnid,value);
}

/* Minor WM events.
 */
 
void cb_close(struct hostio_video *driver) {
  fprintf(stderr,"%s\n",__func__);
  g.terminate=1;
}
 
void cb_focus(struct hostio_video *driver,int focus) {
  fprintf(stderr,"%s %d\n",__func__,focus);
}
 
void cb_resize(struct hostio_video *driver,int w,int h) {
  fprintf(stderr,"%s %d,%d\n",__func__,w,h);
}

/* Keyboard.
 */
 
int cb_key(struct hostio_video *driver,int keycode,int value) {
  //fprintf(stderr,"%s 0x%08x=%d\n",__func__,keycode,value);
  if (value>=2) return 1;
  if (value) switch (keycode) {
    case 0x00070029: g.terminate=1; break; // Escape
  }
  const char *jsname=0;
  switch (keycode) {
    case 0x00070004: jsname="KeyA"; break;
    case 0x00070007: jsname="KeyD"; break;
    case 0x00070016: jsname="KeyS"; break;
    case 0x0007001a: jsname="KeyW"; break;
    case 0x0007001b: jsname="KeyX"; break;
    case 0x0007001d: jsname="KeyZ"; break;
    case 0x00070028: jsname="Enter"; break;
    case 0x0007002c: jsname="Space"; break;
    case 0x00070036: jsname="Comma"; break;
    case 0x0007004f: jsname="ArrowRight"; break;
    case 0x00070050: jsname="ArrowLeft"; break;
    case 0x00070051: jsname="ArrowDown"; break;
    case 0x00070052: jsname="ArrowUp"; break;
  }
  if (jsname) exec_send_key(jsname,value);
  return 1;
}

void cb_text(struct hostio_video *driver,int codepoint) {
  fprintf(stderr,"%s U+%x\n",__func__,codepoint);
}

/* Mouse.
 */
 
void cb_mmotion(struct hostio_video *driver,int x,int y) {
  //fprintf(stderr,"%s %d,%d\n",__func__,x,y);
}

void cb_mbutton(struct hostio_video *driver,int btnid,int value) {
  fprintf(stderr,"%s %d=%d\n",__func__,btnid,value);
}

void cb_mwheel(struct hostio_video *driver,int dx,int dy) {
  fprintf(stderr,"%s %+d,%+d\n",__func__,dx,dy);
}
