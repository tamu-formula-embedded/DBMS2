#!/usr/bin/env python3
#
# Add license headers to the codebase
#
# (c) Justus Languell 2022

import datetime
import os

license_email = 'justus@tamu.edu'

def make_jdoc(text):
    comment = '/**\n'
    for line in text.split('\n'):
        comment += ' * ' + line + '\n'
    return comment + ' */'


def get_license(path):
    project = find_project(path)
    disclaimer = 'not licensed for distribution'
    if project == 'TorqueLib':
        disclaimer = 'licensed under the MIT license'

    year = get_project_year(project)
    return f'''Copyright (C) Texas A&M University.

This file is part of {project}, which is {disclaimer}.
For more details, see ./license.txt or write <{license_email}>.'''


def extension(path):
    return os.path.splitext(path)[-1]


put_license_before = 'package org.texastorque'


def with_license(path, source):
    license = get_license(path)
    if extension(path).endswith('.h'):
        return make_jdoc(license) + '\n' + put_license_before + source.split(put_license_before)[1:]
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
