from waflib.Configure import conf

@conf
def check_ssl(conf):
    if not conf.check_cfg(package='openssl', args=['--cflags', '--libs'], uselib_store='SSL', mandatory=False):
        libcrypto = conf.check_cc(lib='crypto',
                                  header_name='openssl/crypto.h',
                                  define_name='HAVE_SSL',
                                  uselib_store='SSL')
    else:
        conf.define("HAVE_SSL", 1)
    if not conf.get_define ("HAVE_SSL"):
        conf.fatal("Cannot find SSL libraries")

