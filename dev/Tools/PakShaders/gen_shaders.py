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
import os, argparse, shutil, subprocess, importlib

class _Platform:
    def __init__(self, platform, compiler):
        self.name = platform
        self.compiler =  compiler

class _ShaderType:
    def __init__(self, platform):
        self.type = platform
        self.platforms = []

    def add_platform(self, platform, compiler):
        self.platforms.append(_Platform(platform, compiler))

def error(msg):
    print msg
    exit(1)

def is_windows():
    if os.name == 'nt':
        return True
    else:
        return False

def host_platform():
    if is_windows():
        return 'pc'
    else:
        return 'osx_gl'

def remove_dir(dirPath):
    try:
        if os.path.isdir(dirPath):
            # shutil.rmtree sometimes fails on the build node, so using the system call instead.
            if is_windows():
                os.system('rmdir /S /Q \"{}\"'.format(dirPath))
            else:
                os.system('rm -rf \"{}\"'.format(dirPath))

        if os.path.isdir(dirPath):
            raise
    except:
        error("Failed to delete directory {}".format(dirPath))

def gen_shaders(game_name, shader_type, platform, compiler, shader_list, bin_folder, game_path, engine_path):
    """
    Generates the shaders for a specific platform and shader type using a list of shaders using ShaderCacheGen.
    The generated shaders will be output at Cache/<game_name>/<host_platform>/user/cache/Shaders/Cache/<shader_type>
    """
    shader_list_path = shader_list
    cache_shader_list = os.path.join(game_path, 'Cache', game_name, platform, 'user', 'cache', 'shaders', 'shaderlist.txt')

    if shader_list is None:
        if is_windows():
            shader_compiler_platform = 'x64'
        else:
            shader_compiler_platform = 'osx'

        shader_list_path = os.path.join(engine_path, 'Tools', 'CrySCompileServer', shader_compiler_platform, 'profile', 'Cache', game_name, compiler, 'ShaderList_{}.txt'.format(shader_type))
        if not os.path.isfile(shader_list_path):
            shader_list_path = cache_shader_list

        print "Source Shader List not specified, using {} by default".format(shader_list_path)

    if os.path.realpath(shader_list_path) != os.path.realpath(cache_shader_list):
        cache_shader_list_basename = os.path.split(cache_shader_list)[0]
        if not os.path.exists(cache_shader_list_basename):
            os.makedirs(cache_shader_list_basename)
        print "Copying shader_list from {} to {}".format(shader_list_path, cache_shader_list)
        shutil.copy2(shader_list_path, cache_shader_list)

    platform_shader_cache_path = os.path.join(game_path, 'Cache', game_name, platform, 'user', 'cache', 'shaders', 'cache', shader_type.lower())
    remove_dir(platform_shader_cache_path)

    shadergen_path = os.path.join(game_path, bin_folder, 'ShaderCacheGen')
    if is_windows():
        shadergen_path += '.exe'

    if not os.path.isfile(shadergen_path):
        error("ShaderCacheGen could not be found at {}".format(shadergen_path))
    else:
        subprocess.call([shadergen_path, '/BuildGlobalCache', '/ShadersPlatform={}'.format(shader_type), '/TargetPlatform={}'.format(platform)])

def add_shaders_types():
    """
    Add the shader types for the non restricted platforms.
    The compiler argument is used for locating the shader_list file.
    """
    shaders = []
    d3d11 = _ShaderType('D3D11')
    d3d11.add_platform('pc', 'PC-D3D11_FXC-D3D11')

    gles3 = _ShaderType('GLES3')
    gles3.add_platform('es3', 'Android-GLSL_HLSLcc-GLES3')

    metal = _ShaderType('METAL')
    metal.add_platform('osx_gl', 'Mac-METAL_LLVM_DXC-METAL')
    metal.add_platform('ios', 'iOS-METAL_LLVM_DXC-METAL')

    shaders.append(d3d11)
    shaders.append(gles3)
    shaders.append(metal)

    return shaders

def add_restricted_platform_shader(restricted_platform, shader_types):
    """
    Load the shader types of a restricted platform if available.
    """
    path, filename = os.path.split(os.path.realpath(__file__))
    restricted_platform_path = os.path.join(path, restricted_platform, filename)
    if os.path.isfile(restricted_platform_path):
        module_name = '{}.{}'.format(restricted_platform.lower(), os.path.splitext(filename)[0])
        try:
            module = importlib.import_module(module_name)
        except:
            error("Failed to import restricted platform module {}".format(module_name))

        # Load the shader types for the restricted platform.
        for new_shader in module.get_restricted_platform_shader():
            if len(new_shader) == 3:
                new_shader_type = new_shader[0]
                platform_name = new_shader[1]
                compiler= new_shader[2]

                shader = next((shader_type for shader_type in shader_types if shader_type.type == new_shader_type), None)
                if shader is None:
                    shader = _ShaderType(new_shader_type)
                shader.add_platform(platform_name, compiler)
                shader_types.append(shader)
            else:
                print "Invalid restricted platform {}".restricted_platform

def check_arguments(args, parser, shader_types):
    """
    Check that the platform and shader type arguments are correct.
    """
    shader_names = [shader.type for shader in shader_types]

    shader_found = next((shader for shader in shader_types if shader.type == args.shader_type), None)
    if shader_found is None:
        parser.error('Invalid shader type {}. Must be one of [{}]'.format(args.shader_type, ' '.join(shader_names)))
    else:
        platform_found = next((platform for platform in shader_found.platforms if platform.name == args.platform), None)
        if platform_found is None:
            parser.error('Invalid platform for shader type "{}". It must be one of the following [{}]'.format(shader_found.type, ' '.join(platform.name for platform in shader_found.platforms)))            

shader_types = []
shader_types = add_shaders_types()

parser = argparse.ArgumentParser(description='Generates the shaders for a specific platform and shader type.')
parser.add_argument('game_name', type=str, help="Name of the game")
parser.add_argument('platform', type=str, help="Asset platform to use")
parser.add_argument('shader_type', type=str, help="The shader type to use")
parser.add_argument('bin_folder', type=str, help="Folder where the ShaderCacheGen executable lives. This is used along the game_path (game_path/bin_folder/ShaderCacheGen)")
parser.add_argument('-g', '--game_path', type=str, required=False, default=os.getcwd(), help="Path to the game root folder. This the same as engine_path for non external projects. Default is current directory.")
parser.add_argument('-e', '--engine_path', type=str, required=False, default=os.getcwd(), help="Path to the engine root folder. This the same as game_path for non external projects. Default is current directory.")
parser.add_argument('-s', '--shader_list', type=str, required=False, help="Optional path to the list of shaders. If not provided will use the list generated by the local shader compiler.")

args = parser.parse_args()
add_restricted_platform_shader(args.shader_type, shader_types)

check_arguments(args, parser, shader_types)

compiler = next((platform.compiler for shader in shader_types if shader.type == args.shader_type for platform in shader.platforms if platform.name == args.platform), None)
print '---- Generating shaders for {}, shader={}, platform={}----'.format(args.game_name, args.shader_type, args.platform)
gen_shaders(args.game_name, args.shader_type, args.platform, compiler, args.shader_list, args.bin_folder, args.game_path, args.engine_path)
print '---- Finish generating shaders -----'
