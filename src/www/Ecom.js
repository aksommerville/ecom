/* Ecom.js
 * Starting point for non-generic things. The real game.
 */
 
import { SCREENW, SCREENH } from "./constants.js";
import { BTN_LEFT, BTN_RIGHT, BTN_UP, BTN_DOWN, BTN_A, BTN_B, BTN_START } from "./Input.js";
import { Dot } from "./Dot.js";
 
export class Ecom {
  constructor(g, v, a, i) {
    this.g = g; // Game
    this.v = v; // Video
    this.a = a; // Audio
    this.i = i; // Input
    // Digested button state:
    this.bdx = 0;
    this.bdy = 0;
    this.bj = 0;
    this.bc = 0; // Stroke count since start of level.
    this.pvs = 0;
    // Model state:
    this.dot = new Dot(this, v, a);
    this.wv = []; // {x,y,w,h,dx,dy,c}, walls and platforms
    this.ldrv = []; // {x,y,h}, ladders. Width always 16.
  }
  
  reset(st) {
    this.bdx = 0;
    this.bdy = 0;
    this.bj = 0;
    this.bc = 0;
    this.dot.reset();
    this.wv = [];
    this.ldrv = [];
    const b65 = (s) => {
      s = s.charCodeAt(0);
      if ((s >= 0x41) && (s <= 0x5a)) return s - 0x41;
      if ((s >= 0x61) && (s <= 0x7a)) return s - 0x61 + 26;
      if ((s >= 0x30) && (s <= 0x39)) return s - 0x30 + 52;
      if (s === 0x2b) return 62;
      if (s === 0x2f) return 63;
      if (s === 0x3f) return 64;
      return 0;
    };
    for (let i=0; i<st.length; ) {
      const op = st[i++];
      switch (op) {
      
        case "w": {
            const x = b65(st[i++]) << 2;
            const y = b65(st[i++]) << 2;
            const w = b65(st[i++]) << 2;
            const h = b65(st[i++]) << 2;
            this.wv.push({ x, y, w, h, c: "#040" });
          } break;
          
        case "h": {
            const x = (b65(st[i++]) << 2) - 8;
            const y = (b65(st[i++]) << 2) - 24;
            this.dot.setup(x, y);
          } break;
          
        case "b": {
            this.bgc = "#" + st.substring(i, i + 3);
            i += 3;
          } break;
          
        case "s": {
            const termp = st.indexOf(".", i);
            this.a.playSong(st.substring(i, termp));
            i = termp + 1;
          } break;
      
        case "p": {
            const x = b65(st[i++]) << 2;
            const y = b65(st[i++]) << 2;
            const w = b65(st[i++]) << 2;
            const h = b65(st[i++]) << 2;
            const dx = (b65(st[i++]) << 2) - 128;
            const dy = (b65(st[i++]) << 2) - 128;
            this.wv.push({ x, y, w, h, dx, dy, c: "#840", hold:0 });
          } break;
      
        case "l": {
            const x = b65(st[i++]) << 2;
            const y = b65(st[i++]) << 2;
            const h = b65(st[i++]) << 2;
            this.ldrv.push({ x, y, h });
          } break;
      }
    }
  }
  
  stk() {
    this.bc++;
    console.log(`KEYSTROKE ${this.bc}`);
  }
  
  update(s, state) {
  
    /* Digest input state, track keystrokes.
     */
    if (state !== this.pvs) {
      const kd = (n) => {
        switch (n) {
          case BTN_LEFT: case BTN_UP: return -1;
          case BTN_RIGHT: case BTN_DOWN: return 1;
        }
        return 0;
      };
      const ndx = kd(state & (BTN_LEFT | BTN_RIGHT));
      const ndy = kd(state & (BTN_UP | BTN_DOWN));
      if (ndx !== this.bdx) {
        if (this.bdx = ndx) this.stk();
      }
      if (ndy !== this.bdy) {
        if (this.bdy = ndy) this.stk();
      }
      if (state & BTN_A) {
        if (!this.bj) {
          this.bj = 1;
          this.stk();
          this.dot.jon();
        }
      } else if (this.bj) {
        this.bj = 0;
        this.dot.joff();
      }
      this.pvs = state;
    }
    
    /* Update model.
     */
    for (const wl of this.wv) {
      if (!wl.dx && !wl.dy) continue;
      const dx = wl.dx * s;
      const dy = wl.dy * s;
      wl.x += dx;
      wl.y += dy;
      if (this.dot.flr === wl) {
        this.dot.x += dx;
        this.dot.y += dy;
      }
      if (wl.hold > 0) {
        wl.hold -= s;
      } else if (this.ckwl(wl)) {
        wl.x -= dx;
        wl.y -= dy;
      }
    }
    this.dot.update(s);
  }
  
  /* Check moving wall that just moved.
   * If it collides with any other wall, reverse direction and return true.
   */
  ckwl(wl) {
    if ((wl.x < 0) || (wl.x + wl.w >= SCREENW) || (wl.y < 0) || (wl.y + wl.h >= SCREENH)) {
      wl.dx *= -1;
      wl.dy *= -1;
      wl.hold = 0.250;
      return 1;
    }
    for (const o of this.wv) {
      if (o === wl) continue;
      if (o.x >= wl.x + wl.w) continue;
      if (o.y >= wl.y + wl.h) continue;
      if (o.x + o.w <= wl.x) continue;
      if (o.y + o.h <= wl.y) continue;
      wl.dx *= -1;
      wl.dy *= -1;
      wl.hold = 0.250;
      return 1;
    }
    return 0;
  }
  
  render() {
    this.v.rect(0, 0, SCREENW, SCREENH, "#68a");
    for (const wl of this.wv) {
      if (!wl.c) continue;
      const x = Math.round(wl.x);
      const y = Math.round(wl.y);
      this.v.rect(x, y, wl.w, wl.h, wl.c);
    }
    for (const ldr of this.ldrv) {
      const x = Math.round(ldr.x);
      const y = Math.round(ldr.y);
      this.v.rect(x, y, 16, ldr.h, "#800");
    }
    this.dot.render();
  }
}
