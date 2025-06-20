#!/bin/bash

rm -rf src/dbms
cp -r ../Core/Src/dbms src/dbms
make
rm -rf src/dbms
