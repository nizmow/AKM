#!/bin/bash

# Localise MagiC root fs
# Note that this script can be run from anywhere.
# TODO: should run recursively

# Note that macOS only knows "-v" and "-n" and not "--verbose" and "--update=none".
if [[ "${OSTYPE}" == "linux-gnu"* ]]; then
    OPT_UPDATE_NONE='--update=none'
    OPT_VERBOSE='--verbose'
elif [[ "$OSTYPE" == "darwin"* ]]; then
    OPT_UPDATE_NONE='-n'
    OPT_VERBOSE='-v'
else
    OPT_UPDATE_NONE='-n'
    OPT_VERBOSE='-v'
fi

#VERBOSE=$OPT_VERBOSE
VERBOSE=""

if [ "$#" -eq 1 ]; then
    # go to directory where the script resides
    cd "$(dirname "$0")"
    # convert language code to uppercase
    CODE=`echo $1 | tr a-z A-Z`
    # might not be necessary, just to be sure
    if ! [ -f ../MAGICLIN.OS ]; then
        echo "ERROR: No MAGICLIN.OS in root."
        exit 1
    fi
    # check if kernel file already matches
    cmp --quiet $CODE/MAGICLIN.OS ../MAGICLIN.OS
    STATUS=$?
    if [ $STATUS -eq 2 ]; then
        echo "LOCALISE.SH: Valid country codes are: "; ls -d ??
        exit 1
    fi
    if [ $STATUS -eq 1 ]; then
        # overwrite all programs, documents and kernel
        cp $VERBOSE -pf $CODE/MAGICLIN.OS ../
        cp $VERBOSE -pf $CODE/EXTRAS/*.INF ../EXTRAS/
        cp $VERBOSE -pf $CODE/EXTRAS/AES_LUPE/*.TXT ../EXTRAS/AES_LUPE/
        cp $VERBOSE -pf $CODE/GEMSYS/GEMDESK/*.RSC ../GEMSYS/GEMDESK/
        cp $VERBOSE -pf $CODE/GEMSYS/GEMDESK/*.PRG ../GEMSYS/GEMDESK/ 2>/dev/null
        cp $VERBOSE -pf $CODE/GEMSYS/GEMDESK/*.TOS ../GEMSYS/GEMDESK/
        cp $VERBOSE -pf $CODE/GEMSYS/GEMDESK/*.TXT ../GEMSYS/GEMDESK/
        cp $VERBOSE -pf $CODE/GEMSYS/MAGIC/XTENSION/*.SLB ../GEMSYS/MAGIC/XTENSION/
        cp $VERBOSE -pf $CODE/GEMSYS/MAGIC/XTENSION/*.SYS ../GEMSYS/MAGIC/XTENSION/
    fi
else
    echo "usage: LOCALISE.SH DE|EN|FR"
    #echo $#
    exit 1
fi
