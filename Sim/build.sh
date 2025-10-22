#!/bin/bash

rm -rf src/dbms
rm -rf src/lib
cp -r ../Core/Src/dbms src/dbms
cp -r ../Core/Src/lib src/lib
make
rm -rf src/dbms
rm -rf src/lib
