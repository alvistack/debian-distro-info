#!/usr/bin/python

import glob
import os
import re

from setuptools import setup

# look/set what version we have
changelog = "debian/changelog"
if os.path.exists(changelog):
    head=open(changelog).readline()
    match = re.compile(".*\((.*)\).*").match(head)
    if match:
        version = match.group(1)

scripts = [
    'debian-distro-info',
    'ubuntu-distro-info',
]

if __name__ == '__main__':
    setup(name='distro-info',
          version=version,
          scripts=scripts,
          py_modules=['distro_info'],
          packages=['distro_info_test'],
          data_files=[('share/distro-info', glob.glob('data/*')),
                      ('share/man/man1', glob.glob("doc/*.1")),
                     ],
          test_suite='distro_info_test.discover',
    )
