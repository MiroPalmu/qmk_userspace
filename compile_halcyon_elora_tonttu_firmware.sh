#!/usr/bin/sh
qmk doctor || return 1
qmk compile -j 4 -e HLC_TFT_DISPLAY=1 -e TARGET=tonttu_hlc_display -km tonttu_hlc -kb splitkb/halcyon/elora/rev2
qmk compile -j 4 -e HLC_CIRQUE_TRACKPAD=1 -e TARGET=tonttu_hlc_trackpad -km tonttu_hlc -kb splitkb/halcyon/elora/rev2
