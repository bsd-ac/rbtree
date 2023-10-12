project('rbtree', 'c',
	version: '0.0.1',
	license: 'ISC',
	default_options: [
		'warning_level=1',
	]
)

c       = meson.get_compiler('c')

add_global_arguments(['-Wno-unused-function', '-Wno-format'],
                     language: 'c')

incdir = include_directories('include')

freebsd = include_directories('freebsd')
openbsd = include_directories('openbsd')

subdir('include')
subdir('tests')
