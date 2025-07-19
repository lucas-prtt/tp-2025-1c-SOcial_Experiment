#!/bin/bash
cd ..
echo "Acordate que la password en realidad es el token"
git clone https://github.com/sisoputnfrba/so-commons-library.git
cd so-commons-library
make install
cd ..
