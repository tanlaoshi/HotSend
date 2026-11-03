# 由顶层 HotSend/meson.build 的 subdir('Code') 引入
zlib_dep = dependency('zlib')

protocol_inc = include_directories('Protocol')

protocol_sources = files(
  'Protocol/HotSendCrc32.c',
  'Protocol/HotSendTcpTransport.c',
  'Protocol/HotSendSessionLines.c',
  'Protocol/HotSendBlockPlan.c',
  'Protocol/HotSendTransferSupport.c',
  'Protocol/HotSendTransfer.c',
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
  'Protocol/HotSendSmokeTcp.c',
  dependencies: hotsend_protocol_dep,
  install: false,
)

executable(
  'hotsend',
  files(
    'Cli/HotSendCliMain.c',
    'Cli/HotSendCliArguments.c',
  ),
  include_directories: [protocol_inc, include_directories('Cli')],
  dependencies: hotsend_protocol_dep,
  install: false,
)
