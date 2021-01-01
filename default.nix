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
    make install
    cd ../..
  '';
}
