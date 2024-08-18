import { Dom } from "./Dom.js";
import { Editor } from "./Editor.js";

export class RootUi {
  static getDependencies() {
    return [HTMLElement, Dom, Window];
  }
  constructor(element, dom, window) {
    this.element = element;
    this.dom = dom;
    this.window = window;
  }
  
  setup(scenes) {
    this.scenes = scenes;
    this.element.innerHTML = "";
    
    const sidebar = this.dom.spawn(this.element, "UL", ["sidebar"]);
    for (const scene of scenes) {
      this.dom.spawn(sidebar, "LI", { "data-scene": scene, "on-click": () => this.navigateToScene(scene) }, scene);
    }
    this.dom.spawn(sidebar, "LI", { "on-click": () => this.newScene() }, "NEW");
    
    const workspace = this.dom.spawn(this.element, "DIV", ["workspace"]);
    
    this.window.addEventListener("hashchange", e => this.onHashChange(e.newURL));
    this.onHashChange(this.window.location.hash);
  }
  
  navigateToScene(name) {
    this.window.location = "#" + name;
  }
  
  loadScene(name) {
    for (const element of this.element.querySelectorAll(".sidebar .highlight")) {
      element.classList.remove("highlight");
    }
    this.element.querySelector(`.sidebar li[data-scene='${name}']`)?.classList.add("highlight");
    this.window.fetch("/data/stage/" + name).then(rsp => {
      if (!rsp.ok) throw rsp;
      return rsp.text();
    }).then(serial => {
      const workspace = this.element.querySelector(".workspace");
      workspace.innerHTML = "";
      const editor = this.dom.spawnController(workspace, Editor);
      editor.setup(name, serial);
    }).catch(error => {
      this.window.console.error(error);
    });
  }
  
  newScene() {
    let id = 1;
    for (;; id++) {
      if (this.scenes.find(s => parseInt(s) === id)) continue;
      break;
    }
    const name = id.toString();
    this.scenes.push(name);
    const body = "";
    this.window.fetch("/data/stage/" + name, {
      method: "PUT",
      body,
    }).then(() => {
      const sidebar = this.element.querySelector(".sidebar");
      this.dom.spawn(sidebar, "LI", { "data-scene": name, "on-click": () => this.navigateToScene(name) }, name);
      return this.navigateToScene(name);
    }).catch(e => {
      this.window.console.error(e);
    });
  }
  
  onHashChange(hash) {
    hash = (hash || "").split('#')[1] || "";
    if (hash) this.loadScene(hash);
  }
}
