#include "native_internal.h"
#include <signal.h>
#include <unistd.h>

struct globals g={0};

/* Signal handler.
 */
 
static volatile int sigc=0;

static void rcvsig(int sigid) {
  switch (sigid) {
    case SIGINT: if (++sigc>=3) {
        fprintf(stderr,"Too many unprocessed signals.\n");
        exit(1);
      } break;
  }
}

/* Main.
 */

int main(int argc,char **argv) {
  int err;
  signal(SIGINT,rcvsig);
  if ((err=rom_init())<0) {
    if (err!=-2) fprintf(stderr,"%s: Failed to decode built-in ROM\n",argv[0]);
    return 1;
  }
  if ((err=drivers_init())<0) {
    if (err!=-2) fprintf(stderr,"%s: Unspecified error initializing drivers.\n",argv[0]);
    return 1;
  }
  if ((err=exec_init())<0) {
    if (err!=-2) fprintf(stderr,"%s: Unspecified error standing Javascript runtime.\n",argv[0]);
    return 1;
  }
  
  while (!sigc&&!g.terminate) {
    if ((err=drivers_update())<0) {
      if (err!=-2) fprintf(stderr,"%s: Unspecified error updating drivers.\n",argv[0]);
      return 1;
    }
    usleep(500000);//TODO sane clock
    if ((err=exec_update())<0) {
      if (err!=-2) fprintf(stderr,"%s: Unspecified error updating JS runtime.\n",argv[0]);
      return 1;
    }
    //TODO Commit video frame.
  }
  
  //TODO Report performance.
  drivers_quit();
  return 0;
}
