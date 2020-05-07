#!/bin/bash
set -e

echo getting sudo privs...
sudo whoami

make clean
make

# check whether they have symlinked /usr/local/bin/makegen to here. If not, update the copy in /usr/bin/
#if [ -f /usr/bin/makegen ]; then
	#INSTALLED_MAKEGEN=$(readlink -e /usr/local/bin/makegen)
#else
	#INSTALLED_MAKEGEN="<not installed>"
#fi

#NEW_MAKEGEN=$(readlink -e ./makegen)

#if [ "$INSTALLED_MAKEGEN" != "$NEW_MAKEGEN" ]; then
	echo Copying to /usr/local/bin/...
	if [ "$(whoami)" == "root" ]; then
		rm -f /usr/bin/makegen
		cp makegen /usr/local/bin/
	else
		sudo rm -f /usr/bin/makegen
		sudo cp makegen /usr/local/bin/
	fi
	
	echo " -- Makegen installed into /usr/local/bin."
#else
	#echo " -- Makegen updated."
#fi

echo
