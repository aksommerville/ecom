/* We regulate time, blocking if necessary.
 * But we are not responsible for measuring it, for the game's update.
 * It does that on its own with a call to Date.now().
 */

#include "native_internal.h"
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#define CLOCK_INTERVAL (1.0/60.0)

/* Primitives.
 */
 
static double clock_now_real() {
  struct timeval tv={0};
  gettimeofday(&tv,0);
  return (double)tv.tv_sec+(double)tv.tv_usec/1000000.0;
}

static double clock_now_cpu() {
  struct timespec tv={0};
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&tv);
  return (double)tv.tv_sec+(double)tv.tv_nsec/1000000000.0;
}

static void clock_sleep(double s) {
  usleep((int)(s*1000000.0));
}

/* Init.
 */
 
void clock_init() {
  g.start_real=clock_now_real();
  g.start_cpu=clock_now_cpu();
  g.next_time=g.start_real;
  g.framec=0;
}

/* Update.
 */
 
void clock_update() {
  double now=clock_now_real();
  double pending=g.next_time-now;
  if (pending>0.0) {
    if (pending>0.100) {
      fprintf(stderr,"clock panic! pending=%f\n",pending);
      g.next_time=now;
    } else {
      clock_sleep(pending);
    }
  }
  g.next_time+=CLOCK_INTERVAL;
  if (g.next_time<now) g.next_time=now+CLOCK_INTERVAL;
  g.framec++;
}

/* Final report.
 */
 
void clock_report() {
  if (g.framec<1) return;
  double elapsed_real=clock_now_real()-g.start_real;
  double elapsed_cpu=clock_now_cpu()-g.start_cpu;
  double avgrate=(double)g.framec/elapsed_real;
  double cpuload=elapsed_cpu/elapsed_real;
  fprintf(stderr,"%d frames in %.03f s, average rate %.03f hz, cpu load %.03f\n",g.framec,elapsed_real,avgrate,cpuload);
}
