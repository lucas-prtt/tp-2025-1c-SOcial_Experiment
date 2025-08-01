#!/bin/bash

# Log the run
echo -n "x" >> ejecuciones.log

# Módulo: memoria
cd memoria
rm -f memoria.log
make
xterm -T "Memoria" -e bash -c "valgrind ./bin/memoria; read -n 1" &
cd ..
sleep 1

# Módulo: kernel
cd kernel
rm -f kernel.log
make
xterm -T "Kernel" -e bash -c "valgrind ./bin/kernel ESTABILIDAD_GENERAL 0; read -n 1" &
cd ..
sleep 1

# Módulo: cpu
cd cpu
rm -f cpu1.log
rm -f cpu2.log
rm -f cpu3.log
rm -f cpu4.log
make
xterm -T "Cpu" -e bash -c "valgrind --leak-check=full ./bin/cpu 1; read -n 1" &
xterm -T "Cpu" -e bash -c "valgrind --leak-check=full ./bin/cpu 2; read -n 1" &
xterm -T "Cpu" -e bash -c "valgrind --leak-check=full ./bin/cpu 3; read -n 1" &
xterm -T "Cpu" -e bash -c "valgrind --leak-check=full ./bin/cpu 4; read -n 1" &
cd ..
sleep 1

# Módulo: io
cd io
rm -f io.log
make
xterm -T "IO" -e bash -c "valgrind ./bin/io DISCO"&
xterm -T "IO" -e bash -c "valgrind ./bin/io DISCO"&
xterm -T "IO" -e bash -c "valgrind ./bin/io DISCO"&
xterm -T "IO" -e bash -c "valgrind ./bin/io DISCO"
