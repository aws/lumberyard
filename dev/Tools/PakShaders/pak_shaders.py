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
import os, fnmatch, sys
import argparse, zipfile, re

def _create_zip(source_path, zip_file_path, append, filter_list, ignore_list, compression_level, zip_internal_path = ''):
    """
    Internal function for creating a zip file containing the files that match the list of filters provided.
    File matching is case insensitive.
    """
    ignore_list = [filter.lower() for filter in ignore_list]
    filter_list = [filter.lower() for filter in filter_list]
    
    ignore_list = r'|'.join([fnmatch.translate(x) for x in ignore_list])
    try:
        mode = 'a' if append else 'w'        
        with zipfile.ZipFile(zip_file_path, mode, compression_level) as myzip:
            # Go through all the files in the source path.
            for root, dirnames, filenames in os.walk(source_path):
                # Remove files that match the ignore list.
                files = [os.path.relpath(os.path.join(root, file), source_path).lower() for file in filenames if not re.match(ignore_list, file.lower())]
                # Match the files we have found against the filters.
                for filter in filter_list:
                    for filename in fnmatch.filter(files, filter):
                        # Add the file to the zip using the specified internal destination.
                        myzip.write(os.path.join(source_path, filename), os.path.join(zip_internal_path, filename))
    except IOError as error:
       print "I/O error({0}) while creating zip file {1}: {2}".format(error.errno, zip_file_path, error.strerror)
       return False
    except:
       print "Unexpected error while creating zip file {0}: {1}".format(zip_file_path, sys.exc_info()[0])
       return False

    return True

def pak_shaders(source_path, output_folder, shader_types):
    """
    Creates the shadercache.pak and the shadercachestartup.pak using the shader files located at source_path.
    """
    if not os.path.exists(output_folder):
        os.makedirs(output_folder)

    ignore_list = ['shaderlist.txt', 'shadercachemisses.txt']
    shaders_cache_startup_filters = ['Common.cfib', 'FXConstantDefs.cfib', 'FXSamplerDefs.cfib', 'FXSetupEnvVars.cfib',
                                     'FXStreamDefs.cfib', 'fallback.cfxb', 'FixedPipelineEmu.cfxb','Scaleform.cfxb',
                                     'Stereo.cfxb', 'lookupdata.bin',
                                     os.path.join('CGPShaders', 'Scaleform@*'),
                                     os.path.join('CGVShaders', 'Scaleform@*'),
                                     os.path.join('CGPShaders', 'Scaleform', '*'),
                                     os.path.join('CGVShaders', 'Scaleform', '*'),
                                     os.path.join('CGPShaders', 'FixedPipelineEmu@*'),
                                     os.path.join('CGVShaders', 'FixedPipelineEmu@*'),
                                     os.path.join('CGPShaders', 'FixedPipelineEmu', '*'),
                                     os.path.join('CGVShaders', 'FixedPipelineEmu', '*'),
                                     os.path.join('CGPShaders', 'Stereo@*'),
                                     os.path.join('CGVShaders', 'Stereo@*'),
                                     os.path.join('CGPShaders', 'Stereo', '*'),
                                     os.path.join('CGVShaders', 'Stereo', '*')]

    result = True
    shader_flavors_packed = 0
    for shader_info in shader_types:
        # First element is the type, the second (if present) is the specific source
        shader_type = shader_info[0]
        # We want the files to be added to the "shaders/cache/$shader_type" path inside the pak file.
        zip_interal_path = os.path.join('shaders', 'cache', shader_type)
        if len(shader_info) > 1:
            shader_type_source = shader_info[1]
        else:
            # No specific source path for this shader type so use the global source path
            shader_type_source = os.path.join(source_path, shader_type)
           
        if os.path.exists(shader_type_source):
            # First shader flavor creates a new zip file. The next ones just add files to the zip.
            append = shader_flavors_packed > 0
            result &= _create_zip(shader_type_source, os.path.join(output_folder, 'shadercache.pak'), append, ['*.*'], ignore_list, zipfile.ZIP_STORED, zip_interal_path)
            result &= _create_zip(shader_type_source, os.path.join(output_folder, 'shadercachestartup.pak'), append, shaders_cache_startup_filters, ignore_list, zipfile.ZIP_STORED, zip_interal_path)
            shader_flavors_packed = shader_flavors_packed + 1
        else:
            print '[Warn] Skipping shader type %s because source folder is not available at %s' % (shader_type, shader_type_source)

    if shader_flavors_packed == 0:
        print 'Failed to pack any shader type'

    return shader_flavors_packed > 0 and result

def pair_arg(arg):
    return [str(x) for x in arg.split(',')]

parser = argparse.ArgumentParser(description='Pack the provided shader files into paks.')
parser.add_argument("output", type=str, help="specify the output folder")
parser.add_argument('-r', '--source', type=str, required=False, help="specify global input folder")
parser.add_argument('-s', '--shaders_types', required=True, nargs='+', type=pair_arg,
                    help='list of shader types with optional source path')

args = parser.parse_args()
print 'Packing shaders...'
if pak_shaders(args.source, args.output, args.shaders_types):
    print '---- Finish packing shaders -----'
    print 'Packs have been placed at "' + args.output + '"'
    print 'To use them, deploy them in your assets folder.'
    exit(0)
else:
    print '--- Failed to pack shaders ----'
    exit(1)

