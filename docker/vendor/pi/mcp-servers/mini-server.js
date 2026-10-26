#!/usr/bin/env node
// Minimal dependency-free MCP stdio server (JSON-RPC, newline-delimited).
// Used to test Pi MCP multi-server + tool calling without external deps.
const readline = require("node:readline");
const rl = readline.createInterface({ input: process.stdin });

const tools = [
  { name: "echo", description: "Echo the given text back verbatim.", inputSchema: { type: "object", properties: { text: { type: "string" } }, required: ["text"] } },
  { name: "add", description: "Add two numbers and return the sum.", inputSchema: { type: "object", properties: { a: { type: "number" }, b: { type: "number" } }, required: ["a", "b"] } },
  { name: "now", description: "Return the current UTC ISO timestamp.", inputSchema: { type: "object", properties: {} } },
];

function respond(id, result) {
  process.stdout.write(JSON.stringify({ jsonrpc: "2.0", id, result }) + "\n");
}

rl.on("line", (line) => {
  if (!line.trim()) return;
  let msg;
  try { msg = JSON.parse(line); } catch { return; }
  if (!msg || !msg.method) return;
  const m = msg.method;

  if (m === "initialize") {
    respond(msg.id, {
      protocolVersion: msg.params?.protocolVersion ?? "2024-11-05",
      capabilities: { tools: {} },
      serverInfo: { name: "mini-server", version: "1.0.0" },
    });
  } else if (m === "notifications/initialized" || m === "notifications/tools/list_changed") {
    // notifications carry no id; do not respond
  } else if (m === "tools/list") {
    respond(msg.id, { tools });
  } else if (m === "resources/list") {
    respond(msg.id, { resources: [] });
  } else if (m === "resources/templates/list") {
    respond(msg.id, { resourceTemplates: [] });
  } else if (m === "prompts/list") {
    respond(msg.id, { prompts: [] });
  } else if (m === "tools/call") {
    const name = msg.params?.name;
    const args = msg.params?.arguments ?? {};
    const tool = tools.find((t) => t.name === name);
    if (!tool) {
      respond(msg.id, { content: [{ type: "text", text: `unknown tool: ${name}` }], isError: true });
      return;
    }
    let text;
    if (name === "echo") text = String(args.text ?? "");
    else if (name === "add") text = String((Number(args.a) || 0) + (Number(args.b) || 0));
    else if (name === "now") text = new Date().toISOString();
    respond(msg.id, { content: [{ type: "text", text }] });
  } else {
    respond(msg.id, { content: [{ type: "text", text: `unhandled method: ${m}` }], isError: true });
  }
});

process.on("SIGTERM", () => process.exit(0));
