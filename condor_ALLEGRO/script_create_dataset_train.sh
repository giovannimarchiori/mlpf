#!/bin/bash

python submit_jobs_train.py --config config_spread_031224_fair.gun --outdir /eos/experiment/fcc/users/g/gmarchio/ALLEGRO_o1_v03/mlpf/train/gun_dr_logE_v0_010725/  --condordir /eos/experiment/fcc/users/g/gmarchio/ALLEGRO_o1_v03/mlpf/condor/gun_dr_log_logE_v0_290125/  --njobs 4000 --nev 100 --queue tomorrow
