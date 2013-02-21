VERSION='0.1'
APPNAME='NDNFD'

import waflib


def options(opt):
    opt.load('compiler_c compiler_cxx')
    opt.load('ssl ccnx', tooldir='.')

    opt.add_option('--optimize',action='store_true',default=False,dest='optimize',help='''optimize object code''')
    opt.add_option('--gtest', action='store_true',default=False,dest='gtest',help='''build unit tests''')
    opt.add_option('--markdown', action='store_true',default=False,dest='markdown',help='''build Markdown into HTML''')


def configure(conf):
    conf.load('compiler_c compiler_cxx')
    conf.load('ssl ccnx', tooldir='.')

    conf.check_ssl()
    conf.check_ccnx(path=conf.options.ccnx_dir)

    conf.define('_BSD_SOURCE', 1)
    conf.define('_POSIX_SOURCE', 1)
    flags = ['-Wall', '-Werror', '-Wpointer-arith']
    conf.env.append_unique('CFLAGS', ['-Wall', '-Wpointer-arith', '-Wstrict-prototypes', '-std=c99'])#sadly, ccnd won't compile with -Werror
    conf.env.append_unique('CXXFLAGS', flags + ['-fno-exceptions', '-fno-rtti', '-std=c++0x'])
    conf.env.append_unique('LIBPATH', ['/usr/lib/i386-linux-gnu', '/usr/lib/x86_64-linux-gnu'])

    if conf.options.optimize:
        conf.env.append_unique('CFLAGS', ['-O3', '-g1'])
        conf.env.append_unique('CXXFLAGS', ['-O3', '-g1'])
    else:
        conf.env.append_unique('CFLAGS', ['-O0', '-g3'])
        conf.env.append_unique('CXXFLAGS', ['-O0', '-g3'])

    if conf.options.gtest:
        conf.env.GTEST = 1
        if not conf.env.LIB_PTHREAD:
            conf.check_cxx(lib='pthread')
    
    if conf.options.markdown:
        conf.env.MARKDOWN = 1
        conf.find_program('pandoc', var='PANDOC')


def build(bld):
    source_subdirs = ['core','util','face','message']
    bld.objects(target='ndnfdcommon',
        source=bld.path.ant_glob([subdir+'/*.cc' for subdir in source_subdirs], excl=['**/*_test*.cc']),
        includes='.',
        export_includes='.',
        use='CCNX SSL ccnd/ccndcore',
        )
        
    bld.objects(target='ccnd/ccndcore',
        source=['ccnd/ccnd.c','ccnd/ccnd_internal_client.c','ccnd/ccnd_stats.c','ccnd/ccnd_msg.c'],
        features='c cxxstlib',
        includes='.',
        use='CCNX SSL pthread',
        )
    
    bld.program(target='ndnfd',
        source=['ccnd/ccnd_main.c'],
        features='c cxxprogram',
        includes='.',
        use='ccnd/ccndcore ndnfdcommon',
        )
    
    if bld.env.GTEST:
        try:
            bld.get_tgen_by_name('pthread')
        except:
            bld.read_shlib('pthread', paths=bld.env.LIBPATH)
        bld.stlib(target='gtest/gtest',
            source=['gtest/gtest.cc', 'gtest/gtest_main.cc'],
            includes='. gtest',
            use='pthread',
            )
        bld.program(target='unittest',
            source=bld.path.ant_glob([subdir+'/*_test*.cc' for subdir in source_subdirs]),
            use='common ndnfdcommon gtest/gtest',
            install_path=None,
            )
    
    if bld.env.MARKDOWN:
        waflib.TaskGen.declare_chain(name='markdown2html',
            rule='${PANDOC} -f markdown -t html -o ${TGT} ${SRC}',
            shell=False,
            ext_in='.md',
            ext_out='.htm',
            reentrant=False,
            install_path=None,
            )
        bld(source=bld.path.ant_glob(['**/*.md']))


def check(ctx):
    unittest_node=ctx.root.find_node(waflib.Context.out_dir+'/unittest')
    if unittest_node is None:
        ctx.fatal('unittest is not built; configure with --gtest and build')
    else:
        import subprocess
        subprocess.call(unittest_node.abspath())


