import fs from "node:fs";
import { WASI } from "node:wasi";

console.log("==================================================");
console.log("  Node.js + WebAssembly Engine Runner");
console.log("==================================================");

const wasi = new WASI({
  version: "preview1",
  args: ["server.wasm"],
  env: process.env,
});

const wasmBuffer = fs.readFileSync("server.wasm");
const wasmModule = await WebAssembly.compile(wasmBuffer);
const instance = await WebAssembly.instantiate(wasmModule, {
  ...wasi.getImportObject(),
});

wasi.start(instance);

