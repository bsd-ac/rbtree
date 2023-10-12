test_sources = [
        'test_regress',
]

foreach ts : test_sources
	t_2ptr     = executable('native-2ptr-' + ts, ts + '.c', c_args : ['-DRB_SMALL'], include_directories : incdir)
	t_3ptr     = executable('native-3ptr-' + ts, ts + '.c', include_directories : incdir)
	t_2ptr_aug = executable('native-2ptr-augment-' + ts, ts + '.c', c_args : ['-DRB_SMALL', '-DDOAUGMENT'], include_directories : incdir)
	t_3ptr_aug = executable('native-3ptr-augment-' + ts, ts + '.c', c_args : ['-DDOAUGMENT'], include_directories : incdir)
	t_fbsd     = executable('freebsd-' + ts, ts + '.c', include_directories : freebsd)
	t_fbsd_aug = executable('freebsd-augment-' + ts, ts + '.c', c_args : ['-DDOAUGMENT'], include_directories : freebsd)
	t_obsd     = executable('openbsd-' + ts, ts + '.c', include_directories : openbsd)
	test('native-2ptr-' + ts, t_2ptr)
	test('native-3ptr-' + ts, t_3ptr)
	test('native-2ptr-augment-' + ts, t_2ptr_aug)
	test('native-3ptr-augment-' + ts, t_3ptr_aug)
	benchmark('freebsd-' + ts, t_fbsd)
	benchmark('freebsd-augment-' + ts, t_fbsd_aug)
	benchmark('openbsd-' + ts, t_obsd)
	benchmark('native-2ptr-' + ts, t_2ptr)
	benchmark('native-3ptr-' + ts, t_3ptr)
	benchmark('native-2ptr-augment-' + ts, t_2ptr_aug)
	benchmark('native-3ptr-augment-' + ts, t_3ptr_aug)
endforeach

test_subr = executable('test_subr', ['test_subr.c', 'subr_tree.c'], include_directories : incdir)
test('test_subr', test_subr)
