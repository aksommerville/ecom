#include "min_internal.h"

/* Measure HTML token.
 * This is one of:
 *  - Whitespace.
 *  - A single tag (open or close).
 *  - A comment or PI.
 *  - Text up to the next tag, with trailing whitespace trimmed.
 * Never empty if input isn't, but errors are possible.
 */

#define TTYPE_SPACE 0
#define TTYPE_COMMENT 1
#define TTYPE_PI 2 /* eg DOCTYPE */
#define TTYPE_SINGLETON 3 /* self-closing HTML tag */
#define TTYPE_OPEN 4
#define TTYPE_CLOSE 5
#define TTYPE_TEXT 6

static int measure_html_token(const char *src,int srcc,int *ttype) {
  if (srcc<1) return 0;

  if ((unsigned char)src[0]<=0x20) {
    *ttype=TTYPE_SPACE;
    int srcp=1;
    while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
    return srcp;
  }

  if (src[0]=='<') {
    if (srcc<2) return -1;
    if (src[1]=='?') {
      *ttype=TTYPE_PI;
      int srcp=2;
      for (;;srcp++) {
        if (srcp>=srcc) return 0;
        if (src[srcp]=='>') return srcp+1;
      }
    }
    if (src[1]=='!') {
      if ((srcc>=4)&&(src[2]=='-')&&(src[3]=='-')) {
        *ttype=TTYPE_COMMENT;
        int srcp=4;
        int stopp=srcc-3;
        for (;;srcp++) {
          if (srcp>stopp) return 0;
          if ((src[srcp]=='-')&&(src[srcp+1]=='-')&&(src[srcp+2]=='>')) return srcp+3;
        }
      } else {
        *ttype=TTYPE_PI;
        int srcp=2;
        for (;;srcp++) {
          if (srcp>=srcc) return 0;
          if (src[srcp]=='>') return srcp+1;
        }
      }
    }
    if (src[1]=='/') {
      *ttype=TTYPE_CLOSE;
      int srcp=2;
      for (;;srcp++) {
        if (srcp>=srcc) return 0;
        if (src[srcp]=='>') return srcp+1;
      }
    }
    *ttype=TTYPE_OPEN; // unless we find otherwise
    int srcp=1,quote=0;
    for (;;srcp++) {
      if (srcp>=srcc) return 0;
      if (src[srcp]=='"') quote=!quote;
      else if (quote) ;
      else if (src[srcp]=='/') *ttype=TTYPE_SINGLETON;
      else if (src[srcp]=='>') return srcp+1;
    }
  }

  *ttype=TTYPE_TEXT;
  int srcp=1;
  while ((srcp<srcc)&&(src[srcp]!='<')) srcp++;
  while (srcp&&((unsigned char)src[srcp-1]<=0x20)) srcp--;
  return srcp;
}

/* Tag name for HTML open, close, or singleton tags.
 */

static int html_tag_name(void *dstpp,const char *src,int srcc) {
  int srcp=0;
  while (srcp<srcc) {
    if ((unsigned char)src[srcp]<=0x20) srcp++;
    else if (src[srcp]=='<') srcp++;
    else if (src[srcp]=='/') srcp++;
    else break;
  }
  *(const void**)dstpp=src+srcp;
  int tokenc=0;
  while ((srcp<srcc)&&((unsigned char)src[srcp]>0x20)&&(src[srcp]!='/')&&(src[srcp]!='>')) { srcp++; tokenc++; }
  return tokenc;
}

/* Value of one attribute in an HTML open or singleton tag.
 * We don't do anything special for character entities.
 */

static int html_get_attribute(void *dstpp,const char *src,int srcc,const char *k,int kc) {
  int srcp=0;
  while (srcp<srcc) {
    if ((unsigned char)src[srcp]<=0x20) { srcp++; continue; }
    if (src[srcp]=='<') { srcp++; continue; }
    if (src[srcp]=='>') return 0;
    if (src[srcp]=='/') { srcp++; continue; }
    const char *q=src+srcp;
    int qc=0;
    while ((srcp<srcc)&&(src[srcp]!='=')&&(src[srcp]!='/')&&(src[srcp]!='>')&&((unsigned char)src[srcp]>0x20)) { srcp++; qc++; }
    const char *v=0;
    int vc=0;
    if ((srcp<srcc)&&(src[srcp]=='=')) {
      srcp++;
      if ((srcp<srcc)&&(src[srcp]=='"')) {
        srcp++;
        v=src+srcp;
        for (;;) {
          if (srcp>=srcc) return 0;
          if (src[srcp++]=='"') break;
          vc++;
        }
      }
    }
    if ((qc==kc)&&!memcmp(q,k,kc)) {
      *(const void**)dstpp=v;
      return vc;
    }
  }
  return 0;
}

/* Require a closing tag of the given tagname to be next, return distance across it.
 * Whitespace is permitted but comments are not: This is typically used within <style> or <script> blocks.
 * ...but if (allow_text), we allow and discard all inner text. Still no tags or comments though.
 */

int consume_closing_tag(const char *src,int srcc,const char *name,int namec,int allow_text) {
  if (!name) namec=0; else if (namec<0) { namec=0; while (name[namec]) namec++; }
  if (namec<1) return -1;
  int srcp=0;
  for (;;) {
    if (srcp>=srcc) return -1;
    if ((unsigned char)src[srcp]<=0x20) { srcp++; continue; }
    if (src[srcp++]!='<') {
      if (allow_text) continue;
      return -1;
    }
    while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++; // I think whitespace is actually forbidden here, not sure.
    if ((srcp>=srcc)||(src[srcp++]!='/')) return -1;
    while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
    if ((srcp>srcc-namec)||memcmp(src+srcp,name,namec)) return -1;
    srcp+=namec;
    while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
    if ((srcp>=srcc)||(src[srcp++]!='>')) return -1;
    return srcp;
  }
  return -1;
}

/* Inline Javascript from a <script> tag with 'src'.
 */

static int minify_external_js(const char *href,int hrefc) {
  char jspath[1024];
  int jspathc=resolve_path(jspath,sizeof(jspath),min.srcpath,href,hrefc);
  if ((jspathc<1)||(jspathc>=sizeof(jspath))) {
    fprintf(stderr,"%s: Failed to resolve script path '%.*s'\n",min.srcpath,hrefc,href);
    return -2;
  }
  char *serial=0;
  int serialc=file_read(&serial,jspath);
  if (serialc<0) {
    fprintf(stderr,"%s: Failed to read file\n",jspath);
    return -2;
  }
  int err=minify_js(serial,serialc,0,jspath);
  free(serial);
  return err;
}

/* Inline CSS from a <link>
 */

static int minify_external_css(const char *href,int hrefc) {
  char csspath[1024];
  int csspathc=resolve_path(csspath,sizeof(csspath),min.srcpath,href,hrefc);
  if ((csspathc<1)||(csspathc>=sizeof(csspath))) {
    fprintf(stderr,"%s: Failed to resolve stylesheet path '%.*s'\n",min.srcpath,hrefc,href);
    return -2;
  }
  char *serial=0;
  int serialc=file_read(&serial,csspath);
  if (serialc<0) {
    fprintf(stderr,"%s: Failed to read file\n",csspath);
    return -2;
  }
  int err=minify_css(serial,serialc,0,csspath);
  free(serial);
  return err;
}

/* Convert MIDI to our song format and inline it for a <song> tag.
 */
 
static int minify_external_song(const char *href,int hrefc) {
  char path[1024];
  int pathc=resolve_path(path,sizeof(path),min.srcpath,href,hrefc);
  if ((pathc<1)||(pathc>=sizeof(path))) {
    fprintf(stderr,"%s: Failed to resolve song path '%.*s'\n",min.srcpath,hrefc,href);
    return -2;
  }
  char *serial=0;
  int serialc=file_read(&serial,path);
  if (serialc<0) {
    fprintf(stderr,"%s: Failed to read file\n",path);
    return -2;
  }
  const char *name=href;
  int namec=0;
  int i=0,counting=1;
  for (;i<hrefc;i++) {
    if (href[i]=='/') {
      name=href+i+1;
      namec=0;
      counting=1;
    } else if (href[i]=='.') {
      counting=0;
    } else if (counting) {
      namec++;
    }
  }
  if (sr_encode_fmt(&min.dst,"<song name=\"%.*s\">\n",namec,name)<0) {
    free(serial);
    return -1;
  }
  int err=minify_song(serial,serialc,path);
  free(serial);
  if (err<0) {
    if (err!=-2) fprintf(stderr,"%s: Unspecified error converting MIDI file.\n",path);
    return -2;
  }
  if (sr_encode_fmt(&min.dst,"</song>")<0) return -1;
  return 0;
}

/* Compile stage from loose human-friendly text to the incomprehensible gibberish we ship.
 */
 
static int minify_external_stage(const char *href,int hrefc) {
  char path[1024];
  int pathc=resolve_path(path,sizeof(path),min.srcpath,href,hrefc);
  if ((pathc<1)||(pathc>=sizeof(path))) {
    fprintf(stderr,"%s: Failed to resolve song path '%.*s'\n",min.srcpath,hrefc,href);
    return -2;
  }
  if (sr_encode_raw(&min.dst,"<stage>",-1)<0) return -1;
  char *serial=0;
  int serialc=file_read(&serial,path);
  if (serialc<0) {
    fprintf(stderr,"%s: Failed to read file\n",path);
    return -2;
  }
  int err=minify_stage(serial,serialc,path);
  free(serial);
  if (err<0) {
    if (err!=-2) fprintf(stderr,"%s: Unspecified error compiling stage.\n",path);
    return -2;
  }
  if (sr_encode_raw(&min.dst,"</stage>\n",-1)<0) return -1;
  return 0;
}

/* Inline favicon.
 */

static int minify_favicon(const char *href,int hrefc) {
  if (sr_encode_raw(&min.dst,"<link rel=\"icon\" href=\"",-1)<0) return -1;
  if ((hrefc>=5)&&!memcmp(href,"data:",5)) {
    // Already a data URL. Keep it as is.
    if (sr_encode_raw(&min.dst,href,hrefc)<0) return -1;
  } else {
    // Inline image.
    char imgpath[1024];
    int imgpathc=resolve_path(imgpath,sizeof(imgpath),min.srcpath,href,hrefc);
    if ((imgpathc<1)||(imgpathc>=sizeof(imgpath))) {
      fprintf(stderr,"%s: Failed to resolve image path '%.*s'\n",min.srcpath,hrefc,href);
      return -2;
    }
    void *serial=0;
    int serialc=file_read(&serial,imgpath);
    if (serialc<0) {
      fprintf(stderr,"%s: Failed to read file.\n",imgpath);
      return -2;
    }
    if (
      (sr_encode_raw(&min.dst,"data:image/png;base64,",-1)<0)||
      (sr_encode_base64(&min.dst,serial,serialc)<0)
    ) {
      free(serial);
      return -1;
    }
    free(serial);
  }
  if (sr_encode_raw(&min.dst,"\"/>\n",-1)<0) return -1;
  return 0;
}

/* Inline image.
 */

static int minify_image(const char *src,int srcc) {
  char imgpath[1024];
  int imgpathc=resolve_path(imgpath,sizeof(imgpath),min.srcpath,src,srcc);
  if ((imgpathc<1)||(imgpathc>=sizeof(imgpath))) {
    fprintf(stderr,"%s: Failed to resolve image path '%.*s'\n",min.srcpath,srcc,src);
    return -2;
  }
  void *serial=0;
  int serialc=file_read(&serial,imgpath);
  if (serialc<0) {
    fprintf(stderr,"%s: Failed to read file.\n",imgpath);
    return -2;
  }
  if (
    (sr_encode_raw(&min.dst,"<img src=\"data:image/png;base64,",32)<0)||
    (sr_encode_base64(&min.dst,serial,serialc)<0)||
    (sr_encode_raw(&min.dst,"\"/>\n",-1)<0)
  ) {
    free(serial);
    return -1;
  }
  free(serial);
  return 0;
}

/* Minify singleton HTML tag.
 */

static int minify_singleton(const char *src,int srcc) {
  const char *tag=0;
  int tagc=html_tag_name(&tag,src,srcc);

  if ((tagc==4)&&!memcmp(tag,"link",4)) {
    const char *href=0;
    int hrefc=html_get_attribute(&href,src,srcc,"href",4);
    if (hrefc<0) hrefc=0;
    const char *rel=0;
    int relc=html_get_attribute(&rel,src,srcc,"rel",3);
    if ((relc==4)&&!memcmp(rel,"icon",4)) {
      return minify_favicon(href,hrefc);
    }
    if ((relc==10)&&!memcmp(rel,"stylesheet",10)) {
      return minify_external_css(href,hrefc);
    }
    // Any other "link", pass it thru verbatim.

  } else if ((tagc==3)&&!memcmp(tag,"img",3)) {
    const char *href=0;
    int hrefc=html_get_attribute(&href,src,srcc,"src",3);
    if ((hrefc>=5)&&!memcmp(href,"data:",5)) {
      // Already a data URL; let it pass verbatim.
    } else {
      return minify_image(href,hrefc);
    }
  }
  
  if (sr_encode_raw(&min.dst,src,srcc)<0) return -1;
  return 0;
}

/* Minify HTML from an open tag.
 * We have the option to consume more text, from immediately after the open tag.
 */

static int minify_open(const char *src,int srcc,const char *post,int postc) {
  const char *tag=0;
  int tagc=html_tag_name(&tag,src,srcc);

  if ((tagc==5)&&!memcmp(tag,"style",5)) {
    int err=minify_css(post,postc,1,0);
    if (err<0) return err;
    return err;
  }

  if ((tagc==6)&&!memcmp(tag,"script",6)) {
    const char *href=0;
    int hrefc=html_get_attribute(&href,src,srcc,"src",3);
    if (hrefc>0) {
      int err=minify_external_js(href,hrefc);
      if (err<0) return err;
      return consume_closing_tag(post,postc,"script",6,0);
    }
    int err=minify_js(post,postc,1,0);
    if (err<0) return err;
    return err;
  }
  
  if ((tagc==4)&&!memcmp(tag,"song",4)) {
    // Not allowing finished song tags, though certainly we could, like <script> above.
    const char *href=0;
    int hrefc=html_get_attribute(&href,src,srcc,"href",4);
    if (hrefc<1) {
      fprintf(stderr,"<song> tag must contain 'href' attribute\n");
      return -2;
    }
    int err=minify_external_song(href,hrefc);
    if (err<0) return err;
    return consume_closing_tag(post,postc,"song",4,1);
  }
  
  if ((tagc==5)&&!memcmp(tag,"stage",5)) {
    const char *href=0;
    int hrefc=html_get_attribute(&href,src,srcc,"href",4);
    if (hrefc<1) {
      fprintf(stderr,"<stage> tag must contain 'href' attribute\n");
      return -2;
    }
    int err=minify_external_stage(href,hrefc);
    if (err<0) return err;
    return consume_closing_tag(post,postc,"stage",5,1);
  }

  // General case: Emit it verbatim and push on the tag stack.
  if (min.tagstackc>=min.tagstacka) {
    int na=min.tagstacka+16;
    if (na>INT_MAX/sizeof(void*)) return -1;
    void *nv=realloc(min.tagstackv,sizeof(void*)*na);
    if (!nv) return -1;
    min.tagstackv=nv;
    min.tagstacka=na;
  }
  char *nv=malloc(tagc+1);
  if (!nv) return -1;
  memcpy(nv,tag,tagc);
  nv[tagc]=0;
  min.tagstackv[min.tagstackc++]=nv;
  if (sr_encode_raw(&min.dst,src,srcc)<0) return -1;
  return 0;
}

/* Minify, main entry point.
 */

int minify() {
  int srcp=0,err;
  while (srcp<min.srcc) {
    int ttype=-1;
    const char *token=min.src+srcp;
    int tokenc=measure_html_token(min.src+srcp,min.srcc-srcp,&ttype);
    if (tokenc<1) {
      fprintf(stderr,"%s:%d: HTML tokenization error starting at character '%c'\n",min.srcpath,lineno(min.src,srcp),token[0]);
      return -2;
    }
    switch (ttype) {

      case TTYPE_SINGLETON: {
          if ((err=minify_singleton(token,tokenc))<0) return err;
        } break;

      case TTYPE_OPEN: {
          srcp+=tokenc;
          if ((err=minify_open(token,tokenc,min.src+srcp,min.srcc-srcp))<0) return err;
          srcp+=err;
          continue; // breaking would advance by tokenc, which we already did
        }

      case TTYPE_CLOSE: {
          const char *tag=0;
          int tagc=html_tag_name(&tag,token,tokenc);
          if ((min.tagstackc<1)||memcmp(tag,min.tagstackv[min.tagstackc-1],tagc)||min.tagstackv[min.tagstackc-1][tagc]) {
            fprintf(stderr,"%s:%d: Unexpected closing tag '%.*s'\n",min.srcpath,lineno(min.src,srcp),tagc,tag);
            return -2;
          }
          min.tagstackc--;
          free(min.tagstackv[min.tagstackc]);
          if (sr_encode_raw(&min.dst,token,tokenc)<0) return -1;
        } break;

      // The rest not super interesting:
      case TTYPE_SPACE: break;
      case TTYPE_COMMENT: break;
      case TTYPE_PI: {
          if (sr_encode_raw(&min.dst,token,tokenc)<0) return -1;
          if (sr_encode_u8(&min.dst,0x0a)<0) return -1; // DOCTYPE must be on its own line. Not sure about general PIs, but let's assume so.
        } break;
      case TTYPE_TEXT: {
          if (sr_encode_raw(&min.dst,token,tokenc)<0) return -1;
        } break;
      default: {
          fprintf(stderr,"%s:%d: Unknown token type %d for '%.*s' [%s:%d]\n",min.srcpath,lineno(min.src,srcp),ttype,tokenc,token,__FILE__,__LINE__);
          return -2;
        }
    }
    srcp+=tokenc;
  }
  if (min.tagstackc) {
    fprintf(stderr,"%s: Unclosed '%s' tag\n",min.srcpath,min.tagstackv[min.tagstackc-1]);
    return -2;
  }
  if (sr_encode_u8(&min.dst,0x0a)<0) return -1; // The extra legibility of a trailing newline is worth the one byte.
  return 0;
}
