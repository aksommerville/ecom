/* render.c
 * The important rendering is pure software in 32-bit framebuffers.
 * We use OpenGL only for the final transfer up to the screen.
 */

#include "native_internal.h"
#include <GLES2/gl2.h>

/* Shader.
 */
 
static const char vshader[]=
  "#version 100\n"
  "precision mediump float;\n"
  "uniform vec2 screensize;\n"
  "attribute vec2 apos;\n" // attr 0
  "attribute vec2 atexcoord;\n" // attr 1
  "varying vec2 vtexcoord;\n"
  "void main() {\n"
    "vec2 npos=(apos*2.0)/screensize-1.0;\n"
    "gl_Position=vec4(npos,0.0,1.0);\n"
    "vtexcoord=atexcoord;\n"
  "}\n"
"";

static const char fshader[]=
  "#version 100\n"
  "precision mediump float;\n"
  "uniform sampler2D sampler;\n"
  "varying vec2 vtexcoord;\n"
  "void main() {\n"
    "gl_FragColor=texture2D(sampler,vtexcoord);\n"
  "}\n"
"";

/* Private globals.
 */
 
static struct {
  struct image *srcbits;
  struct image *bgbits;
  struct image *fb;
  int gradx,grady;
  int grad0,grad1; // 0x00rrggbb
  GLuint texid;
  GLint program;
  GLuint u_screensize;
  GLuint u_sampler;
} render={0};

/* Compile half of one program.
 * <0 for error.
 */
 
static int render_compile_shader(const char *src,int srcc,int type) {
  GLint sid=glCreateShader(type);
  if (!sid) return -1;
  glShaderSource(sid,1,&src,&srcc);
  glCompileShader(sid);
  GLint status=0;
  glGetShaderiv(sid,GL_COMPILE_STATUS,&status);
  if (status) {
    glAttachShader(render.program,sid);
    glDeleteShader(sid);
    return 0;
  }
  GLuint loga=0,logged=0;
  glGetShaderiv(sid,GL_INFO_LOG_LENGTH,&loga);
  if (loga>0) {
    GLchar *log=malloc(loga+1);
    if (log) {
      GLsizei logc=0;
      glGetShaderInfoLog(sid,loga,&logc,log);
      if ((logc>0)&&(logc<=loga)) {
        while (logc&&((unsigned char)log[logc-1]<=0x20)) logc--;
        fprintf(stderr,"Failed to compile %s shader:\n%.*s\n",(type==GL_VERTEX_SHADER)?"vertex":"fragment",logc,log);
        logged=1;
      }
      free(log);
    }
  }
  if (!logged) fprintf(stderr,"Failed to compile %s shader, no further detail available.\n",(type==GL_VERTEX_SHADER)?"vertex":"fragment");
  glDeleteShader(sid);
  return -2;
}

/* Compile and link shader.
 */
 
static int render_compile_program() {
  int err;
  if (!(render.program=glCreateProgram())) return -1;
  if ((err=render_compile_shader(vshader,sizeof(vshader)-1,GL_VERTEX_SHADER))<0) return err;
  if ((err=render_compile_shader(fshader,sizeof(fshader)-1,GL_FRAGMENT_SHADER))<0) return err;
  glBindAttribLocation(render.program,0,"apos");
  glBindAttribLocation(render.program,1,"atexcoord");
  glLinkProgram(render.program);
  GLint status=0;
  glGetProgramiv(render.program,GL_LINK_STATUS,&status);
  if (status) {
    render.u_screensize=glGetUniformLocation(render.program,"screensize");
    render.u_sampler=glGetUniformLocation(render.program,"sampler");
    return 0;
  }
  GLuint loga=0,logged=0;
  glGetProgramiv(render.program,GL_INFO_LOG_LENGTH,&loga);
  if (loga>0) {
    GLchar *log=malloc(loga+1);
    if (log) {
      GLsizei logc=0;
      glGetProgramInfoLog(render.program,loga,&logc,log);
      if ((logc>0)&&(logc<=loga)) {
        while (logc&&((unsigned char)log[logc-1]<=0x20)) logc--;
        fprintf(stderr,"Failed to link GLSL program:\n%.*s\n",logc,log);
        logged=1;
      }
      free(log);
    }
  }
  if (!logged) fprintf(stderr,"Failed to link GLSL program, no further detail available.\n");
  return -2;
}
 
/* Init.
 */

int render_init() {
  int err;
  const void *png=0;
  int pngc=rom_get_image(&png);
  if (!(render.srcbits=png_decode(png,pngc))) {
    fprintf(stderr,"Failed to decode embedded %d-byte PNG file.\n",pngc);
    return -2;
  }
  if ((err=image_force_rgba(render.srcbits))<0) {
    if (err!=-2) fprintf(stderr,"Failed to reencoded embedded image as RGBA.\n");
    return -2;
  }
  if (!(render.bgbits=image_new_alloc(32,SCREENW,SCREENH))) return -1;
  if (!(render.fb=image_new_alloc(32,SCREENW,SCREENH))) return -1;
  glGenTextures(1,&render.texid);
  glBindTexture(GL_TEXTURE_2D,render.texid);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
  if ((err==render_compile_program())<0) return err;
  return 0;
}

/* Copy to main.
 */
 
static void render_copy_to_main(struct hostio_video *video) {
  int mainw=video->w,mainh=video->h;
  int scalex=mainw/SCREENW,scaley=mainh/SCREENH;
  int scale=(scalex<scaley)?scalex:scaley;
  if (scale<1) scale=1;
  int dstw=SCREENW*scale,dsth=SCREENH*scale;
  int dstx=(mainw>>1)-(dstw>>1);
  int dsty=(mainh>>1)-(dsth>>1);
  glViewport(0,0,mainw,mainh);
  if ((dstw<mainw)||(dsth<mainh)) {
    glClearColor(0.0f,0.0f,0.0f,1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
  }
  glBindTexture(GL_TEXTURE_2D,render.texid);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,SCREENW,SCREENH,0,GL_RGBA,GL_UNSIGNED_BYTE,render.fb->v);
  glUseProgram(render.program);
  glUniform2f(render.u_screensize,(GLfloat)mainw,(GLfloat)mainh);
  glUniform1i(render.u_sampler,0);
  struct { GLfloat x,y,tx,ty; } vtxv[]={
    { dstx     ,dsty     , 0.0f,1.0f },
    { dstx     ,dsty+dsth, 0.0f,0.0f },
    { dstx+dstw,dsty     , 1.0f,1.0f },
    { dstx+dstw,dsty+dsth, 1.0f,0.0f },
  };
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(0,2,GL_FLOAT,0,sizeof(vtxv[0]),&vtxv[0].x);
  glVertexAttribPointer(1,2,GL_FLOAT,0,sizeof(vtxv[0]),&vtxv[0].tx);
  glDrawArrays(GL_TRIANGLE_STRIP,0,sizeof(vtxv)/sizeof(vtxv[0]));
  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
}

/* Frame fences.
 */
 
int render_begin() {
  int err;
  struct hostio_video *video=drivers_get_video();
  if (video&&video->type->gx_begin) {
    if ((err=video->type->gx_begin(video))<0) return err;
  }
  return 0;
}

int render_end() {
  int err;
  struct hostio_video *video=drivers_get_video();
  if (!video) return -1;
  render_copy_to_main(video);
  if (video->type->gx_end) {
    if ((err=video->type->gx_end(video))<0) return err;
  }
  return 0;
}

/* Radial gradient for the background texture.
 */
 
void render_reset_gradient(int x,int y) {
  render.gradx=x;
  render.grady=y;
}

void render_set_gradient(int p,int rgb) {
  if (p==0) render.grad0=rgb;
  else if (p==1) render.grad1=rgb;
}

void render_fill_bg() {
  uint8_t r0=render.grad0>>16,g0=render.grad0>>8,b0=render.grad0;
  uint8_t r1=render.grad1>>16,g1=render.grad1>>8,b1=render.grad1;
  uint32_t *p=render.bgbits->v;
  int range=SCREENW*SCREENH;
  int y=0; for (;y<SCREENH;y++) {
    int dy=y-render.grady;
    int dy2=dy*dy;
    int x=0; for (;x<SCREENW;x++,p++) {
      int dx=x-render.gradx;
      int dx2=dx*dx;
      int d2=dx2+dy2;
      // We're interpolating based on the square distance, so it's a different curve than web uses.
      // But a little more efficient, and IMHO it looks just fine.
      int r=(r0*(range-d2)+r1*d2)/range; if (r<0) r=0; else if (r>0xff) r=0xff;
      int g=(g0*(range-d2)+g1*d2)/range; if (g<0) g=0; else if (g>0xff) g=0xff;
      int b=(b0*(range-d2)+b1*d2)/range; if (b<0) b=0; else if (b>0xff) b=0xff;
      *p=r|(g<<8)|(b<<16)|0xff000000;
    }
  }
}

/* Overwrite main with background.
 */
 
void render_copy_bg() {
  memcpy(render.fb->v,render.bgbits->v,render.bgbits->stride*render.bgbits->h);
}

/* Fill rect in main.
 */
 
void render_fill_rect(int x,int y,int w,int h,int rgb) {
  if (x<0) { w+=x; x=0; }
  if (y<0) { h+=y; y=0; }
  if (x>render.fb->w-w) w=render.fb->w-x;
  if (y>render.fb->h-h) h=render.fb->h-y;
  uint32_t pixel=((rgb&0xff0000)>>16)|(rgb&0xff00)|(rgb<<16)|0xff000000;
  uint8_t *row=(uint8_t*)render.fb->v+y*render.fb->stride+(x<<2);
  int yi=h; for (;yi-->0;row+=render.fb->stride) {
    uint32_t *p=(uint32_t*)row;
    int xi=w; for (;xi-->0;p++) *p=pixel;
  }
}

/* Blit from tilesheet to main or tilesheet to background.
 */
 
void render_blit(int dsttexid,int dstx,int dsty,int srcx,int srcy,int w,int h,int flop) {
  struct image *dst=dsttexid?render.bgbits:render.fb;
  if (srcx<0) { dstx-=srcx; w+=srcx; srcx=0; }
  if (srcy<0) { dsty-=srcy; h+=srcy; srcy=0; }
  if (dstx<0) { srcx-=dstx; w+=dstx; dstx=0; }
  if (dsty<0) { srcy-=dsty; h+=dsty; dsty=0; }
  if (srcx>render.srcbits->w-w) w=render.srcbits->w-srcx;
  if (srcy>render.srcbits->h-h) h=render.srcbits->h-srcy;
  if (dstx>dst->w-w) w=dst->w-dstx;
  if (dsty>dst->h-h) h=dst->h-dsty;
  const uint8_t *srcrow=render.srcbits->v;
  srcrow+=srcy*render.srcbits->stride+(srcx<<2);
  uint8_t *dstrow=dst->v;
  dstrow+=dsty*dst->stride+(dstx<<2);
  int ddstp=1;
  if (flop) { // To flop, we walk rows backward in (dst).
    ddstp=-1;
    dstrow+=(w-1)<<2;
  }
  int yi=h; for (;yi-->0;srcrow+=render.srcbits->stride,dstrow+=dst->stride) {
    const uint32_t *srcp=(uint32_t*)srcrow;
    uint32_t *dstp=(uint32_t*)dstrow;
    int xi=w; for (;xi-->0;srcp++,dstp+=ddstp) {
      if (*srcp) *dstp=*srcp; // Pixels must either natural zero or fully opaque.
    }
  }
}

/* Render decimal integer to main, from known places in tilesheet.
 */
 
void render_decint(int x,int y,int v,int digitc) {
  if (v<0) v=0; // There won't be any negatives.
  x+=4*digitc; // digitc only for right alignment -- don't print leading zeroes
  render_blit(0,x,y,88+3*(v%10),15,3,5,0); // LSD always
  v/=10;
  while (v) {
    x-=4;
    render_blit(0,x,y,88+3*(v%10),15,3,5,0);
    v/=10;
  }
}
