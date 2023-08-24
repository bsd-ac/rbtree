test_sources = [
        'test_2ptr',
]

foreach ts : test_sources
        t1 = executable('native-' + ts, ts + '.c', include_directories : incdir)
        t2 = executable('freebsd-' + ts, ts + '.c', include_directories : freebsd)
        test(ts, t1)
        benchmark('native-' + ts, t1)
        benchmark('freebsd-' + ts, t2)
endforeach