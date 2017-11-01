from __future__ import absolute_import

import os
import unittest

from .test_context import TestContext


class TestContextTest(unittest.TestCase):
    def test_test_context(self):
        original_cwd = os.getcwd()
        with TestContext() as context:
            cwd = os.getcwd()
            self.assertNotEqual(original_cwd, cwd)
            self.assertEqual(original_cwd, context.cwd)
            self.assertEqual(cwd, context.temp_path)
            self.assertTrue(cwd.endswith("_unnamed"))
            context_path = context.temp_path

        self.assertFalse(os.path.exists(context_path))
        self.assertEqual(original_cwd, os.getcwd())

    def test_named_test_context(self):
        with TestContext(name="named") as context:
            self.assertTrue(context.temp_path.endswith("_named"))

    def test_test_context_copy(self):
        with TestContext() as context:
            small = "test-data/small.sgy"
            context.copy_file(small)
            context.copy_file(small, "test")

            self.assertTrue(os.path.exists("small.sgy"))
            self.assertTrue(os.path.exists("test/small.sgy"))

if __name__ == '__main__':
    unittest.main()
