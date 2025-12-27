#!/bin/bash

#Updates
sudo apt update
sudo apt upgrade -y
sudo apt install libcurl4-openssl-dev -y
#Getting 1.8 release
sudo cp influxdb1_8.service /etc/systemd/system/
cd /tmp
wget https://dl.influxdata.com/influxdb/releases/influxdb-1.8.10_linux_armhf.tar.gz
#Extract and Install
tar xzf influxdb-1.8.10_linux_armhf.tar.gz
cd influxdb-1.8.10-1
sudo cp usr/bin/influx* /usr/local/bin/
sudo mkdir -p /etc/influxdb /var/lib/influxdb
sudo cp etc/influxdb/influxdb.conf /etc/influxdb/
#setting up service file
sudo systemctl daemon-reexec
sudo systemctl daemon-reload
sudo systemctl enable influxdb1_8.service
sudo systemctl start influxdb1_8.service
#Checking version
#sudo systemctl status influxdb1_8.service

