test_sources = [
        'test_2ptr.c',
]

foreach ts : test_sources
        t = executable(ts, ts, include_directories : incdir)
        test(ts, t)
endforeach
