#include "native_internal.h"
#include "quickjs.h"

/* Globals.
 */
 
#define WHEN_ANIMATION 1
#define WHEN_KEYDOWN 2
#define WHEN_KEYUP 3
 
static struct {
  JSRuntime *rt;
  JSContext *ctx;
  struct defer {
    JSValue fn;
    int when;
  } *deferv;
  int deferc,defera;
  
  // A few stubby placeholder objects.
  JSValue bgctx;
  JSValue event;
  JSValue gamepads;
  
} exec={0};

/* Javascript exception?
 */
 
int exec_check_exception(JSValue v,int free_anyway) {
  if (!JS_IsException(v)) {
    if (free_anyway) JS_FreeValue(exec.ctx,v);
    return 0;
  }
  JSValue exception=JS_GetException(exec.ctx);
  JSValue stack=JS_GetPropertyStr(exec.ctx,exception,"stack");
  const char *reprd=JS_ToCString(exec.ctx,exception);
  fprintf(stderr,"EXCEPTION: %s\n",reprd);
  JS_FreeCString(exec.ctx,reprd);
  reprd=JS_ToCString(exec.ctx,stack);
  fprintf(stderr,"%s",reprd);
  JS_FreeCString(exec.ctx,reprd);
  JS_FreeValue(exec.ctx,exception);
  JS_FreeValue(exec.ctx,v);
  return -2;
}

/* Add deferred call.
 */
 
static int exec_defer(JSValue cb,int when) {
  if (exec.deferc>=exec.defera) {
    int na=exec.defera+8;
    if (na>INT_MAX/sizeof(struct defer)) return -1;
    void *nv=realloc(exec.deferv,sizeof(struct defer)*na);
    if (!nv) return -1;
    exec.deferv=nv;
    exec.defera=na;
  }
  struct defer *defer=exec.deferv+exec.deferc++;
  defer->fn=JS_DupValue(exec.ctx,cb);
  defer->when=when;
  return 0;
}

/* log
 */
 
static JSValue exec_jsfn_log(JSContext *ctx,JSValueConst thisValue,int argc,JSValueConst *argv) {
  fprintf(stderr,"JSLOG:");
  int i=0; for (;i<argc;i++) {
    const char *msg=JS_ToCString(ctx,argv[i]);
    if (!msg) continue;
    fprintf(stderr," %s",msg);
    JS_FreeCString(ctx,msg);
  }
  fprintf(stderr,"\n");
}

/* requestAnimationFrame
 */
 
static JSValue exec_jsfn_requestAnimationFrame(JSContext *ctx,JSValueConst thisValue,int argc,JSValueConst *argv) {
  if (argc<1) return JS_NULL;
  if (exec_defer(argv[0],WHEN_ANIMATION)<0) return JS_ThrowInternalError(ctx," ");
  return JS_NULL;
}

/* addEventListener
 */
 
static JSValue exec_jsfn_addEventListener(JSContext *ctx,JSValueConst thisValue,int argc,JSValueConst *argv) {
  if (argc<2) return JS_NULL;
  const char *evttype=JS_ToCString(ctx,argv[0]);
  if (!evttype) return JS_NULL;
  JSValueConst cb=argv[1];
  
  if (!strcmp(evttype,"load")) {
    fprintf(stderr,"XXXXXXXXXXXXXXXXXXXXXXXXXXX addEventListener('load'): This shouldn't happen anymore\n");
    if (exec_defer(cb,WHEN_ANIMATION)<0) return JS_ThrowInternalError(ctx," ");
    
  } else if (!strcmp(evttype,"keydown")) {
    if (exec_defer(cb,WHEN_KEYDOWN)<0) return JS_ThrowInternalError(ctx," ");
    
  } else if (!strcmp(evttype,"keyup")) {
    if (exec_defer(cb,WHEN_KEYUP)<0) return JS_ThrowInternalError(ctx," ");
    
  } else {
    fprintf(stderr,"%s:%d:%s:WARNING: Ignoring unknown event listener for '%s'\n",__FILE__,__LINE__,__func__,evttype);
  }
  
  JS_FreeCString(ctx,evttype);
  return JS_NULL;
}

/* getElementsByTagName
 */
 
static JSValue exec_jsfn_getElementsByTagName(JSContext *ctx,JSValueConst thisValue,int argc,JSValueConst *argv) {
  if (argc<1) return JS_ThrowTypeError(ctx,"getElementsByTagName requires a tag name");
  const char *tagname=JS_ToCString(ctx,argv[0]);
  if (!tagname) return JS_NULL;
  
  if (!strcmp(tagname,"stage")) {
    JSValue array=JS_NewArray(ctx);
    int p=0; for (;;p++) {
      const char *text=0;
      int textc=rom_get_stage(&text,p);
      if (textc<1) break;
      JSValue element=JS_NewObject(ctx);
      JS_SetPropertyStr(ctx,element,"innerText",JS_NewStringLen(ctx,text,textc));
      JS_SetPropertyUint32(ctx,array,p,element);
    }
    return array;
    
  } else {
    return JS_ThrowInternalError(ctx,"%s unexpected tag '%s'",__func__,tagname);
  }
  
  JS_FreeCString(ctx,tagname);
  return JS_NULL;
}

/* playSong,playSound
 */
 
static JSValue exec_jsfn_playSong(JSContext *ctx,JSValueConst thisValue,int argc,JSValueConst *argv) {
  if (argc<1) return JS_NULL;
  const char *name=JS_ToCString(ctx,argv[0]);
  if (!name) return JS_NULL;
  int32_t once=0;
  if (argc>=2) JS_ToInt32(ctx,&once,argv[1]);
  const void *serial=0;
  int serialc=rom_get_song(&serial,name,-1);
  if (serialc>0) {
    if (hostio_audio_lock(drivers_get_hostio())>=0) {
      synth_play_song(serial,serialc,once);
      hostio_audio_unlock(drivers_get_hostio());
    }
  }
  JS_FreeCString(ctx,name);
  return JS_NULL;
}
 
static JSValue exec_jsfn_playSound(JSContext *ctx,JSValueConst thisValue,int argc,JSValueConst *argv) {
  if (argc<1) return JS_NULL;
  int32_t id=0;
  JS_ToInt32(ctx,&id,argv[0]);
  if (hostio_audio_lock(drivers_get_hostio())>=0) {
    synth_play_sound(id);
    hostio_audio_unlock(drivers_get_hostio());
  }
  return JS_NULL;
}

/* createRadialGradient and fillRect -- called when Ecom draws the background.
 * These *only* apply to the background image.
 */
 
static inline int hdeval(char src) {
  if ((src>='0')&&(src<='9')) return src-'0';
  if ((src>='a')&&(src<='f')) return src-'a'+10;
  if ((src>='A')&&(src<='F')) return src-'A'+10;
  return 0;
}
 
static JSValue exec_jsfn_addColorStop(JSContext *ctx,JSValueConst thisValue,int argc,JSValueConst *argv) {
  if (argc!=2) return JS_NULL;
  int32_t p=0,rgb=0;
  JS_ToInt32(ctx,&p,argv[0]);
  const char *color=JS_ToCString(ctx,argv[1]);
  if (!color) return JS_NULL;
  int colorc=0; while (color[colorc]) colorc++;
  if (colorc==4) {
    int r=hdeval(color[1]);
    int g=hdeval(color[2]);
    int b=hdeval(color[3]);
    rgb=(r<<20)|(r<<16)|(g<<12)|(g<<8)|(b<<4)|b;
  }
  JS_FreeCString(ctx,color);
  render_set_gradient(p,rgb);
  return JS_NULL;
}
 
static JSValue exec_jsfn_createRadialGradient(JSContext *ctx,JSValueConst thisValue,int argc,JSValueConst *argv) {
  if (argc<2) return JS_NULL; // There's 6 arguments, but we only use the first two (x,y)
  int32_t x=0,y=0;
  JS_ToInt32(ctx,&x,argv[0]);
  JS_ToInt32(ctx,&y,argv[1]);
  render_reset_gradient(x,y);
  JSValue gradient=JS_NewObject(ctx);
  JS_SetPropertyStr(ctx,gradient,"addColorStop",JS_NewCFunction(ctx,exec_jsfn_addColorStop,"addColorStop",2));
  return gradient;
}
 
static JSValue exec_jsfn_fillRect(JSContext *ctx,JSValueConst thisValue,int argc,JSValueConst *argv) {
  // We get four arguments (x,y,w,h), but they are always the entire background.
  render_fill_bg();
  return JS_NULL;
}

/* videoEnd,videoBg,videoRect,videoBlit,videoDecint
 */
 
static JSValue exec_jsfn_videoEnd(JSContext *ctx,JSValueConst thisValue,int argc,JSValueConst *argv) {
  return JS_NULL;
}
 
static JSValue exec_jsfn_videoBg(JSContext *ctx,JSValueConst thisValue,int argc,JSValueConst *argv) {
  render_copy_bg();
  return JS_NULL;
}
 
static JSValue exec_jsfn_videoRect(JSContext *ctx,JSValueConst thisValue,int argc,JSValueConst *argv) {
  if (argc!=5) return JS_NULL;
  int32_t x=0,y=0,w=0,h=0,rgb=0;
  JS_ToInt32(ctx,&x,argv[0]);
  JS_ToInt32(ctx,&y,argv[1]);
  JS_ToInt32(ctx,&w,argv[2]);
  JS_ToInt32(ctx,&h,argv[3]);
  const char *color=JS_ToCString(ctx,argv[4]);
  if (!color) return JS_NULL;
  int colorc=0; while (color[colorc]) colorc++;
  if (colorc==4) {
    int r=hdeval(color[1]);
    int g=hdeval(color[2]);
    int b=hdeval(color[3]);
    rgb=(r<<20)|(r<<16)|(g<<12)|(g<<8)|(b<<4)|b;
  }
  JS_FreeCString(ctx,color);
  render_fill_rect(x,y,w,h,rgb);
  return JS_NULL;
}
 
static JSValue exec_jsfn_videoBlit(JSContext *ctx,JSValueConst thisValue,int argc,JSValueConst *argv) {
  if (argc!=8) return JS_NULL;
  int32_t dsttexid=0,dstx=0,dsty=0,srcx=0,srcy=0,w=0,h=0,flop=0;
  JS_ToInt32(ctx,&dsttexid,argv[0]);
  JS_ToInt32(ctx,&dstx,argv[1]);
  JS_ToInt32(ctx,&dsty,argv[2]);
  JS_ToInt32(ctx,&srcx,argv[3]);
  JS_ToInt32(ctx,&srcy,argv[4]);
  JS_ToInt32(ctx,&w,argv[5]);
  JS_ToInt32(ctx,&h,argv[6]);
  JS_ToInt32(ctx,&flop,argv[7]);
  render_blit(dsttexid,dstx,dsty,srcx,srcy,w,h,flop);
  return JS_NULL;
}
 
static JSValue exec_jsfn_videoDecint(JSContext *ctx,JSValueConst thisValue,int argc,JSValueConst *argv) {
  if (argc!=4) return JS_NULL;
  int32_t x=0,y=0,v=0,digitc=0;
  JS_ToInt32(ctx,&x,argv[0]);
  JS_ToInt32(ctx,&y,argv[1]);
  JS_ToInt32(ctx,&v,argv[2]);
  JS_ToInt32(ctx,&digitc,argv[3]);
  render_decint(x,y,v,digitc);
  return JS_NULL;
}

/* getGamepads
 */
 
static JSValue exec_jsfn_getGamepads(JSContext *ctx,JSValueConst thisValue,int argc,JSValueConst *argv) {
  return JS_DupValue(exec.ctx,exec.gamepads);
}

/* Dummy function.
 */
 
static JSValue exec_jsfn_dummy(JSContext *ctx,JSValueConst thisValue,int argc,JSValueConst *argv) {
  return JS_NULL;
}

/* Populate globals.
 */
 
static int exec_populate_globals() {

  exec.event=JS_NewObject(exec.ctx);
  JS_SetPropertyStr(exec.ctx,exec.event,"stopPropagation",JS_NewCFunction(exec.ctx,exec_jsfn_dummy,"stopPropagation",0));
  JS_SetPropertyStr(exec.ctx,exec.event,"preventDefault",JS_NewCFunction(exec.ctx,exec_jsfn_dummy,"preventDefault",0));

  exec.bgctx=JS_NewObject(exec.ctx);
  JS_SetPropertyStr(exec.ctx,exec.bgctx,"createRadialGradient",JS_NewCFunction(exec.ctx,exec_jsfn_createRadialGradient,"createRadialGradient",6));
  JS_SetPropertyStr(exec.ctx,exec.bgctx,"fillRect",JS_NewCFunction(exec.ctx,exec_jsfn_fillRect,"fillRect",4));
  
  exec.gamepads=JS_NewArray(exec.ctx);

  JSValue globals=JS_GetGlobalObject(exec.ctx);
  JSValue window=JS_NewObject(exec.ctx); // We'll also install it as "document", "navigator", and "console"
  #define WFN(tag,argc) JS_SetPropertyStr(exec.ctx,window,#tag,JS_NewCFunction(exec.ctx,exec_jsfn_##tag,#tag,argc));
  WFN(log,1)
  WFN(requestAnimationFrame,1)
  WFN(addEventListener,2)
  WFN(getElementsByTagName,1)
  WFN(getGamepads,0)
  #undef WFN
  JS_SetPropertyStr(exec.ctx,globals,"window",window);
  JS_SetPropertyStr(exec.ctx,globals,"document",JS_DupValue(exec.ctx,window));
  JS_SetPropertyStr(exec.ctx,globals,"console",JS_DupValue(exec.ctx,window));
  JS_SetPropertyStr(exec.ctx,globals,"navigator",JS_DupValue(exec.ctx,window));
  JS_SetPropertyStr(exec.ctx,globals,"playSong",JS_NewCFunction(exec.ctx,exec_jsfn_playSong,"playSong",1));
  JS_SetPropertyStr(exec.ctx,globals,"playSound",JS_NewCFunction(exec.ctx,exec_jsfn_playSound,"playSound",1));
  JS_SetPropertyStr(exec.ctx,globals,"videoBgctx",JS_DupValue(exec.ctx,exec.bgctx));
  JS_SetPropertyStr(exec.ctx,globals,"videoEnd",JS_NewCFunction(exec.ctx,exec_jsfn_videoEnd,"videoEnd",0));
  JS_SetPropertyStr(exec.ctx,globals,"videoBg",JS_NewCFunction(exec.ctx,exec_jsfn_videoBg,"videoBg",0));
  JS_SetPropertyStr(exec.ctx,globals,"videoRect",JS_NewCFunction(exec.ctx,exec_jsfn_videoRect,"videoRect",0));
  JS_SetPropertyStr(exec.ctx,globals,"videoBlit",JS_NewCFunction(exec.ctx,exec_jsfn_videoBlit,"videoBlit",0));
  JS_SetPropertyStr(exec.ctx,globals,"videoDecint",JS_NewCFunction(exec.ctx,exec_jsfn_videoDecint,"videoDecint",0));
  
  JS_FreeValue(exec.ctx,globals);
  return 0;
}

/* Init.
 */
 
int exec_init() {
  int err;

  /* Stand the Javascript runtime.
   */
  if (!(exec.rt=JS_NewRuntime())) return -1;
  if (!(exec.ctx=JS_NewContext(exec.rt))) return -1;
  if ((err=exec_populate_globals())<0) return err;
  
  /* Acquire the ROM's script and run it.
   */
  const char *src=0;
  int srcc=rom_get_script(&src);
  JSValue result=JS_Eval(exec.ctx,src,srcc,"ecom",JS_EVAL_TYPE_MODULE|JS_EVAL_FLAG_STRICT);
  if ((err=exec_check_exception(result,1))<0) return err;
  
  return 0;
}

/* Update.
 */
 
int exec_update() {
  int err;
  
  int i=exec.deferc;
  while (i-->0) {
    struct defer *defer=exec.deferv+i;
    if (defer->when!=WHEN_ANIMATION) continue;
    JSValue result=JS_Call(exec.ctx,defer->fn,JS_NULL,0,0);
    JS_FreeValue(exec.ctx,defer->fn);
    exec.deferc--;
    memmove(defer,defer+1,sizeof(struct defer)*(exec.deferc-i));
    if ((err=exec_check_exception(result,1))<0) return err;
  }
  
  JSContext *rctx=0;
  if (err=JS_ExecutePendingJob(exec.rt,&rctx)) {
    // Promises, etc.
    fprintf(stderr,"JS_ExecutePendingJob: %d\n",err);
    return -2;
  }
  
  return 0;
}

/* Send keydown and keyup events.
 */
 
void exec_send_key(const char *jsname,int value) {
  int when=value?WHEN_KEYDOWN:WHEN_KEYUP;
  JS_SetPropertyStr(exec.ctx,exec.event,"type",JS_NewString(exec.ctx,value?"keydown":"keyup"));
  JS_SetPropertyStr(exec.ctx,exec.event,"code",JS_NewString(exec.ctx,jsname));
  int i=exec.deferc;
  while (i-->0) {
    struct defer *defer=exec.deferv+i;
    if (defer->when!=when) continue;
    JSValue result=JS_Call(exec.ctx,defer->fn,JS_NULL,1,&exec.event);
    exec_check_exception(result,1);
  }
}

/* Set gamepad states.
 */
 
void exec_remove_gamepad(int index) {
  JS_SetPropertyUint32(exec.ctx,exec.gamepads,index,JS_NULL);
}
 
void exec_add_gamepad(int index,int axisc,int btnc) {
  int i;
  JSValue gamepad=JS_NewObject(exec.ctx);
  JS_SetPropertyStr(exec.ctx,gamepad,"index",JS_NewInt32(exec.ctx,index));
  JSValue axes=JS_NewArray(exec.ctx);
  for (i=0;i<axisc;i++) JS_SetPropertyUint32(exec.ctx,axes,i,JS_NewFloat64(exec.ctx,0.0));
  JS_SetPropertyStr(exec.ctx,gamepad,"axes",axes);
  JSValue buttons=JS_NewArray(exec.ctx);
  for (i=0;i<btnc;i++) JS_SetPropertyUint32(exec.ctx,buttons,i,JS_NewObject(exec.ctx)); // It's ok to be missing (value); our game only checks it booleanly.
  JS_SetPropertyStr(exec.ctx,gamepad,"buttons",buttons);
  JS_SetPropertyUint32(exec.ctx,exec.gamepads,index,gamepad);
}

void exec_set_gamepad_axis(int index,int axisp,double v) {
  JSValue gamepad=JS_GetPropertyUint32(exec.ctx,exec.gamepads,index);
  if (!JS_IsObject(gamepad)) return;
  JSValue axes=JS_GetPropertyStr(exec.ctx,gamepad,"axes");
  JS_SetPropertyUint32(exec.ctx,axes,axisp,JS_NewFloat64(exec.ctx,v));
  JS_FreeValue(exec.ctx,axes);
  JS_FreeValue(exec.ctx,gamepad);
}

void exec_set_gamepad_button(int index,int btnp,int v) {
  JSValue gamepad=JS_GetPropertyUint32(exec.ctx,exec.gamepads,index);
  if (!JS_IsObject(gamepad)) return;
  JSValue buttons=JS_GetPropertyStr(exec.ctx,gamepad,"buttons");
  JSValue vobj=JS_NewObject(exec.ctx); // so ridiculous, why is each button an object?
  JS_SetPropertyStr(exec.ctx,vobj,"value",JS_NewInt32(exec.ctx,v));
  JS_SetPropertyUint32(exec.ctx,buttons,btnp,vobj);
  JS_FreeValue(exec.ctx,buttons);
  JS_FreeValue(exec.ctx,gamepad);
}
