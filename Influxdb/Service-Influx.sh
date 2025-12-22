#!/bin/bash

cp -r sensors-db ~/
sudo cp -r readings-db.service /etc/systemd/system/
sudo systemctl daemon-reexec
sudo systemctl daemon-reload
sudo systemctl enable readings-db.service
sudo systemctl start readings-db.service
