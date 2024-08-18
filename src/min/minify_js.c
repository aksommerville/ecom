#include "min_internal.h"
#include "jst.h"
#include <stdarg.h>

/* Log error.
 */

static int js_error(const char *src,int srcp,const char *refname,const char *fmt,...) {
  if (!refname) {
    int pfxp=(int)(src-min.src);
    if ((pfxp<0)||(pfxp>min.srcc-srcp)) {
      refname="<unknown>";
    } else {
      refname=min.srcpath;
      src=min.src;
      srcp+=pfxp;
    }
  }
  fprintf(stderr,"%s:%d: ",refname,lineno(src,srcp));
  va_list vargs;
  va_start(vargs,fmt);
  vfprintf(stderr,fmt,vargs);
  fprintf(stderr,"\n");
  return -2;
}

/* Minify Javascript, main entry point.
 */

int minify_js(const char *src,int srcc,int require_closing_tag,const char *refname) {

  if (!src) srcc=0; else if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  int result=srcc;
  if (require_closing_tag) {
    int stopp=srcc-9;
    int i=0; for (;i<stopp;i++) {
      if (!memcmp(src+i,"</script>",9)) {
        srcc=i;
        result=i+9;
        break;
      }
    }
  }
  
  if (sr_encode_raw(&min.dst,"<script>\n",-1)<0) return -1;
  
  struct jst_context jst={0};
  int err=jst_minify(&min.dst,&jst,src,srcc,refname);
  if (err<0) {
    if (err!=-2) return js_error(src,0,refname,"Unspecified error minifying Javascript.");
    jst_context_cleanup(&jst);
    return -2;
  }
  jst_context_cleanup(&jst);
  if (sr_encode_raw(&min.dst,"</script>",-1)<0) return -1;
  return result;
}
