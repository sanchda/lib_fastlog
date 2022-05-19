#!/bin/bash

seq=1

gcc -I include_$seq main.c -o fastlog -lm
