#!/bin/bash

echo "This is a helper script to install dependencies with apt to build dfgTest."
echo "Mostly tested on Ubuntu"
echo ""
#read -p "Proceed with installation (y/n)? " -n 1 -r
read -p "Proceed with installation (y/n)? "
echo ""

if [ ! "$REPLY" == "y" ]; then
    echo "Quiting"
    exit 0
fi

echo "Installing cmake"
echo ""
sudo apt install cmake

echo "Installing g++"
echo ""
sudo apt install g++

echo "Installing boost"
echo ""
sudo apt install libboost-dev

read -p "Install clang (optional)? (y/n)? "
echo ""
if [ "$REPLY" == "y" ]; then
    echo "Installing clang"
    sudo apt install clang
    echo ""

    read -p "Install libc++ (optional)? (y/n) "
    if [ "$REPLY" == "y" ]; then
        echo "Installing libc++"
        echo ""
        sudo apt install libc++-dev libc++abi-dev
    else
        echo "Skipped libc++ installation"
    fi
else
    echo "Skipped clang installation"
fi
