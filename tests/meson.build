test_sources = [
        'test_regress',
]

foreach ts : test_sources
	t_2ptr = executable('native-2ptr-' + ts, ts + '.c', c_args : ['-DRB_SMALL'], include_directories : incdir)
	t_3ptr = executable('native-3ptr-' + ts, ts + '.c', include_directories : incdir)
	t_fbsd = executable('freebsd-' + ts, ts + '.c', include_directories : freebsd)
	t_obsd = executable('openbsd-' + ts, ts + '.c', include_directories : openbsd)
	test('native-2ptr-' + ts, t_2ptr)
	test('native-3ptr-' + ts, t_3ptr)
	benchmark('freebsd-' + ts, t_fbsd)
	benchmark('openbsd-' + ts, t_obsd)
	benchmark('native-2ptr-' + ts, t_2ptr)
	benchmark('native-3ptr-' + ts, t_3ptr)
endforeach
