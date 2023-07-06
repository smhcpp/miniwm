src_files = files(
	'main.c',
  'helper.c',
)
deps=[
  #libevdev,
  #libinput,
  #libxml2,, wlr_protos
  wayland_protos,
  pixman,
  wayland_server,
  wlroots,
  xkbcommon,
]
executable(
	'blake',
	src_files+wl_protos_src,
  include_directories: [inc],
	dependencies: [deps],
)