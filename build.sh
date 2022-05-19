#!/bin/bash

seq=5

gcc -O0 -g -I include_$seq main.c -o fastlog -lm
