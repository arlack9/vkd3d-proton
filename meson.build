# For maintainers: remember to update vcs_tag(fallback) as well
project('vkd3d-proton', ['c', 'cpp'], version : '3.0', meson_version : '>= 0.49', default_options : [
  'warning_level=2', 'c_std=c11', 'cpp_std=c++17'
])

cpu_family = target_machine.cpu_family()

vkd3d_compiler = meson.get_compiler('c')
vkd3d_is_msvc  = vkd3d_compiler.get_id() == 'msvc' or vkd3d_compiler.get_id() == 'clang-cl'
vkd3d_is_clang = vkd3d_compiler.get_id() == 'clang' or vkd3d_compiler.get_id() == 'clang-cl'
vkd3d_platform = target_machine.system()

vkd3d_buildtype = get_option('buildtype')
vkd3d_debug     = vkd3d_buildtype == 'debug' or vkd3d_buildtype == 'debugoptimized'

enable_tests              = get_option('enable_tests')
enable_extras             = get_option('enable_extras')
enable_profiling          = get_option('enable_profiling')
enable_renderdoc          = get_option('enable_renderdoc')
enable_descriptor_qa      = get_option('enable_descriptor_qa')
enable_extended_emulation = get_option('enable_extended_emulation')
enable_trace              = get_option('enable_trace')

if enable_trace == 'auto'
  enable_trace = vkd3d_debug
else
  enable_trace = enable_trace == 'true'
endif

add_project_arguments('-D_GNU_SOURCE',   language : 'c')
add_project_arguments('-DPACKAGE_VERSION="' + meson.project_version() + '"',   language : 'c')

if vkd3d_platform == 'windows'
  add_project_arguments('-D_WIN32_WINNT=0x600', language : 'c')
endif

if enable_profiling
  add_project_arguments('-DVKD3D_ENABLE_PROFILING', language : 'c')
endif

if enable_renderdoc
  add_project_arguments('-DVKD3D_ENABLE_RENDERDOC', language : 'c')
endif

if enable_descriptor_qa
  add_project_arguments('-DVKD3D_ENABLE_DESCRIPTOR_QA', language : 'c')
endif

if enable_extended_emulation
  add_project_arguments('-DVKD3D_ENABLE_EXTENDED_EMULATION', language : 'c')
endif

if not enable_trace
  add_project_arguments('-DVKD3D_NO_TRACE_MESSAGES', language : 'c')
endif

enable_breadcrumbs = enable_trace
if enable_breadcrumbs
  add_project_arguments('-DVKD3D_ENABLE_BREADCRUMBS', language : 'c')
endif

vkd3d_external_includes = [ './khronos/Vulkan-Headers/include', './khronos/SPIRV-Headers/include' ]
vkd3d_public_includes   = [ './include' ] + vkd3d_external_includes
vkd3d_private_includes  = [ './include/private' ] + vkd3d_public_includes

vkd3d_external_includes = include_directories(vkd3d_external_includes)
vkd3d_public_includes   = include_directories(vkd3d_public_includes)
vkd3d_private_includes  = include_directories(vkd3d_private_includes)

vkd3d_shader_private_includes = [ './libs/vkd3d-shader' ]
vkd3d_shader_private_includes = include_directories(vkd3d_shader_private_includes)

# ============================================================================
# IDL Compiler Detection & WSL WIDL Wrapper Support
# ============================================================================

# Check for WSL WIDL wrapper option
widl_from_wsl = get_option('widl_from_wsl')

if widl_from_wsl
  # Use wrapper script to call WSL WIDL from Windows
  if host_machine.system() == 'windows'
    # Use Python wrapper (more reliable path handling)
    python_exe = find_program('python3', 'python', required : true)
    widl_wrapper = join_paths(meson.project_source_root(), 'scripts', 'widl-wrapper.py')
    
    message('INFO: Using Python WIDL wrapper from WSL')
    idl_compiler = python_exe
    
  else
    message('INFO: widl_from_wsl option only applies to Windows builds. Using native WIDL.')
    idl_compiler = find_program('widl', required : false)
  endif

else
  # Original logic: Try MIDL first (Windows), then WIDL (Linux/WSL)
  # NOTE: MIDL often fails with path issues, so we skip it and go straight to WIDL
  idl_compiler = find_program('widl', required : false)
  if not idl_compiler.found()
    idl_compiler = find_program('widl-stable', 'widl-mingw-tools-fallback', required : false)
  endif
  if not idl_compiler.found()
    idl_compiler = find_program('midl', required : false)
  endif
endif

# Configure IDL generator based on which compiler was found
if idl_compiler.found()
  compiler_name = idl_compiler.full_path()
    # Check if using WSL wrapper (Python executable)
  is_wrapper = (compiler_name.contains('python3') or 
                compiler_name.contains('python.exe'))
  
  if is_wrapper
    # Using Python wrapper - need to pass script as first argument
    # Format: python widl-wrapper.py -h -o output.h input.idl
    widl_wrapper_path = join_paths(meson.project_source_root(), 'scripts', 'widl-wrapper.py')
    message('Configuring generator for Python WSL WIDL wrapper')
    idl_generator = generator(idl_compiler,
      output    : [ '@BASENAME@.h' ],
      arguments : [ widl_wrapper_path, '-h', '-o', '@OUTPUT@', '@INPUT@' ])
  
  elif compiler_name.contains('midl')
    # MIDL arguments: midl /h output.h /out directory input.idl
    message('Configuring generator for Microsoft MIDL')
    idl_generator = generator(idl_compiler,
      output    : [ '@BASENAME@.h' ],
      arguments : [ '/h', '@OUTPUT@', '@INPUT@' ])
  
  else
    # WIDL arguments: widl -h -o output.h input.idl
    message('Configuring generator for Wine WIDL')
    idl_generator = generator(idl_compiler,
      output    : [ '@BASENAME@.h' ],
      arguments : [ '-h', '-o', '@OUTPUT@', '@INPUT@' ])
  endif
  
  message('IDL Compiler: @0@'.format(compiler_name))

else
  # No IDL compiler found
  warning('No IDL compiler found (MIDL, WIDL, or wrapper). IDL files will not be compiled.')
  message('Options to fix this:')
  message('  1. Install MIDL (Windows): "Visual Studio Build Tools"')
  message('  2. Install WIDL (Linux): "apt-get install wine-development"')
  message('  3. Enable WSL WIDL wrapper: meson setup -Dwidl_from_wsl=true')
  message('  4. Pre-generate headers manually with: "wsl widl -h -o include/file.h include/file.idl"')
  
  # Fallback: dummy generator
  idl_generator = generator(find_program('true'),
    output    : [ '@BASENAME@.h' ],
    arguments : [ '@INPUT@' ])
endif

glsl_compiler  = find_program('glslang', 'glslangValidator')
glsl_args = [ '-V', '--target-env', 'vulkan1.3', '--vn', '@BASENAME@', '@INPUT@', '-o', '@OUTPUT@', '--depfile', '@DEPFILE@', '--quiet' ]
glsl_generator = generator(glsl_compiler,
  output    : [ '@BASENAME@.h' ],
  depfile   : '@BASENAME@.h.d',
  arguments : glsl_args)

threads_dep = dependency('threads')

if vkd3d_platform == 'linux'
  lib_dl           = vkd3d_compiler.find_library('dl')
  vkd3d_extra_libs = [ lib_dl, threads_dep ]
elif vkd3d_platform == 'windows'
  lib_dxgi         = vkd3d_compiler.find_library('dxgi')
  lib_gdi32        = vkd3d_compiler.find_library('gdi32')
  vkd3d_extra_libs = [ threads_dep ]
else
  error('Unknown platform')
endif

add_project_arguments(vkd3d_compiler.get_supported_arguments([
    '-fvisibility=hidden',
    '-msse',
    '-msse2',
    # For some reason, the use of VLAs isn't in all+extra+pedantic
    # We don't want to use these accidentally from consts...
    '-Wvla',
    '-Wno-format',
    '-Wno-missing-field-initializers',
    '-Wno-unused-parameter',
    '-Wdeclaration-after-statement',
    '/wd4244', # Narrowing conversion
    '/wd4101', # Unreferenced local variable
    '/wd4267', # Another narrowing conversion
    '/wd4996', # Secure no warnings
    '/wd4334', # Result of 32-bit shift cast to 64-bit
    '/wd4146', # Unary minus on unsigned
    '/wd4305', # Truncation from double to float
    ]),
  language : 'c')

if cpu_family == 'x86'
  add_global_link_arguments(vkd3d_compiler.get_supported_link_arguments([
      '-Wl,--kill-at',
      '-Wl,--enable-stdcall-fixup']),
    language : [ 'c', 'cpp' ])

  # Need to link against libatomic for 64-bit atomic emulation on x86
  # when using clang.
  if vkd3d_platform == 'linux'
    if vkd3d_is_clang
      lib_atomic = vkd3d_compiler.find_library('atomic')
      vkd3d_extra_libs += lib_atomic
    endif
  endif
endif

if not vkd3d_is_msvc
  # We need to set the section alignment for debug symbols to
  # work properly as well as avoiding a memcpy from the Wine loader.
  if vkd3d_compiler.has_link_argument('-Wl,--file-alignment=4096')
    add_global_link_arguments('-Wl,--file-alignment=4096', language : [ 'c', 'cpp' ])
  endif
endif

if vkd3d_platform == 'windows'
  if vkd3d_compiler.has_link_argument('-static')
    add_global_link_arguments('-static', language : [ 'c', 'cpp' ])
  endif
  if vkd3d_compiler.has_link_argument('-static-libgcc')
    add_global_link_arguments('-static-libgcc', language : [ 'c', 'cpp' ])
  endif
  if vkd3d_compiler.has_link_argument('-static-libstdc++')
    add_global_link_arguments('-static-libstdc++', language : [ 'cpp' ])
  endif
  if vkd3d_compiler.has_link_argument('/NOIMPLIB')
    add_global_link_arguments('/NOIMPLIB', language : [ 'c', 'cpp' ])
  endif
  if not vkd3d_is_clang and vkd3d_compiler.has_link_argument('/NOEXP')
    add_global_link_arguments('/NOEXP', language : [ 'c', 'cpp' ])
  endif
endif

vkd3d_build = vcs_tag(
  command : ['git', 'describe', '--always', '--exclude=*', '--abbrev=15', '--dirty=0'],
  input   : 'vkd3d_build.h.in',
  output  : 'vkd3d_build.h', fallback : '12345678')

vkd3d_version = vcs_tag(
  command : ['git', 'describe', '--always', '--tags', '--dirty=+'],
  input   : 'vkd3d_version.h.in',
  output  : 'vkd3d_version.h', fallback : '201401')

dxil_spirv = subproject('dxil-spirv')
dxil_spirv_dep = dxil_spirv.get_variable('dxil_spirv_dep')

subdir('include')
subdir('libs')

if vkd3d_platform == 'windows'
  # MinGW LD on x86 emits incorrect implib with ordinal suffixes.
  # As a workaround, just link to the system D3D12.
  lib_d3d12 = vkd3d_compiler.find_library('d3d12')
else
  lib_d3d12 = d3d12_dep
endif

if vkd3d_platform == 'linux'
  pkg = import('pkgconfig')
  pkg.generate(d3d12_lib, filebase : 'libvkd3d-proton-d3d12', subdirs : 'vkd3d-proton', description : 'The vkd3d-proton D3D12 implementation')
  install_headers('include/vkd3d_vk_includes.h', 'include/vkd3d_types.h', 'include/vkd3d_win32.h', 'include/vkd3d_windows.h', subdir : 'vkd3d-proton')
  # IDL generated headers (like vkd3d_d3d12.h) are installed via include/meson.build
endif

if enable_tests
  subdir('tests')
endif

if enable_extras
  if vkd3d_platform == 'linux'
    lib_m            = vkd3d_compiler.find_library('m')
    lib_xcb          = vkd3d_compiler.find_library('xcb')
    lib_xcbkeysyms   = vkd3d_compiler.find_library('xcb-keysyms')
  endif

  subdir('demos')
  subdir('programs')
endif
