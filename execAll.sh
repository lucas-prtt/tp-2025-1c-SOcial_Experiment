#!/bin/bash

# Log the run
echo -n "x" >> ejecuciones.log

# Módulo: memoria
cd memoria
rm -f memory.log
make
xterm -T "Memoria" -e bash -c "valgrind ./bin/memoria; read -n 1" &
cd ..
sleep 1

# Módulo: kernel
cd kernel
rm -f kernel.log
make
xterm -T "Kernel" -e bash -c "valgrind ./bin/kernel MEMORIA_BASE_TLB 256; read -n 1" &
cd ..
sleep 1

# Módulo: cpu
cd cpu
rm -f cpu1.log
make
xterm -T "Cpu" -e bash -c "valgrind ./bin/cpu 1; read -n 1" &
cd ..
sleep 1

# Módulo: io
cd io
rm -f io.log
make
xterm -T "IO" -e bash -c "valgrind ./bin/io DISCO"
