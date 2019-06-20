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
from lumberyard import deprecated

@deprecated("Logic replaced in load_msvc_common_settings with a combination of setting ENV values and env settings in the common.msvc.json")
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
    v['MTFLAGS'] = ['/NOLOGO']
    
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
        v['SET_LTCG_THREADS_FLAG'] = True
        ltcg_thread_count = min(hw_thread_count, 8)
        v['LTCG_THREADS_COUNT'] = str(ltcg_thread_count)
    else:
        v['SET_LTCG_THREADS_FLAG'] = False

