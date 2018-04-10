#!/bin/bash
# $1 - nome do executavel
# $2 - numeros de nodes
# $3 - numeros de elementos
# $4 - arquivo bmp
# $5 - repetições


if [ ! $# -ge 5 ]
  then
    echo " $0 <EXEC> <NODES> <ELEMENTS> <BMP> <REPEAT>"
	exit 1
fi
		
echo "step; process; value" > ../results/$1_$2_$3_$5.csv
for step in $(seq "$5"); do
#	smpirun -hostfile ./simple_host_file.txt -platform ./simple_cluster.xml -np $2 ./$1 $3 $4 &> tmp
	mpirun -np $2 ./$1 $3 $4 &> tmp
	for p in $(seq "$2"); do
		let p=$p-1 
		echo "$step"";" "$p"";" `cat tmp | egrep -w "$p:" | cut -d ':' -f2 | sed 's/\t//g'` >> ../results/$1_$2_$3_$5.csv
	done
	sleep 10
	echo "step : $step"
done
#rm -rf tmp
