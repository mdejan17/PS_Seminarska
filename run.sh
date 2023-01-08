#!/bin/bash
mkdir img
mkdir final
mkdir bin
gcc  src/pt_rows.c -o serial -fopenmp
./serial
python3 src/toImage.py
cd final
ffmpeg -framerate 20 -i "%05d.png" ../output.mp4 -y
cd ..
rm serial
rm -rf bin
rm -rf img
rm -rf final