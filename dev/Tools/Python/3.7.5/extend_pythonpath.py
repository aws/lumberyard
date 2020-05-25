"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

Prints PYTHONPATH with additional include paths to stdout. Values are separated by a semicolon on Windows, else colon.
"""

import os
import platform
import sys


def main():
    if sys.platform.startswith('win'):
        os_folder = "windows"
    elif sys.platform.startswith('darwin'):
        os_folder = "mac"
    elif sys.platform.startswith('linux'):
        os_folder = "linux_x64"
    else:
        raise RuntimeError(f"Unexpectedly executing on operating system '{sys.platform}'")

    current_pythonpath = os.environ.get("PYTHONPATH")
    if current_pythonpath:
        new_paths = [current_pythonpath]
    else:
        new_paths = []
    parent_directory = os.path.dirname(os.path.abspath(__file__))
    internal_site_packages = os.path.join(parent_directory, "internal", "site-packages", os_folder)
    if os.path.exists(internal_site_packages):
        new_paths.append(internal_site_packages)

    print(os.pathsep.join(new_paths))


if __name__ == "__main__":
    sys.exit(main())
