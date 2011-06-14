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
    bld.program(source='sureelec_test.c', target='sureelec_test', use=APPNAME)
    bld.program(source='sureelec_test2.c', target='sureelec_test2', use=APPNAME, lib=['readline'])


