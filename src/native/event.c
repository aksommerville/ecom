#include "native_internal.h"

/* Private globals.
 */

// (devid) should be assigned monotonically from 1. We'll use them as gamepad index.
// If they go too high, start ignoring devices.
#define JOYSTICK_LIMIT 32

#define MAPC_SANITY_LIMIT 1024

static struct {
  struct map {
    int devid,btnid; // per driver
    int mode; // 0=ignore, 1=button, 2=axis
    int dstp; // button or axis index
    int srclo,srchi; // inclusive range for buttons, or limits for axis
    int dstvalue;
    int effective; // Axes will predict the JS app's mapping, and don't report a change if insignificant. Because src btns share outputs.
  } *mapv;
  int mapc,mapa;
} input={0};

/* Map list.
 */
 
static int input_mapv_search(int devid,int btnid) {
  int lo=0,hi=input.mapc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    const struct map *map=input.mapv+ck;
         if (devid<map->devid) hi=ck;
    else if (devid>map->devid) lo=ck+1;
    else if (btnid<map->btnid) hi=ck;
    else if (btnid>map->btnid) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

static struct map *input_mapv_add(int devid,int btnid) {
  int p=input_mapv_search(devid,btnid);
  if (p>=0) return input.mapv+p;
  if (input.mapc>MAPC_SANITY_LIMIT) return 0;
  p=-p-1;
  if (input.mapc>=input.mapa) {
    int na=input.mapa+32;
    if (na>INT_MAX/sizeof(struct map)) return 0;
    void *nv=realloc(input.mapv,sizeof(struct map)*na);
    if (!nv) return 0;
    input.mapv=nv;
    input.mapa=na;
  }
  struct map *map=input.mapv+p;
  memmove(map+1,map,sizeof(struct map)*(input.mapc-p));
  input.mapc++;
  memset(map,0,sizeof(struct map));
  map->devid=devid;
  map->btnid=btnid;
  return map;
}

/* Connect input device.
 */
 
static int cb_devcap(int btnid,int hidusage,int lo,int hi,int value,void *userdata) {
  int devid=*(int*)userdata;
  int mode=0,dstp=0;
  switch (hidusage) {
    case 0x00010030: mode=2; dstp=0; break;
    case 0x00010031: mode=2; dstp=1; break;
  }
  if ((lo==0)&&((hi==1)||(hi==2))) {
    mode=1;
    dstp=0; // We declared 3 buttons, but ended up only using A.
  }
  if (!mode) return 0;
  struct map *map=input_mapv_add(devid,btnid);
  if (!map) return 0;
  map->mode=mode;
  map->dstp=dstp;
  if (mode==1) {
    map->srclo=1;
    map->srchi=INT_MAX;
  } else {
    map->srclo=lo;
    map->srchi=hi;
  }
  return 0;
}
 
void cb_connect(struct hostio_input *driver,int devid) {
  if ((devid<1)||(devid>JOYSTICK_LIMIT)) return;
  if (!driver->type->for_each_button) return;
  driver->type->for_each_button(driver,devid,cb_devcap,&devid);
  
  int axisc=0,btnc=0;
  int p=input_mapv_search(devid,0);
  if (p<0) p=-p-1;
  while ((p<input.mapc)&&(input.mapv[p].devid==devid)) {
    switch (input.mapv[p].mode) {
      case 1: if (input.mapv[p].dstp>=btnc) btnc=input.mapv[p].dstp+1; break;
      case 2: if (input.mapv[p].dstp>=axisc) axisc=input.mapv[p].dstp+1; break;
    }
    p++;
  }
  
  exec_add_gamepad(devid,axisc,btnc);
}

/* Disconnect input device.
 */
 
void cb_disconnect(struct hostio_input *driver,int devid) {
  exec_remove_gamepad(devid);
  int p=input_mapv_search(devid,0);
  if (p<0) p=-p-1;
  int rmc=0;
  while ((p+rmc<input.mapc)&&(input.mapv[p+rmc].devid==devid)) rmc++;
  if (rmc) {
    input.mapc-=rmc;
    memmove(input.mapv+p,input.mapv+p+rmc,sizeof(struct map)*(input.mapc-p));
  }
}

/* Joystick event.
 */
 
void cb_button(struct hostio_input *driver,int devid,int btnid,int value) {
  int p=input_mapv_search(devid,btnid);
  if (p<0) return;
  struct map *map=input.mapv+p;
  switch (map->mode) {
    case 1: {
        int dstvalue=((value>=map->srclo)&&(value<=map->srchi))?1:0;
        if (dstvalue==map->dstvalue) return;
        map->dstvalue=dstvalue;
        exec_set_gamepad_button(devid,map->dstp,dstvalue);
      } break;
    case 2: {
        int dstvalue=value;
        if (dstvalue<map->srclo) dstvalue=map->srclo;
        else if (dstvalue>map->srchi) dstvalue=map->srchi;
        if (dstvalue==map->dstvalue) return;
        map->dstvalue=dstvalue;
        double fv=((dstvalue-map->srclo)*2.0)/(map->srchi-map->srclo)-1.0;
        int iv=(fv<-0.25)?-1:(fv>0.25)?1:0;
        if (iv==map->effective) return;
        map->effective=iv;
        exec_set_gamepad_axis(devid,map->dstp,fv);
      } break;
  }
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
}

/* Mouse.
 */
 
void cb_mmotion(struct hostio_video *driver,int x,int y) {
}

void cb_mbutton(struct hostio_video *driver,int btnid,int value) {
}

void cb_mwheel(struct hostio_video *driver,int dx,int dy) {
}
