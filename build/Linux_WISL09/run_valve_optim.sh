#/bin/bash
input_file=../../Input_Files/Demand_Zero_1.inp
outflow_file=../../Input_Files/temp_demand.csv
optimisation_period=24
LD_LIBRARY_PATH=./:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH
echo $input_file $outflow_file $optimisation_period
./epanet_valve_optim $input_file $ouflow_file $optimisation_period
