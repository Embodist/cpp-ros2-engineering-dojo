#!/usr/bin/env node
// SearXNG MCP server — dependency-free stdio JSON-RPC wrapper for a local SearXNG JSON API.
// Reuses the same pattern as mini-server.js (no npm deps → no Zod v3/v4 conflicts).
// env: SEARXNG_URL (default http://localhost:11199), SEARXNG_TIMEOUT_MS (default 20000)
const readline = require("node:readline");
const rl = readline.createInterface({ input: process.stdin });

const SEARXNG_URL = (process.env.SEARXNG_URL || "http://localhost:11199").replace(/\/+$/, "");
const TIMEOUT_MS = Number(process.env.SEARXNG_TIMEOUT_MS || 20000);

const tools = [
  {
    name: "search",
    description: "Web search via local SearXNG (metasearch engine). Returns ranked title/url/snippet results.",
    inputSchema: {
      type: "object",
      properties: {
        query: { type: "string", description: "Search query" },
        maxResults: { type: "number", description: "Max results to return (default 10)" },
        categories: { type: "string", description: "Optional: general, news, images, videos, files (comma-separated)" },
        language: { type: "string", description: "Optional: auto, en, zh-CN, ..." },
        safesearch: { type: "number", description: "0 = off, 1 = moderate (default 0)" },
        timeRange: { type: "string", description: "Optional: day, week, month, year" },
      },
      required: ["query"],
    },
  },
];

function respond(id, result) {
  process.stdout.write(JSON.stringify({ jsonrpc: "2.0", id, result }) + "\n");
}
function respondError(id, message) {
  respond(id, { content: [{ type: "text", text: `error: ${message}` }], isError: true });
}

async function searxngSearch(args) {
  const query = String(args.query ?? "").trim();
  if (!query) return "error: query 不能为空";
  const maxResults = Math.min(Number(args.maxResults) || 10, 50);
  const url = new URL(`${SEARXNG_URL}/search`);
  url.searchParams.set("q", query);
  url.searchParams.set("format", "json");
  url.searchParams.set("safesearch", String(args.safesearch ?? 0));
  if (args.categories) url.searchParams.set("categories", String(args.categories));
  if (args.language) url.searchParams.set("language", String(args.language));
  if (args.timeRange) url.searchParams.set("time_range", String(args.timeRange));

  const ctrl = new AbortController();
  const timer = setTimeout(() => ctrl.abort(), TIMEOUT_MS);
  try {
    const res = await fetch(url, { signal: ctrl.signal, headers: { Accept: "application/json" } });
    if (!res.ok) throw new Error(`SearXNG HTTP ${res.status}`);
    const data = await res.json();
    const results = (data.results || []).slice(0, maxResults);
    if (results.length === 0) {
      const answers = (data.answers || []).join("; ");
      return `没有找到结果。${answers ? `SearXNG 提示: ${answers}` : "(尝试换关键词或 categories)"}`;
    }
    return results
      .map((r, i) => {
        const lines = [`[${i + 1}] ${r.title || "(无标题)"}`];
        if (r.url) lines.push(`URL: ${r.url}`);
        if (r.content) lines.push(`摘要: ${r.content}`);
        if (r.publishedDate) lines.push(`日期: ${r.publishedDate}`);
        if (r.engine) lines.push(`引擎: ${Array.isArray(r.engine) ? r.engine.join(",") : r.engine}`);
        return lines.join("\n");
      })
      .join("\n\n");
  } finally {
    clearTimeout(timer);
  }
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
      serverInfo: { name: "searxng-server", version: "1.0.0" },
    });
  } else if (m === "notifications/initialized" || m === "notifications/tools/list_changed") {
    // notifications carry no id; do not respond
  } else if (m === "tools/list") {
    respond(msg.id, { tools });
  } else if (m === "resources/list") {
    respond(msg.id, { resourceTemplates: [] });
  } else if (m === "resources/templates/list") {
    respond(msg.id, { resourceTemplates: [] });
  } else if (m === "prompts/list") {
    respond(msg.id, { prompts: [] });
  } else if (m === "tools/call") {
    const name = msg.params?.name;
    const args = msg.params?.arguments ?? {};
    if (name !== "search") {
      respondError(msg.id, `unknown tool: ${name}`);
      return;
    }
    searxngSearch(args)
      .then((text) => respond(msg.id, { content: [{ type: "text", text }] }))
      .catch((e) => respondError(msg.id, e instanceof Error ? e.message : String(e)));
  } else {
    respondError(msg.id, `unhandled method: ${m}`);
  }
});

process.on("SIGTERM", () => process.exit(0));
