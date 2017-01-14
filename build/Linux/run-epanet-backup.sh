#!/bin/sh

#clean older files
rm -rf epanet_valve_optim_wisl09

#make the epanet_valve_optim.c file
make -f Makefile_wisl09

#run the simulation
LD_LIBRARY_PATH=./:$LD_LIBRARY_PATH ./epanet_valve_optim_wisl09 ../../Input_Files/Devanoor_10DMA_demand_zero_changed.inp ../../Input_Files/temp_demand_all.csv ../../Input_Files/joblist.txt 24|tee ../../Output_Files/7may_7.out



#valgrind checking
#LD_LIBRARY_PATH=./:$LD_LIBRARY_PATH valgrind --leak-check=full --show-leak-kinds=all ./epanet_valve_optim_wisl09 ../../Input_Files/Devanoor_10DMA_demand_zero.inp ../../Input_Files/temp_demand_all.csv 24|tee ../../Output_Files/7may_2.out
