#!/bin/bash

#Updates and dependency install
sudo apt update
sudo apt upgrade -y
sudo apt install curl gnupg2 lsb-release -y
#Adding GPG key
curl -sL https://repos.influxdata.com/influxdb.key | sudo gpg --dearmor -o /usr/share/keyrings/influxdb-archive-keyring.gpg
echo "deb [signed-by=/usr/share/keyrings/influxdb-archive-keyring.gpg] https://repos.influxdata.com/debian bookworm stable" | sudo tee /etc/apt/sources.list.d/influxdb.list
#Installing Influxdb
sudo apt update
sudo apt install influxdb=1.8.* -y
#Enabling service and starting
sudo systemctl enable influxdb
sudo systemctl start influxdb
#Checking status
sudo systemctl status influxdb
