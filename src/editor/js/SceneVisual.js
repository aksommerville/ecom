const SCREENW = 256; // Must match game.
const SCREENH = 144; // Must match game.

export class SceneVisual {
  static getDependencies() {
    return [HTMLCanvasElement, Window];
  }
  constructor(element, window) {
    this.element = element;
    this.window = window;
    
    // Owner should replace:
    this.sceneDirty = () => {};
    
    this.scene = null;
    this.renderTimeout = null;
    this.clickables = []; // Rebuilds during render.
    this.mouseListener = null;
    this.drag = null; // String or object, see findClickable().
    this.dragAnchorX = 0; // Initial mouse position during drag, fb coords
    this.dragAnchorY = 0;
    this.dragObjX = 0; // Initial object position during drag, fb coords
    this.dragObjY = 0;
    this.dragObjW = 0;
    this.dragObjH = 0;
    
    this.element.width = SCREENW;
    this.element.height = SCREENH;
    this.context = this.element.getContext("2d");
    this.element.addEventListener("mousedown", e => this.onMouseDown(e));
    this.element.addEventListener("contextmenu", e => {
      e.preventDefault();
    });
  }
  
  onRemoveFromDom() {
    if (this.renderTimeout) {
      this.window.clearTimeout(this.renderTimeout);
      this.renderTimeout = null;
    }
    if (this.mouseListener) {
      this.window.removeEventListener("mousemove", this.mouseListener);
      this.window.removeEventListener("mouseup", this.mouseListener);
      this.mouseListener = null;
    }
  }
  
  setup(scene) {
    this.scene = scene;
    this.renderSoon();
  }
  
  renderSoon() {
    if (this.renderTimeout) return;
    this.renderTimeout = this.window.setTimeout(() => {
      this.renderTimeout = null;
      this.renderNow();
    }, 20);
  }
  
  renderNow() {
    this.clickables = [];
    this.context.fillStyle = "#" + (this.scene?.bg || "888");
    this.context.fillRect(0, 0, this.element.width, this.element.height);
    if (!this.scene) return;
    
    // Walls
    this.context.beginPath();
    for (const {x,y,w,h} of this.scene.walls) {
      this.context.rect(x, y, w, h);
    }
    this.context.fillStyle = "#080";
    this.context.globalAlpha = 0.5;
    this.context.fill();
    this.context.globalAlpha = 1.0;
    this.context.strokeStyle = "#080";
    this.context.stroke();
    
    // Ladders
    this.context.beginPath();
    for (const {x,y,h} of this.scene.ladders) {
      this.context.rect(x, y, 16, h);
    }
    this.context.fillStyle = "#f80";
    this.context.globalAlpha = 0.5;
    this.context.fill();
    this.context.globalAlpha = 1.0;
    this.context.strokeStyle = "#f80";
    this.context.stroke();
    
    // Platforms
    this.context.beginPath();
    for (const {x,y,w} of this.scene.platforms) {
      this.context.rect(x, y, w, 8);
    }
    this.context.fillStyle = "#840";
    this.context.globalAlpha = 0.5;
    this.context.fill();
    this.context.globalAlpha = 1.0;
    this.context.strokeStyle = "#840";
    this.context.stroke();
    
    // Win
    this.context.fillStyle = "#0f0";
    this.context.globalAlpha = 0.5;
    this.context.fillRect(this.scene.win.x, this.scene.win.y, this.scene.win.w, this.scene.win.h);
    this.context.globalAlpha = 1.0;
    
    // Hero
    this.context.fillStyle = "#408";
    this.context.fillRect(this.scene.hero.x - 8, this.scene.hero.y - 24, 16, 24);
  }
  
  onMoveOrUp(event) {
    if (event.type === "mouseup") {
      this.window.removeEventListener("mousemove", this.mouseListener);
      this.window.removeEventListener("mouseup", this.mouseListener);
      this.mouseListener = null;
      this.sceneDirty();
      return;
    }
    
    const bounds = this.element.getBoundingClientRect();
    const x = (event.clientX - bounds.left) >> 1;
    const y = (event.clientY - bounds.top) >> 1;
    const dx = x - this.dragAnchorX;
    const dy = y - this.dragAnchorY;
    
    const r4 = (src) => ((src + 2) & ~3);
    
    if (this.drag === "hero") {
      this.scene.hero.x = r4(this.dragObjX + dx);
      this.scene.hero.y = r4(this.dragObjY + dy);
      
    } else if (this.drag === "win") {
      this.scene.win.x = r4(this.dragObjX + dx);
      this.scene.win.y = r4(this.dragObjY + dy);
    } else if (this.drag === "win.l") {
      this.scene.win.x = r4(this.dragObjX + dx);
      this.scene.win.w = this.dragObjX + this.dragObjW - this.scene.win.x;
    } else if (this.drag === "win.r") {
      this.scene.win.w = r4(this.dragObjW + dx);
    } else if (this.drag === "win.t") {
      this.scene.win.y = r4(this.dragObjY + dy);
      this.scene.win.h = this.dragObjY + this.dragObjH - this.scene.win.y;
    } else if (this.drag === "win.b") {
      this.scene.win.h = r4(this.dragObjH + dy);

    } else {
      let o;
      if (this.drag.wall) o = this.drag.wall;
      else if (this.drag.ladder) o = this.drag.ladder;
      else if (this.drag.platform) o = this.drag.platform;
      else return;
      switch (this.drag.p) {
        case "l": o.x = r4(this.dragObjX + dx); o.w = this.dragObjX + this.dragObjW - o.x; break;
        case "t": o.y = r4(this.dragObjY + dy); o.h = this.dragObjY + this.dragObjH - o.y; break;
        case "r": o.w = r4(this.dragObjW + dx); break;
        case "b": o.h = r4(this.dragObjH + dy); break;
        default: {
            o.x = r4(this.dragObjX + dx);
            o.y = r4(this.dragObjY + dy);
          }
      }
    }
    
    this.renderSoon();
  }
  
  deleteClickable(o) {
    if (!o) return;
    if (typeof(o) === "string") return; // "hero" or "win" -- not deletable.
    if (o.wall) {
      const p = this.scene.walls.indexOf(o.wall);
      if (p < 0) return;
      this.scene.walls.splice(p, 1);
    } else if (o.ladder) {
      const p = this.scene.ladders.indexOf(o.ladder);
      if (p < 0) return;
      this.scene.ladders.splice(p, 1);
    } else if (o.platform) {
      const p = this.scene.platforms.indexOf(o.platform);
      if (p < 0) return;
      this.scene.platforms.splice(p, 1);
    } else {
      return;
    }
    this.renderSoon();
    this.sceneDirty();
  }
  
  onMouseDown(event) {
    if (this.mouseListener) return;
    
    const x = event.offsetX >> 1;
    const y = event.offsetY >> 1;
    
    const found = this.findClickable(x, y);
    if (!found) return;
    
    if (event.button === 2) {
      this.deleteClickable(found);
      return;
    }
    
    if (found === "hero") {
      this.dragObjX = this.scene.hero.x;
      this.dragObjY = this.scene.hero.y;
    } else if (found.startsWith?.("win")) {
      this.dragObjX = this.scene.win.x;
      this.dragObjY = this.scene.win.y;
      this.dragObjW = this.scene.win.w;
      this.dragObjH = this.scene.win.h;
    } else if (found.wall) {
      this.dragObjX = found.wall.x;
      this.dragObjY = found.wall.y;
      this.dragObjW = found.wall.w;
      this.dragObjH = found.wall.h;
    } else if (found.ladder) {
      this.dragObjX = found.ladder.x;
      this.dragObjY = found.ladder.y;
      this.dragObjW = found.ladder.w;
      this.dragObjH = found.ladder.h;
    } else if (found.platform) {
      this.dragObjX = found.platform.x;
      this.dragObjY = found.platform.y;
      this.dragObjW = found.platform.w;
      this.dragObjH = found.platform.h;
    } else {
      return;
    }
    this.drag = found;
    this.dragAnchorX = x;
    this.dragAnchorY = y;
    
    this.mouseListener = e => this.onMoveOrUp(e);
    this.window.addEventListener("mousemove", this.mouseListener);
    this.window.addEventListener("mouseup", this.mouseListener);
  }
  
  findClickable(x, y) {
    if (!this.scene) return null;
    const margin = 4;
    
    if ((x >= this.scene.hero.x - 8) && (y >= this.scene.hero.y - 24) && (x < this.scene.hero.x + 8) && (y < this.scene.hero.y)) {
      return "hero";
    }
    
    {
      const dx = x - this.scene.win.x;
      const dy = y - this.scene.win.y;
      if ((dx >= 0) && (dy >= 0) && (dx < this.scene.win.w) && (dy < this.scene.win.h)) {
        if (dx < margin) return "win.l";
        if (dy < margin) return "win.t";
        if (dx > this.scene.win.w - margin) return "win.r";
        if (dy > this.scene.win.h - margin) return "win.b";
        return "win";
      }
    }
    
    for (const wall of this.scene.walls) {
      const dx = x - wall.x;
      const dy = y - wall.y;
      if ((dx >= 0) && (dy >= 0) && (dx < wall.w) && (dy < wall.h)) {
        if (dx < margin) return { wall, p: "l" };
        if (dy < margin) return { wall, p: "t" };
        if (dx > wall.w - margin) return { wall, p: "r" };
        if (dy > wall.h - margin) return { wall, p: "b" };
        return { wall };
      }
    }
    
    for (const platform of this.scene.platforms) {
      const dx = x - platform.x;
      const dy = y - platform.y;
      if ((dx >= 0) && (dy >= 0) && (dx < platform.w) && (dy < 8)) {
        if (dx < margin) return { platform, p: "l" };
        if (dx > platform.w - margin) return { platform, p: "r" };
        return { platform };
      }
    }
    
    for (const ladder of this.scene.ladders) {
      const dx = x - ladder.x;
      const dy = y - ladder.y;
      if ((dx >= 0) && (dy >= 0) && (dx < 16) && (dy < ladder.h)) {
        if (dy < margin) return { ladder, p: "t" };
        if (dy > ladder.h - margin) return { ladder, p: "b" };
        return { ladder };
      }
    }
    
    return null;
  }
}
