/* drivers.c
 * Our generic 'hostio' unit does the real work (it's under "opt/" but not actually optional).
 * This bit here is just an adapter.
 * TODO It would be polite of us to expose driver configuration via the command line: That should get wired up here.
 */
 
#include "native_internal.h"
#include "inmgr/inmgr.h"

/* Globals.
 */
 
static struct {
  struct hostio *hostio;
} drivers={0};

/* Quit.
 */
 
void drivers_quit() {
  inmgr_quit();
  hostio_del(drivers.hostio);
  memset(&drivers,0,sizeof(drivers));
}

/* Init.
 */
 
int drivers_init() {
  struct hostio_video_delegate vdel={
    .cb_close=cb_close,
    .cb_focus=cb_focus,
    .cb_resize=cb_resize,
    .cb_key=cb_key,
    .cb_text=cb_text,
    .cb_mmotion=cb_mmotion,
    .cb_mbutton=cb_mbutton,
    .cb_mwheel=cb_mwheel,
  };
  struct hostio_audio_delegate adel={
    .cb_pcm_out=synth_update,
  };
  struct hostio_input_delegate idel={
    .cb_connect=cb_connect,
    .cb_disconnect=cb_disconnect,
    .cb_button=cb_button,
  };
  if (!(drivers.hostio=hostio_new(&vdel,&adel,&idel))) return -1;
  
  if (inmgr_init()<0) return -1;
  inmgr_set_button_mask(INMGR_BTN_DPAD|INMGR_BTN_SOUTH|INMGR_BTN_AUX3);
  inmgr_set_signal(INMGR_BTN_QUIT,cb_quit);
  inmgr_set_signal(INMGR_BTN_FULLSCREEN,cb_fullscreen);
  
  {
    struct hostio_video_setup setup={
      .title="Economy of Motion",
      .iconrgba=0,//TODO
      .iconw=0,
      .iconh=0,
      .w=0,
      .h=0,
      .fullscreen=0,
      .fbw=SCREENW,
      .fbh=SCREENH,
      .device=0,
    };
    if (hostio_init_video(drivers.hostio,0,&setup)<0) return -1;
    if (drivers.hostio->video->type->provides_input) {
      g.devid_keyboard=hostio_input_devid_next();
      inmgr_connect_keyboard(g.devid_keyboard);
    }
  }
  
  {
    struct hostio_audio_setup setup={
      .rate=44100,
      .chanc=1,
      .device=0,
      .buffer_size=0,
    };
    if (hostio_init_audio(drivers.hostio,0,&setup)<0) return -1;
  }
  
  {
    struct hostio_input_setup setup={
      .path=0,
    };
    if (hostio_init_input(drivers.hostio,0,&setup)<0) return -1;
  }
  
  if (synth_init(drivers.hostio->audio->rate,drivers.hostio->audio->chanc)<0) return -1;
  
  hostio_log_driver_names(drivers.hostio);
  hostio_audio_play(drivers.hostio,1);
  return 0;
}

/* Update.
 */
 
int drivers_update() {
  if (hostio_update(drivers.hostio)<0) return -1;
  
  // Painstakingly set every button if they changed.
  // There's some amount of map-and-unmap, fake-it-and-un-fake-it, in order to keep the JS app unchanged.
  int input=inmgr_get_player(0);
  if (input!=g.pvinput) {
    #define _(tag,dstp) if ((input&INMGR_BTN_##tag)!=(g.pvinput&INMGR_BTN_##tag)) exec_set_gamepad_button(0,dstp,(input&INMGR_BTN_##tag)?1:0);
    _(LEFT,14)
    _(RIGHT,15)
    _(UP,12)
    _(DOWN,13)
    _(SOUTH,0)
    _(WEST,1)
    //_(AUX1,9)
    #undef _
    if ((input&INMGR_BTN_AUX3)&&!(g.pvinput&INMGR_BTN_AUX3)) g.terminate=1;
    if ((input&INMGR_BTN_AUX1)&&!(g.pvinput&INMGR_BTN_AUX1)) g.terminate=1; // Need AUX1 to quit too. The game doesn't use it. Some of my kiosk gamepads have no AUX3.
    g.pvinput=input;
  }
  
  return 0;
}

/* Accessors.
 */
 
struct hostio *drivers_get_hostio() {
  return drivers.hostio;
}
 
struct hostio_video *drivers_get_video() {
  return drivers.hostio->video;
}
