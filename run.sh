#!/bin/bash
gcc src/serial.c -o serial
./serial
sleep 2
python3 toImage.py
rm serial
rm rezultati.txt