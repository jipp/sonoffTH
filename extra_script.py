Import("env")

from shutil import copyfile
import os


def after_bin(source, target, env):
    print "source: " + str(source[0])
    print "target: " + str(target[0])

    path = "bin"
    srcFile = str(target[0])
    dstFile = os.path.join(path, env.get('CPPDEFINES')[9][1].strip('\\"') + ".bin")

    if not os.path.exists(path):
        os.makedirs(path)

    print "src: " + srcFile
    print "dst: " + dstFile
    copyfile(srcFile, dstFile)


print "Current build targets", map(str, BUILD_TARGETS)


env.AddPostAction("$BUILD_DIR/firmware.bin", after_bin)
