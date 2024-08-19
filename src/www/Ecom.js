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
    this.tkv = []; // {x,y,y0,dx,n,t}, tickets, the little "1", "2", "3" for each keystroke.
    this.wz = { x:0, y:0, w:0, h:0 }; // Win zone.
    this.wt = 0; // Win time. Counts up after winning.
    this.dt = 0; // Dead time. Counts up.
  }
  
  reset(st) {
    this.st = st;
    this.bdx = 0;
    this.bdy = 0;
    this.bj = 0;
    this.bc = 0;
    this.dot.reset();
    this.wv = [];
    this.ldrv = [];
    this.tkv = [];
    this.wz = { x:0, y:0, w:0, h:0 };
    this.bgc = "#8af";
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
            const dx = (b65(st[i++]) << 2) - 128;
            const dy = (b65(st[i++]) << 2) - 128;
            this.wv.push({ x, y, w, h:8, dx, dy, hold:0 });
          } break;
      
        case "l": {
            const x = b65(st[i++]) << 2;
            const y = b65(st[i++]) << 2;
            const h = b65(st[i++]) << 2;
            this.ldrv.push({ x, y, h });
          } break;
      
        case "e": {
            const x = b65(st[i++]) << 2;
            const y = b65(st[i++]) << 2;
            const w = b65(st[i++]) << 2;
            const h = b65(st[i++]) << 2;
            this.wz = { x, y, w, h };
          } break;
      }
    }
    this.rbg();
  }
  
  win() {
    if (this.dot.ded) return;
    this.a.playSong("taf", 1);
    this.wt = 0.01;
  }
  
  stk() {
    if (this.bc >= 13) return;
    this.tkv.push({
      x: this.dot.x + 8 - 3.5,
      y: this.dot.y - 8,
      y0: this.dot.y - 8,
      dx: (this.bdx < 0) ? 1 : (this.bdx > 0) ? -1 : this.dot.flp ? 1 : -1,
      t: 0,
      n: this.bc,
    });
    if (++this.bc >= 13) {
      this.dot.die();
    }
  }
  
  update(s, state) {
  
    if (this.wt > 0) {
      this.wt += s;
      this.bdx = 0;
      this.bdy = 0;
    }
    if (this.dot.ded) {
      this.dt += s;
    }
  
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
      if (!this.wt) {
        const ndx = kd(state & (BTN_LEFT | BTN_RIGHT));
        const ndy = kd(state & (BTN_UP | BTN_DOWN));
        if (ndx !== this.bdx) {
          if (this.bdx = ndx) this.stk();
        }
        if (ndy !== this.bdy) {
          if ((this.bdy = ndy) && this.dot.fldr()) this.stk();
        }
      }
      if (state & BTN_A) {
        if (!this.bj) {
          this.bj = 1;
          if (this.wt) {
            this.wt = 0;
            const st = this.g.nextStage();
            if (st) {
              this.reset(st);
            } else {
              console.log(`Game over, victory!`);
              this.reset(this.g.nextStage());
            }
          } else if (this.dot.ded > 0.5) {
            this.reset(this.st);
          } else if (this.dot.ded > 0.0) {
            // Hold
          } else if (!this.wt) {
            this.stk();
            this.dot.jon();
          }
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
      if (!this.dot.ded && !this.wt && (this.dot.flr === wl)) {
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
    for (let i=this.tkv.length; i-->0; ) {
      const tk = this.tkv[i];
      tk.t += s;
      tk.x += tk.dx * s * 10;
      tk.y = tk.y0 + (tk.t - 0.250) ** 2 * 100;
      if (tk.y > SCREENH) this.tkv.splice(i, 1);
    }
    this.dot.update(s);
    
    /* Check win.
     */
    if (!this.dot.ded && !this.wt) {
      const x = this.dot.x + 8;
      const y = this.dot.y + 12;
      if ((x >= this.wz.x) && (y >= this.wz.y) && (x < this.wz.x + this.wz.w) && (y < this.wz.y + this.wz.h)) {
        this.win();
      }
    }
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
    this.v.bg();
    for (const wl of this.wv) {
      if (wl.c) continue;
      let x = Math.round(wl.x);
      let y = Math.round(wl.y);
      for (let subx=x+8, i=Math.floor(wl.w/8)-2; i-->0; subx+=8) {
        this.v.blit(subx, y, 88, 7, 8, 8);
      }
      this.v.blit(x, y, 80, 7, 8, 8);
      this.v.flop(x + wl.w - 8, y, 79, 7, 8, 8);
    }
    this.dot.render();
    for (const tk of this.tkv) {
      this.v.blit(Math.floor(tk.x), Math.floor(tk.y), 28 + tk.n * 7, 0, 7, 7);
    }
    this.v.blit(1, SCREENH - 6, 88 + Math.floor(this.bc / 10) * 3, 15, 3, 5);
    this.v.blit(5, SCREENH - 6, 88 + (this.bc % 10) * 3, 15, 3, 5);
    if (this.wt) {
      this.v.rect(99, 137, 58, 6, ((this.wt * 2) & 1) ? "#000" : "#084");
      this.v.blit(100, 138, 64, 23, 56, 4);
    } else if (this.dot.ded) {
      this.v.rect(99, 137, 64, 6, ((this.dt * 2) & 1) ? "#000" : "#408");
      this.v.blit(100, 138, 64, 27, 62, 4);
    }
  }
  
  rbg() {
    this.v.bc.fillStyle = this.bgc;
    this.v.bc.fillRect(0, 0, SCREENW, SCREENH);
    
    for (const wl of this.wv) {
      if (!wl.c) continue;
      const colc = wl.w >> 2;
      const rowc = wl.h >> 2;
      const x0 = Math.round(wl.x);
      let y = Math.round(wl.y);
      for (let yi=rowc; yi-->0; y+=4) {
        for (let x=x0, xi=colc; xi-->0; x+=4) {
          // This is criminally inefficient but meh. It only happens once per level.
          const nv = [
            this.ckw(x - 2, y - 2),
            this.ckw(x + 2, y - 2),
            this.ckw(x + 6, y - 2),
            this.ckw(x - 2, y + 2),
            this.ckw(x + 6, y + 2),
            this.ckw(x - 2, y + 6),
            this.ckw(x + 2, y + 6),
            this.ckw(x + 6, y + 6),
          ];
          if (nv[1] && nv[3] && nv[4] && nv[6]) { // All cardinals.
            if (nv[0] && nv[2] && nv[5] && nv[7]) { // All diagonals.
              this.v.bblit(x, y, 120, 7, 4, 4);
            } else if (!nv[0]) { // NW missing. It's not possible for more than one to be missing; walls must be at least 2 rows and columns thick (8 pixels).
              this.v.bblit(x, y, 108, 11, 4, 4);
            } else if (!nv[2]) {
              this.v.bblit(x, y, 104, 11, 4, 4);
            } else if (!nv[5]) {
              this.v.bblit(x, y, 108, 7, 4, 4);
            } else {
              this.v.bblit(x, y, 104, 7, 4, 4);
            }
          } else if (nv[1] && nv[3] && nv[4]) { // 3 cardinals: flat edge.
            this.v.bblit(x, y, 112, 7, 4, 4);
          } else if (nv[1] && nv[3] && nv[6]) {
            this.v.bblit(x, y, 116, 11, 4, 4);
          } else if (nv[1] && nv[4] && nv[6]) {
            this.v.bblit(x, y, 112, 11, 4, 4);
          } else if (nv[3] && nv[4] && nv[6]) {
            this.v.bblit(x, y, 116, 7, 4, 4);
          } else if (nv[1] && nv[3]) { // 2 cardinals: corner.
            this.v.bblit(x, y, 100, 11, 4, 4);
          } else if (nv[1] && nv[4]) {
            this.v.bblit(x, y, 96, 11, 4, 4);
          } else if (nv[3] && nv[6]) {
            this.v.bblit(x, y, 100, 7, 4, 4);
          } else if (nv[4] && nv[6]) {
            this.v.bblit(x, y, 96, 7, 4, 4);
          } else { // Nothing else should be possible. But we do have a singleton tile just in case.
            this.v.bblit(x, y, 124, 11, 4, 4);
          }
        }
      }
    }
    
    for (const ldr of this.ldrv) {
      const x = Math.round(ldr.x);
      let y = Math.round(ldr.y + ldr.h); // Bottom.
      while (y > ldr.y) {
        y -= 6;
        this.v.bblit(x, y, 64, 7, 16, 6);
      }
    }
    
    let x = this.wz.x + 8;
    const y = this.wz.y + this.wz.h;
    let i = (this.wz.w - 16) / 8;
    for (; i-->0; x+=8) this.v.bblit(x, y, 72, 15, 8, 8);
    this.v.bblit(this.wz.x, y, 64, 15, 8, 8);
    this.v.bblit(this.wz.x + this.wz.w - 8, y, 80, 15, 8, 8);
  }
  
  ckw(x, y) {
    if (x < 0) return 1;
    if (y < 0) return 1;
    if (x >= SCREENW) return 1;
    if (y >= SCREENH) return 1;
    for (const wl of this.wv) {
      if (!wl.c) continue; // Don't count moving platforms.
      if (x < wl.x) continue;
      if (y < wl.y) continue;
      if (x >= wl.x + wl.w) continue;
      if (y >= wl.y + wl.h) continue;
      return 1;
    }
    return 0;
  }
}
