import { SCREENW, SCREENH } from "./constants.js";

export class Dot {
  constructor(e, v, a) {
    this.e = e; // Ecom
    this.v = v; // Video
    this.a = a; // Audio
  }
  
  reset() {
    this.x = 0;
    this.y = 0;
    this.gr = 80;
    this.flr = 0; // Wall from (this.e.wv), 1 for screen bottom, 0 if not seated.
    this.flp = 0; // Nonzero if facing right.
  }
  
  setup(x, y) {
    this.x = x;
    this.y = y;
  }
  
  foot() {
    for (const o of this.e.wv) {
      const dy = this.y + this.h - o.y;
      if (dy <= -1) continue;
      if (dy >= 1) continue;
      if (this.x >= o.x + o.w) continue;
      if (this.x + 16 <= o.x) continue;
      return o;
    }
    return 0;
  }
  
  update(s) {
    // If there's extra motion from a platform, Ecom applies it before each update here.
    // Horizontal motion:
    if (this.e.bdx) {
      this.x += this.e.bdx * s * 120; // px/s
      this.flp = this.e.bdx < 0;
    }
    // Gravity etc:
    if (this.flr === 1) {
      this.y = SCREENH - 24;
    } else if (this.flr) {
      this.y = this.flr.y - 24;
    } else {
      if (this.gr < 160) this.gr += 80 * s;
      this.y += this.gr * s;
    }
    // Resolve collisions and check footing:
    this.collide();
  }
  
  collide() {
    let nflr = 0;
    // Unceremoniously clamp into screen boundaries:
    if (this.x < 0) this.x = 0;
    else if (this.x > SCREENW - 16) this.x = SCREENW - 16;
    if (this.y < 0) this.y = 0;
    else if (this.y > SCREENH - 24) {
      nflr = 1;
      this.y = SCREENH - 24;
    }
    // Check all walls:
    for (const wl of this.e.wv) {
      const le = wl.x + wl.w - this.x; if (le < 0) continue;
      const ue = wl.y + wl.h - this.y; if (ue < 0) continue;
      const re = this.x + 16 - wl.x; if (re < 0) continue;
      const de = this.y + 24 - wl.y; if (de < 0) continue;
      if ((le <= re) && (le <= ue) && (le <= de)) {
        this.x = wl.x + wl.w;
      } else if ((re <= ue) && (re <= de)) {
        this.x = wl.x - 16;
      } else if (ue <= de) {
        this.y = wl.y + wl.h;
      } else {
        this.y = wl.y - 24;
        nflr = wl;
      }
    }
    // Reset gravity etc when floor changes:
    if (nflr !== this.flr) {
      this.flr = nflr;
      this.gr = 80;
    }
  }
  
  render() {
    const dx = Math.round(this.x);
    const dy = Math.round(this.y);
    if (this.flp) this.v.flop(dx, dy, 0, 48, 16, 24);
    else this.v.blit(dx, dy, 0, 48, 16, 24);
  }
}
