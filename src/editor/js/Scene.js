export class Scene {
  constructor(serial) {
    this.decode(serial);
  }
  
  encode() {
    let dst = "";
    dst += `song ${this.song}\n`;
    dst += `bg ${this.bg}\n`;
    dst += `hero ${this.hero.x} ${this.hero.y}\n`;
    dst += `win ${this.win.x} ${this.win.y} ${this.win.w} ${this.win.h}\n`;
    for (const {x,y,w,h} of this.walls) dst += `wall ${x} ${y} ${w} ${h}\n`;
    for (const {x,y,w,dx,dy} of this.platforms) dst += `platform ${x} ${y} ${w} ${dx} ${dy}\n`;
    for (const {x,y,h} of this.ladders) dst += `ladder ${x} ${y} ${h}\n`;
    return dst;
  }
  
  decode(serial) {
    this.bg = "888";
    this.walls = []; // {x,y,w,h}
    this.hero = {x:128, y:72};
    this.song = "";
    this.platforms = []; // {x,y,w,dx,dy} h always 8
    this.ladders = []; // {x,y,h} w always 16
    this.win = {x:0, y:0, w:32,h:32};
    
    for (const line of serial.split("\n").map(line => line.split("#")[0].trim())) {
      if (!line) continue;
      const words = line.split(/\s+/g);
      switch (words[0]) {
        case "bg": if (words[1]?.match?.(/^[0-9a-f]{3}$/)) this.bg = words[1]; break;
        case "wall": {
            this.walls.push({
              x: +words[1] || 0,
              y: +words[2] || 0,
              w: +words[3] || 0,
              h: +words[4] || 0,
            });
          } break;
        case "hero": {
            this.hero.x = +words[1] || 0;
            this.hero.y = +words[2] || 0;
          } break;
        case "song": {
            this.song = words[1] || "";
          } break;
        case "platform": {
            this.platforms.push({
              x: +words[1] || 0,
              y: +words[2] || 0,
              w: +words[3] || 0,
              dx: +words[4] || 0,
              dy: +words[5] || 0,
            });
          } break;
        case "ladder": {
            this.ladders.push({
              x: +words[1] || 0,
              y: +words[2] || 0,
              h: +words[3] || 0,
            });
          } break;
        case "win": {
            this.win.x = +words[1] || 0;
            this.win.y = +words[2] || 0;
            this.win.w = +words[3] || 0;
            this.win.h = +words[4] || 0;
          } break;
      }
    }
  }
}
