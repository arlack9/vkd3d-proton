vkd3d_shader_src = [
  'checksum.c',
  'dxil.c',
  'dxbc.c',
  'vkd3d_shader_main.c',
  'dxil_cfg_logger.c',
  '3rdparty/md5/md5.c',
]

# CFG extraction now handled via dxil_spirv C API (dxil_spv_converter_get_cfg)
vkd3d_shader_lib = static_library('vkd3d-shader', vkd3d_shader_src,
  dependencies        : [ vkd3d_common_dep, dxil_spirv_dep ],
  include_directories : [ 
    vkd3d_common_lib.private_dir_include(), 
    vkd3d_private_includes,
  ])

vkd3d_shader_dep = declare_dependency(
  link_with           : vkd3d_shader_lib,
  dependencies        : vkd3d_common_dep,
  include_directories : vkd3d_public_includes)