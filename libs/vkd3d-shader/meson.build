vkd3d_shader_src = [
  'checksum.c',
  'dxil.c',
  'dxbc.c',
  'vkd3d_shader_main.c',
  '3rdparty/md5/md5.c',
]

# DXIL ABI Bridge integration
dxil_bridge_inc = include_directories('dxil-bridge')

# Check if DXIL bridge library exists and add as dependency
dxil_bridge_lib = []
if meson.is_cross_build() or host_machine.system() == 'windows'
  # For Windows, link against precompiled static library
  dxil_bridge_lib = [vkd3d_compiler.find_library('dxil_bridge', dirs: meson.current_source_dir() / 'dxil-bridge')]
endif

vkd3d_shader_lib = static_library('vkd3d-shader', vkd3d_shader_src,
  dependencies        : [ vkd3d_common_dep, dxil_spirv_dep, dxil_bridge_lib ],
  include_directories : [ vkd3d_common_lib.private_dir_include(), vkd3d_private_includes, dxil_bridge_inc ])

vkd3d_shader_dep = declare_dependency(
  link_with           : vkd3d_shader_lib,
  dependencies        : vkd3d_common_dep,
  include_directories : vkd3d_public_includes)
