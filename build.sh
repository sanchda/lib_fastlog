#!/bin/bash

seq=4

gcc -O0 -g -I include_$seq main.c -o fastlog -lm
