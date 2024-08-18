import { SCREENW, SCREENH } from "./constants.js";

export class Dot {
  constructor(e, v, a) {
    this.e = e;
    this.v = v;
    this.a = a;
  }
  
  reset() {
    this.gr = 80;
  }
  
  foot(b) {
    for (const o of this.e.ph.bv) {
      const dy = b.y + b.h - o.y;
      if (dy <= -1) continue;
      if (dy >= 1) continue;
      if (b.x >= o.x + o.w) continue;
      if (b.x + b.w <= o.x) continue;
      return 1;
    }
    return 0;
  }
  
  update(s) {
    if (this.e.ph.bv.length < 1) return;
    const b = this.e.ph.bv[0];
    if (this.e.bdx) {
      b.x += this.e.bdx * s * 120; // px/s
    }
    if (this.e.bdy) {//XXX Similar vertical motion, just testing physics
      b.y += this.e.bdy * s * 240;
    }
    if (this.foot(b)) {
      this.gr = 80;
    } else {
      if (this.gr < 160) this.gr += 80 * s;
      b.y += this.gr * s;
    }
  }
  
  render() {
    if (this.e.ph.bv.length < 1) return;
    const b = this.e.ph.bv[0];
    const dx = Math.round(b.x);
    const dy = Math.round(b.y);
    this.v.blit(dx, dy, 0, 48, 16, 24);
  }
}
