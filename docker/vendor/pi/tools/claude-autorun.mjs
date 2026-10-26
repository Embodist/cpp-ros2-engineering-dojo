#!/usr/bin/env node
// claude-autorun.mjs — 让 Claude Code 自主执行长程任务,最多跑 N 轮(自动循环/自动优化)。
//
// 原理:用 Claude Agent SDK 的 query() 驱动 claude CLI(继承当前环境与 .claude/settings.json 配置),
//       每轮发 prompt(带当前进度 + 轮次预算),等 result 消息结束;
//       若回复中出现 [TASK_COMPLETE](必须单独成行)则提前结束。
// 与 pi-autorun.js 是同一套 N 轮循环逻辑,只是后端从 pi 换成 Claude Code。
//
// 依赖(在 tools/ 下执行一次):
//   npm install        # 装 @anthropic-ai/claude-agent-sdk
//
// 用法:
//   node claude-autorun.mjs "任务描述" [最大轮数] [--out 日志文件]
//
// env 覆盖:
//   CLAUDE_CWD            工作目录(默认脚本所在项目的根目录,保证读到 .claude/settings.json)
//   CLAUDE_MAX_TURNS      每轮内工具循环上限(默认 30)
//   CLAUDE_PERMISSION     权限模式:bypassPermissions(yolo,默认)| acceptEdits | default | plan | dontAsk
//   CLAUDE_ALLOWED_TOOLS  逗号分隔的允许工具,如 "Read,Grep,Glob,Bash";不设=全放行
//   CLAUDE_DISALLOWED_TOOLS 逗号分隔的禁用工具(bypass 模式下也生效),如 "Bash:rm -rf"
//   CLAUDE_MODEL          模型(默认沿用环境配置,如 deepseek-v4-flash)
import { query } from "@anthropic-ai/claude-agent-sdk";
import { fileURLToPath } from "node:url";
import { dirname, join } from "node:path";
import fs from "node:fs";

const task = process.argv[2];
const maxRounds = parseInt(process.argv.find((a) => /^\d+$/.test(a)) || "5", 10);
const outFileIdx = process.argv.indexOf("--out");
const outFile = outFileIdx > -1 ? process.argv[outFileIdx + 1] : null;

if (!task) {
  console.error('用法: node claude-autorun.mjs "任务描述" [最大轮数] [--out 日志]');
  process.exit(1);
}

const PROJECT_ROOT = dirname(dirname(fileURLToPath(import.meta.url)));
const cwd = process.env.CLAUDE_CWD || PROJECT_ROOT;
const maxTurns = Number(process.env.CLAUDE_MAX_TURNS || 30);
const permissionMode = process.env.CLAUDE_PERMISSION || "bypassPermissions";
const allowedTools = process.env.CLAUDE_ALLOWED_TOOLS
  ? process.env.CLAUDE_ALLOWED_TOOLS.split(",").map((s) => s.trim()).filter(Boolean)
  : undefined;
const disallowedTools = process.env.CLAUDE_DISALLOWED_TOOLS
  ? process.env.CLAUDE_DISALLOWED_TOOLS.split(",").map((s) => s.trim()).filter(Boolean)
  : undefined;
const model = process.env.CLAUDE_MODEL || undefined;

let round = 1;
let sessionId;
let finished = false;
const roundLogs = [];

function collectText(msg) {
  const blocks = (msg.message && msg.message.content) || [];
  return blocks.filter((b) => b.type === "text").map((b) => b.text).join("");
}

function buildOptions() {
  const o = { cwd, maxTurns, permissionMode };
  if (allowedTools) o.allowedTools = allowedTools;
  if (disallowedTools) o.disallowedTools = disallowedTools;
  if (model) o.model = model;
  if (sessionId) o.resume = sessionId; // 第二轮起续同一会话,上下文连续
  return o;
}

function finish(code) {
  if (finished) return;
  finished = true;
  const report = [
    "=== claude-autorun 汇总 ===",
    `任务: ${task}`,
    `轮数: ${roundLogs.length}/${maxRounds}`,
    ...roundLogs.map((l) => `\n[第 ${l.round} 轮${l.completed ? " ✓任务完成" : ""}]\n${l.text.slice(0, 800)}`),
  ].join("\n");
  console.log(report);
  if (outFile) {
    fs.writeFileSync(outFile, report + "\n");
    console.log(`\n日志已写入: ${outFile}`);
  }
  process.exit(code);
}

async function runRound(roundPrompt) {
  let text = "";
  const gen = query({ prompt: roundPrompt, options: buildOptions() });
  for await (const msg of gen) {
    if (msg.type === "system" && msg.subtype === "init") {
      sessionId = msg.session_id;
      if (round === 1) console.log(`[会话] ${sessionId} | 模型 ${msg.model} | 权限 ${msg.permissionMode}`);
    } else if (msg.type === "assistant") {
      text += collectText(msg);
    } else if (msg.type === "result") {
      if (msg.subtype && msg.subtype !== "success") console.error(`[result] ${msg.subtype}`);
    }
  }
  // [TASK_COMPLETE] 必须单独成行才算完成(避免模型在解说里提到而误判)
  const completed = text.split("\n").some((l) => l.trim() === "[TASK_COMPLETE]");
  return { text, completed };
}

const guard = setTimeout(() => { console.error("\n[超时终止]"); finish(1); }, 30 * 60 * 1000);
guard.unref();

(async () => {
  for (round = 1; round <= maxRounds; round++) {
    let prompt;
    if (round === 1) {
      prompt =
        `执行以下任务。你不会一次就被打断;最多有 ${maxRounds} 轮,每轮结束我会把上一轮输出尾部喂回给你继续。` +
        `每轮请实际推进工作(调用工具、写文件等),并简要说明本轮做了什么、还剩什么。\n\n任务: ${task}\n\n` +
        `这是第 1/${maxRounds} 轮。当且仅当任务全部完成时,在回复最后单独一行只写 [TASK_COMPLETE](不要在解说里提到这串字符)。`;
    } else {
      const prev = roundLogs[roundLogs.length - 1];
      prompt =
        `继续执行原任务。上一轮(第 ${round - 1} 轮)输出的末尾:\n${prev.text.slice(-1200)}\n\n` +
        `现在是第 ${round}/${maxRounds} 轮。请继续推进,不要重复已做过的部分。` +
        `当且仅当任务全部完成时,在回复最后单独一行只写 [TASK_COMPLETE](不要在解说里提到这串字符)。`;
    }
    console.log(`\n── 第 ${round}/${maxRounds} 轮开始 ──`);
    const { text, completed } = await runRound(prompt);
    roundLogs.push({ round, text, completed });
    console.log(`── 第 ${round}/${maxRounds} 轮结束${completed ? "(任务完成)" : ""} ──`);
    console.log(text.slice(0, 1200));
    if (completed) { finish(0); return; }
  }
  finish(0);
})().catch((e) => {
  console.error("\n[错误]", e);
  finish(1);
});
