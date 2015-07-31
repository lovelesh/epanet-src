
Compile Command : gcc -g -O3 -fPIC -I . -D SOL -c epanet_optim.c
Link command : gcc -g -O3 -fPIC -o epanet_optim epanet_optim.o -lepanet2_custom -L . -Wl,-rpath
