/**
 * @file bun_worker.ts
 * @brief Bun + C-Server WebAssembly Runner
 * 
 * Runs the C-server HTTP engine inside Bun's runtime.
 * Bun acts as the edge host (like Cloudflare Workers), routing HTTP
 * into the compiled WebAssembly state machine.
 */

import { WASI } from "wasi";

console.log("==================================================");
console.log("  Starting C-Server inside Bun (WebAssembly Edge)");
console.log("==================================================");

// 1. Initialize WASI Sandbox
const wasi = new WASI({
  args: ["server.wasm"],
  env: process.env,
});

// 2. Load compiled server.wasm
const wasmPath = "./server.wasm";
const wasmFile = Bun.file(wasmPath);

if (!(await wasmFile.exists())) {
  console.error(`[Error] ${wasmPath} not found! Compile it first with:`);
  console.error("zig cc -target wasm32-wasi -Iinclude examples/main.c src/server.c src/context.c src/http_proto.c src/engine/engine.c src/platform/connection.c src/platform/socket_wasi.c src/platform/daemon.c -o server.wasm");
  process.exit(1);
}

const wasmBytes = await wasmFile.arrayBuffer();
const wasmModule = await WebAssembly.compile(wasmBytes);
const instance = await WebAssembly.instantiate(wasmModule, {
  ...wasi.getImports(wasmModule),
});

wasi.start(instance);

console.log(" WebAssembly C-Server module initialized successfully!");
console.log(" Ready for Edge execution via Bun.");
