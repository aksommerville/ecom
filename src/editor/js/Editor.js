import { Dom } from "./Dom.js";
import { Scene } from "./Scene.js";
import { SceneVisual } from "./SceneVisual.js";

export class Editor {
  static getDependencies() {
    return [HTMLElement, Dom, Window];
  }
  constructor(element, dom, window) {
    this.element = element;
    this.dom = dom;
    this.window = window;
    
    this.name = "";
    this.serial = "";
    this.sceneVisual = null;
    this.scene = null;
    this.rawInputTimeout = null;
    
    this.buildUi();
  }
  
  onRemoveFromDom() {
    if (this.rawInputTimeout) {
      this.window.clearTimeout(this.rawInputTimeout);
      this.rawInputTimeout = null;
    }
  }
  
  setup(name, serial) {
    this.name = name;
    this.serial = serial;
    this.scene = new Scene(serial);
    this.populateUi();
  }
  
  buildUi() {
    this.element.innerHTML = "";
    this.sceneVisual = this.dom.spawnController(this.element, SceneVisual);
    this.sceneVisual.sceneDirty = () => this.onVisualEdit();
    this.dom.spawn(this.element, "TEXTAREA", ["raw"], { "on-input": () => this.onRawInput() });
  }
  
  populateUi() {
    this.sceneVisual.setup(this.scene);
    this.element.querySelector(".raw").value = this.serial;
  }
  
  onVisualEdit() {
    if (!this.scene) return;
    this.serial = this.scene.encode();
    this.element.querySelector(".raw").value = this.serial;
    this.save(this.serial);
  }
  
  onRawInput() {
    if (this.rawInputTimeout) return;
    this.rawInputTimeout = this.window.setTimeout(() => {
      this.rawInputTimeout = null;
      this.serial = this.element.querySelector(".raw").value;
      this.scene = new Scene(this.serial);
      this.sceneVisual.setup(this.scene);
      this.save(this.serial);
    }, 500);
  }
  
  save(serial) {
    this.window.fetch("/data/stage/" + this.name, {
      method: "PUT",
      body: serial,
    }).then(rsp => {
      if (!rsp.ok) throw rsp;
    }).catch(error => {
      this.window.console.error(error);
    });
  }
}
