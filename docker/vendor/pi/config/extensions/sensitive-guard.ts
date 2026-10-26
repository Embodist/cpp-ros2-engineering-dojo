import { isToolCallEventType } from "@earendil-works/pi-coding-agent";
import type { ExtensionAPI } from "@earendil-works/pi-coding-agent";
import os from "node:os";
import path from "node:path";

// ============================================================
// 敏感目录守卫(YOLO 例外)
// 默认最高权限自动执行;仅拦截对"敏感其他目录/文件"的读写。
// 想临时关掉本守卫:环境变量 PI_GUARD_OFF=1
// ============================================================

const HOME = os.homedir();

const SENSITIVE: string[] = [
  path.join(HOME, ".ssh"), // SSH 私钥
  path.join(HOME, ".aws"), // AWS 凭证
  path.join(HOME, ".config", "gcloud"), // GCP 凭证
  path.join(HOME, ".kube"), // K8s 凭证
  path.join(HOME, ".gnupg"), // GPG 密钥
  path.join(HOME, ".netrc"),
  path.join(HOME, ".git-credentials"),
  path.join(HOME, ".npmrc"), // 含 registry token
  path.join(HOME, ".claude"), // Claude Code 配置(含 token)
  path.join(HOME, ".pi", "agent", "auth.json"), // Pi 自身 API key
  "/etc/shadow",
  "/etc/gshadow",
  "/etc/sudoers",
  "/etc/sudoers.d",
  "/etc/passwd",
];

// 项目里要连 .env 一起拦,取消下面注释(注意给绝对路径):
// SENSITIVE.push(path.join("/home/user", ".env"));

function expand(target: string): string {
  let t = target.trim();
  if (t === "~") t = HOME;
  else if (t.startsWith("~/")) t = path.join(HOME, t.slice(2));
  t = t.replace(/\$\{HOME\}/g, HOME).replace(/\$HOME(?![A-Za-z0-9_])/g, HOME);
  return path.resolve(t);
}

function isSensitive(target: string | undefined): string | null {
  if (!target) return null;
  const resolved = expand(target);
  for (const s of SENSITIVE) {
    const rs = path.resolve(s);
    if (resolved === rs || resolved.startsWith(rs + path.sep)) return rs;
  }
  return null;
}

// bash 命令里的敏感路径变体(~ / $HOME / ${HOME} / 绝对路径)
const BASH_VARIANTS: string[] = (() => {
  const out: string[] = [];
  for (const s of SENSITIVE) {
    const abs = path.resolve(s);
    out.push(abs);
    if (abs.startsWith(HOME + path.sep)) {
      out.push("~" + abs.slice(HOME.length));
      out.push("$HOME" + abs.slice(HOME.length));
      out.push("${HOME}" + abs.slice(HOME.length));
    }
  }
  return out;
})();

function commandHitsSensitive(cmd: string): string | null {
  for (const v of BASH_VARIANTS) {
    if (cmd.includes(v)) return v;
  }
  return null;
}

export default function (pi: ExtensionAPI) {
  if (process.env.PI_GUARD_OFF) return;

  pi.on("tool_call", (event) => {
    // 直接路径类工具
    if (
      isToolCallEventType("read", event) ||
      isToolCallEventType("write", event) ||
      isToolCallEventType("edit", event)
    ) {
      const hit = isSensitive(event.input.path);
      if (hit) return { block: true, reason: `已拦截敏感路径(默认 YOLO 禁读): ${hit}` };
    }
    if (isToolCallEventType("ls", event) && event.input.path) {
      const hit = isSensitive(event.input.path);
      if (hit) return { block: true, reason: `已拦截敏感路径: ${hit}` };
    }
    if (isToolCallEventType("grep", event) && event.input.path) {
      const hit = isSensitive(event.input.path);
      if (hit) return { block: true, reason: `已拦截敏感路径: ${hit}` };
    }
    if (isToolCallEventType("find", event) && event.input.path) {
      const hit = isSensitive(event.input.path);
      if (hit) return { block: true, reason: `已拦截敏感路径: ${hit}` };
    }
    // bash 命令扫描
    if (isToolCallEventType("bash", event)) {
      const hit = commandHitsSensitive(event.input.command ?? "");
      if (hit) return { block: true, reason: `已拦截命令中的敏感路径: ${hit}` };
    }
  });
}
