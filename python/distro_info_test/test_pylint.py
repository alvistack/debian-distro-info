# test_pylint.py - Run pylint
#
# Copyright (C) 2010, Stefano Rivera <stefanor@debian.org>
# Copyright (C) 2017, Benjamin Drung <bdrung@debian.org>
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
# REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
# AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
# INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
# LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
# OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
# PERFORMANCE OF THIS SOFTWARE.

import subprocess
import sys

import setup
from distro_info_test import unittest


class PylintTestCase(unittest.TestCase):
    def test_pylint(self):
        "Test: Run pylint on Python source code"
        files = setup.PACKAGES + [m + '.py' for m in setup.PY_MODULES] + ['setup.py']
        for script in setup.SCRIPTS:
            script_file = open(script, 'r')
            if 'python' in script_file.readline():
                files.append(script)
            script_file.close()
        if sys.version_info[0] == 3:
            pylint_binary = 'pylint3'
        else:
            pylint_binary = 'pylint'
        cmd = [pylint_binary, '--rcfile=distro_info_test/pylint.conf', '--reports=n', '--'] + files
        process = subprocess.Popen(cmd, env={'PYLINTHOME': '.pylint.d'}, stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE, close_fds=True)

        out, err = process.communicate()
        self.assertFalse(err, pylint_binary + ' crashed. Error output:\n' + err.decode())
        self.assertFalse(out, pylint_binary + " found errors:\n" + out.decode())
