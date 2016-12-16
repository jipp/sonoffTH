Import("env")
import os
from stat import *

def before_upload(source, target, env):
    print "before_upload"
    # do some actions


def after_upload(source, target, env):
    print "after_upload"
    # do some actions


def after_bin(source, target, env):
    st = os.stat(source)
    print "after_bin"
    print "file size: ", st[ST_SIZE]
    # do some actions


print "Current build targets", map(str, BUILD_TARGETS)


env.AddPostAction("$BUILD_DIR/firmware.bin", after_bin)
env.AddPreAction("upload", before_upload)
env.AddPostAction("upload", after_upload)
