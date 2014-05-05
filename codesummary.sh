#!/bin/sh

cd src/

# General and individual files.
LINES_MAIN=`wc -l *.sp | tail -n1 | sed 's/^ *\(.*\) *$/\1/' | cut -d ' ' -f1`
LINES_BASE=`wc -l zr/base/*.* | tail -n1 | sed 's/^ *\(.*\) *$/\1/' | cut -d ' ' -f1`
LINES_LIBRARIES=`wc -l zr/libraries/*.* | tail -n1 | sed 's/^ *\(.*\) *$/\1/' | cut -d ' ' -f1`
LINES_MODULES=`wc -l zr/modules/*.* | tail -n1 | sed 's/^ *\(.*\) *$/\1/' | cut -d ' ' -f1`
LINES_OTHER=`wc -l zr/*.* | tail -n1 | sed 's/^ *\(.*\) *$/\1/' | cut -d ' ' -f1`

# Cores
LINES_ZRC_CORE=`wc -l zr/modules/zrc_core/*.* | tail -n1 | sed 's/^ *\(.*\) *$/\1/' | cut -d ' ' -f1`
LINES_ZRIOT_CORE=`wc -l zr/modules/zriot_core/*.* | tail -n1 | sed 's/^ *\(.*\) *$/\1/' | cut -d ' ' -f1`

# Larger modules
LINES_WEAPONS=`wc -l zr/modules/weapons/*.* | tail -n1 | sed 's/^ *\(.*\) *$/\1/' | cut -d ' ' -f1`
LINES_MODELS=`wc -l zr/modules/models/*.* | tail -n1 | sed 's/^ *\(.*\) *$/\1/' | cut -d ' ' -f1`
LINES_CLASSES=`wc -l zr/modules/classes/*.* | tail -n1 | sed 's/^ *\(.*\) *$/\1/' | cut -d ' ' -f1`

# Totals
GENERAL_TOTAL="$(($LINES_MAIN + $LINES_BASE + $LINES_LIBRARIES + $LINES_OTHER))"
CORES_TOTAL="$(($LINES_ZRC_CORE + LINES_ZRIOT_CORE))"
MODULES_TOTAL="$(($LINES_WEAPONS + $LINES_MODELS + $LINES_CLASSES + $LINES_MODULES))"

LINES_TOTAL="$(($GENERAL_TOTAL + $CORES_TOTAL + $MODULES_TOTAL))"

echo "Number of lines:"
echo "$LINES_MAIN\tmain sp"
echo "$LINES_BASE\tbase"
echo "$LINES_LIBRARIES\tlibraries"
echo "$LINES_OTHER\tother (config, events)"
echo ""
echo "$LINES_ZRC_CORE\tzrc_core"
echo "$LINES_ZRIOT_CORE\tzriot_core"
echo ""
echo "$LINES_MODELS\tmodels"
echo "$LINES_WEAPONS\tweapons"
echo "$LINES_CLASSES\tclasses"
echo "$LINES_MODULES\tother modules"

echo "\nTotal:"
echo "$LINES_TOTAL"
