import { SCREENW, SCREENH } from "./constants.js";

XXX Don't do generic physics. It's complicated, and I don't think we'll need it.

export class Physics {
  constructor() {
  }
  
  reset() {
    this.bv = []; // { x,y,w,h:pixels, im:0..1(inverse mass) }
  }
  
  update(s) {
    let c = 0, m;
    do {
      m = 0;
      for (let ai=this.bv.length; ai-->0; ) {
        const a = this.bv[ai];
        
        if (a.x < 0) { a.x = 0; m = 1; }
        else if (a.x + a.w > SCREENW) { a.x = SCREENW - a.w; m = 1; }
        if (a.y < 0) { a.y = 0; m = 1; }
        else if (a.y + a.h > SCREENH) { a.y = SCREENH - a.h; m = 1; }
        
        for (let bi=ai; bi-->0; ) {
          const b = this.bv[bi];
          const ims = a.im + b.im;
          if (!ims) continue;
          const r = this.rej(a, b);
          if (!r) continue;
          m = 1;
          if ((a.im <= 0) || (b.im >= 1)) {
            b.x -= r[0];
            b.y -= r[1];
          } else if ((a.im >= 1) || (b.im <= 0)) {
            a.x += r[0];
            a.y += r[1];
          } else {
            const aw = a.im / ims;
            const bw = 1 - aw;
            a.x += r[0] * aw;
            a.y += r[1] * aw;
            b.x -= r[0] * bw;
            b.y -= r[1] * bw;
          }
        }
      }
      if (++c >= 8) m = false; // Panic, otherwise rounding errors could get us stuck.
    } while (m);
  }
  
  /* Calculate rejection for two overlapping bodies.
   * 0 if they don't overlap.
   * Otherwise [dx,dy] how (a) should move to clear the collision.
   */
  rej(a, b) {
    let dxl, dxr, dxu, dxd;
    if ((dxl = a.x + a.w - b.x) <= 0) return 0;
    if ((dxr = b.x + b.w - a.x) <= 0) return 0;
    if ((dxu = a.y + a.h - b.y) <= 0) return 0;
    if ((dxd = b.y + b.h - a.y) <= 0) return 0;
    if ((dxl <= dxr) && (dxl <= dxu) && (dxl <= dxd)) return [-dxl, 0];
    if ((dxr <= dxu) && (dxr <= dxd)) return [dxr, 0];
    if (dxu <= dxd) return [0, -dxu];
    return [0, dxd];
  }
}
