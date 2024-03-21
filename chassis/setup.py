#!/usr/bin/env python

import os
import sys
from setuptools import setup
os.listdir

setup(
    name='sonic_platform',
    version='1.0',
    description='Module to initialize Nokia IXR7250 platforms',

    packages=[
        'platform_ndk',
        'platform_tests',
        'sonic_platform',
    ],
    setup_requires = [
        'grpcio',
        'pytest-runner',
        'wheel'
    ],
    tests_require = [
        'pytest',
        'pytest-cov',
    ],
    package_dir={
        'platform_ndk': 'platform_ndk',
        'platform_tests': 'platform_tests',
        'sonic_platform': 'sonic_platform'
    }
)
