import sys
import math
import ROOT
from array import array
from ROOT import TFile, TTree
import numpy as np
from podio import root_io
import edm4hep
from tree_tools import (
    initialize,
    clear_dic,
    gen_particles_find,
    store_gen_particles,
    store_tracks,
    store_calo_hits,
)

# TODO
# is last track state position at calo?
# Bz should be stored in the tree
# should allow for multiple gen links to hit (probably has to be done in the previous edm4hep formation stage)

debug = False
"""
source /cvmfs/sw-nightlies.hsf.org/key4hep/setup.sh

python make_pftree_clic_bindings.py /afs/cern.ch/work/s/selvaggi/private/particleflow/fcc/out_reco_edm4hep.root tree.root 

"""

## global params
CALO_RADIUS_IN_MM = 1500   # unused?

# parse command line
if len(sys.argv) < 2:
    print(" Usage: make_pftree_clic_bindings.py input_file output_file")
    sys.exit(1)

input_file = sys.argv[1]
output_file = sys.argv[2]
store_pandora_hits = sys.argv[3]
print("will store calo hits", store_pandora_hits)
CLIC = sys.argv[4]
print("is it CLIC", CLIC)

# create podio reader for input file
reader = root_io.Reader(input_file)

# initialize output file
out_root = TFile(output_file, "RECREATE")
t = TTree("events", "pf tree lar")

# initialize output tree using method in tree_tools
event_number, n_hit, n_part, dic, t = initialize(t)

event_number[0] = 0
for i, event in enumerate(reader.get("events")):
    if debug:
        if i > 15:
            break
    number_of_hist_with_no_genlinks = 0

    # clear all the vectors
    dic = clear_dic(dic)
    if (i + 1) % 1000 == 0:
        print(" ... processed {} events ...".format(i + 1))

    ## STORE ALL STATUS 1 GENPARTICLES
    n_part[0] = 0

    print("")
    print(" ----- new event: {} ----------".format(event_number[0]))
    print("")

    (
        genpart_indexes_pre,
        indexes_genpart_pre,
        n_part_pre,
        total_e,
        e_pp,
        gen_part_coll,
    ) = gen_particles_find(event, debug)

    dic, genpart_indexes = store_gen_particles(
        n_part_pre,
        gen_part_coll,
        indexes_genpart_pre,
        dic,
        n_part,
        debug,
    )

    n_hit[0] = 0
    n_hit, dic, number_of_hist_with_no_genlinks = store_tracks(
        event,
        debug,
        dic,
        genpart_indexes,
        n_hit,
        number_of_hist_with_no_genlinks,
        store_pandora_hits,
        CLIC,
    )

    (
        n_hit,
        dic,
        total_calohit_,
        number_of_hist_with_no_genlinks,
        total_calohit_pandora,
    ) = store_calo_hits(
        event,
        debug,
        dic,
        n_hit,
        genpart_indexes,
        gen_part_coll,
        number_of_hist_with_no_genlinks,
        store_pandora_hits,
        CLIC,
    )

    # if debug:
    #     print("total_e", total_e)
    #     print(total_calohit_)
    #     print(e_pp)
    #     print("pandora", total_calohit_pandora)
    #     fracs = []
    #     for i in range(2, 11):
    #         print(total_calohit_[i] / e_pp[i])
    #         fracs.append(total_calohit_[i] / e_pp[i])
    #     # if np.sum(np.array(fracs) < 0.1) > 1:
    #     #     break
    #     print("total number of hits: {}".format(n_hit[0]))
    #     print(
    #         "total number of hits with no gen links: {}".format(
    #             number_of_hist_with_no_genlinks
    #         )
    #     )
    #     print(": {}".format(number_of_hist_with_no_genlinks))

    if n_hit[0] <= number_of_hist_with_no_genlinks:
        print(
            "  --> WARNING: all hists in this event have no gen link associated or simply no hits, skipping event"
        )

    else:
        event_number[0] += 1
        t.Fill()

t.SetDirectory(out_root)
t.Write()
