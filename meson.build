project('rbtree', 'c',
	version: '0.0.1',
	license: 'ISC',
	default_options: [
		'warning_level=1',
	]
)

c       = meson.get_compiler('c')

add_global_arguments(['-Wno-unused-function'],
                     language: 'c')

incdir = include_directories('include')

subdir('include')
subdir('tests')
