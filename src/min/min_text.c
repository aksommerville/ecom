#include "min_internal.h"

/* Line number.
 */
 
int lineno(const char *src,int srcc) {
  if (!src) return 0;
  if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  int lineno=1,srcp=0;
  for (;srcp<srcc;srcp++) {
    if (src[srcp]==0x0a) lineno++;
  }
  return lineno;
}

/* Resolve relative path.
 */
 
int resolve_path(char *dst,int dsta,const char *ref,const char *src,int srcc) {
  int refc=0; if (ref) while (ref[refc]) refc++;
  if (refc>=dsta) return -1;
  memcpy(dst,ref,refc);
  int dstc=refc;
  // Trim trailing slash from (dst), and more importantly, trim the last component.
  while (dstc&&(dst[dstc-1]=='/')) dstc--;
  while (dstc&&(dst[dstc-1]!='/')) dstc--;
  while (dstc&&(dst[dstc-1]=='/')) dstc--;
  // No need to get fancy about this. Just strip prefixes "../", "./", and "/" from src.
  // When it's "../", back (dstc) to before the last slash.
  for (;;) {
    if ((srcc>=3)&&!memcmp(src,"../",3)) {
      src+=3;
      srcc-=3;
      while (dstc&&(dst[dstc-1]!='/')) dstc--;
      while (dstc&&(dst[dstc-1]=='/')) dstc--;
    } else if ((srcc>=2)&&!memcmp(src,"./",2)) {
      src+=2;
      srcc-=2;
    } else if ((srcc>=1)&&(src[0]=='/')) {
      src++;
      srcc--;
    } else {
      break;
    }
  }
  if (dstc>=dsta) return -1;
  dst[dstc++]='/';
  if (dstc>=dsta-srcc) return -1;
  memcpy(dst+dstc,src,srcc);
  dstc+=srcc;
  if (dstc<dsta) dst[dstc]=0;
  return dstc;
}

/* Identifier from sequence.
 * First character can't be a digit.
 * Second character can't be lowercase, to prevent us from using keywords.
 * After the second, anything goes.
 */

static const char id_first[]="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_$";
static const char id_second[]="ABCDEFGHIJKLMNOPQRSTUVWXYZ_$0123456789";
static const char id_later[]="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_$0123456789";
 
int identifier_from_sequence(char *dst,int dsta,int seq) {
  if (seq<0) return -1;
  const int id1c=sizeof(id_first)-1;
  const int id2c=sizeof(id_second)-1;
  const int id3c=sizeof(id_later)-1;
  int dstc=0;
  if (dstc<dsta) dst[dstc]=id_first[seq%id1c];
  dstc++;
  seq/=id1c;
  if (seq>0) {
    if (dstc<dsta) dst[dstc]=id_second[seq%id2c];
    dstc++;
    seq/=id2c;
  }
  while (seq>0) {
    if (dstc<dsta) dst[dstc]=id_later[seq%id3c];
    dstc++;
    seq/=id3c;
  }
  if (dstc<dsta) dst[dstc]=0;
  return dstc;
}
