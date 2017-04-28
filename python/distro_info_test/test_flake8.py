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

"""test_flake8.py - Run flake8 check"""

import subprocess

import setup
from distro_info_test import unittest


class Flake8TestCase(unittest.TestCase):
    def test_flake8(self):
        "Test: Run flake8 on Python source code"
        files = setup.PACKAGES + [m + '.py' for m in setup.PY_MODULES] + ['setup.py']
        for script in setup.SCRIPTS:
            script_file = open(script, 'r')
            if 'python' in script_file.readline():
                files.append(script)
            script_file.close()
        with open('/proc/self/cmdline', 'r') as cmdline_file:
            python_binary = cmdline_file.read().split('\0')[0]
        cmd = [python_binary, '-m', 'flake8', '--max-line-length=99'] + files
        process = subprocess.Popen(cmd, stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE, close_fds=True)

        out, err = process.communicate()
        self.assertFalse(err, "Unexpected standard error from flake8 run:\n" + err.decode())
        self.assertFalse(out, "flake8 found issues:\n" + out.decode())
