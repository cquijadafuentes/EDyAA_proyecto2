#!/bin/bash
die () {
    echo >&2 "$@"
    exit 1
}

[ "$#" -eq 1 ] || die "1 argument required, $# provided"

# Parámetro: folder a datasets
DATA=$1


for j in 2 3 4 5 6; do
	for i in 8 128 512; do
		echo "--------------- EJERCICIO ${j} (${i}x${i}) ---------------"
		./ejercicio_${j} ${DATA}${i}x${i}/ 120 ${i}
		echo "++++++++++++++++++++++++++++++++++++++++++++++"
	done
done