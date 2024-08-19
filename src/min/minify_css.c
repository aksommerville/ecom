#include "min_internal.h"
#include <stdarg.h>

/* Log error.
 */

static int css_error(const char *src,int srcp,const char *refname,const char *fmt,...) {
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

/* Minify full selector list, should stop on the '{' (or on space before it).
 * Whitespace is meaningful in a selector. I'm not sure of the exact rules, and don't care to study it just now.
 * We'll preserve these verbatim, only eliminating leading and trailing space.
 * No provision for quoted '{' eg in an attribute selector. If you write a selector like that, you'll get what you deserve.
 */

static int minify_css_selector(const char *src,int srcc,const char *refname) {
  int stop=0;
  for (;;) {
    if (stop>=srcc) return css_error(src,0,refname,"Expected '{' after CSS selector.");
    if (src[stop]=='{') break;
    stop++;
  }
  while (stop&&((unsigned char)src[stop-1]<=0x20)) stop--;
  int srcp=0;
  while ((srcp<stop)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  if (sr_encode_raw(&min.dst,src+srcp,stop-srcp)<0) return -1; // <-- verbatim output, no real digestion
  return stop;
}

/* Minify one CSS property (both key and value).
 */

static int minify_css_property(const char *src,int srcc,const char *refname) {
  int srcp=0;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  const char *k=src+srcp;
  int kc=0;
  while ((srcp<srcc)&&(src[srcp]!=':')) { srcp++; kc++; }
  if (srcp>=srcc) return css_error(src,srcp,refname,"Expected ':' after CSS property key.");
  srcp++;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  if (sr_encode_raw(&min.dst,k,kc)<0) return -1;
  if (sr_encode_u8(&min.dst,':')<0) return -1;
  int needspace=0;
  for (;;) {
    if (srcp>=srcc) return css_error(src,srcp,refname,"Unterminated CSS property.");
    if (src[srcp]==';') {
      if (sr_encode_u8(&min.dst,';')<0) return -1;
      srcp++;
      return srcp;
    }
    if ((unsigned char)src[srcp]<=0x20) { srcp++; continue; }
    const char *v=src+srcp;
    int vc=0;
    while ((srcp<srcc)&&((unsigned char)src[srcp]>0x20)&&(src[srcp]!=';')) { srcp++; vc++; }
    if (needspace) {
      if (sr_encode_u8(&min.dst,' ')<0) return -1;
    } else {
      needspace=1;
    }
    //TODO Opportunity to reduce individual CSS values. eg "#cccccc" => "#ccc". Is there anything worth doing?
    if (sr_encode_raw(&min.dst,v,vc)<0) return -1;
  }
}

/* Minify one CSS rule.
 * We'll put a newline after each rule to avoid ridiculously long lines. It's not technically necessary.
 */

static int minify_css_rule(const char *src,int srcc,const char *refname) {
  int srcp=0,err;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  if ((err=minify_css_selector(src,srcc,refname))<0) return err;
  srcp+=err;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  if ((srcp>=srcc)||(src[srcp++]!='{')) return css_error(src,srcp,refname,"Expected '{'");
  if (sr_encode_u8(&min.dst,'{')<0) return -1;
  for (;;) {
    if (srcp>=srcc) return css_error(src,srcp,refname,"Unclosed CSS block.");
    if (src[srcp]=='}') {
      srcp++;
      if ((min.dst.c>0)&&(((char*)min.dst.v)[min.dst.c-1]==';')) min.dst.c--; // Eliminate last semicolon in block.
      if (sr_encode_u8(&min.dst,'}')<0) return -1;
      if (sr_encode_u8(&min.dst,0x0a)<0) return -1; // Spurious extra newline between rules.
      return srcp;
    }
    if ((unsigned char)src[srcp]<=0x20) { srcp++; continue; }
    if ((err=minify_css_property(src+srcp,srcc-srcp,refname))<0) return err;
    if (!err) return css_error(src,srcp,refname,"Failed to parse CSS rules.");
    srcp+=err;
  }
}

/* Minify CSS, main entry point.
 */

int minify_css(const char *src,int srcc,int require_closing_tag,const char *refname) {
  if (sr_encode_raw(&min.dst,"<style>\n",-1)<0) return -1;
  int srcp=0;
  while (srcp<srcc) {
    if ((unsigned char)src[srcp]<=0x20) { srcp++; continue; }

    if (src[srcp]=='<') {
      if (!require_closing_tag) break;
      int err=consume_closing_tag(src+srcp,srcc-srcp,"style",5,0);
      if (err<0) return css_error(src,srcp,refname,"Expected '</style>'");
      srcp+=err;
      break;
    }
    
    int err=minify_css_rule(src+srcp,srcc-srcp,refname);
    if (!err) {
      if (require_closing_tag) return css_error(src,srcp,refname,"Expected '</style>'");
      break;
    }
    if (err<0) return css_error(src,srcp,refname,"Failed to minify CSS.");
    srcp+=err;
  }
  if (sr_encode_raw(&min.dst,"</style>\n",-1)<0) return -1;
  return srcp;
}
