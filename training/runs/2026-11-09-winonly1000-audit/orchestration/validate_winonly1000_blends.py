from pathlib import Path
import json,subprocess,hashlib
r=Path('/workspace/work/tffere-strike');w=r/'work/winonly1000';base=r/'native/assets/bot-policy-trained.txt';learned=w/'exploratory-4/after-bot.txt';frozen=r/'training/runs/2026-11-08-real-dual/before-player-policy.txt'
(w/'PLAN-AMENDMENT-BEFORE-FRESH-TEST.json').write_text(json.dumps({'reason':'Raw checkpoints barely exceed incumbent; test conservative score blending to retain existing decisions','timing':'After1000 training rounds and validation checkpoint inspection, before any fresh test access','additional_training_rounds':0,'learned_checkpoint':'exploratory-4','learned_weights':[.1,.25,.5,.75,.9],'selection':'Largest validation win count across original11 and these5 candidates; ties incumbent/earlier. Fresh safety gate unchanged.'},indent=2)+'\n')
for weight in [.1,.25,.5,.75,.9]:
 name=f'blend-{weight:g}';p=w/f'{name}.txt';subprocess.run(['/tmp/blend-policy',str(base),str(learned),str(weight),str(p)],check=True)
 with (w/f'{name}-console.txt').open('w') as log:subprocess.run(['/workspace/work/strike-campaign-winonly',str(p),str(frozen),str(frozen),str(w/name),'0',str(w/'baseline-config.txt'),'test-only'],cwd=r,stdout=log,check=True)
 print(name,(w/name/'summary.csv').read_text().splitlines()[-1],flush=True)
