import argparse
import os
import shutil
import tarfile

script_directory = os.path.dirname(os.path.abspath(__file__))

fbx_tgz_filename = 'fbx20161_2_fbxsdk_clang_mac.pkg.tgz'
fbx_tgz_filepath = os.path.join(script_directory, fbx_tgz_filename)

fbx_pkg_filename = 'fbx20161_2_fbxsdk_clang_macos.pkg'
fbx_pkg_filepath = os.path.join(script_directory, fbx_pkg_filename)

parser = argparse.ArgumentParser(description='Install and setup FBXSDK on the Mac for Lumberyard.')

parser.add_argument('fbxthirdpartypath',
    default='',
    help='Path to your 3rdParty folder.')
parser.add_argument('--installerPath', 
    default=fbx_tgz_filepath,
    help='The path to the installer archive file.')

args, unknown = parser.parse_known_args()

installer_filepath = args.installerPath

if os.path.exists(installer_filepath) is False:
    raise Exception('Cannot find file: {}'.format(installer_filepath))

_, file_extension = os.path.splitext(installer_filepath)

# Unzip if input is a tgz file
if file_extension == '.tgz':
    tar = tarfile.open(installer_filepath)
    tar.extractall(script_directory)
    tar.close()
    installer_filepath = fbx_pkg_filepath

if not os.path.exists(installer_filepath) or not installer_filepath.endswith('.pkg'):
    raise Exception('Invalid file extracted: {}'.format(installer_filepath))

# Remove quarantine mark on the installer file before opening it
os.system('xattr -dr com.apple.quarantine {}'.format(installer_filepath))

# Run the installer package
os.system('open -W {}'.format(installer_filepath))

# Clean up extracted file if input is a tgz file
if file_extension == '.tgz':
    shutil.rmtree(installer_filepath)

# This points to the default location where the pkg installs the files
installed_path = os.path.join(os.path.abspath(os.sep), 'Applications', 'Autodesk', 'FBX SDK', '2016.1.2') 

if os.path.exists(installed_path) is False:
    raise Exception('Failed to install FBX SDK, cannot find installed files: {}'.format(installed_path))

# Move the installed files to the 3rdParty location
shutil.move(installed_path, args.fbxthirdpartypath)
