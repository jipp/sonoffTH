Import("env")

from shutil import copyfile
import os


def before_upload(source, target, env):
    print "before_upload"
    # do some actions


def after_upload(source, target, env):
    print "after_upload"
    # do some actions


def after_bin(source, target, env):
    # copyfile(source, "hello")
    # print source[0]
    # print str(source)
    # print str(env)
    # print os.getcwd()+"/bin"
    # print os.environ
    # print env.Dump()
    print env.get('PIOENV')
    print env.get('CPPDEFINES')[9][1]
    print "after_bin"
    # do some actions


print "Current build targets", map(str, BUILD_TARGETS)


env.AddPostAction("$BUILD_DIR/firmware.bin", after_bin)
env.AddPreAction("upload", before_upload)
env.AddPostAction("upload", after_upload)
