#!/bin/bash
g++ *.cpp -L ./ -lsqlite -lpthread -ldl -std=c++11 -g
