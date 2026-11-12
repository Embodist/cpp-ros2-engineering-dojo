#!/usr/bin/env python3
"""Small local CLI for the independent C++/ROS2 dojo. Python stdlib only."""
from __future__ import annotations
import argparse
import datetime
import json
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = json.loads((ROOT / "stage1/manifest.json").read_text(encoding="utf-8"))
INDEX = {x["id"]: x for x in MANIFEST}
STATE = ROOT / ".dojo_progress.json"

def load_state():
    if STATE.exists():
        try:
            data = json.loads(STATE.read_text(encoding="utf-8"))
            if isinstance(data,dict): return data
        except (ValueError, OSError): pass
    return {}

def save_state(data):
    STATE.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')

def get(item):
    key=item.strip().split('-')[0]
    key=key.upper() if key[:1].lower()=='e' else key.zfill(2)
    if key not in INDEX: raise SystemExit(f"未知题目 ID: {item}；执行 python3 tools/train.py list")
    return INDEX[key]

def binary_dir(sanitize): return ROOT / {'none':'build','asan':'build-asan','tsan':'build-tsan'}[sanitize]

def execute(args,timeout=240):
    print('+',' '.join(str(a) for a in args), flush=True)
    try: return subprocess.run([str(a) for a in args],cwd=ROOT, timeout=timeout,check=False).returncode
    except subprocess.TimeoutExpired: return 124
    except FileNotFoundError as exc:
        print(f'程序缺失: {exc}',file=sys.stderr);return 127

def configure(sanitize):
    if not shutil.which('cmake'):
        print('缺少 cmake，请先安装',file=sys.stderr);return 127
    path=binary_dir(sanitize)
    flags=['-DCMAKE_BUILD_TYPE=Debug']
    if sanitize=='asan':flags+=['-DDOJO_SANITIZE_ADDRESS=ON']
    if sanitize=='tsan':flags+=['-DDOJO_SANITIZE_THREAD=ON']
    return execute(['cmake','-S',ROOT,'-B',path,*flags])

def test_one(item,sanitize):
    if item['kind']!='scaffold':
        print('开放题没有预置自动测试；请创建 labs/ 下的代码、测试和 evidence.md。')
        return 2
    if configure(sanitize):return 2
    path=binary_dir(sanitize)
    result=execute(['cmake','--build',path,'--target',item['target'],'--parallel','2'])
    if result: return result
    result=execute([path/'stage1'/item['target']],timeout=18)
    print('PASS：可以记录笔记并执行 done' if result==0 else 'FAIL：骨架默认失败；请保留测试，修改实现后重试')
    return result

def create_lab(item):
    if item['kind']!='open':
        print('核心题直接修改 stage1/include/dojo 的骨架。');return
    p=ROOT/'stage1/labs'/item['id'];p.mkdir(parents=True,exist_ok=True)
    docs={
      'main.cpp':'#include <iostream>\nint main() {\n    // TODO: first design your testable experiment, not the solution.\n    std::cout << "TODO: exercise '+item['id']+'" << std::endl;\n    return 0;\n}\n',
      'README.md':'# '+item['id']+' '+item['title']+'\n\n## 接口与不变量\nTODO\n\n## 失败条件和边界\nTODO\n\n## 测试命令\n`g++ -std=c++20 -Wall -Wextra -Wpedantic main.cpp -o run && ./run`\n',
      'evidence.md':'# 实验记录\n\n输入：TODO\n工具/命令：TODO\n预期：TODO\n实际：TODO\n未覆盖风险：TODO\n'}
    for name,body in docs.items():
        f=p/name
        if not f.exists(): f.write_text(body,encoding='utf-8')
    print('Created/preserved',p.relative_to(ROOT))

def main():
    parser=argparse.ArgumentParser(description='C++ -> ROS2 executable practice and progress tracker')
    sub=parser.add_subparsers(dest='command',required=True)
    sub.add_parser('list',help='list exercises')
    sub.add_parser('status',help='local progress')
    sub.add_parser('report',help='Markdown progress report')
    p=sub.add_parser('show',help='print task card');p.add_argument('id')
    p=sub.add_parser('lab',help='create open-task lab directory');p.add_argument('id')
    p=sub.add_parser('build',help='configure + build all scaffold tasks');p.add_argument('--sanitize',choices=['none','asan','tsan'],default='none')
    p=sub.add_parser('test',help='compile + run a scaffold test');p.add_argument('id');p.add_argument('--sanitize',choices=['none','asan','tsan'],default='none')
    p=sub.add_parser('start',help='mark in progress');p.add_argument('id')
    p=sub.add_parser('done',help='mark completed; scaffold requires passing tests');p.add_argument('id');p.add_argument('--manual',action='store_true',help='for open tasks only: requires written evidence')
    args=parser.parse_args()
    if args.command=='list':
        for i in MANIFEST: print(f"{i['id']}  {'[C++]' if i['kind']=='scaffold' else '[lab]'}  {i['title']}")
        return 0
    if args.command=='show':print((ROOT/get(args.id)['path']).read_text(encoding='utf-8'));return 0
    if args.command=='lab':create_lab(get(args.id));return 0
    if args.command=='build':
        if configure(args.sanitize):return 2
        return execute(['cmake','--build',binary_dir(args.sanitize),'--parallel','2'])
    if args.command=='test':return test_one(get(args.id),args.sanitize)
    state=load_state()
    if args.command in ('start','done'):
        i=get(args.id)
        if args.command=='done':
            if i['kind']=='scaffold':
                if test_one(i,'none')!=0:
                    print('测试尚未通过，不标记完成。',file=sys.stderr);return 1
            else:
                evidence=ROOT/'stage1/labs'/i['id']/'evidence.md'
                if not args.manual or not evidence.exists() or 'TODO' in evidence.read_text(encoding='utf-8'):
                    print('开放题先执行 lab ID、写 evidence.md，然后 done ID --manual（自我认证）。',file=sys.stderr);return 1
            status='PASS'
        else: status='WIP'
        state[i['id']]={'status':status,'updated':datetime.date.today().isoformat()}
        save_state(state)
        print(i['id'],status)
        return 0
    completed=sum(1 for s in state.values() if s.get('status')=='PASS')
    if args.command=='status':
        total=len(MANIFEST)
        core=sum(1 for i in MANIFEST if i['id'].isdigit())
        ext=total-core
        scaffold=sum(1 for i in MANIFEST if i['kind']=='scaffold')
        open_tasks=sum(1 for i in MANIFEST if i['kind']=='open')
        print(f'已完成 {completed}/{total}（核心 {core} + 扩展 {ext}；{scaffold} 个自动测试题 + {open_tasks} 个手动证据题）')
        for i in MANIFEST:
            v=state.get(i['id'],{}).get('status','TODO')
            print(f"{i['id']} {v:<4} {i['title']}")
        return 0
    if args.command=='report':
        total=len(MANIFEST)
        lines=['# C++ Dojo 本地学习报告','','已通过：'+str(completed)+'/'+str(total),'','|ID|状态|题目|','|---|---|---|']
        for i in MANIFEST:lines.append('|'+i['id']+'|'+state.get(i['id'],{}).get('status','TODO')+'|'+i['title']+'|')
        (ROOT/'training_report.md').write_text('\n'.join(lines)+'\n',encoding='utf-8')
        print('Created training_report.md')
        return 0
    return 0
if __name__=='__main__':sys.exit(main())
