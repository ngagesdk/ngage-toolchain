#!/usr/bin/env python3
# Copyright 2011 The Emscripten Authors.  All rights reserved.
# Emscripten is available under two separate licenses, the MIT license and the
# University of Illinois/NCSA Open Source License.  Both these licenses can be
# found in the LICENSE file.

"""emcc - compiler helper script
=============================

emcc is a drop-in replacement for a compiler like gcc or clang.

See  emcc --help  for details.

emcc can be influenced by a few environment variables:

  NGAGESDK_DEBUG - "1" will log out useful information during compilation, as well as
                   save each compiler step as an emcc-* file in the temp dir
                   (by default /tmp/emscripten_temp). "2" will save additional emcc-*
                   steps, that would normally not be separately produced (so this
                   slows down compilation).
"""
import textwrap

from tools.toolchain_profiler import ToolchainProfiler

import json
import logging
import os
import re
import shlex
import shutil
import sys
import time
import tarfile
from enum import Enum, auto, unique
from subprocess import PIPE


from tools import building
from tools import cache
from tools import colored_logger
from tools import diagnostics
from tools import ports
from tools import shared
from tools import utils
# from tools import shared, system_libs, utils, ports
from tools.shared import unsuffixed, unsuffixed_basename, get_file_suffix
from tools.shared import run_process, exit_with_error, DEBUG
from tools.shared import in_temp, OFormat
# from tools.shared import DYLIB_EXTENSIONS
from tools.response_file import substitute_response_files
from tools import config
# from tools import cache
from tools.settings import default_setting, user_settings, settings, MEM_SIZE_SETTINGS, COMPILE_TIME_SETTINGS
from tools.utils import read_file, removeprefix, memoize, WINDOWS
# from tools import feature_matrix

logger = logging.getLogger('ngagesdk')

# In git checkouts of emscripten `bootstrap.py` exists to run post-checkout
# steps.  In packaged versions (e.g. emsdk) this file does not exist (because
# it is excluded in tools/install.py) and these steps are assumed to have been
# run already.
# if os.path.exists(utils.path_from_root('.git')) and os.path.exists(utils.path_from_root('bootstrap.py')):
#     import bootstrap
#     bootstrap.check()

PREPROCESSED_EXTENSIONS = {'.i', '.ii'}
ASSEMBLY_EXTENSIONS = {'.s'}
HEADER_EXTENSIONS = {'.h', '.hxx', '.hpp', '.hh', '.H', '.HXX', '.HPP', '.HH'}
SOURCE_EXTENSIONS = {
                        '.c', '.i', # C
                        '.cppm', '.pcm', '.cpp', '.cxx', '.cc', '.c++', '.CPP', '.CXX', '.C', '.CC', '.C++', '.ii', # C++
                        # '.m', '.mi', '.mm', '.mii', # ObjC/ObjC++
                        # '.bc', '.ll', # LLVM IR
                        '.S', # asm with preprocessor
                        os.devnull # consider the special endingless filenames like /dev/null to be C
                    } | PREPROCESSED_EXTENSIONS

# These symbol names are allowed in INCOMING_MODULE_JS_API but are not part of the
# default set.
EXTRA_INCOMING_JS_API = [
    'fetchSettings'
]

SIMD_INTEL_FEATURE_TOWER = ['-msse', '-msse2', '-msse3', '-mssse3', '-msse4.1', '-msse4.2', '-msse4', '-mavx', '-mavx2']
SIMD_NEON_FLAGS = ['-mfpu=neon']
LINK_ONLY_FLAGS = {
    '--cpuprofiler', '--embed-file',
    '--emit-symbol-map', '--exclude-file',
    '--ignore-dynamic-linking',
    '--oformat', '--output_eol',
    '--preload-file', '--profiling-funcs',
    '--proxy-to-worker', '--shell-file', '--source-map-base',
    '--threadprofiler', '--use-preload-plugins'
}
CLANG_FLAGS_WITH_ARGS = {
    '-MT', '-MF', '-MJ', '-MQ', '-D', '-U', '-o', '-x',
    '-Xpreprocessor', '-include', '-imacros', '-idirafter',
    '-iprefix', '-iwithprefix', '-iwithprefixbefore',
    '-isysroot', '-imultilib', '-A', '-isystem', '-iquote',
    '-install_name', '-compatibility_version', '-mllvm',
    '-current_version', '-I', '-L', '-include-pch',
    '-undefined', '-target', '-Xlinker', '-Xclang', '-z'
}


@unique
class Mode(Enum):
    # Used any time we are not linking, including PCH, pre-processing, etc
    COMPILE_ONLY = auto()
    # This is the default mode, in the absence of any flags such as -c, -E, etc
    COMPILE_AND_LINK = auto()


class NGageCCState:
    def __init__(self, args):
        self.mode = Mode.COMPILE_AND_LINK
        # Using tuple here to prevent accidental mutation
        self.orig_args = tuple(args)
        # List of link options paired with their position on the command line [(i, option), ...].
        self.link_flags = []
        self.lib_dirs = []

    def has_link_flag(self, f):
        return f in [x for _, x in self.link_flags]

    def add_link_flag(self, i, flag):
        if flag.startswith('-L'):
            self.lib_dirs.append(flag[2:])
        # Link flags should be adding in strictly ascending order
        assert not self.link_flags or i > self.link_flags[-1][0], self.link_flags
        self.link_flags.append((i, flag))

    def append_link_flag(self, flag):
        if self.link_flags:
            index = self.link_flags[-1][0] + 1
        else:
            index = 1
        self.add_link_flag(index, flag)


class NGageOptions:
    def __init__(self):
        self.target = ''
        self.output_file = None
        self.input_files = []
        # self.no_minify = False
        self.post_link = False
        self.save_temps = False
        self.executable = False
        self.compiler_wrapper = None
        self.oformat = None
        self.requested_debug = None
        self.emit_symbol_map = False
        self.exclude_files = []
        self.shell_path = None
        self.source_map_base = ''
        self.emrun = False
        self.cpu_profiler = False
        self.memory_profiler = False
        # self.use_preload_cache = False
        # self.use_preload_plugins = False
        self.valid_abspaths = []
        # Specifies the line ending format to use for all generated text files.
        # Defaults to using the native EOL on each platform (\r\n on Windows, \n on
        # Linux & MacOS)
        self.output_eol = os.linesep
        self.no_entry = False
        self.shared = False
        self.relocatable = False
        # self.reproduce = None
        self.syntax_only = False
        self.dash_c = False
        self.dash_E = False
        self.dash_S = False
        self.dash_M = False
        self.input_language = None
        self.nostdlib = False
        self.nostdlibxx = False
        self.nodefaultlibs = False
        self.nolibc = False
        self.nostartfiles = False
        self.sanitize_minimal_runtime = False
        self.sanitize = set()


def create_reproduce_file(name, args):
    def make_relative(filename):
        filename = os.path.normpath(os.path.abspath(filename))
        filename = os.path.splitdrive(filename)[1]
        filename = filename[1:]
        return filename

    root = unsuffixed_basename(name)
    with tarfile.open(name, 'w') as reproduce_file:
        reproduce_file.add(shared.path_from_root('emscripten-version.txt'), os.path.join(root, 'version.txt'))

        with shared.get_temp_files().get_file(suffix='.tar') as rsp_name:
            with open(rsp_name, 'w') as rsp:
                ignore_next = False
                output_arg = None

                for arg in args:
                    ignore = ignore_next
                    ignore_next = False
                    if arg.startswith('--reproduce='):
                        continue

                    if len(arg) > 2 and arg.startswith('-o'):
                        rsp.write('-o\n')
                        arg = arg[3:]
                        output_arg = True
                        ignore = True

                    if output_arg:
                        # If -o path contains directories, "emcc @response.txt" will likely
                        # fail because the archive we are creating doesn't contain empty
                        # directories for the output path (-o doesn't create directories).
                        # Strip directories to prevent the issue.
                        arg = os.path.basename(arg)
                        output_arg = False

                    if not arg.startswith('-') and not ignore:
                        relpath = make_relative(arg)
                        rsp.write(relpath + '\n')
                        reproduce_file.add(arg, os.path.join(root, relpath))
                    else:
                        rsp.write(arg + '\n')

                    if ignore:
                        continue

                    if arg in CLANG_FLAGS_WITH_ARGS:
                        ignore_next = True

                    if arg == '-o':
                        output_arg = True

            reproduce_file.add(rsp_name, os.path.join(root, 'response.txt'))


def expand_byte_size_suffixes(value):
    """Given a string with KB/MB size suffixes, such as "32MB", computes how
    many bytes that is and returns it as an integer.
    """
    value = value.strip()
    match = re.match(r'^(\d+)\s*([kmgt]?b)?$', value, re.I)
    if not match:
        exit_with_error("invalid byte size `%s`.  Valid suffixes are: kb, mb, gb, tb" % value)
    value, suffix = match.groups()
    value = int(value)
    if suffix:
        size_suffixes = {suffix: 1024 ** i for i, suffix in enumerate(['b', 'kb', 'mb', 'gb', 'tb'])}
        value *= size_suffixes[suffix.lower()]
    return value


def apply_user_settings():
    """Take a map of users settings {NAME: VALUE} and apply them to the global
    settings object.
    """

    # # Stash a copy of all available incoming APIs before the user can potentially override it
    # settings.ALL_INCOMING_MODULE_JS_API = settings.INCOMING_MODULE_JS_API + EXTRA_INCOMING_JS_API

    for key, value in user_settings.items():
        if key in settings.internal_settings:
            exit_with_error('%s is an internal setting and cannot be set from command line', key)

        # map legacy settings which have aliases to the new names
        # but keep the original key so errors are correctly reported via the `setattr` below
        user_key = key
        if key in settings.legacy_settings and key in settings.alt_names:
            key = settings.alt_names[key]

        # In those settings fields that represent amount of memory, translate suffixes to multiples of 1024.
        if key in MEM_SIZE_SETTINGS:
            value = str(expand_byte_size_suffixes(value))

        filename = None
        if value and value[0] == '@':
            filename = removeprefix(value, '@')
            if not os.path.isfile(filename):
                exit_with_error('%s: file not found parsing argument: %s=%s' % (filename, key, value))
            value = read_file(filename).strip()
        else:
            value = value.replace('\\', '\\\\')

        expected_type = settings.types.get(key)

        if filename and expected_type == list and value.strip()[0] != '[':
            # Prefer simpler one-line-per value parser
            value = parse_symbol_list_file(value)
        else:
            try:
                value = parse_value(value, expected_type)
            except Exception as e:
                exit_with_error(f'error parsing "-s" setting "{key}={value}": {e}')

        setattr(settings, user_key, value)

        if key == 'EXPORTED_FUNCTIONS':
            # used for warnings in emscripten.py
            settings.USER_EXPORTS = settings.EXPORTED_FUNCTIONS.copy()

        # TODO(sbc): Remove this legacy way.
        if key == 'WASM_OBJECT_FILES':
            settings.LTO = 0 if value else 'full'

        if key == 'JSPI':
            settings.ASYNCIFY = 2
        if key == 'JSPI_IMPORTS':
            settings.ASYNCIFY_IMPORTS = value
        if key == 'JSPI_EXPORTS':
            settings.ASYNCIFY_EXPORTS = value


def cxx_to_c_compiler(cxx):
    # Convert C++ compiler name into C compiler name
    dirname, basename = os.path.split(cxx)
    basename = basename.replace('clang++', 'clang').replace('g++', 'gcc').replace('em++', 'emcc')
    return os.path.join(dirname, basename)


def is_dash_s_for_emcc(args, i):
    # -s OPT=VALUE or -s OPT or -sOPT are all interpreted as emscripten flags.
    # -s by itself is a linker option (alias for --strip-all)
    if args[i] == '-s':
        if len(args) <= i + 1:
            return False
        arg = args[i + 1]
    else:
        arg = removeprefix(args[i], '-s')
    arg = arg.split('=')[0]
    return arg.isidentifier() and arg.isupper()


def parse_s_args(args):
    settings_changes = []
    for i in range(len(args)):
        if args[i].startswith('-s'):
            if is_dash_s_for_emcc(args, i):
                if args[i] == '-s':
                    key = args[i + 1]
                    args[i + 1] = ''
                else:
                    key = removeprefix(args[i], '-s')
                args[i] = ''

                # If not = is specified default to 1
                if '=' not in key:
                    key += '=1'

                # Special handling of browser version targets. A version -1 means that the specific version
                # is not supported at all. Replace those with INT32_MAX to make it possible to compare e.g.
                # #if MIN_FIREFOX_VERSION < 68
                if re.match(r'MIN_.*_VERSION(=.*)?', key):
                    try:
                        if int(key.split('=')[1]) < 0:
                            key = key.split('=')[0] + '=0x7FFFFFFF'
                    except Exception:
                        pass

                settings_changes.append(key)

    newargs = [a for a in args if a]
    return (settings_changes, newargs)


def get_target_flags():
    return []
    # return ['-target', shared.get_llvm_target()]


def get_clang_flags(user_args):
    flags = get_target_flags()

    # if exception catching is disabled, we can prevent that code from being
    # generated in the frontend
    # if settings.DISABLE_EXCEPTION_CATCHING and not settings.WASM_EXCEPTIONS:
    #     flags.append('-fignore-exceptions')

    if settings.INLINING_LIMIT:
        flags.append('-fno-inline-functions')

    if settings.PTHREADS:
        if '-pthread' not in user_args:
            flags.append('-pthread')

    if settings.RELOCATABLE and '-fPIC' not in user_args:
        flags.append('-fPIC')

    if settings.RELOCATABLE or settings.LINKABLE or '-fPIC' in user_args:
        if not any(a.startswith('-fvisibility') for a in user_args):
            # For relocatable code we default to visibility=default in emscripten even
            # though the upstream backend defaults visibility=hidden.  This matches the
            # expectations of C/C++ code in the wild which expects undecorated symbols
            # to be exported to other DSO's by default.
            flags.append('-fvisibility=default')

    # if settings.LTO:
    #     if not any(a.startswith('-flto') for a in user_args):
    #         flags.append('-flto=' + settings.LTO)
    #     # setjmp/longjmp handling using Wasm EH
    #     # For non-LTO, '-mllvm -wasm-enable-eh' added in
    #     # building.llvm_backend_args() sets this feature in clang. But in LTO, the
    #     # argument is added to wasm-ld instead, so clang needs to know that EH is
    #     # enabled so that it can be added to the attributes in LLVM IR.
    #     if settings.SUPPORT_LONGJMP == 'wasm':
    #         flags.append('-mexception-handling')
    #
    # else:
    #     # In LTO mode these args get passed instead at link time when the backend runs.
    #     for a in building.llvm_backend_args():
    #         flags += ['-mllvm', a]

    return flags


@memoize
def get_cflags(user_args):
    # Flags we pass to the compiler when building C/C++ code
    # We add these to the user's flags (newargs), but not when building .s or .S assembly files
    cflags = get_clang_flags(user_args)
    # cflags.append('--sysroot=' + cache.get_sysroot(absolute=True))

    # if settings.MAIN_GCCMAIN_MACRO:
    #     cflags.append('-Dmain=__gccmain')

    # if not settings.STRICT:
    #     # The preprocessor define EMSCRIPTEN is deprecated. Don't pass it to code
    #     # in strict mode. Code should use the define __EMSCRIPTEN__ instead.
    #     cflags.append('-DEMSCRIPTEN')

    ports.add_cflags(cflags, settings)

    def array_contains_any_of(hay, needles):
        for n in needles:
            if n in hay:
                return True

    if array_contains_any_of(user_args, SIMD_INTEL_FEATURE_TOWER) or array_contains_any_of(user_args, SIMD_NEON_FLAGS):
        if '-msimd128' not in user_args and '-mrelaxed-simd' not in user_args:
            exit_with_error('passing any of ' + ', '.join(SIMD_INTEL_FEATURE_TOWER + SIMD_NEON_FLAGS) + ' flags also requires passing -msimd128 (or -mrelaxed-simd)!')
        cflags += ['-D__SSE__=1']

    if array_contains_any_of(user_args, SIMD_INTEL_FEATURE_TOWER[1:]):
        cflags += ['-D__SSE2__=1']

    if array_contains_any_of(user_args, SIMD_INTEL_FEATURE_TOWER[2:]):
        cflags += ['-D__SSE3__=1']

    if array_contains_any_of(user_args, SIMD_INTEL_FEATURE_TOWER[3:]):
        cflags += ['-D__SSSE3__=1']

    if array_contains_any_of(user_args, SIMD_INTEL_FEATURE_TOWER[4:]):
        cflags += ['-D__SSE4_1__=1']

    # Handle both -msse4.2 and its alias -msse4.
    if array_contains_any_of(user_args, SIMD_INTEL_FEATURE_TOWER[5:]):
        cflags += ['-D__SSE4_2__=1']

    if array_contains_any_of(user_args, SIMD_INTEL_FEATURE_TOWER[7:]):
        cflags += ['-D__AVX__=1']

    if array_contains_any_of(user_args, SIMD_INTEL_FEATURE_TOWER[8:]):
        cflags += ['-D__AVX2__=1']

    if array_contains_any_of(user_args, SIMD_NEON_FLAGS):
        cflags += ['-D__ARM_NEON__=1']

    # if '-nostdinc' not in user_args:
    #     if not settings.USE_SDL:
    #         cflags += ['-Xclang', '-iwithsysroot' + os.path.join('/include', 'fakesdl')]
    #     cflags += ['-Xclang', '-iwithsysroot' + os.path.join('/include', 'compat')]

    return cflags


def get_library_basename(filename):
    """Similar to get_file_suffix this strips off all numeric suffixes and then
    then final non-numeric one.  For example for 'libz.so.1.2.8' returns 'libz'"""
    filename = os.path.basename(filename)
    while filename:
        filename, suffix = os.path.splitext(filename)
        # Keep stipping suffixes until we strip a non-numeric one.
        if not suffix[1:].isdigit():
            return filename


#
# Main run() function
#
def run(args):
    if len(args) <= 1:
        diagnostics.error(f"First argument of wrapper needs to be the compiler executable")
        return 1

    compiler = args[1]
    if not os.path.isfile(compiler):
        diagnostics.error(f"Compiler {compiler} does not exist")
        return 1

    # Special case the handling of `-v` because it has a special/different meaning
    # when used with no other arguments.
    if len(args) == 2 and args[1] == '-v':
        # autoconf likes to see 'GNU' in the output to enable shared object support
        print(version_string(), file=sys.stderr)
        return shared.check_call([compiler, '-v'] + get_target_flags(), check=False).returncode

    if DEBUG:
        logger.warning(f'invocation: {shared.shlex_join(args)} (in {os.getcwd()})')

    # Strip args[0] (program name)
    # Strip args[1] (compiler executable)
    args = args[2:]

    # Handle some global flags

    # read response files very early on
    try:
        args = substitute_response_files(args)
    except IOError as e:
        exit_with_error(e)

    if '--help' in args:
        # Documentation for emcc and its options must be updated in:
        #    site/source/docs/tools_reference/emcc.rst
        # This then gets built (via: `make -C site text`) to:
        #    site/build/text/docs/tools_reference/emcc.txt
        # This then needs to be copied to its final home in docs/emcc.txt from where
        # we read it here.  We have CI rules that ensure its always up-to-date.
        print(read_file(utils.path_from_root('docs/emcc.txt')))

        print(textwrap.dedent('''
            ------------------------------------------------------------------
            
            ngagecc: supported targets: arm-epoc-pe, thumb-epoc-pe, NOT elf
            (autoconf likes to see elf above to enable shared object support)
        '''))
        return 0

    ## Process argument and setup the compiler
    state = NGageCCState(args)
    options, newargs = phase_parse_arguments(state)

    if not shared.SKIP_SUBPROCS:
        shared.check_sanity()

    # Begin early-exit flag handling.

    if '--version' in args:
        print(version_string())
        print(textwrap.dedent('''\
            Copyright (C) 2014 the Emscripten authors (see AUTHORS.txt)
            This is free and open source software under the MIT license.
            There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
            '''))
        return 0

    if '-dumpversion' in args: # gcc's doc states "Print the compiler version [...] and don't do anything else."
        print(utils.NGAGE_TOOLCHAIN_VERSION)
        return 0

    if '-dumpmachine' in args or '-print-target-triple' in args or '--print-target-triple' in args:
        print("epoc32-ngagesdk-gnu")
        return 0

    if '-print-search-dirs' in args or '--print-search-dirs' in args:
        print(f'programs: ={config.LLVM_ROOT}')
        print(f'libraries: ={cache.get_lib_dir(absolute=True)}')
        return 0

    if '-print-resource-dir' in args:
        shared.check_call([compiler] + args)
        return 0

    print_file_name = [a for a in args if a.startswith(('-print-file-name=', '--print-file-name='))]
    if print_file_name:
        libname = print_file_name[-1].split('=')[1]
        system_libpath = cache.get_lib_dir(absolute=True)
        fullpath = os.path.join(system_libpath, libname)
        if os.path.isfile(fullpath):
            print(fullpath)
        else:
            print(libname)
        return 0

    # End early-exit flag handling

    # For internal consistency, ensure we don't attempt or read or write any link time
    # settings until we reach the linking phase.
    settings.limit_settings(COMPILE_TIME_SETTINGS)

    phase_setup(options, state)

    # Compile source code to object files
    # When only compiling this function never returns.
    linker_inputs = phase_compile_inputs(compiler, options, state, newargs)

    if state.mode == Mode.COMPILE_AND_LINK:
        # Delay import of link.py to avoid processing this file when only compiling
        from tools import link
        return link.run(linker_inputs, options, state)
    else:
        logger.debug('stopping after compile phase')
        return 0


def normalize_boolean_setting(name, value):
    # boolean NO_X settings are aliases for X
    # (note that *non*-boolean setting values have special meanings,
    # and we can't just flip them, so leave them as-is to be
    # handled in a special way later)
    if name.startswith('NO_') and value in ('0', '1'):
        name = removeprefix(name, 'NO_')
        value = str(1 - int(value))
    return name, value


@ToolchainProfiler.profile_block('parse arguments')
def phase_parse_arguments(state: NGageCCState):
    """The first phase of the compiler.  Parse command line argument and
    populate settings.
    """
    newargs = list(state.orig_args)

    # Scan and strip emscripten specific cmdline warning flags.
    # This needs to run before other cmdline flags have been parsed, so that
    # warnings are properly printed during arg parse.
    newargs = diagnostics.capture_warnings(newargs)

    if not diagnostics.is_enabled('deprecated'):
        settings.WARN_DEPRECATED = 0

    for i in range(len(newargs)):
        if newargs[i] in ('-l', '-L', '-I', '-z', '-o', '-x'):
            # Scan for flags that can be written as either one or two arguments
            # and normalize them to the single argument form.
            if len(newargs) <= i + 1:
                exit_with_error(f"option '{newargs[i]}' requires an argument")
            newargs[i] += newargs[i + 1]
            newargs[i + 1] = ''

    options, settings_changes, user_js_defines, newargs = parse_args(newargs)

    # if options.post_link or options.oformat == OFormat.BARE:
    #     diagnostics.warning('experimental', '--oformat=bare/--post-link are experimental and subject to change.')

    explicit_settings_changes, newargs = parse_s_args(newargs)
    settings_changes += explicit_settings_changes

    for s in settings_changes:
        key, value = s.split('=', 1)
        key, value = normalize_boolean_setting(key, value)
        user_settings[key] = value

    # STRICT is used when applying settings so it needs to be applied first before
    # calling `apply_user_settings`.
    strict_cmdline = user_settings.get('STRICT')
    if strict_cmdline:
        settings.STRICT = int(strict_cmdline)

    # Apply user -jsD settings
    for s in user_js_defines:
        settings[s[0]] = s[1]

    # Apply -s settings in newargs here (after optimization levels, so they can override them)
    apply_user_settings()

    return options, newargs


def separate_linker_flags(state, newargs):
    """Process argument list separating out input files, compiler flags
    and linker flags.

    - Linker flags are stored in state.link_flags
    - Input files and compiler-only flags are returned as two separate lists.

    Both linker flags and input files are stored as pairs of (i, entry) where
    `i` is the orginal index in the command line arguments.  This allow the two
    lists to be recombined in the correct order by the linker code (link.py).

    Note that this index can have a fractional part for input arguments that
    expand into multiple processed arguments, as in -Wl,-f1,-f2.
    """

    # if settings.RUNTIME_LINKED_LIBS:
    #     newargs += settings.RUNTIME_LINKED_LIBS

    input_files = []
    compiler_args = []

    skip = False
    for i in range(len(newargs)):
        if skip:
            skip = False
            continue

        arg = newargs[i]
        if arg in CLANG_FLAGS_WITH_ARGS:
            skip = True

        def get_next_arg():
            if len(newargs) <= i + 1:
                exit_with_error(f"option '{arg}' requires an argument")
            return newargs[i + 1]

        if not arg.startswith('-') or arg == '-':
            # os.devnul should always be reported as existing but there is bug in windows
            # python before 3.8:
            # https://bugs.python.org/issue1311
            if not os.path.exists(arg) and arg not in (os.devnull, '-'):
                exit_with_error('%s: No such file or directory ("%s" was expected to be an input file, based on the commandline arguments provided)', arg, arg)
            input_files.append((i, arg))
        # elif arg == '-z':
        #     state.add_link_flag(i, newargs[i])
        #     state.add_link_flag(i + 1, get_next_arg())
        elif arg.startswith('-Wl,'):
            # Multiple comma separated link flags can be specified. Create fake
            # fractional indices for these: -Wl,a,b,c,d at index 4 becomes:
            # (4, a), (4.25, b), (4.5, c), (4.75, d)
            link_flags_to_add = arg.split(',')[1:]
            for flag_index, flag in enumerate(link_flags_to_add):
                state.add_link_flag(i + float(flag_index) / len(link_flags_to_add), flag)
        elif arg == '-Xlinker':
            state.add_link_flag(i + 1, get_next_arg())
        elif arg == '-s':
            state.add_link_flag(i, newargs[i])
        elif not arg.startswith('-o') and arg not in ('-nostdlib', '-nostartfiles', '-nolibc', '-nodefaultlibs', '-s'):
            # All other flags are for the compiler
            compiler_args.append(arg)
            if skip:
                compiler_args.append(get_next_arg())

    return compiler_args, input_files


@ToolchainProfiler.profile_block('setup')
def phase_setup(options, state):
    """Second phase: configure and setup the compiler based on the specified settings and arguments.
    """

    has_header_inputs = any(get_file_suffix(f) in HEADER_EXTENSIONS for f in options.input_files)

    if has_header_inputs or options.dash_c or options.dash_S or options.syntax_only or options.dash_E or options.dash_M:
        state.mode = Mode.COMPILE_ONLY

    if state.mode == Mode.COMPILE_ONLY:
        for key in user_settings:
            if key not in COMPILE_TIME_SETTINGS:
                diagnostics.warning(
                    'unused-command-line-argument',
                    "linker setting ignored during compilation: '%s'" % key)
        for arg in state.orig_args:
            if arg in LINK_ONLY_FLAGS:
                diagnostics.warning(
                    'unused-command-line-argument',
                    "linker flag ignored during compilation: '%s'" % arg)

    # if settings.MAIN_MODULE or settings.SIDE_MODULE:
    #     settings.RELOCATABLE = 1

    if 'USE_PTHREADS' in user_settings:
        settings.PTHREADS = settings.USE_PTHREADS


@ToolchainProfiler.profile_block('compile inputs')
def phase_compile_inputs(compiler, options, state, newargs):
    command = [compiler]

    if config.COMPILER_WRAPPER:
        logger.debug('using compiler wrapper: %s', config.COMPILER_WRAPPER)
        command.insert(0, config.COMPILER_WRAPPER)

    # system_libs.ensure_sysroot()

    def get_clang_command():
        return command + get_cflags(state.orig_args)

    def get_clang_command_preprocessed():
        return command + get_clang_flags(state.orig_args)

    def get_clang_command_asm():
        return command + get_target_flags()

    if state.mode == Mode.COMPILE_ONLY:
        # if options.output_file and get_file_suffix(options.output_file) == '.bc' and not settings.LTO and '-emit-llvm' not in state.orig_args:
        #     diagnostics.warning('emcc', '.bc output file suffix used without -flto or -emit-llvm.  Consider using .o extension since emcc will output an object file, not a bitcode file')
        if all(get_file_suffix(i) in ASSEMBLY_EXTENSIONS for i in options.input_files):
            cmd = get_clang_command_asm() + newargs
        else:
            cmd = get_clang_command() + newargs
        shared.exec_process(cmd)
        assert False, 'exec_process does not return'

    # In COMPILE_AND_LINK we need to compile source files too, but we also need to
    # filter out the link flags
    assert state.mode == Mode.COMPILE_AND_LINK
    assert not options.dash_c
    compile_args, input_files = separate_linker_flags(state, newargs)
    linker_inputs = []
    seen_names = {}

    def uniquename(name):
        if name not in seen_names:
            seen_names[name] = str(len(seen_names))
        return unsuffixed(name) + '_' + seen_names[name] + shared.suffix(name)

    def get_object_filename(input_file):
        return in_temp(shared.replace_suffix(uniquename(input_file), '.o'))

    def compile_source_file(i, input_file):
        logger.debug(f'compiling source file: {input_file}')
        output_file = get_object_filename(input_file)
        linker_inputs.append((i, output_file))
        if get_file_suffix(input_file) in ASSEMBLY_EXTENSIONS:
            cmd = get_clang_command_asm()
        elif get_file_suffix(input_file) in PREPROCESSED_EXTENSIONS:
            cmd = get_clang_command_preprocessed()
        else:
            cmd = get_clang_command()
            if get_file_suffix(input_file) in ['.pcm']:
                cmd = [c for c in cmd if not c.startswith('-fprebuilt-module-path=')]
        cmd += compile_args + ['-c', input_file, '-o', output_file]
        if options.requested_debug == '-gsplit-dwarf':
            # When running in COMPILE_AND_LINK mode we compile objects to a temporary location
            # but we want the `.dwo` file to be generated in the current working directory,
            # like it is under clang.  We could avoid this hack if we use the clang driver
            # to generate the temporary files, but that would also involve using the clang
            # driver to perform linking which would be big change.
            cmd += ['-Xclang', '-split-dwarf-file', '-Xclang', unsuffixed_basename(input_file) + '.dwo']
            cmd += ['-Xclang', '-split-dwarf-output', '-Xclang', unsuffixed_basename(input_file) + '.dwo']
        shared.check_call(cmd)
        if not shared.SKIP_SUBPROCS:
            assert os.path.exists(output_file)
            if options.save_temps:
                shutil.copyfile(output_file, shared.unsuffixed_basename(input_file) + '.o')

    # First, generate LLVM bitcode. For each input file, we get base.o with bitcode
    for i, input_file in input_files:
        file_suffix = get_file_suffix(input_file)
        if file_suffix in SOURCE_EXTENSIONS | ASSEMBLY_EXTENSIONS or (options.dash_c and file_suffix == '.bc'):
            compile_source_file(i, input_file)
        # elif file_suffix in DYLIB_EXTENSIONS:
        #     logger.debug(f'using shared library: {input_file}')
        #     linker_inputs.append((i, input_file))
        elif building.is_ar(input_file):
            logger.debug(f'using static library: {input_file}')
            linker_inputs.append((i, input_file))
        elif options.input_language:
            compile_source_file(i, input_file)
        elif input_file == '-':
            exit_with_error('-E or -x required when input is from standard input')
        else:
            # Default to assuming the inputs are object files and pass them to the linker
            logger.debug(f'using object file: {input_file}')
            linker_inputs.append((i, input_file))

    return linker_inputs


def version_string():
    # if the emscripten folder is not a git repo, don't run git show - that can
    # look up and find the revision in a parent directory that is a git repo
    revision_suffix = ''
    if os.path.exists(utils.path_from_root('.git')):
        git_rev = run_process(
            ['git', 'rev-parse', 'HEAD'],
            stdout=PIPE, stderr=PIPE, cwd=utils.path_from_root()).stdout.strip()
        revision_suffix = ' (%s)' % git_rev
    elif os.path.exists(utils.path_from_root('ngage-toolchain-revision.txt')):
        rev = read_file(utils.path_from_root('ngage-toolchain-revision.txt')).strip()
        revision_suffix = ' (%s)' % rev
    return f'ngage-sdk (gcc/clang-like replacement + linker emulating GNU ld) {utils.EMSCRIPTEN_VERSION}{revision_suffix}'


def parse_args(newargs):  # noqa: C901, PLR0912, PLR0915
    """Future modifications should consider refactoring to reduce complexity.

    * The McCabe cyclomatiic complexity is currently 117 vs 10 recommended.
    * There are currently 115 branches vs 12 recommended.
    * There are currently 302 statements vs 50 recommended.

    To revalidate these numbers, run `ruff check --select=C901,PLR091`.
    """
    options = NGageOptions()
    settings_changes = []
    user_js_defines = []
    should_exit = False
    skip = False

    for i in range(len(newargs)):
        if skip:
            skip = False
            continue

        arg = newargs[i]
        arg_value = None

        if arg in CLANG_FLAGS_WITH_ARGS:
            # Ignore the next argument rather than trying to parse it.  This is needed
            # because that next arg could, for example, start with `-o` and we don't want
            # to confuse that with a normal `-o` flag.
            skip = True

        def check_flag(value):
            # Check for and consume a flag
            if arg == value:
                newargs[i] = ''
                return True
            return False

        def check_arg(name):
            nonlocal arg_value
            if arg.startswith(name) and '=' in arg:
                arg_value = arg.split('=', 1)[1]
                newargs[i] = ''
                return True
            if arg == name:
                if len(newargs) <= i + 1:
                    exit_with_error(f"option '{arg}' requires an argument")
                arg_value = newargs[i + 1]
                newargs[i] = ''
                newargs[i + 1] = ''
                return True
            return False

        def consume_arg():
            nonlocal arg_value
            assert arg_value is not None
            rtn = arg_value
            arg_value = None
            return rtn

        def consume_arg_file():
            name = consume_arg()
            if not os.path.isfile(name):
                exit_with_error("'%s': file not found: '%s'" % (arg, name))
            return name

        if arg.startswith('-O'):
            # Let -O default to -O2, which is what gcc does.
            requested_level = removeprefix(arg, '-O') or '2'
            # if requested_level == 's':
            #     requested_level = 2
            #     settings.SHRINK_LEVEL = 1
            # elif requested_level == 'z':
            #     requested_level = 2
            #     settings.SHRINK_LEVEL = 2
            # elif requested_level == 'g':
            #     requested_level = 1
            #     settings.SHRINK_LEVEL = 0
            #     settings.DEBUG_LEVEL = max(settings.DEBUG_LEVEL, 1)
            # elif requested_level == 'fast':
            #     # TODO(https://github.com/emscripten-core/emscripten/issues/21497):
            #     # If we ever map `-ffast-math` to `wasm-opt --fast-math` then
            #     # then we should enable that too here.
            #     requested_level = 3
            #     settings.SHRINK_LEVEL = 0
            # else:
            #     settings.SHRINK_LEVEL = 0
            settings.OPT_LEVEL = validate_arg_level(requested_level, 3, 'invalid optimization level: ' + arg, clamp=True)
        elif arg == "--save-temps":
            options.save_temps = True
        # elif check_arg('--oformat'):
        #     formats = [f.lower() for f in OFormat.__members__]
        #     fmt = consume_arg()
        #     if fmt not in formats:
        #         exit_with_error('invalid output format: `%s` (must be one of %s)' % (fmt, formats))
        #     options.oformat = getattr(OFormat, fmt.upper())
        elif arg.startswith('-g'):
            options.requested_debug = arg
            requested_level = removeprefix(arg, '-g') or '3'
            if is_int(requested_level):
                # the -gX value is the debug level (-g1, -g2, etc.)
                settings.DEBUG_LEVEL = validate_arg_level(requested_level, 4, 'invalid debug level: ' + arg)
                if settings.DEBUG_LEVEL == 0:
                    # Set these explicitly so -g0 overrides previous -g on the cmdline
                    settings.GENERATE_DWARF = 0
                    settings.GENERATE_SOURCE_MAP = 0
                    settings.EMIT_NAME_SECTION = 0
                elif settings.DEBUG_LEVEL > 1:
                    settings.EMIT_NAME_SECTION = 1
                # if we don't need to preserve LLVM debug info, do not keep this flag
                # for clang
                if settings.DEBUG_LEVEL < 3:
                    newargs[i] = '-g0'
                else:
                    # for 3+, report -g3 to clang as -g4 etc. are not accepted
                    newargs[i] = '-g3'
                    if settings.DEBUG_LEVEL == 3:
                        settings.GENERATE_DWARF = 1
                    if settings.DEBUG_LEVEL == 4:
                        settings.GENERATE_SOURCE_MAP = 1
                        diagnostics.warning('deprecated', 'please replace -g4 with -gsource-map')
            else:
                if requested_level.startswith('force_dwarf'):
                    exit_with_error('gforce_dwarf was a temporary option and is no longer necessary (use -g)')
                elif requested_level.startswith('separate-dwarf'):
                    # emit full DWARF but also emit it in a file on the side
                    newargs[i] = '-g'
                    # if a file is provided, use that; otherwise use the default location
                    # (note that we do not know the default location until all args have
                    # been parsed, so just note True for now).
                    if requested_level != 'separate-dwarf':
                        if not requested_level.startswith('separate-dwarf=') or requested_level.count('=') != 1:
                            exit_with_error('invalid -gseparate-dwarf=FILENAME notation')
                        settings.SEPARATE_DWARF = requested_level.split('=')[1]
                    else:
                        settings.SEPARATE_DWARF = True
                    settings.GENERATE_DWARF = 1
                elif requested_level == 'source-map':
                    settings.GENERATE_SOURCE_MAP = 1
                    settings.EMIT_NAME_SECTION = 1
                    newargs[i] = '-g'
                else:
                    # Other non-integer levels (e.g. -gline-tables-only or -gdwarf-5) are
                    # usually clang flags that emit DWARF. So we pass them through to
                    # clang and make the emscripten code treat it like any other DWARF.
                    settings.GENERATE_DWARF = 1
                    settings.EMIT_NAME_SECTION = 1
                # In all cases set the emscripten debug level to 3 so that we do not
                # strip during link (during compile, this does not make a difference).
                settings.DEBUG_LEVEL = 3
        elif arg == '-v':
            shared.PRINT_SUBPROCS = True
        elif arg == '-###':
            shared.SKIP_SUBPROCS = True
        elif check_arg('--cache'):
            config.CACHE = os.path.abspath(consume_arg())
            cache.setup()
            # Ensure child processes share the same cache (e.g. when using emcc to compiler system
            # libraries)
            os.environ['EM_CACHE'] = config.CACHE
        elif check_flag('--clear-cache'):
            logger.info('clearing cache as requested by --clear-cache: `%s`', cache.cachedir)
            cache.erase()
            shared.perform_sanity_checks() # this is a good time for a sanity check
            should_exit = True
        elif check_flag('--clear-ports'):
            logger.info('clearing ports and cache as requested by --clear-ports')
            ports.clear()
            cache.erase()
            shared.perform_sanity_checks() # this is a good time for a sanity check
            should_exit = True
        elif check_flag('--check'):
            print(version_string(), file=sys.stderr)
            shared.check_sanity(force=True)
            should_exit = True
        elif check_flag('--show-ports'):
            ports.show_ports()
            should_exit = True
        elif arg.startswith(('-I', '-L')):
            path_name = arg[2:]
            if os.path.isabs(path_name) and not is_valid_abspath(options, path_name):
                # Of course an absolute path to a non-system-specific library or header
                # is fine, and you can ignore this warning. The danger are system headers
                # that are e.g. x86 specific and non-portable. The emscripten bundled
                # headers are modified to be portable, local system ones are generally not.
                diagnostics.warning(
                    'absolute-paths', f'-I or -L of an absolute path "{arg}" '
                                      'encountered. If this is to a local system header/library, it may '
                                      'cause problems (local system files make sense for compiling natively '
                                      'on your system, but not necessarily to JavaScript).')
        elif arg == '-fexceptions':
            # TODO Currently -fexceptions only means Emscripten EH. Switch to wasm
            # exception handling by default when -fexceptions is given when wasm
            # exception handling becomes stable.
            settings.DISABLE_EXCEPTION_THROWING = 0
            settings.DISABLE_EXCEPTION_CATCHING = 0
        elif arg == '-pthread':
            settings.PTHREADS = 1
            # Also set the legacy setting name, in case use JS code depends on it.
            settings.USE_PTHREADS = 1
        elif arg == '-no-pthread':
            settings.PTHREADS = 0
            # Also set the legacy setting name, in case use JS code depends on it.
            settings.USE_PTHREADS = 0
        elif arg == '-pthreads':
            exit_with_error('unrecognized command-line option `-pthreads`; did you mean `-pthread`?')
        elif arg in ('-fno-diagnostics-color', '-fdiagnostics-color=never'):
            colored_logger.disable()
            diagnostics.color_enabled = False
        elif arg == '-fno-rtti':
            settings.USE_RTTI = 0
        elif arg == '-frtti':
            settings.USE_RTTI = 1
        elif arg.startswith('-jsD'):
            key = removeprefix(arg, '-jsD')
            if '=' in key:
                key, value = key.split('=')
            else:
                value = '1'
            if key in settings.keys():
                exit_with_error(f'{arg}: cannot change built-in settings values with a -jsD directive. Pass -s{key}={value} instead!')
            user_js_defines += [(key, value)]
            newargs[i] = ''
        elif check_flag('-shared'):
            options.shared = True
        elif check_flag('-r'):
            options.relocatable = True
        elif arg.startswith('-o'):
            options.output_file = removeprefix(arg, '-o')
        elif check_arg('-target') or check_arg('--target'):
            options.target = consume_arg()
            if options.target not in ('wasm32', 'wasm64', 'wasm64-unknown-emscripten', 'wasm32-unknown-emscripten'):
                exit_with_error(f'unsupported target: {options.target} (emcc only supports wasm64-unknown-emscripten and wasm32-unknown-emscripten)')
        # elif check_arg('--use-port'):
        #     ports.handle_use_port_arg(settings, consume_arg())
        elif arg in ('-c', '--precompile'):
            options.dash_c = True
        elif arg == '-S':
            options.dash_S = True
        elif arg == '-E':
            options.dash_E = True
        elif arg in ('-M', '-MM'):
            options.dash_M = True
        elif arg.startswith('-x'):
            # TODO(sbc): Handle multiple -x flags on the same command line
            options.input_language = arg
        elif arg == '-fsyntax-only':
            options.syntax_only = True
        elif arg in SIMD_INTEL_FEATURE_TOWER or arg in SIMD_NEON_FLAGS:
            # SSEx is implemented on top of SIMD128 instruction set, but do not pass SSE flags to LLVM
            # so it won't think about generating native x86 SSE code.
            newargs[i] = ''
        elif arg == '-nostdlib':
            options.nostdlib = True
        elif arg == '-nostdlibxx':
            options.nostdlibxx = True
        elif arg == '-nodefaultlibs':
            options.nodefaultlibs = True
        elif arg == '-nolibc':
            options.nolibc = True
        elif arg == '-nostartfiles':
            options.nostartfiles = True
        elif arg == '-fsanitize-minimal-runtime':
            options.sanitize_minimal_runtime = True
        elif arg.startswith('-fsanitize='):
            options.sanitize.update(arg.split('=', 1)[1].split(','))
        elif arg.startswith('-fno-sanitize='):
            options.sanitize.difference_update(arg.split('=', 1)[1].split(','))
        elif arg and (arg == '-' or not arg.startswith('-')):
            options.input_files.append(arg)

    if should_exit:
        sys.exit(0)

    newargs = [a for a in newargs if a]
    return options, settings_changes, user_js_defines, newargs


def is_valid_abspath(options, path_name):
    # Any path that is underneath the emscripten repository root must be ok.
    if utils.normalize_path(path_name).startswith(utils.normalize_path(utils.path_from_root())):
        return True

    def in_directory(root, child):
        # make both path absolute
        root = os.path.realpath(root)
        child = os.path.realpath(child)

        # return true, if the common prefix of both is equal to directory
        # e.g. /a/b/c/d.rst and directory is /a/b, the common prefix is /a/b
        return os.path.commonprefix([root, child]) == root

    for valid_abspath in options.valid_abspaths:
        if in_directory(valid_abspath, path_name):
            return True
    return False


def parse_symbol_list_file(contents):
    """Parse contents of one-symbol-per-line response file.  This format can by used
    with, for example, -sEXPORTED_FUNCTIONS=@filename and avoids the need for any
    kind of quoting or escaping.
    """
    values = contents.splitlines()
    return [v.strip() for v in values if not v.startswith('#')]


def parse_value(text, expected_type):
    # Note that using response files can introduce whitespace, if the file
    # has a newline at the end. For that reason, we rstrip() in relevant
    # places here.
    def parse_string_value(text):
        first = text[0]
        if first in {"'", '"'}:
            text = text.rstrip()
            if text[-1] != text[0] or len(text) < 2:
                raise ValueError(f'unclosed quoted string. expected final character to be "{text[0]}" and length to be greater than 1 in "{text[0]}"')
            return text[1:-1]
        return text

    def parse_string_list_members(text):
        sep = ','
        values = text.split(sep)
        result = []
        index = 0
        while True:
            current = values[index].lstrip() # Cannot safely rstrip for cases like: "HERE-> ,"
            if not len(current):
                raise ValueError('empty value in string list')
            first = current[0]
            if first not in {"'", '"'}:
                result.append(current.rstrip())
            else:
                start = index
                while True: # Continue until closing quote found
                    if index >= len(values):
                        raise ValueError(f"unclosed quoted string. expected final character to be '{first}' in '{values[start]}'")
                    new = values[index].rstrip()
                    if new and new[-1] == first:
                        if start == index:
                            result.append(current.rstrip()[1:-1])
                        else:
                            result.append((current + sep + new)[1:-1])
                        break
                    else:
                        current += sep + values[index]
                        index += 1

            index += 1
            if index >= len(values):
                break
        return result

    def parse_string_list(text):
        text = text.rstrip()
        if text and text[0] == '[':
            if text[-1] != ']':
                raise ValueError('unterminated string list. expected final character to be "]"')
            text = text[1:-1]
        if text.strip() == "":
            return []
        return parse_string_list_members(text)

    if expected_type == list or (text and text[0] == '['):
        # if json parsing fails, we fall back to our own parser, which can handle a few
        # simpler syntaxes
        try:
            parsed = json.loads(text)
        except ValueError:
            return parse_string_list(text)

        # if we succeeded in parsing as json, check some properties of it before returning
        if type(parsed) not in (str, list):
            raise ValueError(f'settings must be strings or lists (not ${type(parsed)})')
        if type(parsed) is list:
            for elem in parsed:
                if type(elem) is not str:
                    raise ValueError(f'list members in settings must be strings (not ${type(elem)})')

        return parsed

    if expected_type == float:
        try:
            return float(text)
        except ValueError:
            pass

    try:
        if text.startswith('0x'):
            base = 16
        else:
            base = 10
        return int(text, base)
    except ValueError:
        return parse_string_value(text)


def validate_arg_level(level_string, max_level, err_msg, clamp=False):
    try:
        level = int(level_string)
    except ValueError:
        exit_with_error(err_msg)
    if clamp:
        if level > max_level:
            logger.warning("optimization level '-O" + level_string + "' is not supported; using '-O" + str(max_level) + "' instead")
            level = max_level
    if not 0 <= level <= max_level:
        exit_with_error(err_msg)
    return level


def is_int(s):
    try:
        int(s)
        return True
    except ValueError:
        return False


@ToolchainProfiler.profile()
def main(args):
    if WINDOWS:
        if "NGAGESDK" in os.environ:
            ngagesdk_path = os.environ["NGAGESDK"]
            os.environ["PATH"] = os.path.pathsep.join((
                os.path.join(ngagesdk_path, "sdk\\sdk\\6.1\\Shared\\EPOC32\\gcc\\bin"),
                os.path.join(ngagesdk_path, "sdk\\sdk\\6.1\\Shared\\EPOC32\\ngagesdk\\bin"),
                os.environ["PATH"],
            ))

    start_time = time.time()
    ret = run(args)
    logger.debug('total time: %.2f seconds', (time.time() - start_time))
    return ret


if __name__ == '__main__':
    try:
        sys.exit(main(sys.argv))
    except KeyboardInterrupt:
        logger.debug('KeyboardInterrupt')
        sys.exit(1)
