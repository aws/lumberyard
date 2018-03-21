"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.



Lumberyard 1.13 "updateMaterials = false" Script


Why do I need to run this script?
================================================
DCC Material Save Thread is a background thread that creates source .mtl files for FBX files that don't have a
corresponding source material, or updates source .mtl files when the .fbx has changed (and hs a setting telling to
update the source file). This includes a one-time update for every existing .mtl file that has the "updateMaterials"
setting set to "true". Which adds the DcMaterialHash node to the .mtl file that can be later used to identity whether
or not the material need to be updated. Part of that process is to automatically check out the .mtl files.

How do I run this script from a command line?
================================================
1) Open a command prompt and navigate to your build's python folder:
    - \dev\Tools\Python\
2) Type in the following and press enter:
    - python <your_build_path>\dev\Editor\Scripts\Lumberyard\1.13_assetinfo_updateMaterials.py
3) Follow the instructions presented.


As the script runs, it will recursively scan the target folder looking
for *.assetinfo files. It will then parse the file and find updateMaterials field,
set the value from "false" to "true".
"""

import os
import stat
import subprocess
from glob import glob
import xml.etree.ElementTree as et


targetDirectory = raw_input("\nPlease input target folder: ")
list = targetDirectory+"\updateMaterials_list.log"
directory = []
filename = []

# Glob all .assetinfo files recursively in the given directory
assetinfo_fileGlob = []
print "Gathering files..."
for dirs in os.walk(targetDirectory):
    for filename in glob(os.path.join(dirs[0], '*.assetinfo')):
        assetinfo_fileGlob.append(filename)

def getFilelist():
    if os.path.exists(list):
        os.remove(list)
    fileWrite = open(list, "a")
    print "File(s) to be processed is written here:", list
    for filename in assetinfo_fileGlob:
        tree = et.parse(filename)
        root = tree.getroot()
        for element in root.findall(".//Class[@field='updateMaterials']"):
            if element.attrib['value']=="true":
                print filename
                fileWrite.write(filename+"\n")
    fileWrite.close()


def updateAssetinfo():
  for filename in assetinfo_fileGlob:
      tree = et.parse(filename)
      root = tree.getroot()
      for element in root.findall(".//Class[@field='updateMaterials']"):
        if element.attrib['value']=="true":
            element.set("value", "false")
            tree.write(filename)


def chmodAssetinfo():
    for filename in assetinfo_fileGlob:
        tree = et.parse(filename)
        root = tree.getroot()
        for element in root.findall(".//Class[@field='updateMaterials']"):
            if element.attrib['value']=="true":
                print "Make writable:", filename
                os.chmod(filename, stat.S_IWRITE)

def main():
    getFilelist()
    if os.stat(list).st_size != 0:
        options = raw_input("\n0. Only save out the files list."
                            "\n1. Check out via Perforce and upgrade."
                            "\n2. Make file(s) writeable and upgrade."
                            "\n\nYour choice: ")
        if options == "0":
            print "The .assetfino files need to be processed saved out file here:", list
        if options == "1":
            print list
            p4Edit = "p4 -x "+list+" edit"
            subprocess.call(p4Edit)
            updateAssetinfo()
        if options == "2":
            chmodAssetinfo()
            updateAssetinfo()
    else:
        print "There is no .assetfinfo to be updated!"

main()