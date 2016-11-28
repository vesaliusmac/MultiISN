for i in {50000..200000..10000}
do 
	
	./GGk_default 10 20 $i 500000 16 > temp_$i &
	# echo $i > temp_$i &
	
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

for i in {50000..200000..10000}
do 
	
	# ./GGk_default 10 20 $i;
	cat temp_$i >> result/log_"$(date +"%Y_%m_%d_%H_%M")";
	
done

for i in {50000..200000..10000}
do 
	
	# ./GGk_default 10 20 $i;
	rm temp_$i;
	
done