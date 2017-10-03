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
# $Revision: #1 $

from __future__ import print_function

import os
import bootstrap


cached_root_directory_path = None
cached_game_directory_path = None
cached_gui_module = None

def execute(command, args):
    
    global cached_root_directory_path, cached_game_directory_path, cached_gui_module

    request_id = args.get('request_id', 0)

    try:

        args_root_directory = os.path.abspath(args['root_directory'])
        args_game_directory = os.path.abspath(args['game_directory'])

        if cached_gui_module is None:

            cached_root_directory_path = args_root_directory
            cached_game_directory_path = args_game_directory

            framework_directory_path = bootstrap.get_framework_directory_path(cached_root_directory_path, cached_game_directory_path)
            if framework_directory_path is None:
                # Handle this error as a special case so that it doesn't get logged to the
                # Editor console when a project that isn't using Cloud Canvas is lodaded.
                args['view_output_function'](request_id, 'framework-not-enabled-error', 'The gems.json file at {} does not contain an entry for the CloudGemFramework gem. You must enable this gem for your project. You can do that using the File, Project Settings, Configure Gems menu item.'.format(cached_game_directory_path))
                return

            cached_gui_module = bootstrap.load_resource_manager_module(framework_directory_path, 'resource_manager.gui')

        else:

            if args_root_directory != cached_root_directory_path:
                raise RuntimeError(
                    'The root directory path changed from {} to {}. You need to restart the Lumberyard Editor.'.format(
                        root_directory_path,
                        args_root_directory
                    )
                )
        
            if args_game_directory != cached_game_directory_path:
                raise RuntimeError(
                    'The game directory path changed from {} to {}. You need to restart the Lumberyard Editor.'.format(
                        game_directory_path,
                        args_game_directory
                    )
                )
        
        cached_gui_module.execute(command, args)

    except RuntimeError as e:
        args['view_output_function'](request_id, 'error', e.message)    

    
