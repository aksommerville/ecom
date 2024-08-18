const http = require("http");
const fs = require("fs");
const child_process = require("child_process");

function guessContentType(path, serial) {
  const sfx = (path.match(/.*\.([^.\/]*)$/) || ['', ''])[1].toLowerCase();
  switch (sfx) {
    case "js": return "application/javascript";
    case "css": return "text/css";
    case "png": return "image/png";
    case "ico": return "image/x-icon";
    case "html": return "text/html";
    default: return "application/octet-stream";
  }
}

function generateJsonDirectoryListing(path) {
  return JSON.stringify(fs.readdirSync(path));
}

function fail(rsp, code, msg) {
  rsp.statusCode = code;
  rsp.statusMessage = msg;
  rsp.end();
}

function serveGet(req, rsp) {
  if (req.url.indexOf('?') >= 0) return fail(rsp, 400, "GETs must not have a query string");
  if (!req.url.startsWith('/') || (req.url.indexOf("..") >= 0)) return fail(rsp, 400, "Invalid path");
  let path;
  if (req.url.startsWith("/data/")) {
    path = "src/www" + req.url.substring(5);
  } else if (req.url === "/") {
    path = "src/editor/index.html";
  } else {
    path = "src/editor" + req.url;
  }
  try {
    const st = fs.statSync(path);
    if (st.isDirectory()) {
      rsp.setHeader("Content-Type", "application/json");
      rsp.setHeader("X-Is-Directory", "true"); // important signal to our editor
      rsp.statusCode = 200;
      rsp.end(generateJsonDirectoryListing(path));
    } else {
      const serial = fs.readFileSync(path);
      // Browsers are picky about Content-Type, for some things like Javascript. Highly stupid :P
      rsp.setHeader("Content-Type", guessContentType(path, serial));
      rsp.statusCode = 200;
      rsp.end(serial);
    }
  } catch (e) {
    console.log(e);
    fail(rsp, 404, "File not found");
  }
}

function servePut(req, rsp) {
  if (req.url.indexOf('?') >= 0) return fail(rsp, 400, "PUTs must not have a query string");
  if (!req.url.startsWith("/data/")) return fail(resp, 404, "File not found");
  if (req.url.indexOf("..") >= 0) return fail(rsp, 404, "File not found");
  const path = "src/www" + req.url.substring(5);
  try {
    fs.writeFileSync(path, req.body);
    rsp.statusCode = 200;
    rsp.end();
  } catch (e) {
    fail(rsp, 500, "Error writing file");
  }
}

const server = http.createServer();
server.listen(8080, () => {
  console.log(`Serving on port 8080`);
  server.on("request", (req, rsp) => {
    let body = "";
    req.on("data", (chunk) => body += chunk);
    req.on("end", () => {
      req.body = body;
      switch (req.method) {
        case "GET": return serveGet(req, rsp);
        case "PUT": return servePut(req, rsp);
        default: {
            rsp.statusCode = 405;
            rsp.end();
          }
      }
    });
  });
});
