/* drivers.c
 * Our generic 'hostio' unit does the real work (it's under "opt/" but not actually optional).
 * This bit here is just an adapter.
 * TODO It would be polite of us to expose driver configuration via the command line: That should get wired up here.
 */
 
#include "native_internal.h"

/* Globals.
 */
 
static struct {
  struct hostio *hostio;
} drivers={0};

/* Quit.
 */
 
void drivers_quit() {
  hostio_del(drivers.hostio);
  memset(&drivers,0,sizeof(drivers));
}

/* Audio/synth glue.
 */
 
void cb_pcm_out(int16_t *v,int c,struct hostio_audio *driver) {
  memset(v,0,c<<1);//TODO
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
    .cb_pcm_out=cb_pcm_out,
  };
  struct hostio_input_delegate idel={
    .cb_connect=cb_connect,
    .cb_disconnect=cb_disconnect,
    .cb_button=cb_button,
  };
  if (!(drivers.hostio=hostio_new(&vdel,&adel,&idel))) return -1;
  
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
  
  //TODO synth
  
  hostio_log_driver_names(drivers.hostio);
  hostio_audio_play(drivers.hostio,1);
  return 0;
}

/* Update.
 */
 
int drivers_update() {
  if (hostio_update(drivers.hostio)<0) return -1;
  return 0;
}

/* Accessors.
 */
 
struct hostio_video *drivers_get_video() {
  return drivers.hostio->video;
}
