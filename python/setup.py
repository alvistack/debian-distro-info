#!/usr/bin/python

import glob
import os
import re

from setuptools import setup

def get_debian_version():
    """look what Debian version we have"""
    version = None
    changelog = "../debian/changelog"
    if os.path.exists(changelog):
        head = open(changelog).readline()
        match = re.compile(".*\((.*)\).*").match(head)
        if match:
            version = match.group(1)
    return version

SCRIPTS = [
    'debian-distro-info',
    'ubuntu-distro-info',
]

if __name__ == '__main__':
    setup(name='distro-info',
          version=get_debian_version(),
          py_modules=['distro_info'],
          packages=['distro_info_test'],
          test_suite='distro_info_test.discover',
    )
