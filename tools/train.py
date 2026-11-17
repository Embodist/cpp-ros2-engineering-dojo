#!/usr/bin/env python3
"""Small local CLI for the independent C++/ROS2 dojo. Python stdlib only."""
from __future__ import annotations
import argparse
import datetime
import json
import platform
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = json.loads((ROOT / "stage1/manifest.json").read_text(encoding="utf-8"))
INDEX = {x["id"]: x for x in MANIFEST}
STATE = ROOT / ".dojo_progress.json"
PROJECT_FILE = ROOT / "stage1/projects/manifest.json"
PROJECT_STATE = ROOT / ".dojo_projects.json"

def load_projects():
    if not PROJECT_FILE.exists():
        return [], []
    try:
        data = json.loads(PROJECT_FILE.read_text(encoding="utf-8"))
    except (ValueError, OSError):
        return [], []
    return data.get("projects", []), data.get("minimal", [])

PROJECTS, MINIMAL = load_projects()
PROJECT_INDEX = {p["id"]: p for p in PROJECTS}

def load_state():
    if STATE.exists():
        try:
            data = json.loads(STATE.read_text(encoding="utf-8"))
            if isinstance(data,dict): return data
        except (ValueError, OSError): pass
    return {}

def save_state(data):
    STATE.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')

def load_project_state():
    if PROJECT_STATE.exists():
        try:
            data = json.loads(PROJECT_STATE.read_text(encoding="utf-8"))
            if isinstance(data, dict):
                return data
        except (ValueError, OSError):
            pass
    return {}

def save_project_state(data):
    PROJECT_STATE.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')

def get(item):
    key=item.strip().split('-')[0]
    key=key.upper() if key[:1].lower()=='e' else key.zfill(2)
    if key not in INDEX: raise SystemExit(f"未知题目 ID: {item}；执行 python3 tools/train.py list")
    return INDEX[key]

def binary_dir(sanitize, gtest=False):
    prefix = 'build-gtest' if gtest else 'build'
    if sanitize == 'none':
        return ROOT / prefix
    return ROOT / f'{prefix}-{sanitize}'

def execute(args,timeout=240):
    print('+',' '.join(str(a) for a in args), flush=True)
    try: return subprocess.run([str(a) for a in args],cwd=ROOT, timeout=timeout,check=False).returncode
    except subprocess.TimeoutExpired: return 124
    except FileNotFoundError as exc:
        print(f'程序缺失: {exc}',file=sys.stderr);return 127

def configure(sanitize, gtest=False, gtest_source=''):
    if not shutil.which('cmake'):
        print('缺少 cmake，请先安装',file=sys.stderr);return 127
    path=binary_dir(sanitize, gtest)
    flags=['-DCMAKE_BUILD_TYPE=Debug']
    if sanitize=='asan':flags+=['-DDOJO_SANITIZE_ADDRESS=ON']
    if sanitize=='tsan':flags+=['-DDOJO_SANITIZE_THREAD=ON']
    if gtest:
        flags+=['-DDOJO_USE_GTEST=ON']
        if gtest_source:
            flags+=[f'-DDOJO_GTEST_SOURCE_DIR={gtest_source}']
    result=execute(['cmake','-S',ROOT,'-B',path,*flags])
    if result and gtest:
        print('提示：GoogleTest 默认用 FetchContent 联网拉取；离线时用 --gtest-source 指向本地 googletest 源码。',file=sys.stderr)
    return result

def gtest_target(item):
    slug=item['slug'].split('-',1)[1]
    return f"dojo_gtest_{item['id']}_{slug.replace('-','_')}"

def test_one(item,sanitize,gtest=False,gtest_source=''):
    if item['kind']!='scaffold':
        print('开放题没有预置自动测试；请创建 labs/ 下的代码、测试和 evidence.md。')
        return 2
    if configure(sanitize,gtest,gtest_source):return 2
    path=binary_dir(sanitize,gtest)
    target=gtest_target(item) if gtest else item['target']
    result=execute(['cmake','--build',path,'--target',target,'--parallel','2'])
    if result: return result
    result=execute([path/'stage1'/target],timeout=30 if gtest else 18)
    if gtest:
        print('GTEST PASS：也可以直接 ctest -L gtest' if result==0 else 'GTEST FAIL')
    else:
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

def get_project(item):
    key=item.strip().lower()
    if key not in PROJECT_INDEX: raise SystemExit(f"未知项目 ID: {item}；执行 python3 tools/train.py projects")
    return PROJECT_INDEX[key]

def task_status(state,item): return state.get(item,{}).get('status','TODO')

def project_evidence_path(project): return ROOT/'notes/projects'/f"{project['id']}.md"

def missing_for_project(project,task_state,project_state):
    missing_tasks=[i for i in project.get('prereq',[]) if task_status(task_state,i)!='PASS']
    missing_projects=[i for i in project.get('depends',[]) if project_state.get(i,{}).get('status')!='PASS']
    return missing_tasks,missing_projects

def project_readme(project):
    pid=project['id']
    prereq=', '.join(project.get('prereq',[])) or '-'
    depends=', '.join(project.get('depends',[])) or '-'
    recommends=', '.join(project.get('recommends',[])) or '-'
    stage2=', '.join(project.get('stage2',[])) or '-'
    milestones='\n'.join(f"- [ ] {m}" for m in project.get('milestones',[])) or '-'
    return f"""# {pid} {project['title']}

- 阶段：{project.get('phase','-')}
- 优先级：{project.get('priority','-')}
- 代码位置：`{project.get('repo','-')}`
- 参考实现：`{project.get('reference','-')}`
- 项目卡：`{project.get('repo','-')}/README.md`
- 起跑计划：`stage1/projects/{pid}/PLAN.md`
- 功能要求：`stage1/projects/docs/功能要求清单.md`
- 先修 Stage 1：{prereq}
- 项目依赖：{depends}
- 推荐：{recommends}
- 对应 Stage 2：{stage2}

## 里程碑
{milestones}

## 能力连续
- 承接：{project.get('carry_in','-')}
- 输出：{project.get('carry_out','-')}

## 工作流
1. 先做先修题：`python3 tools/train.py projects` 会显示缺口。
2. 在 `{project.get('repo','stage1/projects/pXX')}` 写项目；AI 只做 review，不代写。
3. 先跑参考测试（如已移植）：`python3 tools/train.py project {pid} --test`；再跑自己的测试、ASan/UBSan；并发题加 TSan；性能题记录环境。
4. 把命令、原始输出、反例写入 `notes/projects/{pid}.md`。
5. 完成：`python3 tools/train.py project {pid} --done`。

## 完成标准
- 项目 README 与功能要求清单的验收项逐条有证据。
- `notes/projects/{pid}.md` 没有 TODO。
"""

def project_checklist(project):
    pid=project['id']
    prereq=', '.join(project.get('prereq',[])) or '-'
    milestones='\n'.join(f"- [ ] {m}" for m in project.get('milestones',[])) or '-'
    return f"""# {pid} 检查表

## 先修
- [ ] 先修题全部 PASS：{prereq}

## 项目里程碑
{milestones}

## 工具
- [ ] `python3 tools/train.py project {pid} --test` 参考测试通过（如已移植）
- [ ] 编译零警告
- [ ] ASan/UBSan 干净
- [ ] 并发题 TSan 干净（如适用）
- [ ] 性能/分配证据（如适用）

## 证据
- [ ] `notes/projects/{pid}.md` 每条核查点有命令和原始输出
- [ ] 项目 README/功能要求清单的验收项逐条打勾
- [ ] 至少 2 个反例/失败注入
- [ ] AI review 记录在 `notes/projects/{pid}.md`
- [ ] 24–72 小时脱稿复写

## 完成
- [ ] `python3 tools/train.py project {pid} --done`
"""

def project_evidence(project):
    pid=project['id']
    prereq=', '.join(project.get('prereq',[])) or '-'
    stage2=', '.join(project.get('stage2',[])) or '-'
    return f"""# {pid} {project['title']}

## 项目信息
- 代码位置：`{project.get('repo','-')}`
- 参考实现：`{project.get('reference','-')}`
- 功能要求：`stage1/projects/docs/功能要求清单.md`
- 先修题：{prereq}
- 对应 Stage 2：{stage2}

## 范围与不变量
TODO

## 证据
| 核查点 | 测试名/命令 | 实际输出 | 环境 |
|---|---|---|---|
|  |  |  |  |

## 工具
- ASan/UBSan：TODO
- TSan：TODO
- 性能/基准：TODO
- 无分配验证：TODO

## 反例与失败注入
TODO

## 评审与复盘
TODO

## 24–72 小时复写
TODO
"""

def configure_projects(sanitize='none', gtest_source=''):
    path=ROOT/('build-projects' if sanitize=='none' else f'build-projects-{sanitize}')
    flags=['-DCMAKE_BUILD_TYPE=Debug','-DDOJO_USE_GTEST=ON','-DDOJO_BUILD_PROJECTS=ON']
    if sanitize=='asan': flags+=['-DDOJO_SANITIZE_ADDRESS=ON']
    if sanitize=='tsan': flags+=['-DDOJO_SANITIZE_THREAD=ON']
    if gtest_source: flags+=[f'-DDOJO_GTEST_SOURCE_DIR={gtest_source}']
    result=execute(['cmake','-S',ROOT,'-B',path,*flags])
    if result:
        print('提示：项目测试需要 GoogleTest；联网 FetchContent，或用 --gtest-source 指向本地源码。',file=sys.stderr)
    return result

def project_test(project,sanitize='none',gtest_source=''):
    target=project.get('test_target')
    if not target:
        print(f"{project['id']} 还没有移植参考实现/测试；先按功能要求自己写。",file=sys.stderr);return 2
    if configure_projects(sanitize,gtest_source):return 2
    path=ROOT/('build-projects' if sanitize=='none' else f'build-projects-{sanitize}')
    result=execute(['cmake','--build',path,'--target',target,'--parallel','2'])
    if result: return result
    cmd=[path/'stage1/projects'/target]
    if sanitize=='tsan' and sys.platform.startswith('linux'):
        cmd=['setarch',platform.machine(),'-R',*cmd]
    result=execute(cmd,timeout=120)
    print(f"{project['id']} PROJECT PASS" if result==0 else f"{project['id']} PROJECT FAIL")
    return result

def project_plan(project):
    pid=project['id']
    first=project.get('milestones',['第一个里程碑'])[0]
    prereq=', '.join(project.get('prereq',[])) or '-'
    return f"""# {pid} 3 次起跑计划

> 每次 60–90 分钟。先做先修题，再写项目；AI 只做 review，不代写。

## Session 1：先修 + 接口草案
- 先修题：{prereq}
- 只写接口声明和注释：参数传递方式、所有权、失败路径。
- 证据：`notes/projects/{pid}.md` 的“范围与不变量”。

## Session 2：核心实现 + 测试
- 第一个里程碑：{first}
- 先写测试，再写实现；覆盖正常路径和至少 2 个反例。
- 证据：测试命令 + 原始输出。

## Session 3：加固 + 复盘
- ASan/UBSan；并发题 TSan；性能题记录环境、规模、重复次数。
- AI review：只问“违反了哪条不变量/给我最小复现”，不要索取完整实现。
- 24–72 小时后从空文件复写核心机制。
- 完成：`python3 tools/train.py project {pid} --done`。
"""

def show_project(project):
    pstate=load_project_state()
    status=pstate.get(project['id'],{}).get('status','TODO')
    missing_tasks,missing_projects=missing_for_project(project,load_state(),pstate)
    print(f"# {project['id']} {project['title']}")
    print(f"阶段：{project.get('phase','-')}；优先级：{project.get('priority','-')}；状态：{status}")
    print(f"代码：{project.get('repo','-')}")
    print(f"参考实现：{project.get('reference','-')}")
    print(f"先修 Stage 1：{', '.join(project.get('prereq',[])) or '-'}")
    print(f"项目依赖：{', '.join(project.get('depends',[])) or '-'}")
    print(f"推荐：{', '.join(project.get('recommends',[])) or '-'}")
    print(f"Stage 2：{', '.join(project.get('stage2',[])) or '-'}")
    print(f"里程碑：{'; '.join(project.get('milestones',[])) or '-'}")
    print(f"能力承接：{project.get('carry_in','-')}")
    print(f"能力输出：{project.get('carry_out','-')}")
    if missing_tasks: print(f"先修缺口：{', '.join(missing_tasks)}")
    if missing_projects: print(f"项目依赖缺口：{', '.join(missing_projects)}")
    print(f"项目卡：{(ROOT/'stage1/projects'/project['id']/'README.md').relative_to(ROOT)}")
    print(f"起跑计划：{(ROOT/'stage1/projects'/project['id']/'PLAN.md').relative_to(ROOT)}")
    print(f"证据：{project_evidence_path(project).relative_to(ROOT)}")
    if status == 'WIP':
        print(f"继续：按 PLAN.md 做，完成后 python3 tools/train.py project {project['id']} --done")
    else:
        print(f"启动：python3 tools/train.py project {project['id']} --start")
    return 0

def start_project(project):
    pid=project['id']
    pdir=ROOT/'stage1/projects'/pid
    pdir.mkdir(parents=True,exist_ok=True)
    for sub in ('include','src','tests'):
        (pdir/sub).mkdir(exist_ok=True)
    (ROOT/'notes/projects').mkdir(parents=True,exist_ok=True)
    readme=pdir/'README.md'; checklist=pdir/'CHECKLIST.md'; plan=pdir/'PLAN.md'; evidence=project_evidence_path(project)
    if not readme.exists(): readme.write_text(project_readme(project),encoding='utf-8')
    if not checklist.exists(): checklist.write_text(project_checklist(project),encoding='utf-8')
    if not plan.exists(): plan.write_text(project_plan(project),encoding='utf-8')
    if not evidence.exists(): evidence.write_text(project_evidence(project),encoding='utf-8')
    state=load_project_state()
    if state.get(pid,{}).get('status')!='PASS':
        state[pid]={'status':'WIP','updated':datetime.date.today().isoformat()}
        save_project_state(state)
    print(f"Created/preserved {readme.relative_to(ROOT)}, {checklist.relative_to(ROOT)}, {plan.relative_to(ROOT)}, {evidence.relative_to(ROOT)}")
    missing_tasks,missing_projects=missing_for_project(project,load_state(),load_project_state())
    if missing_tasks:
        preview=', '.join(missing_tasks[:10])+('...' if len(missing_tasks)>10 else '')
        print(f"先修缺口（不阻止启动，但建议先做）：{preview}",file=sys.stderr)
    if missing_projects: print(f"项目依赖缺口：{', '.join(missing_projects)}",file=sys.stderr)
    print(f"下一步：读 {readme.relative_to(ROOT)}，在 {project.get('repo','stage1/projects/pXX')} 写项目。")
    return 0

def finish_project(project):
    evidence=project_evidence_path(project)
    if not evidence.exists():
        print(f"缺少证据文件：{evidence.relative_to(ROOT)}；先执行 --start 并写证据。",file=sys.stderr);return 1
    if 'TODO' in evidence.read_text(encoding='utf-8'):
        print(f"证据里仍有 TODO：{evidence.relative_to(ROOT)}；补完再 done。",file=sys.stderr);return 1
    state=load_project_state()
    state[project['id']]={'status':'PASS','updated':datetime.date.today().isoformat()}
    save_project_state(state)
    print(project['id'],'PASS')
    return 0

def print_projects(minimal_only=False):
    if not PROJECTS:
        print('没有项目线 manifest。');return 1
    task_state=load_state(); pstate=load_project_state()
    passed=sum(1 for p in PROJECTS if pstate.get(p['id'],{}).get('status')=='PASS')
    min_passed=sum(1 for i in MINIMAL if pstate.get(i,{}).get('status')=='PASS')
    print(f"项目线：{passed}/{len(PROJECTS)}；最小闭环 {min_passed}/{len(MINIMAL)}")
    ids=MINIMAL if minimal_only else [p['id'] for p in PROJECTS]
    for pid in ids:
        p=PROJECT_INDEX[pid]; mt,mp=missing_for_project(p,task_state,pstate)
        note=[]
        if mt: note.append(f"先修缺 {len(mt)}")
        if mp: note.append(f"依赖缺 {len(mp)}")
        note_text='；'.join(note) if note else 'ready'
        print(f"{pid:<4} {pstate.get(pid,{}).get('status','TODO'):<5} {p.get('priority','-'):<5} {p.get('phase','-'):<10} {p['title']}  [{note_text}]")
    for pid in MINIMAL:
        p=PROJECT_INDEX[pid]
        status=pstate.get(pid,{}).get('status','TODO')
        if status=='PASS': continue
        mt,mp=missing_for_project(p,task_state,pstate)
        if status=='WIP':
            if mt:
                preview=', '.join(mt[:6])+('...' if len(mt)>6 else '')
                print(f"下一步：继续 {pid}；先补先修题 {preview}，再按 PLAN.md 做。")
            elif mp:
                print(f"下一步：继续 {pid}；先完成项目依赖 {', '.join(mp)}，再按 PLAN.md 做。")
            else:
                print(f"下一步：继续 {pid}；按 PLAN.md 做，完成后 python3 tools/train.py project {pid} --done。")
            return 0
        if not mt and not mp: print(f"下一步：python3 tools/train.py project {pid} --start")
        elif mt:
            preview=', '.join(mt[:6])+('...' if len(mt)>6 else '')
            print(f"下一步：先修题 {preview}；然后 python3 tools/train.py project {pid} --start")
        else: print(f"下一步：先完成项目依赖 {', '.join(mp)}；然后 python3 tools/train.py project {pid} --start")
        return 0
    print('下一步：最小闭环已完成；可做 p03/p04/p06/p07 或 p13 收官。')
    return 0

def main():
    parser=argparse.ArgumentParser(description='C++ -> ROS2 executable practice and progress tracker')
    sub=parser.add_subparsers(dest='command',required=True)
    sub.add_parser('list',help='list exercises')
    sub.add_parser('status',help='local progress')
    sub.add_parser('report',help='Markdown progress report')
    p=sub.add_parser('show',help='print task card');p.add_argument('id')
    p=sub.add_parser('lab',help='create open-task lab directory');p.add_argument('id')
    p=sub.add_parser('build',help='configure + build all scaffold tasks');p.add_argument('--sanitize',choices=['none','asan','tsan'],default='none');p.add_argument('--gtest',action='store_true',help='build the optional GoogleTest mirror suite');p.add_argument('--gtest-source',default='',help='local googletest source for offline FetchContent')
    p=sub.add_parser('test',help='compile + run a scaffold test');p.add_argument('id');p.add_argument('--sanitize',choices=['none','asan','tsan'],default='none');p.add_argument('--gtest',action='store_true',help='use the GoogleTest mirror target');p.add_argument('--gtest-source',default='',help='local googletest source for offline FetchContent')
    p=sub.add_parser('projects',help='list the optional cpp_testing project track');p.add_argument('--minimal',action='store_true',help='only the minimal closure')
    p=sub.add_parser('project',help='show/start/finish/test a project');p.add_argument('id');p.add_argument('--start',action='store_true',help='create project card/checklist/plan/evidence and mark WIP');p.add_argument('--done',action='store_true',help='mark complete; requires notes/projects/<id>.md without TODO');p.add_argument('--test',action='store_true',help='build/run the ported reference test target');p.add_argument('--sanitize',choices=['none','asan','tsan'],default='none');p.add_argument('--gtest-source',default='',help='local googletest source for offline FetchContent')
    p=sub.add_parser('start',help='mark in progress');p.add_argument('id')
    p=sub.add_parser('done',help='mark completed; scaffold requires passing tests');p.add_argument('id');p.add_argument('--manual',action='store_true',help='for open tasks only: requires written evidence')
    args=parser.parse_args()
    if args.command=='projects':return print_projects(args.minimal)
    if args.command=='project':
        if sum([args.start,args.done,args.test])>1:
            print('--start/--done/--test 只能选一个',file=sys.stderr);return 2
        project=get_project(args.id)
        if args.start: return start_project(project)
        if args.done: return finish_project(project)
        if args.test: return project_test(project,args.sanitize,args.gtest_source)
        return show_project(project)
    if args.command=='list':
        for i in MANIFEST: print(f"{i['id']}  {'[C++]' if i['kind']=='scaffold' else '[lab]'}  {i['title']}")
        return 0
    if args.command=='show':print((ROOT/get(args.id)['path']).read_text(encoding='utf-8'));return 0
    if args.command=='lab':create_lab(get(args.id));return 0
    if args.command=='build':
        if configure(args.sanitize,args.gtest,args.gtest_source):return 2
        return execute(['cmake','--build',binary_dir(args.sanitize,args.gtest),'--parallel','2'])
    if args.command=='test':return test_one(get(args.id),args.sanitize,args.gtest,args.gtest_source)
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
        if PROJECTS:
            pstate=load_project_state()
            p_passed=sum(1 for p in PROJECTS if pstate.get(p['id'],{}).get('status')=='PASS')
            m_passed=sum(1 for i in MINIMAL if pstate.get(i,{}).get('status')=='PASS')
            print(f'项目线：{p_passed}/{len(PROJECTS)}；最小闭环 {m_passed}/{len(MINIMAL)}')
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
