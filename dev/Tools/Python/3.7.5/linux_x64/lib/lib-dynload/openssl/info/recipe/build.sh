#!/bin/bash

PERL=${PREFIX}/bin/perl
declare -a _CONFIG_OPTS
_CONFIG_OPTS+=(--prefix=${PREFIX})
_CONFIG_OPTS+=(--libdir=lib)
_CONFIG_OPTS+=(shared)
_CONFIG_OPTS+=(threads)
_CONFIG_OPTS+=(enable-ssl2)
_CONFIG_OPTS+=(no-zlib)

_BASE_CC=$(basename "${CC}")
if [[ ${_BASE_CC} == *-* ]]; then
  # We are cross-compiling or using a specific compiler.
  # do not allow config to make any guesses based on uname.
  _CONFIGURATOR="perl ./Configure"
  case ${_BASE_CC} in
    i?86-*linux*)
      _CONFIG_OPTS+=(linux-generic32)
      CFLAGS="${CFLAGS} -Wa,--noexecstack"
      ;;
    x86_64-*linux*)
      _CONFIG_OPTS+=(linux-x86_64)
      CFLAGS="${CFLAGS} -Wa,--noexecstack"
      ;;
    aarch64-*-linux*)
      _CONFIG_OPTS+=(linux-aarch64)
      CFLAGS="${CFLAGS} -Wa,--noexecstack"
      ;;
    *powerpc64le-*linux*)
      _CONFIG_OPTS+=(linux-ppc64le)
      CFLAGS="${CFLAGS} -Wa,--noexecstack"
      ;;
    *darwin*)
      _CONFIG_OPTS+=(darwin64-x86_64-cc)
      ;;
  esac
else
  if [[ $(uname) == Darwin ]]; then
    _CONFIG_OPTS+=(darwin64-x86_64-cc)
    _CONFIGURATOR="perl ./Configure"
  else
    # Use config, which is a config.guess-like wrapper around Configure
    _CONFIGURATOR=./config
  fi
fi

CC=${CC}" ${CPPFLAGS} ${CFLAGS}" \
  ${_CONFIGURATOR} ${_CONFIG_OPTS[@]} ${LDFLAGS}

# This is not working yet. It may be important if we want to perform a parallel build
# as enabled by openssl-1.0.2d-parallel-build.patch where the dependency info is old.
# makedepend is a tool from xorg, but it seems to be little more than a wrapper for
# '${CC} -M', so my plan is to replace it with that, or add a package for it? This
# tool uses xorg headers (and maybe libraries) which is unfortunate.
# http://stackoverflow.com/questions/6362705/replacing-makedepend-with-cc-mm
# echo "echo \$*" > "${SRC_DIR}"/makedepend
# echo "${CC} -M $(echo \"\$*\" | sed s'# --##g')" >> "${SRC_DIR}"/makedepend
# chmod +x "${SRC_DIR}"/makedepend
# PATH=${SRC_DIR}:${PATH} make -j1 depend

make -j${CPU_COUNT} ${VERBOSE_AT}

# expected error: https://github.com/openssl/openssl/issues/6953
#    OK to ignore: https://github.com/openssl/openssl/issues/6953#issuecomment-415428340
rm test/recipes/04-test_err.t

# When testing this via QEMU, even though it ends printing:
# "ALL TESTS SUCCESSFUL."
# .. it exits with a failure code.
if [[ "${HOST}" == "${BUILD}" ]]; then
  make test > testsuite.log 2>&1 || true
  if ! cat testsuite.log | grep -i "all tests successful"; then
    echo "Testsuite failed!  See $(pwd)/testsuite.log for more info."
    exit 1
  fi
fi
make install_sw install_ssldirs

# https://github.com/ContinuumIO/anaconda-issues/issues/6424
if [[ ${HOST} =~ .*linux.* ]]; then
  if execstack -q "${PREFIX}"/lib/libcrypto.so.1.1 | grep -e '^X '; then
    echo "Error, executable stack found in libcrypto.so.1.1"
    exit 1
  fi
fi
