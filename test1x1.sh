#!/bin/bash

if [[ $# -ne 2 || $1 -gt $2 ]] ; then
    echo "Usage: $0 a b, where a is the first id test of range and b is the las one (pre: a <= b)"
    exit 1
fi

RESULTS_FILE="results.txt"
OUTPUT_FILE="output.txt"
INI_TEST=$1
FI_TEST=$2

if [ -f ${RESULTS_FILE} ] ; then
    rm ${RESULTS_FILE}
fi

for i in $(seq ${INI_TEST} ${FI_TEST}) ; do
    sed -i -e "s/runjp_rank.*/runjp_rank($i, $i);/" user.c 
    make clean && make && make emul > ${OUTPUT_FILE} &
    sleep 3
    killall bochs 
    sleep 1 
    grep "Test\[$i\]" ${OUTPUT_FILE} >> ${RESULTS_FILE}
done
exit
