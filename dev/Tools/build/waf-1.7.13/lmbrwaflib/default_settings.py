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
# Original file Copyright Crytek GMBH or its affiliates, used under license.
#
from waflib.Configure import conf
from waflib import Context
from waflib import Logs
from waflib import Build
from waflib import Options
from waflib import Utils

from collections import Counter
from incredibuild import internal_validate_incredibuild_registry_settings,\
                         internal_use_incredibuild,\
                         internal_verify_incredibuild_license

import ConfigParser 
import sys
import os
import multiprocessing

user_settings = None

ATTRIBUTE_CALLBACKS = {}
""" Global Registry of callbacks for options which requires special processing """

ATTRIBUTE_VERIFICATION_CALLBACKS = {}
""" Global Registry of callbacks for options which requires special processing """

ATTRIBUTE_HINT_CALLBACKS = {}
""" Global Registry of callbacks for options which requires special processing """

LOADED_OPTIONS = {}
""" Global List of already loaded options, can be used to skip initializations if a master value is set to False """

def register_attribute_callback(f):
    """ 
    Decorator function to register a callback for an options attribute.
    *note* The callback function must have the same name as the attribute *note*
    """
    ATTRIBUTE_CALLBACKS[f.__name__] = f
    
def register_verify_attribute_callback(f):
    """ 
    Decorator function to register a callback verifying an options attribute.
    *note* The callback function must have the same name as the attribute *note*
    """
    ATTRIBUTE_VERIFICATION_CALLBACKS[f.__name__] = f
    
def register_hint_attribute_callback(f):
    """ 
    Decorator function to register a callback verifying an options attribute.
    *note* The callback function must have the same name as the attribute *note*
    """
    ATTRIBUTE_HINT_CALLBACKS[f.__name__] = f

def _is_user_option_true(value):
    """ Convert multiple user inputs to True, False or None """
    value = str(value)
    if value.lower() == 'true' or value.lower() == 't' or value.lower() == 'yes' or value.lower() == 'y' or value.lower() == '1':
        return True
    if value.lower() == 'false' or value.lower() == 'f' or value.lower() == 'no' or value.lower() == 'n' or value.lower() == '0':
        return False
        
    return None
    
def _is_user_input_allowed(ctx, option_name, value):
    """ Function to check if it is currently allowed to ask for user input """                      
    # If we run a no input build, all new options must be set
    if not ctx.is_option_true('ask_for_user_input'):    
        if value != '':
            return False # If we have a valid default value, keep it without user input
        else:
            ctx.fatal('No valid default value for %s, please provide a valid input on the command line' % option_name)

    return True
    
    
def _get_string_value(ctx, msg, value):
    """ Helper function to ask the user for a string value """
    msg += ' '
    while len(msg) < 53:
        msg += ' '
    msg += '['+value+']: '
    
    user_input = raw_input(msg)
    if user_input == '':    # No input -> return default
        return value
    return user_input

    
def _get_boolean_value(ctx, msg, value):
    """ Helper function to ask the user for a boolean value """
    msg += ' '
    while len(msg) < 53:
        msg += ' '
    msg += '['+value+']: '
    
    while True:
        user_input = raw_input(msg)
        
        # No input -> return default
        if user_input == '':    
            return value
            
        ret_val = _is_user_option_true(user_input)
        if ret_val != None:
            return str(ret_val)
            
        Logs.warn('Unknown input "%s"\n Acceptable values (none case sensitive):' % user_input)
        Logs.warn("True : 'true'/'t' or 'yes'/'y' or '1'")
        Logs.warn("False: 'false'/'f' or 'no'/'n' or '0'")


def _default_settings_node(ctx):
    """ Return a Node object pointing to the defaul_settings.json file """
    return ctx.root.make_node(Context.launch_dir).make_node('_WAF_/default_settings.json')
    
    
def _load_default_settings_file(ctx):
    """ Util function to load the default_settings.json file and cache it within the build context """
    if hasattr(ctx, 'default_settings'):
        return
        
    default_settings_node = _default_settings_node(ctx) 
    ctx.default_settings = ctx.parse_json_file(default_settings_node)       






@register_attribute_callback
def use_incredibuild(ctx, section_name, option_name, value):
    """ If Incredibuild should be used, check for required packages """
    return internal_use_incredibuild(ctx, section_name, option_name, value, ATTRIBUTE_VERIFICATION_CALLBACKS['verify_use_incredibuild'])


@register_attribute_callback
def incredibuild_profile(ctx, section_name, option_name, value):
    """ IB profile file """
    return value


@register_verify_attribute_callback
def verify_incredibuild_profile(ctx, option_name, value):

    if value=="":
        return (True,"","")
    abspath = os.path.abspath(value)
    if not os.path.exists(abspath):
        return (False,"Incredibuild profile file '{}' does not exist, Incredibuild will use the default values".format(abspath),"")
    return (True,"","")


@register_verify_attribute_callback
def verify_use_incredibuild(ctx, option_name, value):
    """ Verify value for use_incredibuild """
    if not _is_user_option_true(value):
        return (True,"","")
    if (ctx.is_option_true('internal_dont_check_recursive_execution')):  # if we're ALREADY inside an incredibuild invocation there is little point to checking this.
        return (True, "", "")
    (res, warning, error) = internal_verify_incredibuild_license('Make && Build Tools', 'All Platforms')
    return (res, warning, error)


def _output_folder_disclaimer(ctx):
    """ Helper function to show a disclaimer over incredibuild before asking for settings """
    if getattr(ctx, 'output_folder_disclaimer_shown', False):
        return
    if not Utils.unversioned_sys_platform() == 'win32':
        return      
    Logs.info("\nPlease specify the output folder for the different build Configurations")
    Logs.info("(Press ENTER to keep the current default value shown in [])")
    ctx.output_folder_disclaimer_shown = True

def _auto_populate_windows_kit(ctx, option_name, compiler):
    wsdk_version = ''
    try:
        wsdk_version = ctx.find_valid_wsdk_version()
        if wsdk_version == '':
            Logs.info('\nFailed to find a valid Windows Kit for {}.'.format(compiler))
        setattr(ctx.options, option_name, wsdk_version)
    except:
        pass
    return wsdk_version

###############################################################################
@register_attribute_callback
def win_vs2017_winkit(ctx, section_name, option_name, value):
    """ Configure vs2017 windows kit. """
    return _auto_populate_windows_kit(ctx, option_name, "Visual Studio 2017")


###############################################################################
@register_attribute_callback
def win_vs2015_winkit(ctx, section_name, option_name, value):
    """ Configure vs2015 windows kit. """
    return _auto_populate_windows_kit(ctx, option_name, "Visual Studio 2015")


###############################################################################
@register_attribute_callback
def out_folder_win32(ctx, section_name, option_name, value):
    """ Configure output folder for win x86 """
    if not _is_user_input_allowed(ctx, option_name, value):
        Logs.info('\nUser Input disabled.\nUsing default value "%s" for option: "%s"' % (value, option_name))
        return value

    # GUI
    if not ctx.is_option_true('console_mode'):
        return ctx.gui_get_attribute(section_name, option_name, value)
        
    _output_folder_disclaimer(ctx)
    return _get_string_value(ctx, 'Win x86 Output Folder', value)


###############################################################################
@register_attribute_callback
def out_folder_win64_vs2017(ctx, section_name, option_name, value):
    """ Configure output folder for win x64 (vs2017) """
    if not _is_user_input_allowed(ctx, option_name, value):
        Logs.info('\nUser Input disabled.\nUsing default value "%s" for option: ""%s"' % (value, option_name))
        return value
        
    # GUI
    if not ctx.is_option_true('console_mode'):
        return ctx.gui_get_attribute(section_name, option_name, value)
        
    _output_folder_disclaimer(ctx)
    return _get_string_value(ctx, 'Win x64 Visual Studio 2017 Output Folder', value)


###############################################################################
@register_attribute_callback
def out_folder_win64_vs2015(ctx, section_name, option_name, value):
    """ Configure output folder for win x64 (vs2015) """
    if not _is_user_input_allowed(ctx, option_name, value):
        Logs.info('\nUser Input disabled.\nUsing default value "%s" for option: ""%s"' % (value, option_name))
        return value
        
    # GUI
    if not ctx.is_option_true('console_mode'):
        return ctx.gui_get_attribute(section_name, option_name, value)
        
    _output_folder_disclaimer(ctx)
    return _get_string_value(ctx, 'Win x64 Visual Studio 2015 Output Folder', value)


###############################################################################
@register_attribute_callback
def out_folder_win64_vs2013(ctx, section_name, option_name, value):
    """ Configure output folder for win x64 (vs2013) """
    if not _is_user_input_allowed(ctx, option_name, value):
        Logs.info('\nUser Input disabled.\nUsing default value "%s" for option: ""%s"' % (value, option_name))
        return value

    # GUI
    if not ctx.is_option_true('console_mode'):
        return ctx.gui_get_attribute(section_name, option_name, value)

    _output_folder_disclaimer(ctx)
    return _get_string_value(ctx, 'Win x64 Visual Studio 2013 Output Folder', value)


###############################################################################


###############################################################################


###############################################################################
@register_attribute_callback
def out_folder_linux64(ctx, section_name, option_name, value):
    """ Configure output folder for linux x64 """
    if not _is_user_input_allowed(ctx, option_name, value):
        Logs.info('\nUser Input disabled.\nUsing default value "%s" for option: "%s"' % (value, option_name))
        return value
        
    # GUI
    if not ctx.is_option_true('console_mode'):
        return ctx.gui_get_attribute(section_name, option_name, value)
        
    _output_folder_disclaimer(ctx)
    return _get_string_value(ctx, 'Linux x64 Output Folder', value)


###############################################################################
@register_attribute_callback
def out_folder_darwin64(ctx, section_name, option_name, value):
    """ Configure output folder for Darwin x64 """
    if not _is_user_input_allowed(ctx, option_name, value):
        Logs.info('\nUser Input disabled.\nUsing default value "%s" for option: "%s"' % (value, option_name))
        return value
        
    # GUI
    if not ctx.is_option_true('console_mode'):
        return ctx.gui_get_attribute(section_name, option_name, value)
        
    _output_folder_disclaimer(ctx)  
    return _get_string_value(ctx, 'Darwin x64 Output Folder', value)


###############################################################################
@register_attribute_callback
def out_folder_appletv(ctx, section_name, option_name, value):
    """ Configure output folder for Apple TV """
    if not _is_user_input_allowed(ctx, option_name, value):
        Logs.info('\nUser Input disabled.\nUsing default value "%s" for option: "%s"' % (value, option_name))
        return value
    
    # GUI
    if not ctx.is_option_true('console_mode'):
        return ctx.gui_get_attribute(section_name, option_name, value)
        
    _output_folder_disclaimer(ctx)
    return _get_string_value(ctx, 'Apple TV Output Folder', value)


###############################################################################
@register_attribute_callback
def out_folder_ios(ctx, section_name, option_name, value):
    """ Configure output folder for iOS """
    if not _is_user_input_allowed(ctx, option_name, value):
        Logs.info('\nUser Input disabled.\nUsing default value "%s" for option: "%s"' % (value, option_name))
        return value
    
    # GUI
    if not ctx.is_option_true('console_mode'):
        return ctx.gui_get_attribute(section_name, option_name, value)
        
    _output_folder_disclaimer(ctx)
    return _get_string_value(ctx, 'Ios Output Folder', value)


###############################################################################
@register_attribute_callback
def out_folder_android_armv7_gcc(ctx, section_name, option_name, value):
    """ Configure output folder for Android ARMv7 GCC """
    if not _is_user_input_allowed(ctx, option_name, value):
        Logs.info('\nUser Input disabled.\nUsing default value "%s" for option: "%s"' % (value, option_name))
        return value

    # GUI
    if not ctx.is_option_true('console_mode'):
        return ctx.gui_get_attribute(section_name, option_name, value)

    _output_folder_disclaimer(ctx)
    return _get_string_value(ctx, 'Android ARMv7 GCC Output Folder', value)


###############################################################################
@register_attribute_callback
def out_folder_android_armv7_clang(ctx, section_name, option_name, value):
    """ Configure output folder for Android ARMv7 Clang """
    if not _is_user_input_allowed(ctx, option_name, value):
        Logs.info('\nUser Input disabled.\nUsing default value "%s" for option: "%s"' % (value, option_name))
        return value

    # GUI
    if not ctx.is_option_true('console_mode'):
        return ctx.gui_get_attribute(section_name, option_name, value)

    _output_folder_disclaimer(ctx)
    return _get_string_value(ctx, 'Android ARMv7 Clang Output Folder', value)


###############################################################################
@register_attribute_callback
def out_folder_android_armv8_clang(ctx, section_name, option_name, value):
    """ Configure output folder for Android ARMv8 Clang """
    if not _is_user_input_allowed(ctx, option_name, value):
        Logs.info('\nUser Input disabled.\nUsing default value "%s" for option: "%s"' % (value, option_name))
        return value

    # GUI
    if not ctx.is_option_true('console_mode'):
        return ctx.gui_get_attribute(section_name, option_name, value)

    _output_folder_disclaimer(ctx)
    return _get_string_value(ctx, 'Android ARMv8 Clang Output Folder', value)


###############################################################################
@register_attribute_callback
def use_uber_files(ctx, section_name, option_name, value):
    """ Configure the usage of UberFiles """
    if not _is_user_input_allowed(ctx, option_name, value):
        Logs.info('\nUser Input disabled.\nUsing default value "%s" for option: "%s"' % (value, option_name))
        return value

    info_str = ['UberFiles significantly improve total compile time of the CryEngine']
    info_str.append('at a slight cost of single file compilation time.')
    
    # GUI
    if not ctx.is_option_true('console_mode'):
        return ctx.gui_get_attribute(section_name, option_name, value, '\n'.join(info_str))
        
    info_str.append("(Press ENTER to keep the current default value shown in [])")
    Logs.info('\n'.join(info_str))
    return _get_boolean_value(ctx, 'Should UberFiles be used', value)


###############################################################################
@register_attribute_callback
def enabled_game_projects(ctx, section_name, option_name, value):
    """ Configure all Game Projects that are enabled for this user """
    if getattr(ctx.options,'execsolution',None) or not ctx.is_option_true('ask_for_user_input'):
        return value
    
    if LOADED_OPTIONS.get('enabled_game_projects', 'False') == 'False':
        return ''

    info_str = ['Specify which game projects to include when compiling and generating project files.']
    info_str.append('Comma seperated list of Game names, from the projects.json root (EmptyTemplate,SamplesProject) for example')

    # GUI
    if not ctx.is_option_true('console_mode'):
        return ctx.gui_get_attribute(section_name, option_name, value, '\n'.join(info_str))

    # Console
    info_str.append("\nQuick option(s) (separate by comma):")

    project_list = ctx.game_projects()
    project_list.sort()
    for idx , project in enumerate(project_list):
        output = '   %s: %s: ' % (idx, project)
        while len(output) < 25:
            output += ' '
        output += ctx.get_launcher_product_name(project)
        info_str.append(output)

    info_str.append("(Press ENTER to keep the current default value shown in [])")
    Logs.info('\n'.join(info_str))

    while True:
        projects = _get_string_value(ctx, 'Comma separated project list', value)
        projects_input_list = projects.replace(' ', '').split(',')

        # Replace quick options
        options_valid = True
        for proj_idx, proj_name in enumerate(projects_input_list):
            if proj_name.isdigit():
                option_idx = int(proj_name)
                try:
                    projects_input_list[proj_idx] = project_list[option_idx]
                except:
                    Logs.warn('[WARNING] - Invalid option: "%s"' % option_idx)
                    options_valid = False

        if not options_valid:
            continue

        projects_enabled = ','.join(projects_input_list)
        (res, warning, error) = ATTRIBUTE_VERIFICATION_CALLBACKS['verify_enabled_game_projects'](ctx, option_name, projects_enabled)

        if error:
            Logs.warn(error)
            continue

        return projects_enabled


###############################################################################
@register_verify_attribute_callback
def verify_enabled_game_projects(ctx, option_name, value):
    """ Configure all Game Projects which should be included in Visual Studio """
    if not value:
        return True, "", "" # its okay to have no game project

    if (len(value) == 0):
        return True, "", ""

    if (value[0] == '' and len(value) == 1):
        return True, "", ""

    project_list = ctx.game_projects()
    project_list.sort()
    project_input_list = value.strip().replace(' ', '').split(',')

    # Get number of occurrences per item in list
    num_of_occurrences = Counter(project_input_list)

    for input in project_input_list:
        # Ensure spec is valid
        if not input in project_list:
            error = ' [ERROR] Unkown game project: "%s".' % input
            return (False, "", error)
        # Ensure each spec only exists once in list
        elif not num_of_occurrences[input] == 1:
            error = ' [ERROR] Multiple occurrences of "%s" in final game project value: "%s"' % (input, value)
            return (False, "", error)

    return True, "", ""


###############################################################################
@register_hint_attribute_callback
def hint_enabled_game_projects(ctx, section_name, option_name, value):
    """ Hint list of specs for projection generation """
    project_list = ctx.game_projects()
    project_list.sort()
    desc_list = []

    for gameproj in project_list:
        desc_list.append(ctx.get_launcher_product_name(gameproj))

    return (project_list, project_list, desc_list, "multi")


###############################################################################
@register_attribute_callback
def generate_vs_projects_automatically(ctx, section_name, option_name, value):
    """ Configure automatic project generation """
    if not _is_user_input_allowed(ctx, option_name, value):
        Logs.info('\nUser Input disabled.\nUsing default value "%s" for option: "%s"' % (value, option_name))
        return value
        
    info_str = ['Allow WAF to track changes and automatically keep Visual Studio projects up-to-date for you?']
    
    # Gui
    if not ctx.is_option_true('console_mode'):
        return ctx.gui_get_attribute(section_name, option_name, value, '\n'.join(info_str)) 
    # Console
    else:       
        info_str.append('\n(Press ENTER to keep the current default value shown in [])')
        Logs.info('\n'.join(info_str))
        return _get_boolean_value(ctx, 'Enable Automatic generation of Visual Studio Projects/Solution', value)

###############################################################################
@register_attribute_callback
def specs_to_include_in_project_generation(ctx, section_name, option_name, value):  
    """ Configure all Specs which should be included in Visual Studio """
    if getattr(ctx.options,'execsolution',None) or not ctx.is_option_true('ask_for_user_input'):
        return value                        
    if LOADED_OPTIONS.get('generate_vs_projects_automatically', 'False') == 'False':
        return ''
    
    info_str = ['Specify which specs to include when generating Visual Studio projects.']
    info_str.append('Each spec defines a list of dependent project for that modules .')
    
    # GUI
    if not ctx.is_option_true('console_mode'):
        return ctx.gui_get_attribute(section_name, option_name, value, '\n'.join(info_str))
    
    # Console   
    info_str.append("\nQuick option(s) (separate by comma):")
    
    spec_list = ctx.loaded_specs()
    spec_list.sort()
    for idx , spec in enumerate(spec_list):
        output = '   %s: %s: ' % (idx, spec)
        while len(output) < 25:
            output += ' '
        output += ctx.spec_description(spec)
        info_str.append(output)
        
    info_str.append("(Press ENTER to keep the current default value shown in [])")
    Logs.info('\n'.join(info_str))
    
    while True:
        specs = _get_string_value(ctx, 'Comma separated spec list', value)      
        spec_input_list = specs.replace(' ', '').split(',')
                
        # Replace quick options
        options_valid = True
        for spec_idx, spec_name in enumerate(spec_input_list):
            if spec_name.isdigit():
                option_idx = int(spec_name)
                try:
                    spec_input_list[spec_idx] = spec_list[option_idx]
                except:
                    Logs.warn('[WARNING] - Invalid option: "%s"' % option_idx)
                    options_valid = False
                    
        if not options_valid:
            continue
                    
        specs = ','.join(spec_input_list)   
        (res, warning, error) = ATTRIBUTE_VERIFICATION_CALLBACKS['verify_specs_to_include_in_project_generation'](ctx, option_name, specs)
        
        if error:
            Logs.warn(error)
            continue
            
        return specs


###############################################################################
@register_verify_attribute_callback
def verify_specs_to_include_in_project_generation(ctx, option_name, value): 
    """ Configure all Specs which should be included in Visual Studio """   
    if not value:
        error = ' [ERROR] - Empty spec list not supported'
        return (False, "", error)
        
    spec_list =  ctx.loaded_specs()
    spec_input_list = value.strip().replace(' ', '').split(',') 
    
    # Get number of occurrences per item in list
    num_of_occurrences = Counter(spec_input_list)   
    
    for input in spec_input_list:
        # Ensure spec is valid
        if not input in spec_list:
            error = ' [ERROR] Unkown spec: "%s".' % input
            return (False, "", error)
        # Ensure each spec only exists once in list
        elif not num_of_occurrences[input] == 1:
            error = ' [ERROR] Multiple occurrences of "%s" in final spec value: "%s"' % (input, value)
            return (False, "", error)
        
    return True, "", "" 


###############################################################################
@register_hint_attribute_callback
def hint_specs_to_include_in_project_generation(ctx, section_name, option_name, value):
    """ Hint list of specs for projection generation """
    spec_list = sorted(ctx.loaded_specs())
    desc_list = []
    
    for spec in spec_list:
        desc_list.append(ctx.spec_description(spec))            

    return (spec_list, spec_list, desc_list, "multi")


###############################################################################
def options(ctx):
    """ Create options for all entries in default_settings.json """
    _load_default_settings_file(ctx)
            
    default_settings_file = _default_settings_node(ctx).abspath()
        
    # Load default settings
    for settings_group, settings_list in ctx.default_settings.items():      
        group = ctx.add_option_group(settings_group)        

        # Iterate over all options in this group
        for settings in settings_list:
            
            # Do some basic sanity checking for each option
            if not settings.get('attribute', None):
                ctx.cry_file_error('One Option in group %s is missing mandatory setting "attribute"' % settings_group, default_settings_file)
            if not settings.get('long_form', None):
                ctx.cry_file_error('Option "%s" is missing mandatory setting "long_form"' % settings['attribute'], default_settings_file)
            if not settings.get('description', None):
                ctx.cry_file_error('Option "%s" is missing mandatory setting "description"' % settings['attribute'], default_settings_file)
                
            #   Add option with its default value
            if settings.get('short_form'):
                group.add_option(settings['short_form'], settings['long_form'],  dest=settings['attribute'], action='store', default=str(settings.get('default_value', '')), help=settings['description'])
            else:
                group.add_option(settings['long_form'],  dest=settings['attribute'], action='store', default=str(settings.get('default_value', '')), help=settings['description'])                          
            
###############################################################################
@conf
def get_user_settings_node(ctx):
    """ Return a Node object pointing to the user_settings.options file """
    return ctx.root.make_node(Context.launch_dir).make_node('_WAF_/user_settings.options')
    
###############################################################################
@conf 
def load_user_settings(ctx):
    """ Apply all loaded options if they are different that the default value, and no cmd line value is presented """
    global user_settings

    _load_default_settings_file(ctx)

    write_user_settings     = False 
    user_settings               = ConfigParser.ConfigParser()   
    user_setting_file       = ctx.get_user_settings_node().abspath()
    new_options                 = {}
        
    # Load existing user settings
    if not os.path.exists( user_setting_file ):
        write_user_settings = True  # No file, hence we need to write it
    else:
        user_settings.read( [user_setting_file] )

    Logs.debug('default_settings: sys.argv = {}'.format(sys.argv))

    # 'Misc Options' must be evaluated first because the values in 'Game Projects' relies on its values
    section_name_and_settings_tuple_list = []
    for section_name, settings_list in ctx.default_settings.items():
        if section_name == 'Misc Options':
            section_name_and_settings_tuple_list.insert(0,(section_name, settings_list))
        else:
            section_name_and_settings_tuple_list.append((section_name, settings_list))

    # Load settings and check for newly set ones
    for section_name, settings_list in section_name_and_settings_tuple_list:
        
        # Add not already present sections
        if not user_settings.has_section(section_name):
            user_settings.add_section(section_name)
            write_user_settings = True
            
        # Iterate over all options in this group
        for settings in settings_list:
            option_name     = settings['attribute']
            default_value = settings.get('default_value', '')

            # Load the value from user settings if it is already present    
            if  user_settings.has_option(section_name, option_name):
                value = user_settings.get(section_name, settings['attribute'])
                LOADED_OPTIONS[ option_name ] = value
            else:
                # Add info about newly added option
                if not new_options.has_key(section_name):
                    new_options[section_name] = []
                
                new_options[section_name].append(option_name)
                
                # Load value for current option and stringify it
                value = settings.get('default_value', '')
                if getattr(ctx.options, option_name) != value:
                    value = getattr(ctx.options, option_name)

                if not isinstance(value, str):
                    value = str(value)

                if  ATTRIBUTE_CALLBACKS.get(option_name, None):
                    value = ATTRIBUTE_CALLBACKS[option_name](ctx, section_name, settings['attribute'], value)

                (isValid, warning, error) = ctx.verify_settings_option(option_name, value)

                # Add option
                if isValid:
                    user_settings.set( section_name, settings['attribute'], str(value) )
                    LOADED_OPTIONS[ option_name ] = value
                    write_user_settings = True

            # Check for settings provided by the cmd line
            long_form           = settings['long_form']
            short_form      = settings.get('short_form', None)
            
            # Settings on cmdline should have priority
            # explicitly search for either the long form or short form argument, make sure to handle both --option=<SomeThing> and --option <Something> cases
            bOptionSetOnCmdLine = False
            for arg in sys.argv:
                arg_tokens = arg.split('=')
                if (arg_tokens[0] == long_form) or (short_form and arg_tokens[0] == short_form):
                    Logs.debug('default_settings: found either long_form, "{}", or short_form, "{}", argument in command line param {}'.format(long_form, short_form, arg))
                    bOptionSetOnCmdLine = True
                    value = getattr(ctx.options, option_name)
                    break

            # Remember option for internal processing
            if bOptionSetOnCmdLine:
                LOADED_OPTIONS[ option_name ] = value           
            elif user_settings.has_option(section_name, option_name): # Load all settings not coming form the cmd line from the config file
                setattr(ctx.options, option_name, user_settings.get(section_name, option_name))

    # Write user settings
    if write_user_settings:
        ctx.save_user_settings(user_settings)

    # If use_incredibuild option was set but did not pass the ib validation, turn it off
    if ctx.is_option_true('use_incredibuild') and not internal_validate_incredibuild_registry_settings(ctx):
        setattr(ctx.options, 'use_incredibuild', 'False')

    # If max_cores option was set to < 0, default it to a good hardware value
    if int(ctx.options.max_cores) <= 0:
        max_cores = 8   # fallback value
        try:
            max_cores = multiprocessing.cpu_count()
        except:
            Logs.warn('unable to query hardware for number of hw threads, using "%d"' % max_cores)
        setattr(ctx.options, 'max_cores', max_cores)

    return user_settings, new_options


###############################################################################
@conf
def get_default_settings(ctx, section_name, setting_name):
    default_value = ""
    default_description = ""
    
    if not hasattr(ctx, 'default_settings'):
        return (default_value, default_description)

    for settings_group, settings_list in ctx.default_settings.items():
        if settings_group == section_name:
            for settings in settings_list:
                if settings['attribute'] == setting_name:
                    default_value = settings.get('default_value', '')
                    default_description = settings.get('description', '')
                    break
            break
    
    return (default_value, default_description)


###############################################################################
@conf 
def save_user_settings(ctx, config_parser):
    # Write user settings   
    with open(ctx.get_user_settings_node().abspath(), 'wb') as configfile:
        config_parser.write(configfile)

        
###############################################################################
@conf
def verify_settings_option(ctx, option_name, value):        
    verify_fn_name = "verify_" + option_name    
    if ATTRIBUTE_VERIFICATION_CALLBACKS.get(verify_fn_name, None):
        return ATTRIBUTE_VERIFICATION_CALLBACKS[verify_fn_name](ctx, option_name, value)
    else:
        return (True, "", "")

    
###############################################################################
@conf
def hint_settings_option(ctx, section_name, option_name, value):        
    verify_fn_name = "hint_" + option_name  
    if ATTRIBUTE_HINT_CALLBACKS.get(verify_fn_name, None):
        return ATTRIBUTE_HINT_CALLBACKS[verify_fn_name](ctx, section_name, option_name, value)
    else:
        return ([], [], [], "single")

    
###############################################################################
@conf
def set_settings_option(ctx, section_name, option_name, value): 
    global user_settings
    user_settings.set(section_name, option_name, str(value))
    LOADED_OPTIONS[option_name] = value 


@conf
def update_settings_options_file(ctx):
    '''
    Update the user_settings.options file if there are any changes from the command line
    '''

    disk_user_settings = ConfigParser.ConfigParser()
    user_setting_file = ctx.get_user_settings_node().abspath()
    write_user_settings = False

    # Load existing user settings
    disk_user_settings.read([user_setting_file])

    # Go through all the sections and its options and check if there are any deltas from whats in memory
    disk_sections = disk_user_settings.sections()
    for disk_section in disk_sections:
        disk_options = disk_user_settings.options(disk_section)
        for disk_option in disk_options:
            if disk_option not in LOADED_OPTIONS:
                continue
            disk_option_value = disk_user_settings.get(disk_section, disk_option)
            current_option_value = LOADED_OPTIONS[disk_option]
            # Change was detected, update the value and flag for update
            if disk_option_value != current_option_value:
                write_user_settings = True
                disk_user_settings.set(disk_section, disk_option, str(current_option_value))

    if write_user_settings:
        ctx.save_user_settings(disk_user_settings)

    return write_user_settings









