#!/usr/bin/env python3
#
# Add license headers to the codebase
#
# (c) Justus Languell 2022

import datetime
import os
import json

# def make_jdoc(text):
#     comment = '/**\n'
#     for line in text.split('\n'):
#         comment += ' * ' + line + '\n'
#     return comment + ' */'


def get_license():
    return '''//  
//  Copyright (c) Texas A&M University.
//  '''


def extension(path):
    return os.path.splitext(path)[-1]

def with_license(path, source):
    k = source.index('#')
    if extension(path).endswith('.h'):
        return get_license() + '\n' + source[k:]
    if extension(path).endswith('.c'):
        return get_license() + '\n' + source[k:]
    return source

def validate(path):
    print('Licensing ' + path)
    source = open(path, 'r').read()
    source = with_license(path, source)
    with open(path, 'w') as f:
        f.write(source)

if __name__ == '__main__':
    for root, subdirs, files in os.walk(os.path.realpath('../Core/Src/dbms')):
        [validate(root + '/' + file) for file in files]
