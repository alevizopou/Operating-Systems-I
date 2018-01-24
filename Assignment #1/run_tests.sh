#!/bin/bash

pizzaria="/tmp/i_kammeni_pizza"
clients=100
if [ $# -eq 1 ] ; then 
	clients=$1
fi

# trexoume to server (aka anoigoume tin pitsaria)
# an iparxei to socket skotonoume tin pizzaria kai to svinoume
if [ -S $pizzaria ] ; then
	pkill -15 pizzaria
	rm $pizzaria
fi

./pizzaria > pizzaria.log &
# dinoume kai 1 sec gia na xekinisei
sleep 1 

if [ ! -f client ] ; then
	echo "Client program is not found"
fi

# arxikopoioume to array me tis paraggelies
pizzes[0]=0
pizzes[1]=0
pizzes[2]=0
# i apostasi tou pelati
apostasi=0

# ftiaxnoume mia tixaia paraggelia gemizontas to array pizzes xrisimopoiontas ti $RANDOM
function create_order() 
{
	# kanonikopoiisi ton random timon apo 1 4 dld me 1 vazoume margarita, 2 peperoni, 3 special kai 4 kamia
	for i in {1..3} ; do
		val=$(echo "scale=0; $RANDOM*4.0/32767.0"|bc)
		if [ $val -lt 3 ] ; then 
			pizzes[$val]=$((${pizzes[$val]} + 1))
		fi
	done

}

function random_distance()
{
	# me 50% dialegoume tin apostasi
	if [ $RANDOM -gt 16384 ] ; then 
		apostasi=5
	else 
		apostasi=10
	fi
}

function run_client() {
#	pare apostasi
	random_distance

	# katanomi pitson
	for (( c=1; c<=15; c++ )) 
		do
			pizzes[0]=0
			pizzes[1]=0
			pizzes[2]=0

			create_order

			./client ${pizzes[@]} $apostasi | grep Server  # emfanise mono to apotelesma tis paraggelias
		done
}

# trexoume $clients parallila. Kathe run client trexei 15 paraggelies seiriaka
for (( p=1; p<=$clients ; p++ ))
	do
		run_client & # trexoume kathe run client sto background => trexoun parallila
	done

