#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#
'''
This script loads every level in a game project and exports them.  You will be prompted with the standard Check-Out/Overwrite/Cancel dialog for each level.
This is useful when there is a version update to a file that requires re-exporting.
'''
import sys, os

level_list = []

game_folder = general.get_game_folder()

# Recursively search every directory in the game project for files ending with .cry.
for root, dirs, files in os.walk(game_folder):
	for file in files:
		if file.endswith(".cry"):
			# The engine expects the full path of the .cry file
			file_full_path = os.path.abspath(os.path.join(root, file))
			# Exclude files in the "_savebackup" directories
			is_save_backup = file_full_path.find("_savebackup")
			if is_save_backup == -1:
				level_list.append(file_full_path)

# For each valid .cry file found, open it in the editor and export it
for level in level_list:
	general.open_level_no_prompt(level)
	general.export_to_engine()