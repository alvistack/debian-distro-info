#!/usr/bin/python3

import re
from pathlib import Path

from setuptools import setup

PACKAGES = []
PY_MODULES = ["distro_info"]
SCRIPTS = ["debian-distro-info", "ubuntu-distro-info"]


def get_debian_version():
    """look what Debian version we have"""
    version = None
    changelog = Path("../debian/changelog")
    if changelog.exists():
        with changelog.open("r", encoding="utf-8") as changelog_f:
            head = changelog_f.readline()
        match = re.compile(r".*\((.*)\).*").match(head)
        if match:
            version = match.group(1)
    return version


def get_pep440_version():
    """Return the a PEP440 compliant version."""
    return re.sub("([a-zA-Z])", r"+\1", get_debian_version(), count=1)


if __name__ == "__main__":
    setup(
        name="distro-info",
        version='1.2',
        py_modules=PY_MODULES,
        packages=PACKAGES,
        test_suite="distro_info_test",
        url="https://salsa.debian.org/debian/distro-info",
        author="Benjamin Drung",
        author_email="bdrung@debian.org",
    )
