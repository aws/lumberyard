#!/usr/bin/env python
# encoding: utf-8

# Modifications copyright Amazon.com, Inc. or its affiliates.

# Carlos Rafael Giani, 2006 (dv)
# Tamas Pal, 2007 (folti)
# Nicolas Mercier, 2009
# Matt Clarkson, 2012

import os, sys, re, tempfile, subprocess, hashlib
from waflib import Utils, Task, Logs, Options, Errors
from waflib.Logs import debug, warn
from waflib.Tools import c_preproc, ccroot, c, cxx, ar
from waflib.Configure import conf
from waflib.TaskGen import feature, after, after_method, before_method
import waflib.Node
from utils import parse_json_file, write_json_file
from waf_branch_spec import LMBR_WAF_VERSION_TAG, BINTEMP_CACHE_TOOLS

# The compiler will issue a warning if some flags are specified more than once.
# The command is constructed from subsets that may have conflicting flags
# This list of lists contains all the set of flags that are made unique
UNIQUE_FLAGS_LIST = [
    ["/arch:IA32", "/arch:SSE", "/arch:SSE2", "/arch:AVX", "/arch:AVX2"],   # code gen for arch
    ["/clr", "/clr:initialAppDomain", "/clr:noAssembly", "/clr:nostdlib", "/clr:pure", "/clr:safe"],    # common language runtime
    ["/EHs", "/EHa", "/EHac", "/EHsc"],                                     # exception handling
    ["/errorReport:none", "/errorReport:prompt", "/errorReport:queue", "/errorReport:send"],        # report internal compiler errors
    ["/favor:blend", "/favor:ATOM", "/favor:AMD64", "/favor:INTEL64"],      # optimize for arch
    ["/fp:precise", "/fp:except", "/fp:except-", "/fp:fast", "/fp:strict"], # floating point behavior
    ["/Gd", "/Gr", "/Gv", "/Gz"],                                           # calling convention
    ["/GL", "/GL-"],                                    # whole program optimization
    ["/GR", "/GR-"],                                    # runtime type information
    ["/GS", "/GS-"],                                    # buffer security checks
    ["/Gs", "/Gs0", "/Gs4096"],                         # control stack checking calls
    ["/Gw", "/Gw-"],                                    # global data optimization
    ["/Gy", "/Gy-"],                                    # enable function level linking
    ["/O1", "/O2", "/Od", "/Ox"],                       # optimization level
    ["/Ob0", "/Ob1", "/Ob2"],                           # inline expansion
    ["/Oi", "/Oi-"],                                    # intrinsics
    ["/Os", "/Ot"],                                     # favor small code/ fast code
    ["/Oy", "/Oy-"],                                    # frame pointer omission
    ["/MD", "/MT", "/LD", "/MDd", "/MTd", "/LDd"],      # runtime library
    ["/RTC1","/RTCc","/RTCs","/RTCu"],                  # runtime error checks
    ["/volatile","/volatile:iso", "/volatile:ms"],      # volatile keyword handling
    ["/vd0", "/vd1", "/vd2"],                           # disable construction displacements
    ["/ZW", "/ZW:nostdlib"],                            # windows runtime compilation
    ["/sdl", "/sdl-"],                                  # enable additional security checks
    ["/vmb", "/vmg"],                                   # always declare a class before using a pointer to it
    ["/vmm", "/vms", "/vmv"],                           # inheritance of yet-to-be-defined classes
    ["/W0", "/W1", "/W2", "/W3", "/W4"],                # error level
    ["/WX", "/WX-", "/WX:NO"],                          # treat warnings as errors
    ["/Z7", "/Zi", "/ZI"],                              # debug information format
    ["/Za", "/Ze"],                                     # disable language extensions
    ["/Zc:forScope", "/Zc:forScope-"],                  # for loop scope conformance
    ["/Zc:wchar_t", "/Zc:wchar_t-"],                    # wchar_t maps to __wchar_t
    ["/Zc:auto", "/Zc:auto-"],                          # deduce variable type
    ["/Zc:trigraphs", "/Zc:trigraphs-"],                # character substitutions if character isn't in charpage
    ["/Zc:rvalueCast", "/Zc:rvalueCast-"],              # enforce type conversion rules
    ["/Zc:strictStrings", "/Zc:strictStrings-"],        # disable string literal type conversion
    ["/Zc:inline", "/Zc:inline-"],                      # remove unreferenced comdat sections
    ["/Zp", "/Zp:1", "/Zp:2", "/Zp:4", "/Zp:8", "/Zp:16"]   # struct member alignment
]

# convert list of flags that must be unique to dictionary
UNIQUE_FLAGS_DICT = {}
for idx, flags in enumerate(UNIQUE_FLAGS_LIST):
    assert(isinstance(flags,list))
    for flag in flags:
        UNIQUE_FLAGS_DICT[flag] = idx     # all flags from the list have the same value, just using index as a dummy unique val


def exec_mf(self):
        """
        Create the manifest file
        """        
        env = self.env
        mtool = env['MT']
        if not mtool:
                return 0

        self.do_manifest = False

        outfile = self.outputs[0].abspath()

        manifest = None
        for out_node in self.outputs:
                if out_node.name.endswith('.manifest'):
                        manifest = out_node.abspath()
                        break

        #Disabling manifest for incremental link
        if '/INCREMENTAL' in env['LINKFLAGS']:
            manifest = None

        if manifest is None:
                # Should never get here.  If we do, it means the manifest file was
                # never added to the outputs list, thus we don't have a manifest file
                # to embed, so we just return.
                return 0

        # embedding mode. Different for EXE's and DLL's.
        # see: http://msdn2.microsoft.com/en-us/library/ms235591(VS.80).aspx
        mode = ''
        if 'cprogram' in self.generator.features or 'cxxprogram' in self.generator.features:
                mode = '1'
        elif 'cshlib' in self.generator.features or 'cxxshlib' in self.generator.features:
                mode = '2'

        debug('msvc: embedding manifest in mode %r' % mode)

        lst = []
        lst.append(env['MT'])
        lst.extend(Utils.to_list(env['MTFLAGS']))
        lst.extend(['-manifest',manifest])
        
        if hasattr(self.generator, 'additional_manifests'):
            if not isinstance(self.generator.additional_manifests, list): # the additional manifests could be a string
                self.generator.additional_manifests = [self.generator.additional_manifests]
            for element in self.generator.additional_manifests: # add each one with its own path
                 lst.append( self.generator.path.abspath() + '/' + element)
        lst.append('-outputresource:%s;%s' % (outfile, mode))

        # note that because we call exec_command and give it a list of params, these become the subprocess argv*
        # and thus it is not necessary for us to escape them with quotes or anything like that.
        lst = [lst]        
        return self.exec_command(*lst)
                
def quote_response_command(self, flag):
        flag = flag.replace('\\', '\\\\') # escape any backslashes
        flag = flag.replace('"', '\\"') # escape any quotes
        if flag.find(' ') > -1:
                for x in ('/LIBPATH:', '/IMPLIB:', '/OUT:', '/I'):
                        if flag.startswith(x):
                                flag = '%s"%s"' % (x, flag[len(x):])
                                break
                else:
                        flag = '"%s"' % flag
        return flag

def exec_response_command(self, cmd, **kw):
    # not public yet
    try:
        tmp = None
        if sys.platform.startswith('win') and isinstance(cmd, list) and len(' '.join(cmd)) >= 16384:
                tmp_files_folder = self.generator.bld.get_bintemp_folder_node().make_node('TempFiles')
                program = cmd[0] #unquoted program name, otherwise exec_command will fail
                cmd = [self.quote_response_command(x) for x in cmd]

                # Determine an appropriate filename for the output file (displayed by Incredibuild)
                if self.outputs and len(self.outputs[0].abspath()):
                    tmp_file_name = os.path.basename(self.outputs[0].abspath())
                else:
                    # strips quotes off the FRONT in case its a string like '"something="somethingelse"' which would cause issues
                    out_file = os.path.split(cmd[-1].strip('"'))
                    tmp_file_name = out_file[1]
                (fd, tmp) = tempfile.mkstemp(prefix=tmp_file_name, dir=tmp_files_folder.abspath())
                os.write(fd, '\r\n'.join(cmd[1:]).encode())
                os.close(fd)
                cmd = [program, '@' + tmp]
        # no return here, that's on purpose
        ret = self.generator.bld.exec_command(cmd, **kw)

    finally:
        if tmp:
            try:
                os.remove(tmp)
            except OSError:
                pass # anti-virus and indexers can keep the files open -_-
    return ret

########## stupid evil command modification: concatenate the tokens /Fx, /doc, and /x: with the next token
def exec_command_msvc(self, *k, **kw):
    """
    Change the command-line execution for msvc programs.
    Instead of quoting all the paths and keep using the shell, we can just join the options msvc is interested in
    """
    # If bullseye coverage tool is in the environment, and we are executing the CXX or C compiler, then
    # we prefix with the bullseye coverage tool. Otherwise, just run the regular tools.
    # Ideally we need a way to do this cleanly on other platforms, but this implies a post hook of some kind to change
    # the CXX task run_str, and I could not immediately see a clean way to do that, especially conditionally as
    # we need to do below.
    if 'BULL_COVC' in self.env:
        
        excluded_modules = getattr(self.generator.bld.options,'bullseye_excluded_modules',"").replace(' ', '').split(',')
        if "" in excluded_modules:
            excluded_modules.remove("")

        # Figure out which package we are building, and check if it is in the list of packages we want coverage for
        # If the package list is empty, then we do coverage building for the whole project.
        # This applies to the CC/CXX steps. We must always link with coverage if we are going to get measurements.
        included_modules = getattr(self.generator.bld.options,'bullseye_included_modules',"").replace(' ', '').split(',')
        if "" in included_modules:
            included_modules.remove("")

        if self.generator.name not in excluded_modules and (not included_modules or self.generator.name in included_modules):
            if k[0][0] == self.env['CXX'] or k[0][0] == self.env['CC']:
                k = ([self.env['BULL_COVC'], '--file', self.env['BULL_COV_FILE']] + k[0],)

    # We must link with bullseye regardless of which way the project is set (with or without coverage) to avoid link errors with included libraries.
    if 'BULL_COVLINK' in self.env and (k[0][0] == self.env['LINK'] or k[0][0] == self.env['LINK_CXX'] or k[0][0] == self.env['LINK_CC']):
        k = ([self.env['BULL_COVLINK']] + k[0],)

    # 1) Join options that carry no space are joined e.g. /Fo FilePath -> /FoFilePath
    # 2) Join options that carry a ':' as last character : e.g. /OUT: FilePath -> /OUT:FilePath
    if isinstance(k[0], list):
        lst = []
        carry = ''
        join_with_next_list_item = ['/Fo', '/doc', '/Fi', '/Fa']    
        for a in k[0]:
            if a in  join_with_next_list_item or a[-1] == ':':
                carry = a
            else:
                lst.append(carry + a)
                carry = ''

        k = [lst]
        
    bld = self.generator.bld
    try:
        if not kw.get('cwd', None):
            kw['cwd'] = bld.cwd
    except AttributeError:
        bld.cwd = kw['cwd'] = bld.variant_dir

    ret = self.exec_response_command(k[0], **kw)
    
    if not ret and getattr(self, 'do_manifest', None):
        ret = self.exec_mf()
    return ret
        
def wrap_class(class_name):
        """
        Manifest file processing and @response file workaround for command-line length limits on Windows systems
        The indicated task class is replaced by a subclass to prevent conflicts in case the class is wrapped more than once
        """    
        cls = Task.classes.get(class_name, None)
        if not cls:
                return

        derived_class = type(class_name, (cls,), {})
        def exec_command(self, *k, **kw):
                if self.env['CC_NAME'] == 'msvc':
                        return self.exec_command_msvc(*k, **kw)
                else:
                        return super(derived_class, self).exec_command(*k, **kw)

        # Chain-up monkeypatch needed since exec_command() is in base class API
        derived_class.exec_command = exec_command

        # No chain-up behavior needed since the following methods aren't in
        # base class API
        derived_class.exec_response_command = exec_response_command
        derived_class.quote_response_command = quote_response_command
        derived_class.exec_command_msvc = exec_command_msvc
        derived_class.exec_mf = exec_mf

        return derived_class

for k in 'c cxx cprogram cxxprogram cshlib cxxshlib cstlib cxxstlib'.split():
    wrap_class(k)


@feature('cxxprogram', 'cxxshlib', 'cprogram', 'cshlib', 'cxx', 'c')
@after_method('apply_incpaths')
@after_method('add_pch_to_dependencies')
def set_pdb_flags(self):            
    if not 'msvc' in (self.env.CC_NAME, self.env.CXX_NAME):
        return  

    if not self.bld.is_option_true('generate_debug_info'):
        return

    # find the last debug symbol type of [/Z7, /Zi, /ZI] applied in cxxflags.
    last_debug_option = ''
    for opt in reversed(self.env['CXXFLAGS']):
        if opt in ['/Z7', '/Zi', '/ZI']:
            last_debug_option = opt
            break

    if last_debug_option in ['/Zi', '/ZI']:
        # Compute PDB file path
        pdb_folder = self.path.get_bld().make_node(str(self.target_uid))
        pdb_cxxflag = '/Fd{}'.format(pdb_folder.abspath())


        # Make sure the PDB folder exists
        pdb_folder.mkdir()
    
        # Add CXX and C Flags
        for t in getattr(self, 'compiled_tasks', []):
            t.env.append_value('CXXFLAGS', pdb_cxxflag)
            t.env.append_value('CFLAGS', pdb_cxxflag)

        # Add PDB also to Precompiled header.  pch_task is not in compiled_tasks
        if getattr(self, 'pch_task', None):
            self.pch_task.env.append_value('CXXFLAGS', pdb_cxxflag)
            self.pch_task.env.append_value('CFLAGS', pdb_cxxflag)


def is_node_qt_rc_generated(self,node):

    if node.is_child_of(self.bld.bldnode):
        raw_name = os.path.splitext(os.path.basename(node.abspath()))[0]
        if raw_name.endswith('_rc'):
            return True
    return False

@feature('cxx')
@before_method('process_source')
def add_pch_msvc(self):
    if not 'msvc' in (self.env.CC_NAME, self.env.CXX_NAME):
        return

    if Utils.unversioned_sys_platform() != 'win32':
        return

    # Create Task to compile PCH
    if not getattr(self, 'pch', ''):
        return

    if not self.bld.is_option_true('use_precompiled_header'):
        return

    # Always assume only one PCH File
    pch_source = self.to_nodes(self.pch)[0]

    self.pch_header = pch_source.change_ext('.h')
    self.pch_header_name = os.path.split(self.pch_header.abspath())[1]

    # Generate PCH per target project idx
    # Avoids the case where two project have the same PCH output path but compile the PCH with different compiler options i.e. defines, includes, ...
    self.pch_file = pch_source.change_ext('.%d.pch' % self.target_uid)
    self.pch_object = pch_source.change_ext('.%d.obj' % self.target_uid)
    # Create PCH Task
    self.pch_task = pch_task = self.create_task('pch_msvc', pch_source, [self.pch_object, self.pch_file])
    pch_task.env.append_value('PCH_NAME', self.pch_header_name)
    pch_task.env.append_value('PCH_FILE', '/Fp' + self.pch_file.abspath())
    pch_task.env.append_value('PCH_OBJ', self.pch_object.abspath())


@feature('cxx')
@after_method('apply_incpaths')
def add_pch_to_dependencies(self):
    if not 'msvc' in (self.env.CC_NAME, self.env.CXX_NAME):
        return

    # Create Task to compile PCH
    if not getattr(self, 'pch_object', ''):
        return

    pch_abs_path = self.pch_file.abspath()
    pch_flag = '/Fp' + pch_abs_path
    pch_header = '/Yu' + self.pch_header_name

    # Append PCH File to each compile task
    for t in getattr(self, 'compiled_tasks', []):   
        input_file = t.inputs[0].abspath()
        file_specific_settings = self.file_specifc_settings.get(input_file, None)
        if file_specific_settings and 'disable_pch' in file_specific_settings and file_specific_settings['disable_pch'] == True:
            continue # Don't append PCH to files for which we don't use them
            
        if getattr(t, 'disable_pch', False) == True:
            continue # Don't append PCH to files for which we don't use them
            
        if t.__class__.__name__ in ['cxx','qxx']: #Is there a better way to ensure cpp only?

            if is_node_qt_rc_generated(self,t.inputs[0]):
                t.env.append_value('CXXFLAGS', '/Y-')
            else:
                t.env.append_value('CXXFLAGS', pch_header)
                t.env.append_value('CXXFLAGS', pch_flag)

                # Append PCH to task input to ensure correct ordering
                t.dep_nodes.append(self.pch_object)

    # Append the pch object to the link task
    if getattr(self, 'link_task', None):
        self.link_task.inputs.append(self.pch_object)


class pch_msvc(waflib.Task.Task):
    run_str = '${CXX} ${PCH_CREATE_ST:PCH_NAME} ${CXXFLAGS} ${CPPPATH_ST:INCPATHS} ${DEFINES_ST:DEFINES} ${SRC} ${CXX_TGT_F}${PCH_OBJ} ${PCH_FILE}'
    scan    = c_preproc.scan
    color   = 'BLUE'    
    
    def exec_command(self, *k, **kw):   
        return exec_command_msvc(self, *k, **kw)
    
    def exec_response_command(self, *k, **kw):  
        return exec_response_command(self, *k, **kw)    

    def quote_response_command(self, *k, **kw): 
        return quote_response_command(self, *k, **kw)

    def exec_mf(self, *k, **kw):    
        return exec_mf(self, *k, **kw)


def strip_all_but_last_dependent_options(flags):
    seen = set()
    delete = []
    for idx, flag in enumerate(reversed(flags)):
        try:
            val = UNIQUE_FLAGS_DICT[flag]
            if val not in seen:
                seen.add(val)
                continue
            # mark for delete
            delete.append(len(flags) -1 -idx)
        except:
            pass
    for idx in delete:
        del flags[idx]


def verify_options_common(env):
    strip_all_but_last_dependent_options(env.CFLAGS)
    strip_all_but_last_dependent_options(env.CXXFLAGS)


@feature('c', 'cxx')
@after_method('apply_link')
@after_method('add_pch_to_dependencies')
def verify_compiler_options_msvc(self):
    if not 'msvc' in (self.env.CC_NAME, self.env.CXX_NAME):
        return
    
    if Utils.unversioned_sys_platform() != 'win32':
        return
        
    # Verify compiler option (strip all but last for dependant options)
    for t in getattr(self, 'compiled_tasks', []):           
        verify_options_common(t.env)

    # Verify pch_task options (strip all but last for dependant options)
    if hasattr(self, 'pch_task'):
        verify_options_common(self.pch_task.env)

    # Strip unsupported ARCH linker option
    if hasattr(self, 'link_task'):
        del self.link_task.env['ARCH']
    
    
#############################################################################
# Code for auto-recognition of Visual Studio Compiler and Windows SDK Path
# Taken from the original WAF code
#############################################################################
all_msvc_platforms = [ ('x64', 'amd64')]
"""List of msvc platforms"""

TOOL_CACHE_ATTR_USE_CACHE_FLAG = 'use_tool_environment_cache'

TOOL_CACHE_ATTR_READ_DICTIONARY = 'tool_environment_cache_read_dict'

@conf
def auto_detect_msvc_compiler(conf, version, target, windows_kit):
    conf.env['MSVC_VERSIONS'] = [version]
    conf.env['MSVC_TARGETS'] = [target]
    vs_version = version.replace('msvc', '').strip()

    # Normalize the input winkit by ensuring its ascii and it has no leading or trailing spaces
    ascii_winkit = windows_kit.encode('utf-8') if isinstance(windows_kit, unicode) else windows_kit
    ascii_winkit = ascii_winkit.strip()

    try:
        # By default use the tool environment cache
        setattr(conf, TOOL_CACHE_ATTR_USE_CACHE_FLAG, True)
        conf.autodetect(ascii_winkit, True)
        conf.find_msvc()
    except:
        cache_read_dict = getattr(conf, TOOL_CACHE_ATTR_READ_DICTIONARY, {})
        if cache_read_dict.get(vs_version, False):
            # If an error occurred trying to detect the compiler and the compiler info was read from the cache, then
            # invalidate the running installed version, turn off the use of the cache, and re-run it
            setattr(conf, TOOL_CACHE_ATTR_USE_CACHE_FLAG, False)
            setattr(conf, TOOL_CACHE_ATTR_READ_DICTIONARY, {})
            global MSVC_INSTALLED_VERSIONS
            MSVC_INSTALLED_VERSIONS[ascii_winkit] = ''

            conf.autodetect(ascii_winkit, True)
            conf.find_msvc()
        else:
            # If the error occurred without using cached information, then just error out, no need to try again
            raise

    
@conf
def autodetect(conf, windows_kit, arch = False):
    v = conf.env
    
    if arch:
        compiler, version, path, includes, libdirs, arch = conf.detect_msvc(windows_kit, True)
        v['DEST_CPU'] = arch
    else:
        compiler, version, path, includes, libdirs = conf.detect_msvc(windows_kit)
    
    v['PATH'] = path
    v['INCLUDES'] = includes
    v['LIBPATH'] = libdirs
    v['MSVC_COMPILER'] = compiler
    try:
        v['MSVC_VERSION'] = float(version)
    except Exception:
        v['MSVC_VERSION'] = float(version[:-3]) 

@conf
def detect_msvc(conf, windows_kit, arch = False):
    versions = get_msvc_versions(conf, windows_kit)
    return setup_msvc(conf, versions, arch) 
    
def setup_msvc(conf, versions, arch = False):
    platforms = getattr(Options.options, 'msvc_targets', '').split(',')
    if platforms == ['']:
        platforms=Utils.to_list(conf.env['MSVC_TARGETS']) or [i for i,j in all_msvc_platforms]
    desired_versions = getattr(Options.options, 'msvc_version', '').split(',')
    if desired_versions == ['']:
        desired_versions = conf.env['MSVC_VERSIONS'] or [v for v,_ in versions][::-1]
    versiondict = dict(versions)

    for version in desired_versions:
        try:
            targets = dict(versiondict [version])
            for target in platforms:
                try:
                    arch,(p1,p2,p3) = targets[target]
                    compiler,revision = version.rsplit(' ', 1)
                    if arch:
                        return compiler,revision,p1,p2,p3,arch
                    else:
                        return compiler,revision,p1,p2,p3
                except KeyError: continue
        except KeyError: continue
    conf.fatal('msvc: Could not find a valid Visual Studio installation for building.')

    
MSVC_INSTALLED_VERSIONS = {}
@conf
def get_msvc_versions(conf, windows_kit):
    """
    :return: list of compilers installed
    :rtype: list of string
    """
    global MSVC_INSTALLED_VERSIONS
    if not windows_kit in MSVC_INSTALLED_VERSIONS:
        MSVC_INSTALLED_VERSIONS[windows_kit] = ''
    if len(MSVC_INSTALLED_VERSIONS[windows_kit]) == 0:
        lst = []
        conf.gather_wsdk_versions(windows_kit, lst)
        gather_msvc_2015_versions(conf, windows_kit, lst)
        gather_msvc_2017_versions(conf, windows_kit, lst)
        MSVC_INSTALLED_VERSIONS[windows_kit] = lst
    return MSVC_INSTALLED_VERSIONS[windows_kit]

def gather_msvc_2015_detected_versions():
    #Detected MSVC versions!
    version_pattern = re.compile('^(\d\d?\.\d\d?)(Exp)?$')
    detected_versions = []
    for vcver,vcvar in [('VCExpress','Exp'), ('VisualStudio','')]:
        try:
            prefix = 'SOFTWARE\\Wow6432node\\Microsoft\\'+vcver
            all_versions = Utils.winreg.OpenKey(Utils.winreg.HKEY_LOCAL_MACHINE, prefix)
        except WindowsError:
            try:
                prefix = 'SOFTWARE\\Microsoft\\'+vcver
                all_versions = Utils.winreg.OpenKey(Utils.winreg.HKEY_LOCAL_MACHINE, prefix)
            except WindowsError:
                continue

        index = 0
        while 1:
            try:
                version = Utils.winreg.EnumKey(all_versions, index)
            except WindowsError:
                break
            index = index + 1
            match = version_pattern.match(version)
            if not match:
                continue
            else:
                versionnumber = float(match.group(1))
            detected_versions.append((versionnumber, version+vcvar, prefix+"\\"+version))
    def fun(tup):
        return tup[0]

    detected_versions.sort(key = fun)
    return detected_versions

CACHED_TOOL_ENVIRONMENT_FILE = 'tool_environment.json'


def _make_vsversion_winkit_key(version, winkit):
    return '{}_{}'.format(str(version), winkit)


def restore_vs_version_from_cached_path(conf, vs_version, windows_kit, fingerprint, versions):
    """
    Attempt to restore the versions array (used for setting up the paths needed for visual studio) from the
    cached value from environment.json if possible.  If it is not set, or the file does not exist, or the
    fingerprint has changed, then return False without returning any cached value.

    :param conf:        Configuration Context
    :param vs_version:  The visual studio version the cache is being lookup up for
    :param windows_kit: The windows kit version the cache is being looked up for
    :param fingerprint: The current input fingerprint to compare against any cached fingerprint if any
    :param versions:    The result array of version tuples to populate if a cached version is found
    :return: True if a cached version of versions is restored, False otherwise
    """

    try:
        if not getattr(conf, TOOL_CACHE_ATTR_USE_CACHE_FLAG, False):
            return False
        environment_json_path = os.path.join(conf.bldnode.abspath(), BINTEMP_CACHE_TOOLS, CACHED_TOOL_ENVIRONMENT_FILE)
        if not os.path.exists(environment_json_path):
            return False

        environment_json = parse_json_file(environment_json_path)
        if 'vs_compilers' not in environment_json:
            return False

        vs_compilers_node = environment_json.get('vs_compilers')
        ver_winkit_key = _make_vsversion_winkit_key(vs_version, windows_kit)
        if str(ver_winkit_key) not in vs_compilers_node:
            return False

        compiler_settings = vs_compilers_node.get(ver_winkit_key)
        previous_fingerprint = compiler_settings.get('fingerprint', '')
        if previous_fingerprint != fingerprint:
            return False

        cached_versions = compiler_settings.get('versions')
        for cached_version in cached_versions:
            versions.append(cached_version)

        # Mark the current vs_version as information that was read from a cache file (in case of error during tool detection)
        cache_read_dict = getattr(conf, TOOL_CACHE_ATTR_READ_DICTIONARY, {})
        cache_read_dict[vs_version] = True
        setattr(conf, TOOL_CACHE_ATTR_READ_DICTIONARY, cache_read_dict)

        return True

    except Exception as err:
        conf.warn_once('Unable to use visual studio environment cache.  Will run msvc tool detection scripts. ({})'.format(err.message or err.msg))
        return False


def store_vs_version_to_cache(conf, vs_version, windows_kit, fingerprint, versions):
    """
    Store the version tuples for a visual studio environment to the environment.json file

    :param conf:        Configuration Context
    :param vs_version:  The visual studio version the cache is being lookup up for
    :param windows_kit: The windows kit value to store to the cache
    :param fingerprint: The current input fingerprint to compare against any cached fingerprint if any
    :param versions:    The result array of version tuples to populate if a cached version is found
    """
    try:
        cache_path = os.path.join(conf.bldnode.abspath(), BINTEMP_CACHE_TOOLS)
        if not os.path.isdir(cache_path):
            os.makedirs(cache_path)

        environment_json_path = os.path.join(cache_path, CACHED_TOOL_ENVIRONMENT_FILE)
        if os.path.exists(environment_json_path):
            environment_json = parse_json_file(environment_json_path)
        else:
            environment_json = {}

        if 'vs_compilers' not in environment_json:
            vs_compilers = {}
            environment_json['vs_compilers'] = vs_compilers
        else:
            vs_compilers = environment_json.get('vs_compilers')

        ver_winkit_key = _make_vsversion_winkit_key(vs_version, windows_kit)
        if not ver_winkit_key in vs_compilers:
            vs_compiler_setting = {}
            vs_compilers[ver_winkit_key] = vs_compiler_setting
        else:
            vs_compiler_setting = vs_compilers.get(ver_winkit_key)

        vs_compiler_setting['fingerprint'] = fingerprint
        vs_compiler_setting['versions'] = versions

        write_json_file(environment_json, environment_json_path)

    except Exception as err:
        conf.warn_once('Unable to use visual studio environment cache.  Will run msvc tool detection scripts. ({})'.format(err.message or err.msg))


def gather_msvc_2015_versions(conf, windows_kit, versions):

    # Prepare a hashing object to construct an md5 fingerprint of the collected vc_paths that was read from the
    # registry.  If the hash changes, we will force a read-read of the VCVARS.BAT environment variablkes
    hasher = hashlib.md5()
    hasher.update(LMBR_WAF_VERSION_TAG)     # Include to force a re-cache if this changes

    vc_paths = []
    for (v, version, reg) in gather_msvc_2015_detected_versions():
        try:
            try:
                msvc_version = Utils.winreg.OpenKey(Utils.winreg.HKEY_LOCAL_MACHINE, reg + "\\Setup\\VC")
            except WindowsError:
                msvc_version = Utils.winreg.OpenKey(Utils.winreg.HKEY_LOCAL_MACHINE, reg + "\\Setup\\Microsoft Visual C++")

            path, _ = Utils.winreg.QueryValueEx(msvc_version, 'ProductDir')

            abs_path = os.path.abspath(str(path))
            vc_paths.append((version, abs_path))

            hasher.update('{}/{}'.format(version, abs_path))
        except WindowsError:
            continue

    paths_hash = hasher.hexdigest()

    # Gather all the version information independent of the input winkit
    all_versions = []
    for version, vc_path in vc_paths:

        # Special case: VS 2012/2013 does not handle winkit versions, so do not force any desired winkit into the detection
        apply_windows_kit = '' if version in ('11.0', '12.0') else windows_kit
        
        # Populate the version list
        gather_msvc_2015_targets(conf, paths_hash, all_versions, version, apply_windows_kit, vc_path)
        
    # From the entire windows kit independent version lists, filter out the versions based on the winkit
    for check_version in all_versions:
        # The winkit is embedded in the first item of the tuple, split by the '/' character
        version_and_winkit = check_version[0].split('/')
        version_lst = check_version[1]
        if windows_kit == version_and_winkit[1]:
            # Rebuild the tuple with only the msvc version string and the version list information
            versions.append([version_and_winkit[0], version_lst])


def gather_msvc_2015_targets(conf, paths_hash, versions, version, windows_kit, vc_path):

    # Attempt to restore the versions from a cache environment if possible
    if restore_vs_version_from_cached_path(conf, version, windows_kit, paths_hash, versions):
        return

    # Looking for normal MSVC compilers!
    targets = []
    if os.path.isfile(os.path.join(vc_path, 'vcvarsall.bat')):
        for target,realtarget in all_msvc_platforms[::-1]:
            try:
                targets.append((target, (realtarget, conf.get_msvc_version('msvc', version, target, windows_kit, os.path.join(vc_path, 'vcvarsall.bat')))))
            except conf.errors.ConfigurationError:
                pass
    elif os.path.isfile(os.path.join(vc_path, 'Common7', 'Tools', 'vsvars32.bat')):
        try:
            targets.append(('x86', ('x86', conf.get_msvc_version('msvc', version, 'x86', windows_kit, os.path.join(vc_path, 'Common7', 'Tools', 'vsvars32.bat')))))
        except conf.errors.ConfigurationError:
            pass
    elif os.path.isfile(os.path.join(vc_path, 'Bin', 'vcvars32.bat')):
        try:
            targets.append(('x86', ('x86', conf.get_msvc_version('msvc', version, '', windows_kit, os.path.join(vc_path, 'Bin', 'vcvars32.bat')))))
        except conf.errors.ConfigurationError:
            pass
    if targets:
        versions.append(('msvc {}/{}'.format(version, windows_kit), targets))

    # Cache the versions tuples for the current vs version and environment so we can cut the cost of conf.get_msvc_version
    store_vs_version_to_cache(conf, version, windows_kit, paths_hash, versions)

def find_vswhere():
    vs_path = os.environ['ProgramFiles(x86)']
    vswhere_exe = os.path.join(vs_path, 'Microsoft Visual Studio\\Installer\\vswhere.exe')
    if not os.path.isfile(vswhere_exe):
        vswhere_exe = ''
    return vswhere_exe


def gather_msvc_2017_versions(conf, windows_kit, versions):

    # Prepare a hashing object to construct an md5 fingerprint of the collected vc_paths that was read from the
    # registry.  If the hash changes, we will force a read-read of the VCVARS.BAT environment variablkes
    hasher = hashlib.md5()
    hasher.update(LMBR_WAF_VERSION_TAG) # Include to force a re-cache if this changes

    vc_paths = []

    vswhere_exe = find_vswhere()
    if not vswhere_exe == '':
        version_string = ''
        path_string = ''
        try:
            vs_where_args = conf.options.win_vs2017_vswhere_args.lower().split()
            version_string = subprocess.check_output([vswhere_exe, '-property', 'installationVersion'] + vs_where_args)
            if not version_string:
                try:
                    version_arg_index = vs_where_args.index('-version')
                    Logs.warn('[WARN] VSWhere could not find an installed version of Visual Studio matching the version requirements provided (-version {}). Attempting to fall back on any available installed version.'.format(vs_where_args[version_arg_index + 1]))
                    Logs.warn('[WARN] Lumberyard defaults the version range to the maximum version tested against before each release. You can modify the version range in the WAF user_settings\' option win_vs2017_vswhere_args under [Windows Options].')
                    del vs_where_args[version_arg_index : version_arg_index + 2]
                    version_string = subprocess.check_output([vswhere_exe, '-property', 'installationVersion'] + vs_where_args)
                except ValueError:
                    pass

            version_parts = version_string.split('.')
            version_string = version_parts[0]
            path_string = subprocess.check_output([vswhere_exe, '-property', 'installationPath'] + vs_where_args)
            path_string = path_string[:len(path_string)-2]
            vc_paths.append((version_string, path_string))

            hasher.update('{}/{}'.format(version_string, path_string))
        except:
            pass

    paths_hash = hasher.hexdigest()

    # Gather all the version information independent of the input winkit
    all_versions = []
    for version, vc_path in vc_paths:
        Logs.info_once('[INFO] Using Visual Studio version {} installed at: {}'.format(version_string, path_string))
        conf.gather_msvc_2017_targets(paths_hash, all_versions, version, windows_kit, vc_path)

    # Populate the version list
    for check_version in all_versions:
        # The winkit is embedded in the first item of the tuple, split by the '/' character
        version_and_winkit = check_version[0].split('/')
        version_lst = check_version[1]
        if windows_kit == version_and_winkit[1]:
            # Rebuild the tuple with only the msvc version string and the version list information
            versions.append([version_and_winkit[0], version_lst])

@conf
def gather_msvc_2017_targets(conf, paths_hash, versions, version, windows_kit, vc_path):

    # Attempt to restore the versions from a cache environment if possible
    if restore_vs_version_from_cached_path(conf, version, windows_kit, paths_hash, versions):
        return

    # Looking for normal MSVC compilers!
    targets = []
    vcvarsall_bat = os.path.join(vc_path, 'VC', 'Auxiliary', 'Build', 'vcvarsall.bat')
    if os.path.isfile(vcvarsall_bat):
        try:
            targets.append(('x64', ('x64', conf.get_msvc_version('msvc', version, 'x64', windows_kit, vcvarsall_bat))))
        except conf.errors.ConfigurationError:
            pass
    if targets:
        versions.append(('msvc {}/{}'.format(version, windows_kit), targets))

    # Cache the versions tuples for the current vs version and environment so we can cut the cost of conf.get_msvc_version
    store_vs_version_to_cache(conf, version, windows_kit, paths_hash, versions)


def _get_prog_names(conf, compiler):
    if compiler=='intel':
        compiler_name = 'ICL'
        linker_name = 'XILINK'
        lib_name = 'XILIB'
    else:
        # assumes CL.exe
        compiler_name = 'CL'
        linker_name = 'LINK'
        lib_name = 'LIB'
    return compiler_name, linker_name, lib_name
    
@conf
def get_msvc_version(conf, compiler, version, target, windows_kit, vcvars):
    """
    Create a bat file to obtain the location of the libraries

    :param compiler: ?
    :param version: ?
    :target: ?
    :vcvars: ?
    :return: the location of msvc, the location of include dirs, and the library paths
    :rtype: tuple of strings
    """
    debug('msvc: get_msvc_version: %r %r %r %r', compiler, version, target, windows_kit)
    batfile = conf.bldnode.make_node('waf-print-msvc.bat')
    batfile.write("""@echo off
set INCLUDE=
set LIB=
call "%s" %s %s
echo PATH=%%PATH%%
echo INCLUDE=%%INCLUDE%%
echo LIB=%%LIB%%;%%LIBPATH%%
""" % (vcvars,target,windows_kit))
    sout = conf.cmd_and_log(['cmd', '/E:on', '/V:on', '/C', batfile.abspath()])
    lines = sout.splitlines()
    
    if not lines[0]:
        lines.pop(0)
    
    MSVC_PATH = MSVC_INCDIR = MSVC_LIBDIR = None
    for line in lines:
        if line.startswith('PATH='):
            path = line[5:]
            MSVC_PATH = path.split(';')
        elif line.startswith('INCLUDE='):
            MSVC_INCDIR = [i for i in line[8:].split(';') if i]
        elif line.startswith('LIB='):
            MSVC_LIBDIR = [i for i in line[4:].split(';') if i]
    if not MSVC_PATH or not MSVC_INCDIR or not MSVC_LIBDIR:
        conf.fatal('msvc: Could not find a valid architecture for building (get_msvc_version_3)')
    
    # Check if the compiler is usable at all.
    # The detection may return 64-bit versions even on 32-bit systems, and these would fail to run.
    env = dict(os.environ)
    env.update(PATH = path)
    compiler_name, linker_name, lib_name = _get_prog_names(conf, compiler)
    cxx = conf.find_program(compiler_name, path_list=MSVC_PATH, silent_output=True)
    cxx = conf.cmd_to_list(cxx)
    
    # delete CL if exists. because it could contain parameters wich can change cl's behaviour rather catastrophically.
    if 'CL' in env:
        del(env['CL'])
    
    try:
        try:
            conf.cmd_and_log(cxx + ['/help'], env=env)
        except Exception as e:
            debug('msvc: get_msvc_version: %r %r %r %r -> failure' % (compiler, version, target, windows_kit))
            debug(str(e))
            conf.fatal('msvc: cannot run the compiler (in get_msvc_version)')
        else:
            debug('msvc: get_msvc_version: %r %r %r %r -> OK', compiler, version, target, windows_kit)
    finally:
        conf.env[compiler_name] = ''

    # vcvarsall does not always resolve the windows sdk path with VS2015 + Win10, but we know where it is
    winsdk_path = _get_win_sdk_path(windows_kit, target)
    if winsdk_path:
        MSVC_PATH.append(winsdk_path)
    
    return (MSVC_PATH, MSVC_INCDIR, MSVC_LIBDIR)

def _get_win_sdk_path(windows_kit, arch):
    path = _find_win_sdk_root(windows_kit)
    if path:
        is_valid, version, bin_path = _is_valid_win_sdk(path, windows_kit.startswith('10'), windows_kit)
        if is_valid:
            if version == windows_kit:
                return str(os.path.join(bin_path, arch))
            else:
                Logs.debug('winsdk: Found a working windows SDK (%s), but it does not match the requested version (%s)' % (version, windows_kit))

    return ''

def _is_valid_win_sdk(path, is_universal_versioning, desired_version=''):
    # Successfully installed windows kits have rc.exe. This file is a downstream dependency of vcvarsall.bat.
    def _check_for_rc_file(path):
        rc_x64 = os.path.join(path, 'x64\\rc.exe')
        rc_x86 = os.path.join(path, 'x86\\rc.exe')
        return os.path.isfile(rc_x64) or os.path.isfile(rc_x86)

    bin_dir = os.path.join(path, 'bin')
    include_dir = os.path.join(path, 'include')
    if is_universal_versioning:
        potential_sdks = [desired_version] if desired_version else []
        if os.path.isdir(include_dir):
            # lexically sort the 10.xxx versions in reverse so that latest/highest is first
            potential_sdks += sorted(os.listdir(include_dir), reverse=True)
        sdk10_versions = [entry for entry in potential_sdks if entry.startswith('10.')]
        for sub_version in sdk10_versions:
            sub_version_folder = os.path.join(include_dir, sub_version)
            if os.path.isdir(os.path.join(sub_version_folder, 'um')):
                # check for rc.exe in the sub_version folder's bin, or in the root 10 bin, we just need at least one
                for bin_path in (os.path.join(os.path.join(path, 'bin'), sub_version), bin_dir):
                    if _check_for_rc_file(bin_path):
                        return True, sub_version, bin_path
    else:
        if _check_for_rc_file(bin_dir):
            version = path.split('\\')[-2]
            return True, version, bin_dir

    return False, '', ''

def _find_win_sdk_root(winsdk_hint):
    """
        Use winreg to find a valid installed windows kit.
        Returns empty string if no valid version was found.

        See visual studio compatibility charts here:
        https://www.visualstudio.com/en-us/productinfo/vs2015-compatibility-vs

        """
    try:
        installed_roots = Utils.winreg.OpenKey(Utils.winreg.HKEY_LOCAL_MACHINE, 'SOFTWARE\\Wow6432node\\Microsoft\\Windows Kits\\Installed Roots')
    except WindowsError:
        try:
            installed_roots = Utils.winreg.OpenKey(Utils.winreg.HKEY_LOCAL_MACHINE, 'SOFTWARE\\Microsoft\\Windows Kits\\Installed Roots')
        except WindowsError:
            return ''

    if winsdk_hint.startswith('10'):
        try:
            path, type = Utils.winreg.QueryValueEx(installed_roots, 'KitsRoot10')
            return path
        except WindowsError:
            pass

    elif winsdk_hint.startswith('8'):
        try:
            path, type = Utils.winreg.QueryValueEx(installed_roots, 'KitsRoot81')
            return path
        except WindowsError:
            pass

    return ''


def find_valid_wsdk_version():
    path = _find_win_sdk_root("10")
    if path:
        is_valid, version, bin_path = _is_valid_win_sdk(path, True)
        if is_valid:
            return version

    # No root for sdk 10 found, try 8.1
    path = _find_win_sdk_root("8.1")
    if path:
        is_valid, version, bin_path = _is_valid_win_sdk(path, False)
        if is_valid:
            return version

    return ''

    
@conf
def gather_wsdk_versions(conf, windows_kit, versions):
    """
    Use winreg to add the msvc versions to the input list

    :param versions: list to modify
    :type versions: list
    """
    version_pattern = re.compile('^v..?.?\...?.?')
    try:
        all_versions = Utils.winreg.OpenKey(Utils.winreg.HKEY_LOCAL_MACHINE, 'SOFTWARE\\Wow6432node\\Microsoft\\Microsoft SDKs\\Windows')
    except WindowsError:
        try:
            all_versions = Utils.winreg.OpenKey(Utils.winreg.HKEY_LOCAL_MACHINE, 'SOFTWARE\\Microsoft\\Microsoft SDKs\\Windows')
        except WindowsError:
            return
    index = 0
    while 1:
        try:
            version = Utils.winreg.EnumKey(all_versions, index)
        except WindowsError:
            break
        index = index + 1
        if not version_pattern.match(version):
            continue
        try:
            msvc_version = Utils.winreg.OpenKey(all_versions, version)
            path,type = Utils.winreg.QueryValueEx(msvc_version,'InstallationFolder')
        except WindowsError:
            continue
        if os.path.isfile(os.path.join(path, 'bin', 'SetEnv.cmd')):
            targets = []
            for target,arch in all_msvc_platforms:
                try:
                    targets.append((target, (arch, conf.get_msvc_version('wsdk', version, '/'+target, windows_kit, os.path.join(path, 'bin', 'SetEnv.cmd')))))
                except conf.errors.ConfigurationError:
                    pass
            versions.append(('wsdk ' + version[1:], targets))
    pass

@conf
def find_msvc(conf):
    """Due to path format limitations, limit operation only to native Win32. Yeah it sucks."""
    if sys.platform == 'cygwin':
        conf.fatal('MSVC module does not work under cygwin Python!')

    # the autodetection is supposed to be performed before entering in this method
    v = conf.env
    path = v['PATH']
    compiler = v['MSVC_COMPILER']
    version = v['MSVC_VERSION']

    compiler_name, linker_name, lib_name = _get_prog_names(conf, compiler)
    v.MSVC_MANIFEST = (compiler == 'msvc' and version >= 8) or (compiler == 'wsdk' and version >= 6) or (compiler == 'intel' and version >= 11)

    # compiler
    cxx = None
    if v['CXX']: cxx = v['CXX']
    elif 'CXX' in conf.environ: cxx = conf.environ['CXX']
    cxx = conf.find_program(compiler_name, var='CXX', path_list=path, silent_output=True)
    cxx = conf.cmd_to_list(cxx)

    # before setting anything, check if the compiler is really msvc
    env = dict(conf.environ)
    if path: env.update(PATH = ';'.join(path))
    if not conf.cmd_and_log(cxx + ['/nologo', '/help'], env=env):
        conf.fatal('the msvc compiler could not be identified')

    # c/c++ compiler
    v['CC'] = v['CXX'] = cxx[0]
    v['CC_NAME'] = v['CXX_NAME'] = 'msvc'

    # Bullseye code coverage
    if conf.is_option_true('use_bullseye_coverage'):
        # TODO: Error handling for this is opaque. This will fail the MSVS 2015 tool check,
        # and not say anything about bullseye being missing.
        try:
            covc = conf.find_program('covc',var='BULL_COVC',path_list = path, silent_output=True)
            covlink = conf.find_program('covlink',var='BULL_COVLINK',path_list = path, silent_output=True)
            covselect = conf.find_program('covselect',var='BULL_COVSELECT',path_list = path, silent_output=True)
            v['BULL_COVC'] = covc
            v['BULL_COVLINK'] = covlink
            v['BULL_COV_FILE'] = conf.CreateRootRelativePath(conf.options.bullseye_cov_file)
            # Update the coverage file with the region selections detailed in the settings regions parameters
            # NOTE: should we clear other settings at this point, or allow them to accumulate?
            # Maybe we need a flag for that in the setup?
            regions = conf.options.bullseye_coverage_regions.replace(' ','').split(',')
            conf.cmd_and_log(([covselect] + ['--file', v['BULL_COV_FILE'], '-a'] + regions))
        except:
            Logs.error('Could not find the Bullseye Coverage tools on the path, or coverage tools are not correctly installed. Coverage build disabled.')


    # linker
    if not v['LINK_CXX']:
        link = conf.find_program(linker_name, path_list=path, silent_output=True)
        if link: v['LINK_CXX'] = link
        else: conf.fatal('%s was not found (linker)' % linker_name)
        v['LINK'] = link

    if not v['LINK_CC']:
        v['LINK_CC'] = v['LINK_CXX']

    # staticlib linker
    if not v['AR']:
        stliblink = conf.find_program(lib_name, path_list=path, var='AR', silent_output=True)
        if not stliblink: return
        if '/NOLOGO' not in v['ARFLAGS']:
            v['ARFLAGS'] = ['/NOLOGO']

    # manifest tool. Not required for VS 2003 and below. Must have for VS 2005 and later
    if v.MSVC_MANIFEST:
        conf.find_program('MT', path_list=path, var='MT', silent_output=True)
        if '/NOLOGO' not in v['MTFLAGS']:
            v['MTFLAGS'] = ['/NOLOGO']

    # call configure on the waflib winres module to setup the environment for configure
    # conf.load('winres') caches the environment as part of the module load key, and we just modified
    # the environment, causing the cache to miss, and extra calls import/load the module
    # winres is loaded
    try:
        module = sys.modules['waflib.Tools.winres']
        func = getattr(module,'configure',None)
        if func:
            func(conf)
    except Exception as e:
        warn('Resource compiler not found. Compiling resource file is disabled')

