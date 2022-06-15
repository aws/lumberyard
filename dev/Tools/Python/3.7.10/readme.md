Python VM 3.7.5

This Python VM is meant to be used through out the Lumberyard ecosystem of tools and code.

# Python Packages

The packages were downloaded from repo.anaconda.com since the packages were pre-compiled for each platform.

## Linux
- PACKAGE: https://repo.anaconda.com/pkgs/main/linux-64/python-3.7.5-h0371630_0.tar.bz2
  - SHA256: 7c86aee6e00fa3d420a96ceb97cc2b88d7b6dda19d634f0b461dcbe1cd91c9b5

- OPENSSL: https://repo.anaconda.com/pkgs/main/linux-64/openssl-1.1.1d-h7b6447c_3.tar.bz2
  - SHA256: faceb4fecd07a79af6a2c03540337dbe3b02a4b24776b09306808c7032367368

## Mac
- Built from the source at python.org from 3.7.10, using the mac package script included in python.

## Win64
- Built from the source at python.org from 3.7.10, using the windows build script included in python.

# The Launchers

The Python launch scripts are used to uniformly execute Python 3.7.5/3.7.10 using the same command line form.

## python3 
- used to launch the Python VM or executes a Python script in Python 3.7.5/3.7.10
- python3.cmd is for Windows use
- python3.sh is for Linux or OSX use

## pip3
- used to run the "Pip Installs Packages" Python package installer
- pip3.cmd is for Windows use (needs libcrypto-1_1-x64.dll & libssl-1_1-x64.dll)
- pip3.sh is for Linux or OSX use

# Pip Install
- https://pypi.org/simple/setuptools/setuptools-41.6.0-py2.py3-none-any.whl
- https://pypi.org/simple/pip/pip-19.3.1-py2.py3-none-any.whl
