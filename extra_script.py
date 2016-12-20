Import("env")

from shutil import copyfile
import os


def before_upload(source, target, env):
    print "before_upload"
    # do some actions


def after_upload(source, target, env):
    print "after_upload"
    # do some actions


def after_bin(target, source, env):
    dir = "bin\\"
    src = str(source[0])
    dst = env.get('CPPDEFINES')[9][1].strip('\\"') + ".bin"

    if not os.path.exists(dir):
        os.makedirs(dir)

    print "source: " + src
    print "target: " + dst
    copyfile(src, dir + dst)

    print "after_bin"
    # do some actions


print "Current build targets", map(str, BUILD_TARGETS)


env.AddPostAction("$BUILD_DIR/firmware.bin", after_bin)
env.AddPreAction("upload", before_upload)
env.AddPostAction("upload", after_upload)
