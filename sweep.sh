first=50;
last=250;
step=10;
max=5000;
outputfilecount=0;
i=50;
j=15;
k=10;
# first=0;
# last=16;
# step=1;
for k in $(seq 10 10 200)
do
	# for j in $(seq 0 1 16)
	# do
		for i in $(seq $first $step $last)
		do 
			./GGk_default $k 20 $max $i $j > temp_$outputfilecount &
			while true; do
				count=$(pgrep -c "GGk_default");
				if [ "$count" -ge 20 ]
				then
					sleep 10;
				else
					# echo "Stopped"
					break;
				fi
			done
			outputfilecount=$(expr $outputfilecount + 1);
		done
	# done
done
while true; do
	if pgrep "GGk_default" > /dev/null
	then
		sleep 10;
	else
		# echo "Stopped"
		break;
	fi
done
outputfilecount=$(expr $outputfilecount - 1);
temp=result/log_"$(date +"%Y_%m_%d_%H_%M_%S")";

echo -e "QPS\treQ_th\tAGG_drop_rate\tISN_drop_rate\tPower\tQuality\t95th_lat\t99th_lat" >> $temp;
for i in $(seq 0 1 $outputfilecount)
do 
	cat temp_$i >> $temp;
	
done

for i in $(seq 0 1 $outputfilecount)
do 
	rm temp_$i;
	
done

cat $temp;
echo $temp;