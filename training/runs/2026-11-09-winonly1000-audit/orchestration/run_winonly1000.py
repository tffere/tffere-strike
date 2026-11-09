from pathlib import Path
import subprocess,json,hashlib,csv
root=Path('/workspace/work/tffere-strike');out=root/'work/winonly1000';out.mkdir(exist_ok=True)
start=root/'native/assets/bot-policy-trained.txt';frozen=root/'training/runs/2026-11-08-real-dual/before-player-policy.txt';exe='/workspace/work/strike-campaign-winonly'
plan={'training_budget':1000,'branches':{'conservative':500,'exploratory':500},'start':'v1.7 selected400','starting_policy_sha256':hashlib.sha256(start.read_bytes()).hexdigest(),'frozen_player_sha256':hashlib.sha256(frozen.read_bytes()).hexdigest(),'reward':'1 win, 0 all other outcomes; no shaping or time discount','validation_seeds':[3100000,3100599],'fresh_test_seeds':[3200000,3200999],'selection':'Most validation wins among baseline and ten100-round branch checkpoints, ties baseline then earliest candidate; lock before fresh tests','fresh_gate':'Strictly more wins and paired one-sided exact binomial p<0.05 on fresh1000 cases; otherwise keep incumbent','physics':'unchanged v1.7,25HP rifle,shared reaction population mean150ms SD50, real desert map random connected spawns/headings'}
(out/'PLAN-LOCKED.json').write_text(json.dumps(plan,indent=2)+'\n')
base=Path('/tmp/winonly500-config.txt').read_text().replace('evaluation_games 400','evaluation_games 600').replace('evaluation_seed_base 2700000','evaluation_seed_base 3100000')
(out/'baseline-config.txt').write_text(base)
def run(bot,dest,cycle,cfg):
 args=[exe,str(bot),str(frozen),str(frozen),str(dest),str(cycle),str(cfg)]
 if cycle==0:args+=['test-only']
 with (out/(dest.name+'-console.txt')).open('w') as log:
  subprocess.run(args,cwd=root,stdout=log,stderr=subprocess.STDOUT,check=True)
 print(dest.name,(dest/'summary.csv').read_text().splitlines()[-1],flush=True)
run(start,out/'baseline',0,out/'baseline-config.txt')
for branch in ['conservative','exploratory']:
 bot=start
 for i in range(1,6):
  if branch=='conservative':alpha=.05-(i-1)*.0075;ep0=.12-(i-1)*.02;ep1=ep0-.02;seed=2900000
  else:alpha=.09-(i-1)*.0125;ep0=.30-(i-1)*.05;ep1=ep0-.05;seed=3000000
  config=base.replace('alpha 0.08',f'alpha {alpha:.6f}').replace('epsilon_start 0.20',f'epsilon_start {ep0:.6f}').replace('epsilon_end 0.04',f'epsilon_end {ep1:.6f}').replace('training_seed_base 2600000',f'training_seed_base {seed}')
  cfg=out/f'{branch}-{i}-config.txt';cfg.write_text(config);dest=out/f'{branch}-{i}'
  run(bot,dest,i,cfg);bot=dest/'after-bot.txt'
assert hashlib.sha256(frozen.read_bytes()).hexdigest()==plan['frozen_player_sha256']
print('Exactly1000 training rounds complete.',flush=True)
