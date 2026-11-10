from pathlib import Path
import subprocess,json,hashlib,csv
r=Path('/workspace/work/tffere-strike');w=r/'work/context1000';frozen=r/'training/runs/2026-11-08-real-dual/before-player-policy.txt';exe='/workspace/work/strike-context-trainer'
plan={'budget':1000,'training_rounds_per_block':100,'blocks':10,'lineage':'One continuousV3 lineage initialized from validated v1.7 selected400','capacity':{'base_states':192,'context_states':49152,'actions':12,'sparse_context_rows':True,'parent_backoff_strength':2},'feature_bits':['aim_aligned','fire_ready','cover_within3m','finer_range','visible_enemy_facing_bot','visible_enemy_sniper'],'own_map_regions':4,'added_actions':['east_route','west_route','cross_route'],'simulation_step_seconds':1/120,'high_level_decision_seconds':.25,'reward':'1win,0allother; first-visit episodic MC; no shaping/discount','training_seeds':[3700100,3701099],'validation_seeds':[3800000,3800399],'fresh_test_seeds':[4000000,4000999],'frozen_sha256':hashlib.sha256(frozen.read_bytes()).hexdigest(),'starting_v2_sha256':hashlib.sha256((r/'native/assets/bot-policy-trained.txt').read_bytes()).hexdigest(),'selection':'Highest validation wins among inherited V3baseline and ten100-round checkpoints; ties incumbent/earlier. Lock before fresh tests.','fresh_gate':'Strictly more wins and paired one-sided exact binomial p<0.05 on1000fresh cases; otherwise retain v1.7','publishing':'No Windows package published while user antivirus alert detail remains unresolved'}
plan['source_sha256']={p:hashlib.sha256((r/p).read_bytes()).hexdigest() for p in ['native/gameplay.hpp','native/contextual_policy.hpp','native/training/context_episode.hpp','native/training/train_context.cpp','native/training/actual_game.hpp']}
(w/'PLAN-LOCKED.json').write_text(json.dumps(plan,indent=2)+'\n')
bot=w/'initial-v3.txt'
for i in range(1,11):
 start=.25-.23*(i-1)/10;end=.25-.23*i/10
 alpha,parent=(.12,.02) if i<=5 else(.08,.01)
 cfg=w/f'block{i}-config.txt';cfg.write_text(f'games 100\nevaluation_games 400\ntraining_seed_base 3700000\nevaluation_seed_base 3800000\nepsilon_start {start:.8f}\nepsilon_end {end:.8f}\nalpha {alpha}\nparent_alpha {parent}\n')
 out=w/f'block{i}'
 with (w/f'block{i}-console.txt').open('w') as log:subprocess.run([exe,'run',str(bot),str(frozen),str(out),str(i),str(cfg)],cwd=r,stdout=log,stderr=subprocess.STDOUT,check=True)
 d=list(csv.DictReader((out/'evaluation-metrics.csv').open()))[0]
 print(f"{i*100}/1000: validation {d['wins']}/400 W, {d['losses']} L, {d['mutual_eliminations']} mutual, damage{d['mean_damage']},taken{d['mean_taken']}",flush=True)
 assert (out/'level0-before.txt').read_bytes()==(out/'level0-after.txt').read_bytes()
 assert (out/'after-bot.txt').read_bytes()==(out/'after-evaluation-bot.txt').read_bytes()
 bot=out/'after-bot.txt'
assert hashlib.sha256(frozen.read_bytes()).hexdigest()==plan['frozen_sha256']
print('Exactly1000 retained campaign learning rounds complete.',flush=True)
