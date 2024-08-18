import { Injector } from "./js/Injector.js";
import { Dom } from "./js/Dom.js";
import { RootUi } from "./js/RootUi.js";

window.addEventListener("load", () => {
  const injector = new Injector(window, document);
  const dom = injector.getInstance(Dom);
  fetch("/data/stage").then(rsp => {
    if (!rsp.ok || (rsp.headers.get("X-Is-Directory") !== "true")) throw rsp;
    return rsp.json();
  }).then((stages) => {
    const rootUi = dom.spawnController(document.body, RootUi);
    rootUi.setup(stages);
  }).catch((error) => {
    console.error(error);
  });
}, { once: true });
