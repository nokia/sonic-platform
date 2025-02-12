#!/usr/bin/env python

import os
import sys
from setuptools import setup
os.listdir

setup(
    name='sonic_platform',
    version='1.0',
    description='Module to initialize Nokia 7250 IXR X3b platforms',

    packages=[        
        'sonic_platform',
        'sonic_platform.test'
    ],
      
    package_dir={        
        'sonic_platform': 'sonic_platform'
    }
)
