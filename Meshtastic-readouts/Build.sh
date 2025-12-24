#!/bin/bash

#Checking for Updates and installing Essential tools
sudo apt update -y
sudo apt install build-essential -y

#Running Makefile
make clean
make

