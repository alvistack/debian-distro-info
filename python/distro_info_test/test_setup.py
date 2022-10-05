# Copyright (C) 2022, Benjamin Drung <bdrung@debian.org>
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

"""Test functions in setup.py."""

import unittest
import unittest.mock

from setup import get_pep440_version


class SetupTestCase(unittest.TestCase):
    """Test functions in setup.py."""

    @unittest.mock.patch("setup.get_debian_version")
    def test_get_pep440_version_unchanged(self, debian_version_mock):
        """Test get_pep440_version() for version '1.2'."""
        debian_version_mock.return_value = "1.2"
        self.assertEqual(get_pep440_version(), "1.2")
        debian_version_mock.assert_called_once_with()

    @unittest.mock.patch("setup.get_debian_version")
    def test_get_pep440_version_ubuntu(self, debian_version_mock):
        """Test get_pep440_version() for version '1.1ubuntu1'."""
        debian_version_mock.return_value = "1.1ubuntu1"
        self.assertEqual(get_pep440_version(), "1.1+ubuntu1")
        debian_version_mock.assert_called_once_with()
