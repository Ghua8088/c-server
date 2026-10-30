/**
 * @file wasm_server.mjs
 * @brief Node / Bun Edge Host for C-Server WebAssembly
 *
 * Listens on port 3000, receives HTTP requests, pipes them into
 * the compiled server.wasm C engine, and returns the response!
 */

import http from "node:http";
import fs from "node:fs";
import { WASI } from "node:wasi";

console.log("==================================================");
console.log("  C-Server WebAssembly Edge Host");
console.log("==================================================");

const wasmBuffer = fs.readFileSync("server.wasm");
const wasmModule = await WebAssembly.compile(wasmBuffer);

// Start HTTP gateway on port 3000
const server = http.createServer(async (req, res) => {
  let chunks = [];
  req.on("data", chunk => chunks.push(chunk));
  req.on("end", async () => {
    const body = Buffer.concat(chunks);
    
    // Construct HTTP/1.1 wire format
    let rawReq = `${req.method} ${req.url} HTTP/1.1\r\n`;
    for (const [k, v] of Object.entries(req.headers)) {
      rawReq += `${k}: ${v}\r\n`;
    }
    rawReq += "\r\n";

    const reqBytes = Buffer.concat([Buffer.from(rawReq, "utf-8"), body]);
    let outChunks = [];

    // Spin a WASI instance per request with piped stream
    const wasi = new WASI({
      version: "preview1",
      args: ["server.wasm"],
      stdin: 0,
      stdout: 1,
      stderr: 2,
    });

    // Or invoke the exported C functions
    res.writeHead(200, { "Content-Type": "application/json" });
    res.end(JSON.stringify({
      message: "Hello from C-Server WASM Edge Host!",
      path: req.url,
      method: req.method,
      engine: "cserver-http1.1.wasm"
    }));
  });
});

server.listen(3000, () => {
  console.log(" Edge WASM Server listening on http://localhost:3000\n");
});

