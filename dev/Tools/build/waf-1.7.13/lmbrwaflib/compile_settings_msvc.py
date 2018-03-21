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
from waflib.Configure import conf, Logs
from waflib import Options

def set_ltcg_threads(conf):
    """

    LTCG defaults to 4 threads, and can be set as high as 8.  Any setting over 8 is overridden to 1 by the linker.
    Any setting over the number of available hw threads incurs some thread scheduling overhead in a multiproc system.
    If the hw thread count is > 4, scale the setting up to 8.  This is independent of jobs or link_jobs running, and
    these threads are only active during the LTCG portion of the linker.  The overhead is ~50k/thread, and execution
    time doesn't scale linearly with threads.  Use the undocument linkflag /Time+ for extended information on the amount
    of time the linker spends in link time code generation
    """

    hw_thread_count = Options.options.jobs
    if hw_thread_count > 4:
        ltcg_thread_count = min(hw_thread_count, 8)

        conf.env['LINKFLAGS'] += [
            '/CGTHREADS:{}'.format(ltcg_thread_count)
        ]

@conf
def load_msvc_common_settings(conf):
    """
    Setup all compiler/linker flags with are shared over all targets using the microsoft compiler
    
    !!! But not the actual compiler, since the compiler depends on the target !!!
    """ 
    v = conf.env    
    
    # MT Tool
    v['MTFLAGS'] = ['/nologo']
    
    # AR Tools
    v['ARFLAGS'] += ['/NOLOGO']
    v['AR_TGT_F'] = '/OUT:'
    
    # CC/CXX Compiler
    v['CC_NAME']    = v['CXX_NAME']     = 'msvc'
    v['CC_SRC_F']   = v['CXX_SRC_F']    = []
    v['CC_TGT_F']   = v['CXX_TGT_F']    = ['/c', '/Fo']
    
    v['CPPPATH_ST']     = '/I%s'
    v['DEFINES_ST']     = '/D%s'
    
    v['PCH_FILE_ST']    = '/Fp%s'
    v['PCH_CREATE_ST']  = '/Yc%s'
    v['PCH_USE_ST']     = '/Yu%s'
    
    v['ARCH_ST']        = ['/arch:']

   # Linker
    v['CCLNK_SRC_F'] = v['CXXLNK_SRC_F'] = []
    v['CCLNK_TGT_F'] = v['CXXLNK_TGT_F'] = '/OUT:'
    
    v['LIB_ST']         = '%s.lib'
    v['LIBPATH_ST']     = '/LIBPATH:%s'
    v['STLIB_ST']       = '%s.lib'
    v['STLIBPATH_ST']   = '/LIBPATH:%s'
    
    v['cprogram_PATTERN']   = '%s.exe'
    v['cxxprogram_PATTERN'] = '%s.exe'      
    
    # shared library settings   
    v['CFLAGS_cshlib'] = v['CFLAGS_cxxshlib']       = []
    v['CXXFLAGS_cshlib'] = v['CXXFLAGS_cxxshlib']   = []
    
    v['LINKFLAGS_cshlib']   = ['/DLL']
    v['LINKFLAGS_cxxshlib'] = ['/DLL']
    
    v['cshlib_PATTERN']     = '%s.dll'
    v['cxxshlib_PATTERN']   = '%s.dll'
    
    # static library settings   
    v['CFLAGS_cstlib'] = v['CFLAGS_cxxstlib'] = []
    v['CXXFLAGS_cstlib'] = v['CXXFLAGS_cxxstlib'] = []
    
    v['LINKFLAGS_cxxstlib'] = []
    v['LINKFLAGS_cxxshtib'] = []
    
    v['cstlib_PATTERN']      = '%s.lib'
    v['cxxstlib_PATTERN']    = '%s.lib'
    
    # Set common compiler flags 
    COMMON_COMPILER_FLAGS = [
        '/nologo',      # Suppress Copyright and version number message
        '/W3',          # Warning Level 3
        '/WX',          # Treat Warnings as Errors
        '/MP',          # Allow Multicore compilation
        '/Gy',          # Enable Function-Level Linking
        '/GF',          # Enable string pooling
        '/Gm-',         # Disable minimal rebuild (causes issues with IB)
        '/fp:fast',     # Use fast (but not as precisce floatin point model)
        '/Zc:wchar_t',  # Use compiler native wchar_t
        '/Zc:forScope', # Force Conformance in for Loop Scope
        '/GR-',         # Disable RTTI
        '/Gd',          # Use _cdecl calling convention for all functions
#        '/FC'      #debug
        '/wd4573',      # Known issue, disabling "C2220: the usage of 'X' requires the compiler to capture 'this' but the current default capture mode does not allow it"
        '/wd4530',      # Known issue, Disabling "C4530: C++ exception handler used, but unwind semantics are not enabled. Specify /EHsc" until it is fixed
        '/wd4351',      # Default initialization of array warning.  This is a mostly minor issue, but MSVC made it L1.
        ]
        
    # Enable timing reports in the output
    if conf.is_option_true('enable_msvc_timings'):
        COMMON_COMPILER_FLAGS += ['/Bt+'] # debug, show compile times
        v['LINKFLAGS'] += ['/time+'] # debug, show link times

    # Copy common flags to prevent modifing references
    v['CFLAGS'] += COMMON_COMPILER_FLAGS[:]
    v['CXXFLAGS'] += COMMON_COMPILER_FLAGS[:]
    
    # Linker Flags
    v['LINKFLAGS'] += [
        '/NOLOGO',              # Suppress Copyright and version number message
        '/MANIFEST',            # Create a manifest file        
        '/LARGEADDRESSAWARE',   # tell the linker that the application can handle addresses larger than 2 gigabytes.        
        '/INCREMENTAL:NO',      # Disable Incremental Linking
        ]   
        
    # Compile options appended if compiler optimization is disabled
    v['COMPILER_FLAGS_DisableOptimization'] = [ '/Od' ] 
    
    # Compile options appended if debug symbols are generated
    # Create a external Program Data Base (PDB) for debugging symbols
    #v['COMPILER_FLAGS_DebugSymbols'] = [ '/Zi' ]
    v['COMPILER_FLAGS_DebugSymbols'] = [ '/Z7' ]
    
    # Linker flags when building with debug symbols
    v['LINKFLAGS_DebugSymbols'] = [ '/DEBUG', '/PDBALTPATH:%_PDB%' ]
    
    # Store settings for show includes option
    v['SHOWINCLUDES_cflags'] = ['/showIncludes']
    v['SHOWINCLUDES_cxxflags'] = ['/showIncludes']
    
    # Store settings for preprocess to file option
    v['PREPROCESS_cflags'] = ['/P', '/C']
    v['PREPROCESS_cxxflags'] = ['/P', '/C']
    v['PREPROCESS_cc_tgt_f'] = ['/c', '/Fi']
    v['PREPROCESS_cxx_tgt_f'] = ['/c', '/Fi']
    
    # Store settings for preprocess to file option
    v['DISASSEMBLY_cflags'] = ['/FAcs']
    v['DISASSEMBLY_cxxflags'] = ['/FAcs']
    v['DISASSEMBLY_cc_tgt_f'] = [ '/c', '/Fa']
    v['DISASSEMBLY_cxx_tgt_f'] = ['/c', '/Fa']

    # ASAN and ASLR
    v['LINKFLAGS_ASLR'] = ['/DYNAMICBASE']
    v['ASAN_cflags'] = ['/GS']
    v['ASAN_cxxflags'] = ['/GS'] 
    
    
@conf
def load_debug_msvc_settings(conf):
    """
    Setup all compiler/linker flags with are shared over all targets using the microsoft compiler
    for the "debug" configuration   
    """
    v = conf.env    
    conf.load_msvc_common_settings()
    
    COMPILER_FLAGS = [
        '/Od',      # Disable all optimizations
        '/Ob0',     # Disable all inling
        '/Oy-',     # Don't omit the frame pointer
        '/RTC1',    # Add basic Runtime Checks
        '/bigobj',  # Ensure we can store enough objects files
        ]
        
    v['CFLAGS'] += COMPILER_FLAGS
    v['CXXFLAGS'] += COMPILER_FLAGS

    if conf.is_option_true('use_recode'):
        Logs.pprint('CYAN', 'Enabling Recode-safe defines for Debug build')
        v['DEFINES'] += ['AZ_PROFILE_DISABLE_THREAD_LOCAL'] # Disable AZ Profiler's thread-local optimization, as it conflicts with Recode

@conf
def load_profile_msvc_settings(conf):
    """
    Setup all compiler/linker flags with are shared over all targets using the microsoft compiler
    for the "profile" configuration 
    """
    v = conf.env
    conf.load_msvc_common_settings()
    
    COMPILER_FLAGS = [
        '/Ox',      # Full optimization
        '/Ob2',     # Inline any suitable function
        '/Ot',      # Favor fast code over small code
        '/Oi',      # Use Intrinsic Functions
        '/Oy-',     # Don't omit the frame pointer      
        ]
        
    v['CFLAGS'] += COMPILER_FLAGS
    v['CXXFLAGS'] += COMPILER_FLAGS
    
    if conf.is_option_true('use_recode'):
        Logs.pprint('CYAN', 'Enabling Recode-safe linker settings and defines for Profile build')
        v['DEFINES'] += ['AZ_PROFILE_DISABLE_THREAD_LOCAL'] # Disable AZ Profiler's thread-local optimization, as it conflicts with Recode
        v['LINKFLAGS'] += [
                                   # No /OPT:REF or /OPT:ICF for Recode builds
            '/FUNCTIONPADMIN:14'   # Reserve function padding for patch injection (improves Recoding)
            ]
    else:
        v['LINKFLAGS'] += [
            '/OPT:REF',            # elimination of unreferenced functions/data
            '/OPT:ICF',            # comdat folding
            ]
        
@conf
def load_performance_msvc_settings(conf):
    """
    Setup all compiler/linker flags with are shared over all targets using the microsoft compiler
    for the "performance" configuration 
    """
    v = conf.env
    conf.load_msvc_common_settings()
    
    COMPILER_FLAGS = [
        '/Ox',      # Full optimization
        '/Ob2',     # Inline any suitable function
        '/Ot',      # Favor fast code over small code
        '/Oi',      # Use Intrinsic Functions
        '/Oy',      # omit the frame pointer        
        '/GL',      # Whole-program optimization
        ]
        
    v['CFLAGS'] += COMPILER_FLAGS
    v['CXXFLAGS'] += COMPILER_FLAGS
    
    v['LINKFLAGS'] += [
        '/OPT:REF', # Eliminate not referenced functions/data
        '/OPT:ICF', # Perform Comdat folding
        '/LTCG',    # Link time code generation
        ]   

    v['ARFLAGS'] += [
        '/LTCG',    # Link time code generation
        ]

    set_ltcg_threads(conf)

@conf
def load_release_msvc_settings(conf):
    """
    Setup all compiler/linker flags with are shared over all targets using the microsoft compiler
    for the "release" configuration 
    """
    v = conf.env
    conf.load_msvc_common_settings()
    
    COMPILER_FLAGS = [
        '/Ox',      # Full optimization
        '/Ob2',     # Inline any suitable function
        '/Ot',      # Favor fast code over small code
        '/Oi',      # Use Intrinsic Functions
        '/Oy',      # omit the frame pointer        
        '/GL',      # Whole-program optimization
        ]
        
    v['CFLAGS'] += COMPILER_FLAGS
    v['CXXFLAGS'] += COMPILER_FLAGS
    
    v['LINKFLAGS'] += [
        '/OPT:REF', # Eliminate not referenced functions/data
        '/OPT:ICF', # Perform Comdat folding
        '/LTCG',    # Link time code generation
        ]   
        
    v['ARFLAGS'] += [
        '/LTCG',    # Link time code generation
        ]
        
    set_ltcg_threads(conf)
        
