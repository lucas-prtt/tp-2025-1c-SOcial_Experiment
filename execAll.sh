#!/bin/bash
# Ejecuta todo en la misma compu
# Sirve para pruebas rapidas, pero es mejor usar un entorno distribuido
# Ejecutable con ./execAll.sh

# Registra cuantas pruebas hiciste, para que tu decenso a la locura quede justificado
echo -n "x" >> ejecuciones.log

# Módulo: memoria
cd memoria
rm memory.log
make
exo-open --launch TerminalEmulator --title="Memoria" -e "bash -c 'valgrind ./bin/memoria; read -n 1'"
cd ..
sleep 1
# Módulo: kernel
cd kernel
rm kernel.log
make
exo-open --launch TerminalEmulator --title="Kernel" -e "bash -c 'valgrind ./bin/kernel MEMORIA_IO 90; read -n 1';"
cd ..
sleep 1

# Módulo: cpu
cd cpu
rm cpu1.log
make
exo-open --launch TerminalEmulator --title="Cpu" -e "bash -c 'valgrind ./bin/cpu 1; read -n 1;'"
cd ..


# Módulo: io
cd io
rm io.log
make
exo-open --launch TerminalEmulator --title="IO" -e "bash -c 'valgrind ./bin/io DISCO; read -n 1;'"
cd ..