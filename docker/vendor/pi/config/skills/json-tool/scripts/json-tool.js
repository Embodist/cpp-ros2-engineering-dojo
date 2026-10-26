#!/usr/bin/env node
// JSON helper for the json-tool skill: validate / pretty / get-by-path
const action = process.argv[2];
const raw = process.argv[3];

function parse() {
  try {
    return JSON.parse(raw);
  } catch (e) {
    return { __error: String(e.message) };
  }
}

function getByPath(obj, path) {
  const parts = path.replace(/\[(\d+)\]/g, ".$1").split(".").filter(Boolean);
  let cur = obj;
  for (const p of parts) {
    if (cur == null || typeof cur !== "object" || !(p in cur)) return "NOT_FOUND";
    cur = cur[p];
  }
  return JSON.stringify(cur);
}

const parsed = parse();
if (action === "validate") {
  process.stdout.write(parsed.__error ? `invalid: ${parsed.__error}` : "valid");
} else if (action === "pretty") {
  process.stdout.write(parsed.__error ? `invalid: ${parsed.__error}` : JSON.stringify(parsed, null, 2));
} else if (action === "get") {
  const path = process.argv[4];
  if (parsed.__error) process.stdout.write(`invalid: ${parsed.__error}`);
  else process.stdout.write(getByPath(parsed, path));
} else {
  process.stdout.write(`unknown action: ${action}`);
}
process.stdout.write("\n");
