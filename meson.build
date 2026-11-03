# 顶层转发到 Code/（产物仍进 Build/）
project(
  'hotsend',
  'c',
  version: '0.1.0',
  meson_version: '>=0.61.0',
  default_options: [
    'c_std=c11',
    'warning_level=2',
    'buildtype=debugoptimized',
  ],
)

add_project_arguments('-D_POSIX_C_SOURCE=200809L', language: 'c')

subdir('Code')
