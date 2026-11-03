# 由顶层 HotSend/meson.build 的 subdir('Code') 引入
zlib_dep = dependency('zlib')

protocol_inc = include_directories('Protocol')

protocol_sources = files(
  'Protocol/Crc32.c',
  'Protocol/TcpTransport.c',
  'Protocol/SerialTransport.c',
  'Protocol/SerialPrompt.c',
  'Protocol/SessionLines.c',
  'Protocol/BlockPlan.c',
  'Protocol/TransferSupport.c',
  'Protocol/TransferSession.c',
  'Protocol/InfoSession.c',
  'Protocol/Transfer.c',
)

hotsend_protocol = static_library(
  'hotsend_protocol',
  protocol_sources,
  include_directories: protocol_inc,
  dependencies: zlib_dep,
)

hotsend_protocol_dep = declare_dependency(
  link_with: hotsend_protocol,
  include_directories: protocol_inc,
  dependencies: zlib_dep,
)

executable(
  'hotsend-smoke-tcp',
  'Protocol/SmokeTcp.c',
  dependencies: hotsend_protocol_dep,
  install: false,
)

executable(
  'hotsend',
  files(
    'Cli/Main.c',
    'Cli/Arguments.c',
  ),
  include_directories: [protocol_inc, include_directories('Cli')],
  dependencies: hotsend_protocol_dep,
  install: false,
)

gtk4_dep = dependency('gtk4')

executable(
  'hotsend-gui',
  files(
    'Gui/Main.c',
    'Gui/Widgets.c',
    'Gui/Worker.c',
  ),
  include_directories: [protocol_inc, include_directories('Gui')],
  dependencies: [hotsend_protocol_dep, gtk4_dep],
  install: false,
)
