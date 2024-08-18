/* Ecom.js
 * Starting point for non-generic things. The real game.
 */
 
import { SCREENW, SCREENH } from "./constants.js";
import { BTN_LEFT, BTN_RIGHT, BTN_UP, BTN_DOWN, BTN_A, BTN_B, BTN_START } from "./Input.js";
import { Dot } from "./Dot.js";
import { Physics } from "./Physics.js";
 
export class Ecom {
  constructor(g, v, a, i) {
    this.g = g;
    this.v = v;
    this.a = a;
    this.i = i;
    // Digested button state:
    this.bdx = 0;
    this.bdy = 0;
    this.bj = 0;
    this.bc = 0; // Stroke count since start of level.
    this.pvs = 0;
    // Model state:
    this.dot = new Dot(this, v, a);
    this.ph = new Physics();
  }
  
  reset(st) {
    this.bdx = 0;
    this.bdy = 0;
    this.bj = 0;
    this.bc = 0;
    this.dot.reset();
    this.ph.reset();
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
            this.ph.bv.push({ x, y, w, h, im: 0, fill: "#040" });
          } break;
          
        case "h": {
            const x = (b65(st[i++]) << 2) - 8;
            const y = (b65(st[i++]) << 2) - 24
            this.ph.bv.push({ x, y, w: 16, h: 24, im: 0.5 });
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
        }
      } else if (this.bj) {
        this.bj = 0;
      }
      this.pvs = state;
    }
    
    /* Update model.
     */
    this.dot.update(s);
    this.ph.update(s);
  }
  
  render() {
    this.v.rect(0, 0, SCREENW, SCREENH, "#68a");
    for (const b of this.ph.bv) {
      if (!b.fill) continue;
      const x = Math.round(b.x);
      const y = Math.round(b.y);
      this.v.rect(x, y, b.w, b.h, b.fill);
    }
    this.dot.render();
  }
}
