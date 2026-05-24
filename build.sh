#!/usr/bin/env bash

filename="$1"
binaryname="$2"
shift 2
extra="$@"
echo "Compiling code"

g++ $filename -o $binaryname \
  -I/opt/sfml2/include \
  -L/opt/sfml2/lib \
  -lsfml-graphics -lsfml-window -lsfml-system -fopenmp -std=c++17

if [ $? -eq 0 ]; then
  echo "Compilation succesful"
  LD_LIBRARY_PATH=/opt/sfml2/lib:$LD_LIBRARY_PATH ./$binaryname
else
  echo "Compilation failed"
fi
