#!/bin/bash

if [[ $(/usr/bin/id -u) -ne 0 ]]; then
    echo "Please run this script with root privileges. e.g. 'sudo ./install.sh'"
    exit
fi

apt-get install libsdl2-2.0-0 libsdl2-image-2.0-0 libsdl2-mixer-2.0-0 -y

echo
echo 'Prepare to enter Mouse Quest! [Press any key]'
read -n 1

./mouse-quest
