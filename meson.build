project(
  'blake',
  'c',
  version: '0.1',
  license: 'MIT',
  #meson_version: '>=0.52.0',
  default_options: [
    'c_std=c11',
    'warning_level=2',
    'werror=true',
  ],
)
add_project_arguments(
	'-Wno-unused-parameter',
	'-Wno-unused-result',
	'-Wundef',

	'-DWL_HIDE_DEPRECATED',
	'-DWLR_USE_UNSTABLE',
	'-D_DEFAULT_SOURCE',
	'-D_POSIX_C_SOURCE=200112L',
	'-DPACKAGE_NAME="' + meson.project_name() + '"',
	'-DPACKAGE_VERSION="' + meson.project_version() + '"',
	language: 'c',
)

cc = meson.get_compiler('c')

# Adding include directory
inc = include_directories('include')

#libevdev = dependency('libevdev')
#libinput = dependency('libinput')
#libxml2 = dependency('libxml-2.0')

wayland_protos=dependency('wayland-protocols')
wayland_server = dependency('wayland-server')
pixman         = dependency('pixman-1')
xkbcommon = dependency('xkbcommon')
wlroots        = dependency('wlroots')

subdir('protocols')
subdir('blake')

