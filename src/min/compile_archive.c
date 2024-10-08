#include "min_internal.h"

/* Compile <img>
 */
 
static int recompile_img(const char *src,int srcc) {
  int srcp=0;
  while (srcp<srcc) {
    if ((unsigned char)src[srcp]<=0x20) { srcp++; continue; }
    if (src[srcp]=='/') { srcp++; continue; }
    if (src[srcp]=='>') { srcp++; break; }
    const char *k=src+srcp,*v=0;
    int kc=0,vc=0;
    while ((srcp<srcc)&&(src[srcp]>='a')&&(src[srcp]<='z')) { srcp++; kc++; }
    if ((srcp<=srcc-2)&&(src[srcp]=='=')&&(src[srcp+1]=='"')) {
      srcp+=2;
      v=src+srcp;
      while ((srcp<srcc)&&(src[srcp++]!='"')) vc++;
    }
    if ((kc!=3)||memcmp(k,"src",3)) continue; // Should only be [src] but whatever, ignore others.
    if ((vc<22)||memcmp(v,"data:image/png;base64,",22)) {
      fprintf(stderr,"%s: <img src> does not begin with 'data:image/png;base64,' as expected\n",min.srcpath);
      return -2;
    }
    v+=22;
    vc-=22;
    if (sr_encode_u8(&min.dst,1)<0) return -1; // type image
    int lenp=min.dst.c;
    if (sr_encode_raw(&min.dst,"\0\0",2)<0) return -1; // length placeholder
    for (;;) {
      int err=sr_base64_decode((char*)min.dst.v+min.dst.c,min.dst.a-min.dst.c,v,vc);
      if (err<0) {
        fprintf(stderr,"%s: Malformed base64 in <img>\n",min.srcpath);
        return -2;
      }
      if (min.dst.c<=min.dst.a-err) {
        min.dst.c+=err;
        ((uint8_t*)min.dst.v)[lenp]=err>>8;
        ((uint8_t*)min.dst.v)[lenp+1]=err;
        return srcp;
      }
      if (sr_encoder_require(&min.dst,err)<0) return -1;
    }
  }
  fprintf(stderr,"%s: <img> tag does not have a [src] attribute\n",min.srcpath);
  return -2;
}

/* Compile <song>
 */
 
static int recompile_song(const char *src,int srcc) {
  if (sr_encode_u8(&min.dst,2)<0) return -1; // type song
  int lenp=min.dst.c;
  if (sr_encode_raw(&min.dst,"\0\0",2)<0) return -1;
  int srcp=0,namec=0;
  const char *name=0;
  while (srcp<srcc) {
    if ((unsigned char)src[srcp]<=0x20) { srcp++; continue; }
    if (src[srcp]=='>') { srcp++; break; }
    const char *k=src+srcp,*v=0;
    int kc=0,vc=0;
    while ((srcp<srcc)&&(src[srcp]>='a')&&(src[srcp]<='z')) { srcp++; kc++; }
    if ((srcp<=srcc-2)&&(src[srcp]=='=')&&(src[srcp+1]=='"')) {
      srcp+=2;
      v=src+srcp;
      while ((srcp<srcc)&&(src[srcp++]!='"')) vc++;
    }
    if (!kc&&!vc) return -1;
    if ((kc==4)&&!memcmp(k,"name",4)) {
      name=v;
      namec=vc;
    }
  }
  if (sr_encode_raw(&min.dst,name,namec)<0) return -1;
  if (sr_encode_u8(&min.dst,';')<0) return -1;
  while (srcp<srcc) {
    if (src[srcp]=='<') break;
    if ((unsigned char)src[srcp]<=0x20) { srcp++; continue; }
    if (sr_encode_u8(&min.dst,src[srcp])<0) return -1;
    srcp++;
  }
  int len=min.dst.c-lenp-2;
  ((uint8_t*)min.dst.v)[lenp]=len>>8;
  ((uint8_t*)min.dst.v)[lenp+1]=len;
  return srcp;
}

/* Compile <stage>
 */
 
static int recompile_stage(const char *src,int srcc) {
  if (sr_encode_u8(&min.dst,3)<0) return -1; // type stage
  int lenp=min.dst.c;
  if (sr_encode_raw(&min.dst,"\0\0",2)<0) return -1;
  int srcp=0,namec=0;
  const char *name=0;
  while ((srcp<srcc)&&(src[srcp++]!='>')) ;
  while (srcp<srcc) {
    if (src[srcp]=='<') break;
    if ((unsigned char)src[srcp]<=0x20) { srcp++; continue; }
    if (sr_encode_u8(&min.dst,src[srcp])<0) return -1;
    srcp++;
  }
  int len=min.dst.c-lenp-2;
  ((uint8_t*)min.dst.v)[lenp]=len>>8;
  ((uint8_t*)min.dst.v)[lenp+1]=len;
  return srcp;
}

/* Rewrite Javascript from the <script> tag.
 * Put output directly on (min.dst).
 * Input files were minified each to their own line.
 * We are going to modify a few:
 *  - Audio.js gets dropped entirely and replaced by a shallow stub.
 *  - bootstrap.js, we strip the `addEventListener("load")` and call the initializer synchronously.
 * TODO It's probably better to treat Video the way we're doing Audio. Currently I have a bunch of stub objects getting created in exec.c
 */
 
static int rewrite_js(const char *src,int srcc) {
  int srcp=0;
  while (srcp<srcc) {
    const char *line=src+srcp;
    int linec=0;
    while ((srcp<srcc)&&(src[srcp++]!=0x0a)) linec++;
    
    // bootstrap.js
    if ((linec>=39)&&!memcmp(line,"window.addEventListener(\"load\",()=>{",36)&&!memcmp(line+linec-3,"});",3)) {
      if (sr_encode_raw(&min.dst,line+36,linec-39)<0) return -1;
      if (sr_encode_u8(&min.dst,0x0a)<0) return -1;
      continue;
    }
    
    // Everything else should have "class NAME" near the front -- but not necessarily the very front.
    const char *clsname="";
    int clsnamec=0;
    int linep=0;
    while (linep<linec) {
      const char *token=line+linep;
      int tokenc=0;
      while ((linep<linec)&&(line[linep]>='a')&&(line[linep]<='z')) { linep++; tokenc++; }
      linep++;
      if ((tokenc==5)&&!memcmp(token,"class",5)) {
        while ((linep<linec)&&((unsigned char)line[linep]<=0x20)) linep++;
        clsname=line+linep;
        clsnamec=0;
        while (linep<linec) {
               if ((line[linep]>='a')&&(line[linep]<='z')) ;
          else if ((line[linep]>='A')&&(line[linep]<='Z')) ;
          else if ((line[linep]>='0')&&(line[linep]<='9')) ;
          else if (line[linep]=='_') ;
          else break;
          linep++;
          clsnamec++;
        }
        break;
      }
    }
    
    // Audio.js
    if ((clsnamec==5)&&!memcmp(clsname,"Audio",5)) {
      if (sr_encode_raw(&min.dst,"class Audio { playSong(name) { playSong(name); } update() {} snd(id) { playSound(id) } }\n",-1)<0) return -1;
      continue;
    }
    
    // Video.js
    if ((clsnamec==5)&&!memcmp(clsname,"Video",5)) {
      if (sr_encode_raw(&min.dst,
        "class Video {"
          "constructor() { this.bc=videoBgctx; }"
          "end() { videoEnd(); }"
          "bg() { videoBg(); }"
          "rect(a,b,c,d,e) { videoRect(a,b,c,d,e); }"
          "blit(a,b,c,d,e,f) { videoBlit(0,a,b,c,d,e,f,0); }"
          "flop(a,b,c,d,e,f) { videoBlit(0,a,b,c,d,e,f,1); }"
          "decint(a,b,c,d) { videoDecint(a,b,c,d); }"
          "bblit(a,b,c,d,e,f) { videoBlit(1,a,b,c,d,e,f,0); }"
          "bflop(a,b,c,d,e,f) { videoBlit(1,a,b,c,d,e,f,1); }"
        "}\n",-1
      )<0) return -1;
      continue;
    }
    
    // Everything else, keep as is.
    if (sr_encode_raw(&min.dst,line,linec)<0) return -1;
    if (sr_encode_u8(&min.dst,0x0a)<0) return -1;
  }
  return 0;
}

/* Compile <script>
 */
 
static int recompile_script(const char *src,int srcc) {
  int srcp=0,namec=0;
  const char *name=0;
  while ((srcp<srcc)&&(src[srcp++]!='>')) ;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  const char *js=src+srcp;
  int jsc=0;
  for (;;) { // There might be some '<' in the javascript. Look for "</script>" explicitly.
    if (srcp>srcc-9) {
      fprintf(stderr,"%s: Closing tag for <script> not found\n",min.srcpath);
      return -2;
    }
    if (!memcmp(src+srcp,"</script>",9)) {
      srcp+=9;
      break;
    }
    srcp++;
    jsc++;
  }
  while (jsc&&((unsigned char)js[jsc-1]<=0x20)) jsc--;
  if (sr_encode_u8(&min.dst,4)<0) return -1; // type script
  int lenp=min.dst.c;
  if (sr_encode_raw(&min.dst,"\0\0",2)<0) return -1;
  int err=rewrite_js(js,jsc);
  if (err<0) return err;
  if (sr_encode_u8(&min.dst,0)<0) return -1; // quickjs requires scripts to be NUL-terminated; easy to do that right here.
  int len=min.dst.c-lenp-2;
  ((uint8_t*)min.dst.v)[lenp]=len>>8;
  ((uint8_t*)min.dst.v)[lenp+1]=len;
  return srcp;
}

/* Compile native archive, main entry point.
 */
 
int compile_archive() {
  
  /* What we're going to find:
   *  - <img src="data:image/png;base64, : Just one.
   *  - <song name="sm">... : Three, with unique names.
   *  - <stage>... : Thirteen, order matters.
   *  - <script>... : Just one.
   * The HTML we're reading was produced by this very tool, so we don't need to be very tolerant of formatting.
   * Songs and stages are in weird text formats that we can pass along verbatim. Leading and trailing space don't matter.
   * Image, decode the base64.
   * img tags are singletons; all the others have a body but never nested tags.
   * Script will probably need invasive modification. But for now, just pass it through as is.
   */
  /* Format to produce:
   * Unordered stream of (u8 type,u16 len,... payload).
   * Type:
   *  - 0: EOF
   *  - 1: image
   *  - 2: song
   *  - 3: stage
   *  - 4: script
   * Songs, prepend name and semicolon.
   */
   
  int srcp=0;
  while (srcp<min.srcc) {
    if (min.src[srcp++]!='<') continue;
    const char *tagname=min.src+srcp;
    int tagnamec=0,err=0;
    while ((srcp<min.srcc)&&(min.src[srcp]>='a')&&(min.src[srcp]<='z')) { srcp++; tagnamec++; }
    
    if ((tagnamec==3)&&!memcmp(tagname,"img",3)) err=recompile_img(min.src+srcp,min.srcc-srcp);
    else if ((tagnamec==4)&&!memcmp(tagname,"song",4)) err=recompile_song(min.src+srcp,min.srcc-srcp);
    else if ((tagnamec==5)&&!memcmp(tagname,"stage",5)) err=recompile_stage(min.src+srcp,min.srcc-srcp);
    else if ((tagnamec==6)&&!memcmp(tagname,"script",6)) err=recompile_script(min.src+srcp,min.srcc-srcp);
    if (err<0) {
      if (err!=-2) fprintf(stderr,"%s: Unspecified error processing <%.*s> tag\n",min.srcpath,tagnamec,tagname);
      return -2;
    }
    srcp+=err;
  }
  
  //fprintf(stderr,"%s => %s: Produced %d byte archive from %d bytes input html\n",min.dstpath,min.srcpath,min.dst.c,min.srcc);
   
  return 0;
}
