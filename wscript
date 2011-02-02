VERSION='0.0.1'
APPNAME='sureelec'

def options(opt):
    opt.tool_options('compiler_c')

def configure(conf):
    conf.check_tool('compiler_c')

def build(bld):
    bld.new_task_gen(
        features = 'c cshlib',
        source = ('libsureelec.c'),
        headers = ('libsureelec.h'),
        target=APPNAME)
    bld.install_files('${PREFIX}/include', 'libsureelec.h')

