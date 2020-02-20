Python VM 3.7.5

This Python VM is meant to be used through out the Lumberyard ecosystem of tools and code.

# Python Packages

The packages where downloaded from repo.anaconda.com since the packages where pre-compiled for each platform.

## Linux
- PACKAGE: https://repo.anaconda.com/pkgs/main/linux-64/python-3.7.5-h0371630_0.tar.bz2
  - SHA256: 7c86aee6e00fa3d420a96ceb97cc2b88d7b6dda19d634f0b461dcbe1cd91c9b5

- OPENSSL: https://repo.anaconda.com/pkgs/main/linux-64/openssl-1.1.1d-h7b6447c_3.tar.bz2
  - SHA256: faceb4fecd07a79af6a2c03540337dbe3b02a4b24776b09306808c7032367368

## Mac
- PACKAGE: https://repo.anaconda.com/pkgs/main/osx-64/python-3.7.5-h359304d_0.tar.bz2
  - SHA256: 75ddfa5923ad4fe6580b4eba86cf46f7e0f1c21c691ad35d81d7d24d030e8f6f

## Win64
- PACKAGE: https://repo.anaconda.com/pkgs/main/win-64/python-3.7.5-h8c8aaf0_0.tar.bz2
  - SHA256: e3ce28504a05be6aca91b92625861b54a411d7871e3517c6b0b448170977a454
- OPENSSL: https://repo.anaconda.com/pkgs/main/win-64/openssl-1.1.1d-he774522_3.tar.bz2
  - SHA256: f07b079d819d3ec6fdddd7cb43f54688ed49a6b42c7fb14fa02ac74ef1e7c390

# The Launchers

The Python launch scripts are used to uniformly execute Python 3.7.5 using the same command line form.

## python3 
- used to launch the Python VM or executes a Python script in Python 3.7.5
- python3.cmd is for Windows use
- python3.sh is for Linux or OSX use

## pip3
- used to run the "Pip Installs Packages" Python package installer
- pip3.cmd is for Windows use (needs libcrypto-1_1-x64.dll & libssl-1_1-x64.dll)
- pip3.sh is for Linux or OSX use

# Pip Instal
- https://pypi.org/simple/setuptools/setuptools-41.6.0-py2.py3-none-any.whl
- https://pypi.org/simple/pip/pip-19.3.1-py2.py3-none-any.whl
