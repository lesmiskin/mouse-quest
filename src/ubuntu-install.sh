#!/bin/bash

if [[ $(/usr/bin/id -u) -ne 0 ]]; then
    echo "You're almost there - please run this script with root privileges. e.g. 'sudo ./install.sh'"
    exit
fi

apt-get install libsdl2-2.0-02 libsdl2-image-2.0-02 libsdl2-mixer-2.0-02 -y || echo 'Sorry, we could not install the support files. Are you connected to the internet? Otherwise, please contact trilinear.nz@gmail.com for assistance.' &&

echo
echo 'Supporting files have been installed, thank you for your patience.'
echo
echo 'Prepare to enter Mouse Quest! [Press any key]'
read -n 1

./surface
