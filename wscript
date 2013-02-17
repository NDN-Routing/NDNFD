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
    flags = ['-Wall', '-Werror', '-Wpointer-arith']
    conf.env.append_unique('CFLAGS', flags + ['-Wstrict-prototypes', '-std=c99'])
    conf.env.append_unique('CXXFLAGS', flags + ['-fno-exceptions', '-std=c++0x'])

    if conf.options.optimize:
        conf.env.append_unique('CFLAGS', ['-O3', '-g1'])
        conf.env.append_unique('CXXFLAGS', ['-O3', '-g1'])
    else:
        conf.env.append_unique('CFLAGS', ['-O0', '-g3'])
        conf.env.append_unique('CXXFLAGS', ['-O0', '-g3'])

    if conf.options.gtest:
        conf.env.GTEST = 1
        if not conf.env['LIB_PTHREAD']:
            conf.check_cxx(lib='pthread')


def build(bld):
    source_subdirs = ['core','util','face','message']
    bld.objects(target='common',
        source=bld.path.ant_glob([subdir+'/*.cc' for subdir in source_subdirs], excl=['**/*_test*.cc']),
        includes='.',
        export_includes='.',
        use='CCNX SSL',
        )
    #bld.program(target='ndnfd',
    #    source='tools/ndnfd.cc',
    #    includes='.',
    #    use='common',
    #    )
    
    if bld.env.GTEST:
        try:
            bld.get_tgen_by_name('pthread')
        except:
            bld.read_shlib('pthread', paths=['/usr/lib/i386-linux-gnu', '/usr/lib/x86_64-linux-gnu'])
        bld.stlib(target='gtest',
            source=['gtest/gtest.cc', 'gtest/gtest_main.cc'],
            includes='. gtest',
            use='pthread',
            )
        bld.program(target='unittest',
            source=bld.path.ant_glob([subdir+'/*_test*.cc' for subdir in source_subdirs]),
            use='common gtest',
            install_path=None,
            )


