manuals = [
        'rbtree.3'
]

install_man(manuals)

mandoc = find_program('mandoc', required : false)
if mandoc.found()
        foreach mp : manuals
                test('mandoc linting - ' + mp, mandoc, args : ['-T', 'lint', '-W', 'warning', mp])
        endforeach
endif
