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

import argparse
import os
import sys

import bootstrap

def main():
    try:
        framework_directory_path = get_framework_directory_path()
        cli = bootstrap.load_resource_manager_module(framework_directory_path, 'resource_manager.cli')
        return cli.main()
    except RuntimeError as e:
        print('\nERROR: ' + e.message);

    
def get_framework_directory_path():
    '''Determines the CloudGemFramework directory to use based on select command line arguments and the game project's gems.json file.'''
    
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument('--root-directory', default=os.getcwd(), help='Lumberyard install directory and location of bootstrap.cfg file. Default is the current working directory.')
    parser.add_argument('--game-directory', help='Location of the game project directory. The default is {root-directory}\{game} where {game} is determined by the sys_game_folder setting in the {root-directory}\bootstrap.cfg file.')
    parser.add_argument('--framework-directory', help='Specify the CloudGemFramework version used to execute the command. The default is determined by examining the gems.json file for the project identified by the bootstrap.cfg file.')
    args, unknown_args = parser.parse_known_args()

    if args.framework_directory:

        i = sys.argv.index('--framework-directory')
        sys.argv[i:i+2] = [] # remove arg because cli in resource manager doesn't know about it.

        return args.framework_directory
    
    else:
    
        if args.game_directory:
    
            game_directory_path = os.path.abspath(args.game_directory)
            if not os.path.isdir(game_directory_path):
                raise RuntimeError('The specified game directory does not exist: {}.'.format(game_directory_path))
            
        else:
    
            game_directory_path = bootstrap.get_game_directory_path(args.root_directory)

        framework_directory_path = bootstrap.get_framework_directory_path(args.root_directory, game_directory_path)
        if framework_directory_path is None:
            raise RuntimeError('The gems.json file at {} does not contain an entry for the CloudGemFramework gem. You must enable this gem for your project.'.format(game_directory_path))
        return framework_directory_path
    
if __name__ == "__main__":
    sys.exit(main())
