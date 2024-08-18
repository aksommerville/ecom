import { SCREENW, SCREENH } from "./constants.js";
import { BTN_A, BTN_B, BTN_START } from "./Input.js";

export class Video {
  constructor() {
    this.src = document.querySelector("img"); // If the game needs multiple images, you might need to adjust the HTML minifier.
    this.cv = document.querySelector("canvas");
    this.cv.width = SCREENW;
    this.cv.height = SCREENH;
    this.c = this.cv.getContext("2d");
  }
  
  end() {
    this.c.fillStyle = "#888";
    this.c.fillRect(0, 0, SCREENW, SCREENH);
  }

  rect(x, y, w, h, c) {
    this.c.fillStyle = c;
    this.c.fillRect(x, y, w, h);
  }
  
  blit(dx, dy, sx, sy, w, h) {
    this.c.drawImage(this.src, sx, sy, w, h, dx, dy, w, h);
  }
}
