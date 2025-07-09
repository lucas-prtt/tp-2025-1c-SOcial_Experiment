#!/bin/bash
# Requiere instalarse gnome terminal
# Ejecuta todo en la misma compu
# Sirve para pruebas rapidas, pero es mejor usar un entorno distribuido


# Módulo: memoria
cd memoria
make
gnome-terminal --title="Memoria" -- bash -c "./bin/memoria"
cd ..

# Módulo: kernel
cd kernel
make
gnome-terminal --title="Kernel" -- bash -c "./bin/kernel PLANI_CORTO_PLAZO 0"
cd ..

# Módulo: cpu
cd cpu
make
gnome-terminal --title="Cpu" -- bash -c "./bin/cpu 1"
cd ..


# Módulo: io
cd io
make
gnome-terminal --title="IO" -- bash -c "./bin/io DISCO"
cd ..