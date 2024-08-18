#include "min_internal.h"

int minify_stage(const char *src,int srcc,const char *refname) {
  const char *b65alphabet=
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789"
    "+/?";
  struct sr_decoder decoder={.v=src,.c=srcc};
  const char *line;
  int linec,lineno=1;
  for (;(linec=sr_decode_line(&line,&decoder))>0;lineno++) {
    while (linec&&((unsigned char)line[linec-1]<=0x20)) linec--;
    while (linec&&((unsigned char)line[0]<=0x20)) { linec--; line++; }
    if (!linec||(line[0]=='#')) continue;
    int linep=0;
    
    const char *k=line+linep;
    int kc=0;
    while ((linep<linec)&&((unsigned char)line[linep++]>0x20)) kc++;
    while ((linep<linec)&&((unsigned char)line[linep]<=0x20)) linep++;
    
    #define OPCODE(ch) if (sr_encode_u8(&min.dst,ch)<0) return -1;
    #define RDTOKEN(name) \
      const char *token=line+linep; int tokenc=0; \
      while ((linep<linec)&&((unsigned char)line[linep++]>0x20)) tokenc++; \
      while ((linep<linec)&&((unsigned char)line[linep]<=0x20)) linep++; \
      if (!tokenc) { \
        fprintf(stderr,"%s:%d: Expected value for '%s' before end of line\n",refname,lineno,name); \
        return -2; \
      }
    #define INT_B65_D4(name) { \
      RDTOKEN(name) \
      int v; \
      if ((sr_int_eval(&v,token,tokenc)<2)||(v<0)||(v>256)||(v&3)) { \
        fprintf(stderr,"%s:%d: Expected multiple of 4 in 0..256 for '%s', found '%.*s'\n",refname,lineno,name,tokenc,token); \
        return -2; \
      } \
      v>>=2; \
      if (sr_encode_u8(&min.dst,b65alphabet[v])<0) return -1; \
    }
    #define FIXED_HEX(len,name) { \
      RDTOKEN(name) \
      if (tokenc!=len) { \
        fprintf(stderr,"%s:%d: Expected %d hexadecimal digits for '%s', found '%.*s'\n",refname,lineno,len,name,tokenc,token); \
        return -2; \
      } \
      const char *ck=token; \
      int i=len; while (i-->0) { \
        if ((ck[i]>='0')&&(ck[i]<='9')) continue; \
        if ((ck[i]>='a')&&(ck[i]<='f')) continue; \
        if ((ck[i]>='A')&&(ck[i]<='F')) continue; \
        fprintf(stderr,"%s:%d: Illegal character '%c' in hexadecimal number for '%s'\n",refname,lineno,ck[i],name); \
        return -2; \
      } \
      if (sr_encode_raw(&min.dst,token,tokenc)<0) return -1; \
    }
    #define STRING(terminator,name) { \
      RDTOKEN(name) \
      const char *ck=token; \
      int i=tokenc; while (i-->0) { \
        if (ck[i]==terminator) { \
          fprintf(stderr,"%s:%d: String for '%s' must not contain its terminator '%c'\n",refname,lineno,name,terminator); \
          return -2; \
        } \
      } \
      if (sr_encode_raw(&min.dst,token,tokenc)<0) return -1; \
      if (sr_encode_u8(&min.dst,terminator)<0) return -1; \
    }
    
    if ((kc==4)&&!memcmp(k,"wall",4)) {
      OPCODE('w')
      INT_B65_D4("x")
      INT_B65_D4("y")
      INT_B65_D4("w")
      INT_B65_D4("h")
      
    } else if ((kc==4)&&!memcmp(k,"hero",4)) {
      OPCODE('h')
      INT_B65_D4("x")
      INT_B65_D4("y")
      
    } else if ((kc==2)&&!memcmp(k,"bg",2)) {
      OPCODE('b')
      FIXED_HEX(3,"rgb")
      
    } else if ((kc==4)&&!memcmp(k,"song",4)) {
      OPCODE('s')
      STRING('.',"name")
      
    } else {
      fprintf(stderr,"%s:%d: Unexpected stage command '%.*s'\n",refname,lineno,kc,k);
      return -2;
    }
    if (linep<linec) {
      fprintf(stderr,"%s:%d: Unexpected tokens '%.*s' after '%.*s' command.\n",refname,lineno,linec-linep,line+linep,kc,k);
      return -2;
    }
  }
  return 0;
}
