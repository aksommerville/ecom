#include "min_internal.h"
#include "fs.h"

struct min min={0};

/* --help
 */

static void print_usage() {
  fprintf(stderr,
    "Usage: %s -oOUTPUT INPUT [--native]\n"
    "OUTPUT and INPUT are both HTML files.\n"
    "With --native, INPUT is the packed HTML file and OUTPUT is a binary archive for embedding in the native app.\n"
    ,min.exename
  );
}

/* Main.
 */

int main(int argc,char **argv) {

  if ((argc>=1)&&argv&&argv[0]&&argv[0][0]) min.exename=argv[0];
  else min.exename="min";
  int i=1; for (;i<argc;i++) {
    const char *arg=argv[i];
    if (!arg||!arg[0]) continue;
    if (!strcmp(arg,"--help")) {
      print_usage();
      return 0;
    }
    if (!memcmp(arg,"-o",2)) {
      if (min.dstpath) {
        fprintf(stderr,"%s: Multiple output paths.\n",min.exename);
        return 1;
      }
      min.dstpath=arg+2;
    } else if (!strcmp(arg,"--native")) {
      min.native=1;
    } else if (arg[0]=='-') {
      fprintf(stderr,"%s: Unexpected argument '%s'\n",min.exename,arg);
      return 1;
    } else if (min.srcpath) {
      fprintf(stderr,"%s: Multiple input paths.\n",min.exename);
      return 1;
    } else {
      min.srcpath=arg;
    }
  }
  if (!min.srcpath||!min.dstpath) {
    print_usage();
    return 1;
  }

  if ((min.srcc=file_read(&min.src,min.srcpath))<0) {
    fprintf(stderr,"%s: Failed to read file.\n",min.srcpath);
    return 1;
  }

  int err=min.native?compile_archive():minify();
  if (err<0) {
    if (err!=-2) fprintf(stderr,"%s: Unspecified error during %s.\n",min.srcpath,min.native?"recompilation":"minification");
    return 1;
  }

  if (file_write(min.dstpath,min.dst.v,min.dst.c)<0) {
    fprintf(stderr,"%s: Failed to write output file, %d bytes\n",min.dstpath,min.dst.c);
    return 1;
  }

  return 0;
}
