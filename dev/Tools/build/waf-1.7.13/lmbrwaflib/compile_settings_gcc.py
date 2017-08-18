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

@conf
def load_gcc_common_settings(conf):
    """
    Setup all compiler/linker flags with are shared over all targets using the gcc compiler
    
    !!! But not the actual compiler, since the compiler depends on the target !!!
    """
    v = conf.env
    
    # Figure out GCC compiler version
    try:
        conf.get_cc_version( [ v['CC'] ], gcc=True)
    except: # Can happen if we don't have an GCC installed (code drop stripped of GCC cross compiler)
        conf.env.CC_VERSION = ('0', '0', '0')

    # AR Tools
    v['ARFLAGS'] = 'rcs'
    v['AR_TGT_F'] = ''
    
    # CC/CXX Compiler   
    v['CC_NAME']    = v['CXX_NAME']     = 'gcc' 
    v['CC_SRC_F']   = v['CXX_SRC_F']    = []
    v['CC_TGT_F']   = v['CXX_TGT_F']    = ['-c', '-o']
    
    v['CPPPATH_ST']     = '-I%s'
    v['DEFINES_ST']     = '-D%s'

    # Linker
    v['CCLNK_SRC_F'] = v['CXXLNK_SRC_F'] = []
    v['CCLNK_TGT_F'] = v['CXXLNK_TGT_F'] = '-o'
    
    v['LIB_ST']         = '-l%s'
    v['LIBPATH_ST']     = '-L%s'
    v['STLIB_ST']       = '-l%s'
    v['STLIBPATH_ST']   = '-L%s'
    
    # shared library settings   
    v['CFLAGS_cshlib'] = v['CFLAGS_cxxshlib']       = ['-fpic'] 
    v['CXXFLAGS_cshlib'] = v['CXXFLAGS_cxxshlib']   = ['-fpic']
        
    v['LINKFLAGS_cshlib']   = ['-shared']
    v['LINKFLAGS_cxxshlib'] = ['-shared']
    
    # static library settings   
    v['CFLAGS_cstlib'] = v['CFLAGS_cxxstlib']   = []     
    v['CXXFLAGS_cstlib'] = v['CXXFLAGS_cxxstlib']   = []
    
    v['LINKFLAGS_cxxstlib'] = ['-Wl,-Bstatic']
    v['LINKFLAGS_cxxshtib'] = ['-Wl,-Bstatic']
        
    # Set common compiler flags 
    COMMON_COMPILER_FLAGS = [
        '-Wall',                    # Generate more warnings
        '-Werror',                  # Tread Warnings as Errors
        '-ffast-math',              # Enable fast math
        
        '-fvisibility=hidden',      # ELF image symbol visibility
        
        # Disable some warnings
        '-Wno-char-subscripts',     # using type char as array index, which can be signed on some machines
        '-Wno-unknown-pragmas',     # pragmas not understood by GCC
        '-Wno-unused-variable',     # 
        '-Wno-unused-value',        #
        '-Wno-parentheses',         # missing parentheses is some abiguous contexts
        '-Wno-switch',              # missing case for enum types
        '-Wno-unused-function',     # unused inline static functions / no definition to static functions
        '-Wno-multichar',           # multicharacter constant being used
        '-Wno-format-security',     # non string literal format for printf and scanf
        '-Wno-empty-body',          # 
        '-Wno-comment',             # comment start sequence within a comment
        '-Wno-sign-compare',        # comparison between signed and unsigned
        '-Wno-narrowing',           # narrowing conversions within '{ }'
        '-Wno-write-strings',       # string literal to non-const char *
        '-Wno-format',              # type checks for printf and scanf
        
        '-Wno-strict-aliasing',             #
        '-Wno-unused-but-set-variable',     #
        '-Wno-maybe-uninitialized',         # if GCC detects a code path where a variable *might* be uninitialized
        '-Wno-strict-overflow',             # when the compiler optimizations assume signed overflow does not occur
        '-Wno-uninitialized',               # when a variable is used without being uninitialized or *might* be clobbered by a setjmp
        '-Wno-unused-local-typedefs',       #
        ]

    if conf.env.CC_VERSION >= ('4', '8', '0'):
        COMMON_COMPILER_FLAGS += [
            '-Wno-unused-result',               # warn_unused_result function attribute usage
            '-Wno-sizeof-pointer-memaccess',    # suspicious use of sizeof in certain built-in memory/string functions
            '-Wno-array-bounds',                # subscripts to arrays that are always out of bounds when -ftree-vrp is active
        ]
        
    # Copy common flags to prevent modifing references
    v['CFLAGS'] += COMMON_COMPILER_FLAGS[:]
    
    
    v['CXXFLAGS'] += COMMON_COMPILER_FLAGS[:] + [
        '-fno-rtti',                    # Disable RTTI
        '-fno-exceptions',              # Disable Exceptions    
        '-fvisibility-inlines-hidden',  # comparing function pointers between two shared objects
        
        # Disable some C++ specific warnings    
        '-Wno-invalid-offsetof',        # offsetof used on non POD types
        '-Wno-reorder',                 # member initialization differs from declaration order
        '-Wno-conversion-null',         # conversion between NULL and non pointer types
        '-Wno-overloaded-virtual',      # function declaration hides a virtual from a base class
        '-Wno-c++0x-compat',            # 
    ]
    
    # Linker Flags
    v['LINKFLAGS'] += []    
    
    v['SHLIB_MARKER']   = '-Wl,-Bdynamic'
    v['STLIB_MARKER']   = '-Wl,-Bstatic'

    if not len(v['SONAME_ST']):
        v['SONAME_ST']      = '-Wl,-h,%s'
    
    # Compile options appended if compiler optimization is disabled
    v['COMPILER_FLAGS_DisableOptimization'] = [ '-O0' ] 
    
    # Compile options appended if debug symbols are generated   
    v['COMPILER_FLAGS_DebugSymbols'] = [ '-g' ] 
    
    # Linker flags when building with debug symbols
    v['LINKFLAGS_DebugSymbols'] = []
    
    # Store settings for show includes option
    v['SHOWINCLUDES_cflags'] = ['-H']
    v['SHOWINCLUDES_cxxflags'] = ['-H']
    
    # Store settings for preprocess to file option
    v['PREPROCESS_cflags'] = ['-E', '-dD']
    v['PREPROCESS_cxxflags'] = ['-E', '-dD']
    v['PREPROCESS_cc_tgt_f'] = ['-o']
    v['PREPROCESS_cxx_tgt_f'] = ['-o']
    
    # Store settings for preprocess to file option
    v['DISASSEMBLY_cflags'] = ['-S', '-fverbose-asm']
    v['DISASSEMBLY_cxxflags'] = ['-S', '-fverbose-asm']
    v['DISASSEMBLY_cc_tgt_f'] = ['-o']
    v['DISASSEMBLY_cxx_tgt_f'] = ['-o']

    # ASAN and ASLR
    v['LINKFLAGS_ASLR'] = []
    v['ASAN_cflags'] = []
    v['ASAN_cxxflags'] = []

@conf
def load_debug_gcc_settings(conf):
    """
    Setup all compiler/linker flags with are shared over all targets using the gcc compiler
    for the "debug" configuration   
    """ 
    v = conf.env    
    
    conf.load_gcc_common_settings()
    
    COMPILER_FLAGS = [
        '-O0',              # No optimization
        '-g',               # debug symbols
        '-fno-inline',      # don't inline functions
        '-fstack-protector' # Add additional checks to catch stack corruption issues
        ]
    
    v['CFLAGS'] += COMPILER_FLAGS
    v['CXXFLAGS'] += COMPILER_FLAGS
    
@conf
def load_profile_gcc_settings(conf):
    """
    Setup all compiler/linker flags with are shared over all targets using the gcc compiler
    for the "profile" configuration 
    """
    v = conf.env
    conf.load_gcc_common_settings()
    
    COMPILER_FLAGS = [
        '-O2',
        ]
    
    v['CFLAGS'] += COMPILER_FLAGS
    v['CXXFLAGS'] += COMPILER_FLAGS 
    
@conf
def load_performance_gcc_settings(conf):
    """
    Setup all compiler/linker flags with are shared over all targets using the gcc compiler
    for the "performance" configuration 
    """
    v = conf.env
    conf.load_gcc_common_settings()
    
    COMPILER_FLAGS = [
        '-O2',
        ]
    
    v['CFLAGS'] += COMPILER_FLAGS
    v['CXXFLAGS'] += COMPILER_FLAGS
    
@conf
def load_release_gcc_settings(conf):
    """
    Setup all compiler/linker flags with are shared over all targets using the gcc compiler
    for the "release" configuration 
    """
    v = conf.env
    conf.load_gcc_common_settings()
    
    COMPILER_FLAGS = [
        '-O2',
        ]
    
    v['CFLAGS'] += COMPILER_FLAGS
    v['CXXFLAGS'] += COMPILER_FLAGS 
    