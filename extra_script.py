Import("env")

from shutil import copyfile
import os


def after_bin(target, source, env):
    dir = "bin\\"
    src = str(source[0])
    dst = env.get('CPPDEFINES')[9][1].strip('\\"') + ".bin"

    if not os.path.exists(dir):
        os.makedirs(dir)

    print "src: " + src
    print "dst: " + dir + dst
    copyfile(src, dir + dst)


print "Current build targets", map(str, BUILD_TARGETS)


env.AddPostAction("$BUILD_DIR/firmware.bin", after_bin)
