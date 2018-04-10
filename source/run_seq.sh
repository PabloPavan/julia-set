#!/bin/bash
# $1 - nome do executavel
# $2 - numeros de elementos
# $3 - arquivo bmp
# $4 - repetições


if [ ! $# -ge 4 ]
  then
    echo " $0 <EXEC> <ELEMENTS> <BMP> <REPEAT>"
	exit 1
fi
		
echo "step; value" > ../results/$1_$2_$4.csv
for step in $(seq "$4"); do
	./$1 $2 $3 &> tmp
	echo "$step"";" `cat tmp | grep "Tempo total=" | cut -d '=' -f2 | sort -n | tail -n1 | sed 's/ms//g' | sed 's/ //g'` >> ../results/$1_$2_$4.csv
	echo "step : $step"
	sleep 10
done
rm -rf tmp
