set -e

# append times
../scripts/linux/plot-batched-appends.py aad-append-time-avg.png *append-time*r4.16xlarge*.csv

# append-only proofs
../scripts/linux/plot-append-only-proof.py aad-append-only-proof-worst-case.png append-only-all-cases-07-22-2018/aad-append-only-proof-2.csv

# avg-case lookups
../scripts/linux/plot-memb-proof.py " (average case)" aad-memb-avg-case.png avg-case-memb-proof-07-15-2018/10/*.csv

# worst-case lookups
../scripts/linux/plot-memb-proof.py " (worst case)" aad-memb-worst-case.png worst-case-memb-proof-07-14-2018/*.csv

# stitch together append-only proof size and append time
montage aad-append-time-avg.png aad-append-only-proof-worst-case.png -tile 1x2 -geometry +0+0 aad-merged-append-time-and-proof.png

# open all files
open aad-merged-append-time-and-proof.png aad-memb-avg-case.png aad-memb-worst-case.png
