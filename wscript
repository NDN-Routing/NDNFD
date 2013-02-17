VERSION='0.1'
APPNAME='NDNFD'


def options(opt):
    opt.load('compiler_c compiler_cxx')
    opt.load('ssl ccnx', tooldir='.')

    opt.add_option('--optimize',action='store_true',default=False,dest='optimize',help='''optimize object code''')
    opt.add_option('--gtest', action='store_true',default=False,dest='gtest',help='''build unit tests''')


def configure(conf):
    conf.load('compiler_c compiler_cxx')
    conf.load('ssl ccnx', tooldir='.')

    conf.check_ssl()
    conf.check_ccnx(path=conf.options.ccnx_dir)

    conf.define('_BSD_SOURCE', 1)
    conf.env.append_unique('CFLAGS', ['-Wall', '-Werror', '-Wpointer-arith', '-Wstrict-prototypes', '-std=c99'])
    conf.env.append_unique('CXXFLAGS', ['-Wall', '-Werror', '-Wpointer-arith', '-fno-exceptions', '-std=c++0x'])

    if conf.options.optimize:
        conf.env.append_unique('CFLAGS', ['-O3', '-g1'])
        conf.env.append_unique('CXXFLAGS', ['-O3', '-g1'])
    else:
        conf.env.append_unique('CFLAGS', ['-O0', '-g3'])
        conf.env.append_unique('CXXFLAGS', ['-O0', '-g3'])

    if conf.options.gtest:
        conf.env.TEST = 1


def build(bld):
    source_subdirs = ['core','util','face','message']
    bld.objects(target='common',
        source=bld.path.ant_glob([subdir+'/*.cc' for subdir in source_subdirs], excl=['**/*_test*.cc']),
        includes='.',
        use='CCNX SSL',
        )
    #bld.program(target='ndnfd',
    #    source='tools/ndnfd.cc',
    #    includes='.',
    #    use='common',
    #    )
    
    if bld.env.TEST:
        bld.program(target='unittest',
            source=['gtest/gtest.cc', 'gtest/gtest_main.cc'] + bld.path.ant_glob([subdir+'/*_test*.cc' for subdir in source_subdirs]),
            includes='. gtest',
            use='common',
            install_path=None,
            )


