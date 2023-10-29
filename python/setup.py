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

def make_pep440_compliant(version):
    """Convert the version into a PEP440 compliant version."""
    public_version_re = re.compile(r"^([0-9][0-9.]*(?:(?:a|b|rc|.post|.dev)[0-9]+)*)\+?")
    _, public, local = public_version_re.split(version, maxsplit=1)
    if not local:
        return version
    sanitized_local = re.sub("[+~]+", ".", local).strip(".")
    pep440_version = "{}+{}".format(public, sanitized_local)
    assert re.match("^[a-zA-Z0-9.]+$", sanitized_local), (
        "'{}' not PEP440 compliant".format(pep440_version))
    return pep440_version


if __name__ == '__main__':
    setup(name='distro-info',
          version=make_pep440_compliant(get_debian_version()),
          py_modules=['distro_info'],
          packages=['distro_info_test'],
          test_suite='distro_info_test.discover',
    )
