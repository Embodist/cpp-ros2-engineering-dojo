#!/usr/bin/env node
// pi-autorun.js — 让 Pi 自主执行长程任务,最多跑 N 轮(自动循环 / 自动优化)。
//
// 原理:以 RPC 模式起一个 pi 子进程,每轮发送 prompt(带当前进度 + 轮次预算),
//       等 agent_settled 后再发下一轮;若回复中出现 [TASK_COMPLETE] 提前结束。
// 用途:持续重构、批量任务、多步分析……一直执行到任务完成或达到轮数上限。
//
// 用法:
//   node pi-autorun.js "任务描述" [最大轮数] [--out 日志文件]
//   env: PI_PROVIDER / PI_MODEL 覆盖默认 meraai / deepseek-v4-flash
const { spawn } = require("node:child_process");
const readline = require("node:readline");
const fs = require("node:fs");

const task = process.argv[2];
const maxRounds = parseInt(process.argv.find((a) => /^\d+$/.test(a)) || "5", 10);
const outFileIdx = process.argv.indexOf("--out");
const outFile = outFileIdx > -1 ? process.argv[outFileIdx + 1] : null;

if (!task) {
  console.error('用法: node pi-autorun.js "任务描述" [最大轮数] [--out 日志]');
  process.exit(1);
}

const provider = process.env.PI_PROVIDER || "meraai";
const model = process.env.PI_MODEL || "deepseek-v4-flash";
const cwd = process.env.PI_CWD || process.cwd();

const pi = spawn("pi", ["--mode", "rpc", "--provider", provider, "--model", model], {
  cwd,
  stdio: ["pipe", "pipe", "inherit"],
});
const rl = readline.createInterface({ input: pi.stdout });

let round = 1;
let currentText = "";
let finished = false;
const roundLogs = [];

function sendPrompt(message) {
  pi.stdin.write(JSON.stringify({ type: "prompt", message, streamingBehavior: "followUp" }) + "\n");
}

function finish(code) {
  if (finished) return;
  finished = true;
  const report = ["=== pi-autorun 汇总 ===",
    `任务: ${task}`,
    `轮数: ${roundLogs.length}/${maxRounds}`,
    ...roundLogs.map((l) => `\n[第 ${l.round} 轮${l.completed ? " ✓任务完成" : ""}]\n${l.text.slice(0, 800)}`)].join("\n");
  console.log(report);
  if (outFile) {
    fs.writeFileSync(outFile, report + "\n");
    console.log(`\n日志已写入: ${outFile}`);
  }
  try { pi.stdin.end(); } catch {}
  setTimeout(() => process.exit(code), 200);
}

pi.on("exit", (code) => {
  if (!finished) {
    console.error(`\n[pi 子进程退出 code=${code},共 ${round - 1} 轮]`);
    finish(code === 0 ? 0 : 1);
  }
});

rl.on("line", (line) => {
  let ev;
  try { ev = JSON.parse(line); } catch { return; }

  if (ev.type === "message_update" && ev.assistantMessageEvent?.type === "text_delta") {
    currentText += ev.assistantMessageEvent.delta;
  }

  if (ev.type === "agent_settled") {
    const roundText = currentText;
    currentText = "";
    // [TASK_COMPLETE] 必须单独成行才算完成(避免模型在解说里提到它而误判)
    const completed = roundText.split("\n").some((l) => l.trim() === "[TASK_COMPLETE]");
    roundLogs.push({ round, text: roundText, completed });
    console.log(`\n── 第 ${round}/${maxRounds} 轮结束${completed ? "(任务完成)" : ""} ──`);
    console.log(roundText.slice(0, 1200));

    if (completed || round >= maxRounds) {
      finish(completed ? 0 : 0);
      return;
    }
    round++;
    sendPrompt(
      `继续执行原任务。上一轮(第 ${round - 1} 轮)输出的末尾:\n${roundText.slice(-1200)}\n\n` +
      `现在是第 ${round}/${maxRounds} 轮。请继续推进,不要重复已做过的部分。` +
      `当且仅当任务全部完成时,在回复最后单独一行只写 [TASK_COMPLETE](不要在解说里提到这串字符)。`
    );
  }
});

sendPrompt(
  `执行以下任务。你不会一次就被打断;最多有 ${maxRounds} 轮,每轮结束我会把上一轮输出尾部喂回给你继续。` +
  `每轮请实际推进工作(调用工具、写文件等),并简要说明本轮做了什么、还剩什么。\n\n任务: ${task}\n\n` +
  `这是第 1/${maxRounds} 轮。当且仅当任务全部完成时,在回复最后单独一行只写 [TASK_COMPLETE](不要在解说里提到这串字符)。`
);

const guard = setTimeout(() => { console.error("\n[超时终止]"); finish(1); }, 30 * 60 * 1000);
guard.unref();
