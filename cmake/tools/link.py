# Copyright 2011 The Emscripten Authors.  All rights reserved.
# Emscripten is available under two separate licenses, the MIT license and the
# University of Illinois/NCSA Open Source License.  Both these licenses can be
# found in the LICENSE file.
import dataclasses

from .toolchain_profiler import ToolchainProfiler

import base64
import glob
import hashlib
import json
import logging
import os
import re
import shlex
import stat
import shutil
import time
import typing
from subprocess import PIPE
from urllib.parse import quote

from . import building
from . import cache
from . import config
from . import diagnostics
# from . import emscripten
# from . import feature_matrix
from . import filelock
# from . import js_manipulation
from . import ports
from . import shared
# from . import system_libs
from . import utils
# from . import webassembly
# from . import extract_metadata
from .utils import read_file, write_file, delete_file
from .utils import removeprefix, exit_with_error
from .shared import in_temp, safe_copy, do_replace, OFormat
from .shared import DEBUG, WINDOWS
from .shared import DYLIB_EXTENSIONS
from .shared import unsuffixed, unsuffixed_basename, get_file_suffix
from .settings import settings, default_setting, user_settings, JS_ONLY_SETTINGS, DEPRECATED_SETTINGS
# from .minimal_runtime_shell import generate_minimal_runtime_html

# import tools.line_endings

logger = logging.getLogger('link')

# DEFAULT_SHELL_HTML = utils.path_from_root('src/shell.html')

DEFAULT_ASYNCIFY_IMPORTS = ['__asyncjs__*']

# DEFAULT_ASYNCIFY_EXPORTS = [
#     'main',
#     '__main_argc_argv',
# ]

# VALID_ENVIRONMENTS = ('web', 'webview', 'worker', 'node', 'shell')

EXECUTABLE_EXTENSIONS = ['.exe']
# EXECUTABLE_EXTENSIONS = ['.wasm', '.html', '.js', '.mjs', '.out', '']

# Supported LLD flags which we will pass through to the linker.
SUPPORTED_LINKER_FLAGS = (
    '--start-group', '--end-group',
    '-(', '-)',
    '--whole-archive', '--no-whole-archive',
    '-whole-archive', '-no-whole-archive'
)

# Unsupported LLD flags which we will ignore.
# Maps to true if the flag takes an argument.
UNSUPPORTED_LLD_FLAGS = {
    # macOS-specific linker flag that libtool (ltmain.sh) will if macOS is detected.
    '-bind_at_load': False,
    # wasm-ld doesn't support soname or other dynamic linking flags (yet).   Ignore them
    # in order to aid build systems that want to pass these flags.
    '-allow-shlib-undefined': False,
    '-rpath': True,
    '-rpath-link': True,
    '-version-script': True,
    '-install_name': True,
}

UBSAN_SANITIZERS = {
    'alignment',
    'bool',
    'builtin',
    'bounds',
    'enum',
    'float-cast-overflow',
    'float-divide-by-zero',
    'function',
    'implicit-unsigned-integer-truncation',
    'implicit-signed-integer-truncation',
    'implicit-integer-sign-change',
    'integer-divide-by-zero',
    'nonnull-attribute',
    'null',
    'nullability-arg',
    'nullability-assign',
    'nullability-return',
    'object-size',
    'pointer-overflow',
    'return',
    'returns-nonnull-attribute',
    'shift',
    'signed-integer-overflow',
    'unreachable',
    'unsigned-integer-overflow',
    'vla-bound',
    'vptr',
    'undefined',
    'undefined-trap',
    'implicit-integer-truncation',
    'implicit-integer-arithmetic-value-change',
    'implicit-conversion',
    'integer',
    'nullability',
}


final_js = None


# this function uses the global 'final' variable, which contains the current
# final output file. if a method alters final, and calls this method, then it
# must modify final globally (i.e. it can't receive final as a param and
# return it)
# TODO: refactor all this, a singleton that abstracts over the final output
#       and saving of intermediates
def save_intermediate(name, suffix='js'):
    if not DEBUG:
        return
    if not final_js:
        logger.debug(f'(not saving intermediate {name} because not generating JS)')
        return
    building.save_intermediate(final_js, f'{name}.{suffix}')


def save_intermediate_with_wasm(name, wasm_binary):
    if not DEBUG:
        return
    save_intermediate(name) # save the js
    building.save_intermediate(wasm_binary, name + '.wasm')


def base64_encode(filename):
    data = utils.read_binary(filename)
    b64 = base64.b64encode(data)
    return b64.decode('ascii')


def align_to_wasm_page_boundary(address):
    page_size = webassembly.WASM_PAGE_SIZE
    return ((address + (page_size - 1)) // page_size) * page_size


def will_metadce():
    # The metadce JS parsing code does not currently support the JS that gets generated
    # when assertions are enabled.
    if settings.ASSERTIONS:
        return False
    return settings.OPT_LEVEL >= 3 or settings.SHRINK_LEVEL >= 1


def setup_environment_settings():
    # Environment setting based on user input
    environments = settings.ENVIRONMENT.split(',')
    if any(x for x in environments if x not in VALID_ENVIRONMENTS):
        exit_with_error(f'Invalid environment specified in "ENVIRONMENT": {settings.ENVIRONMENT}. Should be one of: {",".join(VALID_ENVIRONMENTS)}')

    settings.ENVIRONMENT_MAY_BE_WEB = not settings.ENVIRONMENT or 'web' in environments
    settings.ENVIRONMENT_MAY_BE_WEBVIEW = not settings.ENVIRONMENT or 'webview' in environments
    settings.ENVIRONMENT_MAY_BE_NODE = not settings.ENVIRONMENT or 'node' in environments
    settings.ENVIRONMENT_MAY_BE_SHELL = not settings.ENVIRONMENT or 'shell' in environments

    # The worker case also includes Node.js workers when pthreads are
    # enabled and Node.js is one of the supported environments for the build to
    # run on. Node.js workers are detected as a combination of
    # ENVIRONMENT_IS_WORKER and ENVIRONMENT_IS_NODE.
    settings.ENVIRONMENT_MAY_BE_WORKER = \
        not settings.ENVIRONMENT or \
        'worker' in environments or \
        (settings.ENVIRONMENT_MAY_BE_NODE and settings.PTHREADS)

    if not settings.ENVIRONMENT_MAY_BE_WORKER and settings.PROXY_TO_WORKER:
        exit_with_error('if you specify --proxy-to-worker and specify a "-sENVIRONMENT=" directive, it must include "worker" as a target! (Try e.g. -sENVIRONMENT=web,worker)')

    if not settings.ENVIRONMENT_MAY_BE_WORKER and settings.SHARED_MEMORY:
        exit_with_error('when building with multithreading enabled and a "-sENVIRONMENT=" directive is specified, it must include "worker" as a target! (Try e.g. -sENVIRONMENT=web,worker)')


def generate_js_sym_info():
    # Runs the js compiler to generate a list of all symbols available in the JS
    # libraries.  This must be done separately for each linker invocation since the
    # list of symbols depends on what settings are used.
    # TODO(sbc): Find a way to optimize this.  Potentially we could add a super-set
    # mode of the js compiler that would generate a list of all possible symbols
    # that could be checked in.
    _, forwarded_data = emscripten.compile_javascript(symbols_only=True)
    # When running in symbols_only mode compiler.mjs outputs a flat list of C symbols.
    return json.loads(forwarded_data)


@ToolchainProfiler.profile_block('JS symbol generation')
def get_js_sym_info():
    # Avoiding using the cache when generating struct info since
    # this step is performed while the cache is locked.
    if DEBUG or settings.BOOTSTRAPPING_STRUCT_INFO or config.FROZEN_CACHE:
        return generate_js_sym_info()

    # We define a cache hit as when the settings and `--js-library` contents are
    # identical.
    # Ignore certain settings that can are no relevant to library deps.  Here we
    # skip PRE_JS_FILES/POST_JS_FILES which don't effect the library symbol list
    # and can contain full paths to temporary files.
    skip_settings = {'PRE_JS_FILES', 'POST_JS_FILES'}
    input_files = [json.dumps(settings.external_dict(skip_keys=skip_settings), sort_keys=True, indent=2)]
    jslibs = glob.glob(utils.path_from_root('src') + '/library*.js')
    input_files.extend(read_file(jslib) for jslib in sorted(jslibs))
    for jslib in settings.JS_LIBRARIES:
        if not os.path.isabs(jslib):
            jslib = utils.path_from_root('src/lib', jslib)
        input_files.append(read_file(jslib))
    content = '\n'.join(input_files)
    content_hash = hashlib.sha1(content.encode('utf-8')).hexdigest()

    def build_symbol_list(filename):
        """Only called when there is no existing symbol list for a given content hash.
        """
        library_syms = generate_js_sym_info()

        write_file(filename, json.dumps(library_syms, separators=(',', ':'), indent=2))

    # We need to use a separate lock here for symbol lists because, unlike with system libraries,
    # it's normally for these file to get pruned as part of normal operation.  This means that it
    # can be deleted between the `cache.get()` then the `read_file`.
    with filelock.FileLock(cache.get_path(cache.get_path('symbol_lists.lock'))):
        filename = cache.get(f'symbol_lists/{content_hash}.json', build_symbol_list)
        library_syms = json.loads(read_file(filename))

        # Limit of the overall size of the cache to 100 files.
        # This code will get test coverage since a full test run of `other` or `core`
        # generates ~1000 unique symbol lists.
        cache_limit = 500
        root = cache.get_path('symbol_lists')
        if len(os.listdir(root)) > cache_limit:
            files = []
            for f in os.listdir(root):
                f = os.path.join(root, f)
                files.append((f, os.path.getmtime(f)))
            files.sort(key=lambda x: x[1])
            # Delete all but the newest N files
            for f, _ in files[:-cache_limit]:
                delete_file(f)

    return library_syms


def filter_link_flags(flags, using_lld):
    def is_supported(f):
        if using_lld:
            for flag, takes_arg in UNSUPPORTED_LLD_FLAGS.items():
                # lld allows various flags to have either a single -foo or double --foo
                if f.startswith((flag, '-' + flag)):
                    diagnostics.warning('linkflags', 'ignoring unsupported linker flag: `%s`', f)
                    # Skip the next argument if this linker flag takes and argument and that
                    # argument was not specified as a separately (i.e. it was specified as
                    # single arg containing an `=` char.)
                    skip_next = takes_arg and '=' not in f
                    return False, skip_next
            return True, False
        else:
            if f in SUPPORTED_LINKER_FLAGS:
                return True, False
            # Silently ignore -l/-L flags when not using lld.  If using lld allow
            # them to pass through the linker
            if f.startswith(('-l', '-L')):
                return False, False
            diagnostics.warning('linkflags', 'ignoring unsupported linker flag: `%s`', f)
            return False, False

    results = []
    skip_next = False
    for f in flags:
        if skip_next:
            skip_next = False
            continue
        keep, skip_next = is_supported(f[1])
        if keep:
            results.append(f)

    return results


def fix_windows_newlines(text):
    # Avoid duplicating \r\n to \r\r\n when writing out text.
    if WINDOWS:
        text = text.replace('\r\n', '\n')
    return text


def read_js_files(files):
    contents = []
    for f in files:
        content = read_file(f)
        if content.startswith('#preprocess\n'):
            contents.append(shared.read_and_preprocess(f, expand_macros=True))
        else:
            contents.append(content)
    contents = '\n'.join(contents)
    return fix_windows_newlines(contents)


def should_run_binaryen_optimizer():
    # run the binaryen optimizer in -O2+. in -O0 we don't need it obviously, while
    # in -O1 we don't run it as the LLVM optimizer has been run, and it does the
    # great majority of the work; not running the binaryen optimizer in that case
    # keeps -O1 mostly-optimized while compiling quickly and without rewriting
    # DWARF etc.
    return settings.OPT_LEVEL >= 2


def setup_pthreads():
    if settings.RELOCATABLE:
        # pthreads + dynamic linking has certain limitations
        if settings.SIDE_MODULE:
            diagnostics.warning('experimental', '-sSIDE_MODULE + pthreads is experimental')
        elif settings.MAIN_MODULE:
            diagnostics.warning('experimental', '-sMAIN_MODULE + pthreads is experimental')
        elif settings.LINKABLE:
            diagnostics.warning('experimental', '-sLINKABLE + pthreads is experimental')
    default_setting('DEFAULT_PTHREAD_STACK_SIZE', settings.STACK_SIZE)

    # Functions needs by runtime_pthread.js
    settings.REQUIRED_EXPORTS += [
        '_emscripten_thread_free_data',
        '_emscripten_thread_crashed',
    ]

    if settings.EMBIND:
        settings.REQUIRED_EXPORTS.append('_embind_initialize_bindings')

    if settings.MAIN_MODULE:
        settings.REQUIRED_EXPORTS += [
            '_emscripten_dlsync_self',
            '_emscripten_dlsync_self_async',
            '_emscripten_proxy_dlsync',
            '_emscripten_proxy_dlsync_async',
            '__dl_seterr',
        ]

    # runtime_pthread.js depends on these library symbols
    settings.DEFAULT_LIBRARY_FUNCS_TO_INCLUDE += [
        '$PThread',
        '$establishStackSpace',
        '$invokeEntryPoint',
    ]

    if settings.MINIMAL_RUNTIME:
        building.user_requested_exports.add('exit')


def set_initial_memory():
    user_specified_initial_heap = 'INITIAL_HEAP' in user_settings

    # INITIAL_HEAP cannot be used when the memory object is created in JS: we don't know
    # the size of static data here and thus the total initial memory size.
    if settings.IMPORTED_MEMORY:
        if user_specified_initial_heap:
            # Some of these could (and should) be implemented.
            exit_with_error('INITIAL_HEAP is currently not compatible with IMPORTED_MEMORY (which is enabled indirectly via SHARED_MEMORY, RELOCATABLE, ASYNCIFY_LAZY_LOAD_CODE)')
        # The default for imported memory is to fall back to INITIAL_MEMORY.
        settings.INITIAL_HEAP = -1

    if not user_specified_initial_heap:
        # For backwards compatibility, we will only use INITIAL_HEAP by default when the user
        # specified neither INITIAL_MEMORY nor MAXIMUM_MEMORY. Both place an upper bounds on
        # the overall initial linear memory (stack + static data + heap), and we do not know
        # the size of static data at this stage. Setting any non-zero initial heap value in
        # this scenario would risk pushing users over the limit they have set.
        user_specified_initial = settings.INITIAL_MEMORY != -1
        user_specified_maximum = 'MAXIMUM_MEMORY' in user_settings or 'WASM_MEM_MAX' in user_settings or 'BINARYEN_MEM_MAX' in user_settings
        if user_specified_initial or user_specified_maximum:
            settings.INITIAL_HEAP = -1

    # Apply the default if we are going with INITIAL_MEMORY.
    if settings.INITIAL_HEAP == -1 and settings.INITIAL_MEMORY == -1:
        default_setting('INITIAL_MEMORY', 16 * 1024 * 1024)

    def check_memory_setting(setting):
        if settings[setting] % webassembly.WASM_PAGE_SIZE != 0:
            exit_with_error(f'{setting} must be a multiple of WebAssembly page size (64KiB), was {settings[setting]}')
        if settings[setting] >= 2**53:
            exit_with_error(f'{setting} must be smaller than 2^53 bytes due to JS Numbers (doubles) being used to hold pointer addresses in JS side')

    # Due to the aforementioned lack of knowledge about the static data size, we delegate
    # checking the overall consistency of these settings to wasm-ld.
    if settings.INITIAL_HEAP != -1:
        check_memory_setting('INITIAL_HEAP')

    if settings.INITIAL_MEMORY != -1:
        check_memory_setting('INITIAL_MEMORY')
        if settings.INITIAL_MEMORY < settings.STACK_SIZE:
            exit_with_error(f'INITIAL_MEMORY must be larger than STACK_SIZE, was {settings.INITIAL_MEMORY} (STACK_SIZE={settings.STACK_SIZE})')

    check_memory_setting('MAXIMUM_MEMORY')
    if settings.MEMORY_GROWTH_LINEAR_STEP != -1:
        check_memory_setting('MEMORY_GROWTH_LINEAR_STEP')


# Set an upper estimate of what MAXIMUM_MEMORY should be. Take note that this value
# may not be precise, and is only an upper bound of the exact value calculated later
# by the linker.
def set_max_memory():
    # With INITIAL_HEAP, we only know the lower bound on initial memory size.
    initial_memory_known = settings.INITIAL_MEMORY != -1

    if not settings.ALLOW_MEMORY_GROWTH:
        if 'MAXIMUM_MEMORY' in user_settings:
            diagnostics.warning('unused-command-line-argument', 'MAXIMUM_MEMORY is only meaningful with ALLOW_MEMORY_GROWTH')
        # Optimization: lower the default maximum memory to initial memory if possible.
        if initial_memory_known:
            settings.MAXIMUM_MEMORY = settings.INITIAL_MEMORY

    # Automatically up the default maximum when the user requested a large minimum.
    if 'MAXIMUM_MEMORY' not in user_settings:
        if settings.ALLOW_MEMORY_GROWTH:
            if any([settings.INITIAL_HEAP != -1 and settings.INITIAL_HEAP >= 2 * 1024 * 1024 * 1024,
                    initial_memory_known and settings.INITIAL_MEMORY > 2 * 1024 * 1024 * 1024]):
                settings.MAXIMUM_MEMORY = 4 * 1024 * 1024 * 1024

        # INITIAL_MEMORY sets a lower bound for MAXIMUM_MEMORY
        if initial_memory_known and settings.INITIAL_MEMORY > settings.MAXIMUM_MEMORY:
            settings.MAXIMUM_MEMORY = settings.INITIAL_MEMORY

    # A similar check for INITIAL_HEAP would not be precise and so is delegated to wasm-ld.
    if initial_memory_known and settings.MAXIMUM_MEMORY < settings.INITIAL_MEMORY:
        exit_with_error('MAXIMUM_MEMORY cannot be less than INITIAL_MEMORY')


def inc_initial_memory(delta):
    # Both INITIAL_HEAP and INITIAL_MEMORY can be set at the same time. Increment both.
    if settings.INITIAL_HEAP != -1:
        settings.INITIAL_HEAP += delta
    if settings.INITIAL_MEMORY != -1:
        settings.INITIAL_MEMORY += delta


def check_browser_versions():
    # Map of setting all VM version settings to the minimum version
    # we support.
    min_version_settings = {
        'MIN_FIREFOX_VERSION': feature_matrix.OLDEST_SUPPORTED_FIREFOX,
        'MIN_CHROME_VERSION': feature_matrix.OLDEST_SUPPORTED_CHROME,
        'MIN_SAFARI_VERSION': feature_matrix.OLDEST_SUPPORTED_SAFARI,
        'MIN_NODE_VERSION': feature_matrix.OLDEST_SUPPORTED_NODE,
    }

    if settings.LEGACY_VM_SUPPORT:
        # Default all browser versions to zero
        for key in min_version_settings:
            default_setting(key, 0)

    for key, oldest in min_version_settings.items():
        if settings[key] != 0 and settings[key] < oldest:
            exit_with_error(f'{key} older than {oldest} is not supported')


@dataclasses.dataclass
class LinkArtifactNames:
    step1_ld_exe: str
    step1_ld_base: typing.Optional[str] = None
    step2_dlltool_exp: typing.Optional[str] = None
    step3_ld_exe: typing.Optional[str] = None
    step4_petran_exe: typing.Optional[str] = None


@ToolchainProfiler.profile_block('linker_setup')
def phase_linker_setup(options, state) -> LinkArtifactNames:
    system_libpath = '-L' + str(cache.get_lib_dir(absolute=True))
    system_js_path = '-L' + utils.path_from_root('src', 'lib')
    state.append_link_flag(system_libpath)
    state.append_link_flag(system_js_path)

    # We used to do this check during on startup during `check_sanity`, but
    # we now only do it when linking, in order to reduce the overhead when
    # only compiling.
    # if not shared.SKIP_SUBPROCS:
    #     shared.check_llvm_version()

    # options.output_file is the user-specified one, target is what we will generate
    if options.output_file:
        target = options.output_file
        # check for the existence of the output directory now, to avoid having
        # to do so repeatedly when each of the various output files (.mem, .wasm,
        # etc) are written. This gives a more useful error message than the
        # IOError and python backtrace that users would otherwise see.
        dirname = os.path.dirname(target)
        if dirname and not os.path.isdir(dirname):
            exit_with_error("specified output file (%s) is in a directory that does not exist" % target)
    elif options.shared:
        target = "a.dll"
    else:
        target = "a.exe"

    final_suffix = get_file_suffix(target)
    if not final_suffix:
        final_suffix = ".exe"
        target += final_suffix

    for s, reason in DEPRECATED_SETTINGS.items():
        if s in user_settings:
            diagnostics.warning('deprecated', f'{s} is deprecated ({reason}). Please open a bug if you have a continuing need for this setting')

    if not options.oformat:
        if final_suffix == '.exe':
            options.oformat = OFormat.EXE
        elif final_suffix == '.dll':
            options.oformat = OFormat.DLL

    need_uids = False

    # For users that opt out of WARN_ON_UNDEFINED_SYMBOLS we assume they also
    # want to opt out of ERROR_ON_UNDEFINED_SYMBOLS.
    if user_settings.get('WARN_ON_UNDEFINED_SYMBOLS') == '0':
        default_setting('ERROR_ON_UNDEFINED_SYMBOLS', 0)

    if options.oformat == OFormat.EXE:
        if settings.FIXME_DLLTOOL_LD_PETRAN:
            # FIXME: store intermediates in emscripten_temp directory (as we do for intermediate compiled objects)
            targetdir = os.path.dirname(target)
            target_basename, target_ext = os.path.splitext(os.path.basename(target))
            if target_ext and target_ext[:1] != ".":
                target_ext = f".{target_ext}"
            need_uids = True
            targets = LinkArtifactNames(
                step1_ld_exe=os.path.join(targetdir, f"{target_basename}_base{target_ext}"),
                step1_ld_base=os.path.join(targetdir, f"{target_basename}_base.bas"),
                step2_dlltool_exp=os.path.join(targetdir, f"{target_basename}.exp"),
                step3_ld_exe=os.path.join(targetdir, f"{target_basename}_notran{target_ext}"),
                step4_petran_exe=target,
            )
        else:
            targets = LinkArtifactNames(
                step1_ld_exe=target,
            )
    else:
        raise NotImplementedError("Unsupported output format:", options.oformat)

    if need_uids:
        if options.oformat == OFormat.EXE:
            if settings.UID1 < 0:
                settings.UID1 = 0x1000007a  # KExecutableImageUidValue, e32uid.h
            if settings.UID2 < 0:
                settings.UID2 = 0x100039ce  # KAppUidValue16, apadef.h
        elif options.oformat == OFormat.DLL:
            if settings.UID1 < 0:
                settings.UID1 = 0x10000079  # KDynamicLibraryUidValue, e32uid.h
            if settings.UID2 < 0:
                settings.UID2 = 0x100039ce  # KAppUidValue16, apadef.h
        else:
            raise NotImplementedError

        if settings.UID1 < 0:
            diagnostics.warning('uid', 'Assuming UID1=0x0. Add "-s UID1=0xabcdef01" to set an explicit uid3')
            settings.UID1 = 0x00000000
        if settings.UID2 < 0:
            diagnostics.warning('uid', 'Assuming UID2=0x0. Add "-s UID2=0xabcdef01" to set an explicit uid3')
            settings.UID2 = 0x00000000
        if settings.UID3 < 0:
            diagnostics.warning('uid', 'Assuming UID3=0x0. Add "-s UID3=0xabcdef01" to set an explicit uid3')
            settings.UID3 = 0x00000000

        if settings.UID1 >= 0x100000000:
            diagnostics.warning('uid', 'UID1 is out of range')
        if settings.UID2 >= 0x100000000:
            diagnostics.warning('uid', 'UID2 is out of range')
        if settings.UID3 >= 0x100000000:
            diagnostics.warning('uid', 'UID3 is out of range')

    return targets


@ToolchainProfiler.profile_block('calculate system libraries')
def phase_calculate_system_libraries(options, linker_arguments):
    extra_files_to_link = []
    # Link in ports and system libraries, if necessary
    # if not settings.SIDE_MODULE:
    #     # Ports are always linked into the main module, never the side module.
    #     extra_files_to_link += ports.get_libs(settings)
    # extra_files_to_link += system_libs.calculate(options)
    linker_arguments.extend(extra_files_to_link)


def filter_link_arguments_for_multilink(link_args: list[str]) -> list[str]:
    new_link_args = []

    def get_link_arg():
        yield from link_args

    link_arg_generator = get_link_arg()

    for arg in link_arg_generator:
        if arg in "-Map":
            next(link_arg_generator)
            continue
        if arg in "-base-file":
            next(link_arg_generator)
            continue
        new_link_args.append(arg)
    return new_link_args


@ToolchainProfiler.profile_block('link')
def phase_link(linker_arguments, targets: LinkArtifactNames):
    logger.debug(f'linking: {linker_arguments}')

    if settings.FIXME_DLLTOOL_LD_PETRAN:
        # Step 1
        filtered_link_args = filter_link_arguments_for_multilink(linker_arguments)
        step1_ld_args = filtered_link_args + ["--base-file", targets.step1_ld_base]
        building.link_lld(step1_ld_args, targets.step1_ld_exe)
        if not os.path.isfile(targets.step1_ld_exe):
            exit_with_error(f"step3: {shared.EPOC32_LD} failed to generate {targets.step1_ld_exe}")
        if not os.path.isfile(targets.step1_ld_base):
            exit_with_error(f"step3: {shared.EPOC32_LD} failed to generate {targets.step1_ld_base}")

        # Step 2
        building.check_call([shared.EPOC32_DLLTOOL, "-m", "arm_interwork", "--base-file", targets.step1_ld_base, "--output-exp", targets.step2_dlltool_exp])
        if not os.path.isfile(targets.step2_dlltool_exp):
            exit_with_error(f"step2: {shared.EPOC32_DLLTOOL} failed to generate {targets.step2_dlltool_exp}")

        # Step 3
        step3_ld_args = filtered_link_args + [targets.step2_dlltool_exp, "-o", targets.step3_ld_exe]
        building.link_lld(step3_ld_args, targets.step3_ld_exe)
        if not os.path.isfile(targets.step3_ld_exe):
            exit_with_error(f"step3: {shared.EPOC32_LD} failed to generate {targets.step3_ld_exe}")

        # Step 4
        building.check_call([shared.EPOC32_PETRAN, targets.step3_ld_exe, targets.step4_petran_exe, "-nocall", "-uid1", f"0x{settings.UID1:08x}", "-uid2", f"0x{settings.UID2:08x}", "-uid3", f"0x{settings.UID3:08x}", "-stack", str(settings.STACK_SIZE), "-heap", str(settings.HEAP_START), str(settings.HEAP_MAXIMUM)])
        if not os.path.isfile(targets.step4_petran_exe):
            exit_with_error(f"step4: {shared.EPOC32_PETRAN} failed to generate {targets.step4_petran_exe}")

        building.link_lld(linker_arguments, targets.step1_ld_exe)
    else:
        building.link_lld(linker_arguments, targets.step1_ld_exe)
    rtn = None
    if settings.LINKABLE and not settings.EXPORT_ALL:
        # In LINKABLE mode we pass `--export-dynamic` along with `--whole-archive`.  This results
        # in over 7000 exports, which cannot be distinguished from the few symbols we explicitly
        # export via EMSCRIPTEN_KEEPALIVE or EXPORTED_FUNCTIONS.
        # In order to avoid unnecessary exported symbols on the `Module` object we run the linker
        # twice in this mode:
        # 1. Without `--export-dynamic` to get the base exports
        # 2. With `--export-dynamic` to get the actual linkable Wasm binary
        # TODO(sbc): Remove this double execution of wasm-ld if we ever find a way to
        # distinguish EMSCRIPTEN_KEEPALIVE exports from `--export-dynamic` exports.
        settings.LINKABLE = False
        building.link_lld(linker_arguments, target)
        settings.LINKABLE = True
        rtn = extract_metadata.extract_metadata(target)

    return rtn


@ToolchainProfiler.profile_block('post link')
def phase_post_link(options, in_wasm, wasm_target, target, js_syms, base_metadata=None, linker_inputs=None):
    global final_js

    target_basename = unsuffixed_basename(target)

    if options.oformat != OFormat.WASM:
        final_js = in_temp(target_basename + '.js')

    settings.TARGET_BASENAME = unsuffixed_basename(target)

    if options.oformat in (OFormat.JS, OFormat.MJS):
        js_target = target
    else:
        js_target = get_secondary_target(target, '.js')

    settings.TARGET_JS_NAME = os.path.basename(js_target)

    metadata = phase_emscript(in_wasm, wasm_target, js_syms, base_metadata)


    # If we are not emitting any JS then we are all done now
    if options.oformat != OFormat.WASM:
        phase_final_emitting(options, target, js_target, wasm_target)


# for Popen, we cannot have doublequotes, so provide functionality to
# remove them when needed.
def remove_quotes(arg):
    if isinstance(arg, list):
        return [remove_quotes(a) for a in arg]

    if arg.startswith('"') and arg.endswith('"'):
        return arg[1:-1].replace('\\"', '"')
    elif arg.startswith("'") and arg.endswith("'"):
        return arg[1:-1].replace("\\'", "'")
    else:
        return arg


def modularize():
    global final_js
    logger.debug(f'Modularizing, assigning to var {settings.EXPORT_NAME}')
    generated_js = read_file(final_js)

    final_js += '.modular.js'
    write_file(final_js, src)
    shared.get_temp_files().note(final_js)
    save_intermediate('modularized')


def module_export_name_substitution():
    assert not settings.MODULARIZE
    global final_js
    logger.debug(f'Private module export name substitution with {settings.EXPORT_NAME}')
    src = read_file(final_js)
    final_js += '.module_export_name_substitution.js'
    if settings.MINIMAL_RUNTIME and not settings.ENVIRONMENT_MAY_BE_NODE and not settings.ENVIRONMENT_MAY_BE_SHELL and not settings.AUDIO_WORKLET:
        # On the web, with MINIMAL_RUNTIME, the Module object is always provided
        # via the shell html in order to provide the .asm.js/.wasm content.
        replacement = settings.EXPORT_NAME
    else:
        replacement = "typeof %(EXPORT_NAME)s != 'undefined' ? %(EXPORT_NAME)s : {}" % {"EXPORT_NAME": settings.EXPORT_NAME}
    new_src = re.sub(r'{\s*[\'"]?__EMSCRIPTEN_PRIVATE_MODULE_EXPORT_NAME_SUBSTITUTION__[\'"]?:\s*1\s*}', replacement, src)
    assert new_src != src, 'Unable to find Closure syntax __EMSCRIPTEN_PRIVATE_MODULE_EXPORT_NAME_SUBSTITUTION__ in source!'
    write_file(final_js, new_src)
    shared.get_temp_files().note(final_js)
    save_intermediate('module_export_name_substitution')


def generate_traditional_runtime_html(target, options, js_target, target_basename,
                                      wasm_target):
    script = ScriptSource()

    if settings.EXPORT_NAME != 'Module' and options.shell_path == DEFAULT_SHELL_HTML:
        # the minimal runtime shell HTML is designed to support changing the export
        # name, but the normal one does not support that currently
        exit_with_error('customizing EXPORT_NAME requires that the HTML be customized to use that name (see https://github.com/emscripten-core/emscripten/issues/10086)')

    shell = shared.read_and_preprocess(options.shell_path)
    if '{{{ SCRIPT }}}' not in shell:
        exit_with_error('HTML shell must contain {{{ SCRIPT }}}, see src/shell.html for an example')
    base_js_target = os.path.basename(js_target)

    if settings.PROXY_TO_WORKER:
        proxy_worker_filename = (settings.PROXY_TO_WORKER_FILENAME or target_basename) + '.js'
        script.inline = worker_js_script(proxy_worker_filename)
    else:
        # Normal code generation path
        script.src = base_js_target

    if settings.SINGLE_FILE:
        # In SINGLE_FILE mode we either inline the script, or in the case
        # of SHARED_MEMORY convert the entire thing into a data URL.
        if settings.SHARED_MEMORY:
            assert not script.inline
            script.src = get_subresource_location(js_target)
        else:
            js_contents = script.inline or ''
            if script.src:
                js_contents += read_file(js_target)
            script.src = None
            script.inline = read_file(js_target)
        delete_file(js_target)
    else:
        if not settings.WASM_ASYNC_COMPILATION:
            # We need to load the wasm file before anything else, since it
            # has be synchronously ready.
            script.un_src()
            script.inline = '''
          fetch('%s').then((result) => result.arrayBuffer())
                     .then((buf) => {
                             Module.wasmBinary = buf;
                             %s;
                           });
''' % (get_subresource_location(wasm_target), script.inline)

        if settings.WASM == 2:
            # If target browser does not support WebAssembly, we need to load
            # the .wasm.js file before the main .js file.
            script.un_src()
            script.inline = '''
          function loadMainJs() {
%s
          }
          if (!window.WebAssembly || location.search.indexOf('_rwasm=0') > 0) {
            // Current browser does not support WebAssembly, load the .wasm.js JavaScript fallback
            // before the main JS runtime.
            var wasm2js = document.createElement('script');
            wasm2js.src = '%s';
            wasm2js.onload = loadMainJs;
            document.body.appendChild(wasm2js);
          } else {
            // Current browser supports Wasm, proceed with loading the main JS runtime.
            loadMainJs();
          }
''' % (script.inline, get_subresource_location_js(wasm_target + '.js'))

    shell = do_replace(shell, '{{{ SCRIPT }}}', script.replacement())
    shell = shell.replace('{{{ SHELL_CSS }}}', utils.read_file(utils.path_from_root('src/shell.css')))
    logo_filename = utils.path_from_root('media/powered_by_logo_shell.png')
    logo_b64 = base64_encode(logo_filename)
    shell = shell.replace('{{{ SHELL_LOGO }}}', f'<img id="emscripten_logo" src="data:image/png;base64,{logo_b64}">')

    check_output_file(target)
    write_file(target, shell)


def minify_html(filename):
    if settings.DEBUG_LEVEL >= 2:
        return

    opts = []
    # -g1 and greater retain whitespace and comments in source
    if settings.DEBUG_LEVEL == 0:
        opts += ['--collapse-whitespace',
                 '--remove-comments',
                 '--remove-tag-whitespace',
                 '--sort-attributes',
                 '--sort-class-name']
    # -g2 and greater do not minify HTML at all
    if settings.DEBUG_LEVEL <= 1:
        opts += ['--decode-entities',
                 '--collapse-boolean-attributes',
                 '--remove-attribute-quotes',
                 '--remove-redundant-attributes',
                 '--remove-script-type-attributes',
                 '--remove-style-link-type-attributes',
                 '--use-short-doctype',
                 '--minify-css', 'true',
                 '--minify-js', 'true']

    logger.debug(f'minifying HTML file {filename}')
    size_before = os.path.getsize(filename)
    start_time = time.time()
    shared.check_call(shared.get_npm_cmd('html-minifier-terser') + [filename, '-o', filename] + opts, env=shared.env_with_node_in_path())

    elapsed_time = time.time() - start_time
    size_after = os.path.getsize(filename)
    delta = size_after - size_before
    logger.debug(f'HTML minification took {elapsed_time:.2f} seconds, and shrunk size of {filename} from {size_before} to {size_after} bytes, delta={delta} ({delta * 100.0 / size_before:+.2f}%)')


def generate_html(target, options, js_target, target_basename, wasm_target):
    logger.debug('generating HTML')

    if settings.MINIMAL_RUNTIME:
        generate_minimal_runtime_html(target, options, js_target, target_basename)
    else:
        generate_traditional_runtime_html(target, options, js_target, target_basename, wasm_target)

    if settings.MINIFY_HTML and (settings.OPT_LEVEL >= 1 or settings.SHRINK_LEVEL >= 1):
        minify_html(target)

    tools.line_endings.convert_line_endings_in_file(target, os.linesep, options.output_eol)


def generate_worker_js(target, js_target, target_basename):
    if settings.SINGLE_FILE:
        # compiler output is embedded as base64 data URL
        proxy_worker_filename = get_subresource_location_js(js_target)
    else:
        # compiler output goes in .worker.js file
        move_file(js_target, shared.replace_suffix(js_target, get_worker_js_suffix()))
        worker_target_basename = target_basename + '.worker'
        proxy_worker_filename = (settings.PROXY_TO_WORKER_FILENAME or worker_target_basename) + '.js'

    target_contents = worker_js_script(proxy_worker_filename)
    write_file(target, target_contents)


def worker_js_script(proxy_worker_filename):
    web_gl_client_src = read_file(utils.path_from_root('src/webGLClient.js'))
    proxy_client_src = shared.read_and_preprocess(utils.path_from_root('src/proxyClient.js'), expand_macros=True)
    if not settings.SINGLE_FILE and not os.path.dirname(proxy_worker_filename):
        proxy_worker_filename = './' + proxy_worker_filename
    proxy_client_src = do_replace(proxy_client_src, '<<< filename >>>', proxy_worker_filename)
    return web_gl_client_src + '\n' + proxy_client_src


def find_library(lib, lib_dirs):
    for lib_dir in lib_dirs:
        path = os.path.join(lib_dir, lib)
        if os.path.isfile(path):
            logger.debug('found library "%s" at %s', lib, path)
            return path
    return None


def map_to_js_libs(library_name):
    """Given the name of a special Emscripten-implemented system library, returns an
    pair containing
    1. Array of absolute paths to JS library files, inside emscripten/src/ that corresponds to the
       library name. `None` means there is no mapping and the library will be processed by the linker
       as a require for normal native library.
    2. Optional name of a corresponding native library to link in.
    """
    # Some native libraries are implemented in Emscripten as system side JS libraries
    library_map = {
        'embind': ['libembind.js', 'libemval.js'],
        'EGL': ['libegl.js'],
        'GL': ['libwebgl.js', 'libhtml5_webgl.js'],
        'webgl.js': ['libwebgl.js', 'libhtml5_webgl.js'],
        'GLESv2': ['libwebgl.js'],
        # N.b. there is no GLESv3 to link to (note [f] in https://www.khronos.org/registry/implementers_guide.html)
        'GLEW': ['libglew.js'],
        'glfw': ['libglfw.js'],
        'glfw3': ['libglfw.js'],
        'GLU': [],
        'glut': ['libglut.js'],
        'openal': ['libopenal.js'],
        'X11': ['libxlib.js'],
        'SDL': ['libsdl.js'],
        'uuid': ['libuuid.js'],
        'fetch': ['libfetch.js'],
        'websocket': ['libwebsocket.js'],
        # These 4 libraries are separate under glibc but are all rolled into
        # libc with musl.  For compatibility with glibc we just ignore them
        # completely.
        'dl': [],
        'm': [],
        'rt': [],
        'pthread': [],
        # This is the name of GNU's C++ standard library. We ignore it here
        # for compatibility with GNU toolchains.
        'stdc++': [],
        'SDL2_mixer': [],
    }
    settings_map = {
        'glfw': {'USE_GLFW': 2},
        'glfw3': {'USE_GLFW': 3},
        'SDL': {'USE_SDL': 1},
        'SDL2_mixer': {'USE_SDL_MIXER': 2},
    }

    if library_name in settings_map:
        for key, value in settings_map[library_name].items():
            default_setting(key, value)

    if library_name in library_map:
        libs = library_map[library_name]
        logger.debug('Mapping library `%s` to JS libraries: %s' % (library_name, libs))
        return libs

    return None


class ScriptSource:
    def __init__(self):
        self.src = None # if set, we have a script to load with a src attribute
        self.inline = None # if set, we have the contents of a script to write inline in a script

    def un_src(self):
        """Use this if you want to modify the script and need it to be inline."""
        if self.src is None:
            return
        quoted_src = quote(self.src)
        if settings.EXPORT_ES6:
            self.inline = f'''
        import("./{quoted_src}").then(exports => exports.default(Module))
      '''
        else:
            self.inline = f'''
            var script = document.createElement('script');
            script.src = "{quoted_src}";
            document.body.appendChild(script);
      '''
        self.src = None

    def replacement(self):
        """Returns the script tag to replace the {{{ SCRIPT }}} tag in the target"""
        assert (self.src or self.inline) and not (self.src and self.inline)
        if self.src:
            src = self.src
            if src.startswith('data:'):
                filename = src
            else:
                src = quote(self.src)
                filename = f'./{src}'
            if settings.EXPORT_ES6:
                return f'''
        <script type="module">
          import initModule from "{filename}";
          initModule(Module);
        </script>
        '''
            else:
                return f'<script async type="text/javascript" src="{src}"></script>'
        else:
            return '<script>\n%s\n</script>' % self.inline


def process_dynamic_libs(dylibs, lib_dirs):
    extras = []
    seen = set()
    to_process = dylibs.copy()
    while to_process:
        dylib = to_process.pop()
        dylink = webassembly.parse_dylink_section(dylib)
        for needed in dylink.needed:
            if needed in seen:
                continue
            path = find_library(needed, lib_dirs)
            if path:
                extras.append(path)
                seen.add(needed)
            else:
                exit_with_error(f'{os.path.normpath(dylib)}: shared library dependency not found in library path: `{needed}`. (library path: {lib_dirs}')
            to_process.append(path)

    dylibs += extras
    for dylib in dylibs:
        exports = webassembly.get_exports(dylib)
        exports = {e.name for e in exports}
        # EM_JS function are exports with a special prefix.  We need to strip
        # this prefix to get the actual symbol name.  For the main module, this
        # is handled by extract_metadata.py.
        exports = [removeprefix(e, '__em_js__') for e in exports]
        settings.SIDE_MODULE_EXPORTS.extend(sorted(exports))

        imports = webassembly.get_imports(dylib)
        imports = [i.field for i in imports if i.kind in (webassembly.ExternType.FUNC, webassembly.ExternType.GLOBAL, webassembly.ExternType.TAG)]
        # For now we ignore `invoke_` functions imported by side modules and rely
        # on the dynamic linker to create them on the fly.
        # TODO(sbc): Integrate with metadata.invoke_funcs that comes from the
        # main module to avoid creating new invoke functions at runtime.
        imports = set(imports)
        imports = {i for i in imports if not i.startswith('invoke_')}
        weak_imports = webassembly.get_weak_imports(dylib)
        strong_imports = sorted(imports.difference(weak_imports))
        logger.debug('Adding symbols requirements from `%s`: %s', dylib, imports)

        mangled_imports = [shared.asmjs_mangle(e) for e in sorted(imports)]
        mangled_strong_imports = [shared.asmjs_mangle(e) for e in strong_imports]
        for sym in weak_imports:
            mangled = shared.asmjs_mangle(sym)
            if mangled not in settings.SIDE_MODULE_IMPORTS and mangled not in building.user_requested_exports:
                settings.WEAK_IMPORTS.append(sym)
        settings.SIDE_MODULE_IMPORTS.extend(mangled_imports)
        settings.EXPORT_IF_DEFINED.extend(sorted(imports))
        settings.DEFAULT_LIBRARY_FUNCS_TO_INCLUDE.extend(sorted(imports))
        building.user_requested_exports.update(mangled_strong_imports)


def unmangle_symbols_from_cmdline(symbols):
    def unmangle(x):
        return x.replace('.', ' ').replace('#', '&').replace('?', ',')

    if type(symbols) is list:
        return [unmangle(x) for x in symbols]
    return unmangle(symbols)


def get_secondary_target(target, ext):
    # Depending on the output format emscripten creates zero or more secondary
    # output files (e.g. the .wasm file when creating JS output, or the
    # .js and the .wasm file when creating html output.
    # Thus function names the secondary output files, while ensuring they
    # never collide with the primary one.
    base = unsuffixed(target)
    if get_file_suffix(target) == ext:
        base += '_'
    return base + ext


def dedup_list(lst):
    # Since we require python 3.6, that ordering of dictionaries is guaranteed
    # to be insertion order so we can use 'dict' here but not 'set'.
    return list(dict.fromkeys(lst))


def check_output_file(f):
    if os.path.isdir(f):
        exit_with_error(f'cannot write output file `{f}`: Is a directory')


def move_file(src, dst):
    logging.debug('move: %s -> %s', src, dst)
    check_output_file(dst)
    src = os.path.abspath(src)
    dst = os.path.abspath(dst)
    if src == dst:
        return
    if dst == os.devnull:
        return
    shutil.move(src, dst)


# Returns the subresource location for run-time access
def get_subresource_location(path, mimetype='application/octet-stream'):
    if settings.SINGLE_FILE:
        return f'data:{mimetype};base64,{base64_encode(path)}'
    else:
        return os.path.basename(path)


def get_subresource_location_js(path):
    return get_subresource_location(path, 'text/javascript')


@ToolchainProfiler.profile()
def package_files(options, target):
    rtn = []
    logger.debug('setting up files')
    file_args = ['--from-emcc']
    if options.preload_files:
        file_args.append('--preload')
        file_args += options.preload_files
    if options.embed_files:
        file_args.append('--embed')
        file_args += options.embed_files
    if options.exclude_files:
        file_args.append('--exclude')
        file_args += options.exclude_files
    if options.use_preload_cache:
        file_args.append('--use-preload-cache')
    if settings.LZ4:
        file_args.append('--lz4')
    if options.use_preload_plugins:
        file_args.append('--use-preload-plugins')
    if not settings.ENVIRONMENT_MAY_BE_NODE:
        file_args.append('--no-node')
    if options.embed_files:
        if settings.MEMORY64:
            file_args += ['--wasm64']
        object_file = in_temp('embedded_files.o')
        file_args += ['--obj-output=' + object_file]
        rtn.append(object_file)

    cmd = [shared.FILE_PACKAGER, shared.replace_suffix(target, '.data')] + file_args
    if options.preload_files:
        # Preloading files uses --pre-js code that runs before the module is loaded.
        file_code = shared.check_call(cmd, stdout=PIPE).stdout
        js_manipulation.add_files_pre_js(settings.PRE_JS_FILES, file_code)
    else:
        # Otherwise, we are embedding files, which does not require --pre-js code,
        # and instead relies on a static constructor to populate the filesystem.
        shared.check_call(cmd)

    return rtn


@ToolchainProfiler.profile_block('calculate linker inputs')
def phase_calculate_linker_inputs(options, state, linker_inputs):
    using_lld = not (options.oformat == OFormat.OBJECT and settings.LTO)
    state.link_flags = filter_link_flags(state.link_flags, using_lld)

    # Interleave the linker inputs with the linker flags while maintainging their
    # relative order on the command line (both of these list are pairs, with the
    # first element being their command line position).
    linker_args = [val for _, val in sorted(linker_inputs + state.link_flags)]

    if "-nostlib" not in linker_args:
        linker_args = [
             "-e", "_E32Startup", "-u", "_E32Startup",
            os.path.join(os.environ["NGAGESDK"], "sdk", "ngagesdk_entry.o"),
        ] + linker_args

    return linker_args


def run_post_link(wasm_input, options, state):
    settings.limit_settings(None)
    target = phase_linker_setup(options, state)
    phase_post_link(options, wasm_input, target, {})


def run(linker_inputs, options, state):
    # We have now passed the compile phase, allow reading/writing of all settings.
    settings.limit_settings(None)

    if not linker_inputs and not state.link_flags:
        exit_with_error('no input files')

    if options.output_file and options.output_file.startswith('-'):
        exit_with_error(f'invalid output filename: `{options.output_file}`')

    targets = phase_linker_setup(options, state)

    # Link object files using wasm-ld or llvm-link (for bitcode linking)
    linker_arguments = phase_calculate_linker_inputs(options, state, linker_inputs)

    phase_calculate_system_libraries(options, linker_arguments)

    base_metadata = phase_link(linker_arguments, targets)

    # Special handling for when the user passed '-Wl,--version'.  In this case the linker
    # does not create the output file, but just prints its version and exits with 0.
    if '--version' in linker_arguments:
        return 0

    return 0
