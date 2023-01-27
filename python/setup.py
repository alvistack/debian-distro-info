#!/usr/bin/python3

import os
import re

from setuptools import setup


PACKAGES = []
PY_MODULES = ["distro_info"]
SCRIPTS = ["debian-distro-info", "ubuntu-distro-info"]


def get_debian_version():
    """look what Debian version we have"""
    version = None
    changelog = "../debian/changelog"
    if os.path.exists(changelog):
        head = open(changelog, "rb").readline().decode("utf-8")
        match = re.compile(r".*\((.*)\).*").match(head)
        if match:
            version = match.group(1)
    return version


def make_pep440_compliant(version: str) -> str:
    """Convert the version into a PEP440 compliant version."""
    public_version_re = re.compile(r"^([0-9][0-9.]*(?:(?:a|b|rc|.post|.dev)[0-9]+)*)\+?")
    _, public, local = public_version_re.split(version, maxsplit=1)
    if not local:
        return version
    sanitized_local = re.sub("[+~]+", ".", local).strip(".")
    pep440_version = f"{public}+{sanitized_local}"
    assert re.match("^[a-zA-Z0-9.]+$", sanitized_local), f"'{pep440_version}' not PEP440 compliant"
    return pep440_version


if __name__ == "__main__":
    setup(
        name="distro-info",
        version=make_pep440_compliant(get_debian_version()),
        py_modules=PY_MODULES,
        packages=PACKAGES,
        test_suite="distro_info_test",
    )
