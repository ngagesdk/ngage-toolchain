# Copyright 2020 The Emscripten Authors.  All rights reserved.
# Emscripten is available under two separate licenses, the MIT license and the
# University of Illinois/NCSA Open Source License.  Both these licenses can be
# found in the LICENSE file.

from .toolchain_profiler import ToolchainProfiler

import json
import logging
import os
import re
# import shlex
import shutil
# import subprocess
# import sys
from typing import Set, Dict
# from subprocess import PIPE

# from . import cache
# from . import diagnostics
from . import response_file
from . import shared
# from . import webassembly
from . import config
from . import utils
# from .shared import EPOC32_CC, EPOC32_CXX
# from .shared import LLVM_NM, EMCC, EMAR, EMXX, EMRANLIB
from .shared import EPOC32_LD
# from .shared import LLVM_OBJCOPY
from .shared import run_process, check_call, exit_with_error
# from .shared import path_from_root
from .shared import asmjs_mangle, DEBUG
# from .shared import LLVM_DWARFDUMP
from .shared import demangle_c_symbol_name
from .shared import get_emscripten_temp_dir, exe_suffix, is_c_symbol
# from .utils import WINDOWS
from .settings import settings
# from .feature_matrix import UNSUPPORTED

logger = logging.getLogger('building')

#  Building
binaryen_checked = False
EXPECTED_BINARYEN_VERSION = 121

_is_ar_cache: Dict[str, bool] = {}
# the exports the user requested
user_requested_exports: Set[str] = set()


# def get_building_env():
#     cache.ensure()
#     env = os.environ.copy()
#     # point CC etc. to the em* tools.
#     env['CC'] = EMCC
#     env['CXX'] = EMXX
#     env['AR'] = EMAR
#     env['LD'] = EMCC
#     env['NM'] = LLVM_NM
#     env['LDSHARED'] = EMCC
#     env['RANLIB'] = EMRANLIB
#     env['EMSCRIPTEN_TOOLS'] = path_from_root('tools')
#     env['HOST_CC'] = EPOC32_CC
#     env['HOST_CXX'] = EPOC32_CXX
#     env['HOST_CFLAGS'] = '-W' # if set to nothing, CFLAGS is used, which we don't want
#     env['HOST_CXXFLAGS'] = '-W' # if set to nothing, CXXFLAGS is used, which we don't want
#     env['PKG_CONFIG_LIBDIR'] = cache.get_sysroot_dir('local/lib/pkgconfig') + os.path.pathsep + cache.get_sysroot_dir('lib/pkgconfig')
#     env['PKG_CONFIG_PATH'] = os.environ.get('EM_PKG_CONFIG_PATH', '')
#     env['EMSCRIPTEN'] = path_from_root()
#     env['PATH'] = cache.get_sysroot_dir('bin') + os.pathsep + env['PATH']
#     env['ACLOCAL_PATH'] = cache.get_sysroot_dir('share/aclocal')
#     env['CROSS_COMPILE'] = path_from_root('em') # produces /path/to/emscripten/em , which then can have 'cc', 'ar', etc appended to it
#     return env


def lld_flags_for_executable(external_symbols):
    cmd = []
    if external_symbols:
        if settings.INCLUDE_FULL_LIBRARY:
            # When INCLUDE_FULL_LIBRARY is set try to export every possible
            # native dependency of a JS function.
            all_deps = set()
            for deps in external_symbols.values():
                for dep in deps:
                    if dep not in all_deps:
                        cmd.append('--export-if-defined=' + dep)
                    all_deps.add(dep)
        stub = create_stub_object(external_symbols)
        cmd.append(stub)

    if not settings.ERROR_ON_UNDEFINED_SYMBOLS:
        cmd.append('--import-undefined')

    c_exports = [e for e in settings.EXPORTED_FUNCTIONS if is_c_symbol(e)]
    # Strip the leading underscores
    c_exports = [demangle_c_symbol_name(e) for e in c_exports]
    # Filter out symbols external/JS symbols
    c_exports = [e for e in c_exports if e not in external_symbols]
    c_exports += settings.REQUIRED_EXPORTS
    # if settings.MAIN_MODULE:
    #     c_exports += side_module_external_deps(external_symbols)
    for export in c_exports:
        if settings.ERROR_ON_UNDEFINED_SYMBOLS:
            cmd.append(f'--export={export}')
        else:
            cmd.append(f'--export-if-defined={export}')

    return cmd


def link_lld(args, target: str, external_symbols=None):
    if not os.path.exists(EPOC32_LD):
        exit_with_error('linker binary not found in LLVM directory: %s', EPOC32_LD)
    # runs lld to link things.
    # lld doesn't currently support --start-group/--end-group since the
    # semantics are more like the windows linker where there is no need for
    # grouping.
    args = [a for a in args if a not in ('--start-group', '--end-group')]

    # Emscripten currently expects linkable output (SIDE_MODULE/MAIN_MODULE) to
    # include all archive contents.
    if settings.LINKABLE:
        args.insert(0, '--whole-archive')
        args.append('--no-whole-archive')

    cmd = [EPOC32_LD, '-o', target] + args

    # For relocatable output (generating an object file) we don't pass any of the
    # normal linker flags that are used when building and executable
    if '--relocatable' not in args and '-r' not in args:
        cmd += lld_flags_for_executable(external_symbols)

    cmd = get_command_with_possible_response_file(cmd)
    check_call(cmd)


def get_command_with_possible_response_file(cmd):
    # One of None, 0 or 1. (None: do default decision, 0: force disable, 1: force enable)
    force_response_files = os.getenv('EM_FORCE_RESPONSE_FILES')

    # Different OS have different limits. The most limiting usually is Windows one
    # which is set at 8191 characters. We could just use that, but it leads to
    # problems when invoking shell wrappers (e.g. emcc.bat), which, in turn,
    # pass arguments to some longer command like `(full path to Clang) ...args`.
    # In that scenario, even if the initial command line is short enough, the
    # subprocess can still run into the Command Line Too Long error.
    # Reduce the limit by ~1K for now to be on the safe side, but we might need to
    # adjust this in the future if it turns out not to be enough.
    if (len(shared.shlex_join(cmd)) <= 7000 and force_response_files != '1') or force_response_files == '0':
        return cmd

    logger.debug('using response file for %s' % cmd[0])
    filename = response_file.create_response_file(cmd[1:], shared.TEMP_DIR)
    new_cmd = [cmd[0], "@" + filename]
    return new_cmd


# def emar(action, output_filename, filenames, stdout=None, stderr=None, env=None):
#     utils.delete_file(output_filename)
#     cmd = [EMAR, action, output_filename] + filenames
#     cmd = get_command_with_possible_response_file(cmd)
#     run_process(cmd, stdout=stdout, stderr=stderr, env=env)
#
#     if 'c' in action:
#         assert os.path.exists(output_filename), 'emar could not create output file: ' + output_filename


def opt_level_to_str(opt_level, shrink_level=0):
    # convert opt_level/shrink_level pair to a string argument like -O1
    if opt_level == 0:
        return '-O0'
    if shrink_level == 1:
        return '-Os'
    elif shrink_level >= 2:
        return '-Oz'
    else:
        return f'-O{min(opt_level, 3)}'


def version_split(v):
    """Split version setting number (e.g. 162000) into versions string (e.g. "16.2.0")
    """
    v = str(v).rjust(6, '0')
    assert len(v) == 6
    m = re.match(r'(\d{2})(\d{2})(\d{2})', v)
    major, minor, rev = m.group(1, 2, 3)
    return f'{int(major)}.{int(minor)}.{int(rev)}'


def strip(infile, outfile, debug=False, sections=None):
    """Strip DWARF and/or other specified sections from a wasm file"""
    cmd = [LLVM_OBJCOPY, infile, outfile]
    if debug:
        cmd += ['--remove-section=.debug*']
    if sections:
        cmd += ['--remove-section=' + section for section in sections]
    check_call(cmd)


def instrument_js_for_asan(js_file):
    logger.debug('instrumenting JS memory accesses for ASan')
    return acorn_optimizer(js_file, ['asanify'])


def is_ar(filename):
    """Return True if a the given filename is an ar archive, False otherwise.
    """
    try:
        header = open(filename, 'rb').read(8)
    except Exception as e:
        logger.debug('is_ar failed to test whether file \'%s\' is a llvm archive file! Failed on exception: %s' % (filename, e))
        return False

    return header in (b'!<arch>\n', b'!<thin>\n')


def save_intermediate(src, dst):
    if DEBUG:
        dst = 'emcc-%02d-%s' % (save_intermediate.counter, dst)
        save_intermediate.counter += 1
        dst = os.path.join(shared.CANONICAL_TEMP_DIR, dst)
        logger.debug('saving debug copy %s' % dst)
        shutil.copyfile(src, dst)


save_intermediate.counter = 0  # type: ignore
