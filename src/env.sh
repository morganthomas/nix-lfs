#!/usr/bin/env sh
# LFS specifies that /usr/bin/awk should be a symlink to gawk and /usr/bin/yacc should be a symlink to bison, but we are not doing that
echo $(which bash)
echo $(which sh)
echo $(which awk)
echo $(which yacc)
set +h
umask 022
LC_ALL=POSIX
echo $(uname -m)
LFS_TGT=$(uname -m)-lfs-linux-gnu
# LFS specifies that PATH should be /bin:/usr/bin but we are not doing that
PATH=$LFS/tools/bin:$PATH
echo PATH=$PATH
export LFS LC_ALL LFS_TGT PATH
