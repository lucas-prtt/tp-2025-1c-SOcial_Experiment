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
exo-open --launch TerminalEmulator --title="Memoria" -e "bash -c './bin/memoria'"
cd ..

# Módulo: kernel
cd kernel
rm kernel.log
make
exo-open --launch TerminalEmulator --title="Kernel" -e "bash -c './bin/kernel PLANI_LYM_PLAZO 0'"
cd ..
sleep 2

# Módulo: cpu
cd cpu
rm cpu_1.log
make
exo-open --launch TerminalEmulator --title="Cpu" -e "bash -c './bin/cpu 1'"
cd ..


# Módulo: io
cd io
rm io.log
make
exo-open --launch TerminalEmulator --title="IO" -e "bash -c './bin/io DISCO'"
cd ..