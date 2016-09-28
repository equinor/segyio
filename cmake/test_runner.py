#!/usr/bin/env python
import inspect
import os
import sys

import imp

try:
    from unittest2 import TextTestRunner, TestLoader, TestCase
except ImportError:
    from unittest import TextTestRunner, TestLoader, TestCase


def runTestCase(tests, verbosity=0):
    test_result = TextTestRunner(verbosity=verbosity).run(tests)

    if len(test_result.errors) or len(test_result.failures):
        test_result.printErrors()
        return False
    else:
        return True

def getTestClassFromModule(module_path):
    test_module = imp.load_source('test', module_path)
    for name, obj in inspect.getmembers(test_module):
        if inspect.isclass(obj) and issubclass(obj, TestCase) and not obj == TestCase:
            return obj

def getTestsFromModule(module_path):
    klass = getTestClassFromModule(module_path)
    if klass is None:
        raise UserWarning("No tests classes found in: '%s'" % module_path)
    
    loader = TestLoader()
    return loader.loadTestsFromTestCase(klass)
            

if __name__ == '__main__':
    test_module = sys.argv[1]
    argv = []

    tests = getTestsFromModule(test_module)

    # Set verbosity to 2 to see which test method in a class that fails.
    if runTestCase(tests, verbosity=0):
        sys.exit(0)
    else:
        sys.exit(1)
