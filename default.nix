# This assumes x86_64
with (import <nixpkgs> {});
stdenv.mkDerivation {
  name = "lfs";
  src = ./src;
  buildInputs = [
    bash binutils bison bzip2 coreutils findutils gawk gcc glibc gnugrep gzip m4 gnumake patch perl python gnused gnutar texinfo xz which
  ];
  configurePhase = ''
  '';
  buildPhase = ''
  '';
  installPhase = ''
    # LFS specifies that /usr/bin/awk should be a symlink to gawk and /usr/bin/yacc should be a symlink to bison, but we are not doing that
    echo $(which bash)
    echo $(which sh)
    echo $(which awk)
    echo $(which yacc)
    set +h
    umask 022
    LFS=$out
    LC_ALL=POSIX
    echo $(uname -m)
    LFS_TGT=$(uname -m)-lfs-linux-gnu
    # LFS specifies that PATH should be /bin:/usr/bin but we are not doing that
    PATH=$LFS/tools/bin:$PATH
    echo PATH=$PATH
    export LFS LC_ALL LFS_TGT PATH
    mkdir -p $LFS/tools/bin

    #
    # Build cross toolchain
    #

    cd binutils-2.35
    mkdir build
    cd build
    ../configure --prefix $LFS/tools \
                 --with-sysroot=$LFS \
                 --target=$LFS_TGT \
                 --disable-nls \
                 --disable-werror
    make
    # make -k check
    make install
    cd ../..

    cd gcc-10.2.0
    mv ../mpfr-4.1.0 mpfr
    mv ../gmp-6.2.0 gmp
    mv ../mpc-1.1.0 mpc
    sed -e '/m64=/s/lib64/lib/' \
        -i.orig gcc/config/i386/t-linux64
    mkdir build
    cd build
    CXXFLAGS=-Wno-error=format-security ../configure \
        --target=$LFS_TGT                              \
        --prefix=$LFS/tools                            \
        --with-glibc-version=2.11                      \
        --with-sysroot=$LFS                            \
        --with-newlib                                  \
        --without-headers                              \
        --enable-initfini-array                        \
        --disable-nls                                  \
        --disable-shared                               \
        --disable-multilib                             \
        --disable-decimal-float                        \
        --disable-threads                              \
        --disable-libatomic                            \
        --disable-libgomp                              \
        --disable-libquadmath                          \
        --disable-libssp                               \
        --disable-libvtv                               \
        --disable-libstdcxx                            \
        --enable-languages=c,c++
    make
    make install
    cd ..
    cat gcc/limitx.h gcc/glimits.h gcc/limity.h > \
        `dirname $($LFS_TGT-gcc -print-libgcc-file-name)`/install-tools/include/limits.h
  '';
}
