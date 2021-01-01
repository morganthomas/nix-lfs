with (import <nixpkgs> {});
let bootstrap1 = import ./bootstrap1.nix; in
stdenv.mkDerivation {
  name = "lfs";
  src = ./src;
  buildInputs = [
    bash binutils bison bzip2 coreutils findutils gawk gcc gnugrep gzip m4 gnumake patch perl python3 gnused gnutar texinfo xz which
  ];
  configurePhase = ''
  '';
  buildPhase = ''
  '';
  installPhase = ''
    export LFS=$out
    source ./env.sh
    mkdir -p $LFS
    tar xzf ${bootstrap1.outPath}/result.tar.gz -C $LFS
    mv $LFS/${bootstrap1.outPath}/* $LFS

    #
    # Build cross toolchain part 2
    #

    echo "INSTALLING LINUX HEADERS"
    cd linux-5.8.3
    make mrproper
    make headers
    find usr/include -name '.*' -delete
    rm usr/include/Makefile
    cp -rv usr/include $LFS/usr
    cd ..

    echo "BUILDING GLIBC"
    cd glibc-2.32
    ln -sfv ../lib/ld-linux-x86-64.so.2 $LFS/lib64
    ln -sfv ../lib/ld-linux-x64-64.so.2 $LFS/lib64/ld-lsb-x86-64.so.3
    patch -Np1 -i ../glibc-2.32-fhs-1.patch
    mkdir build
    cd build
    ../configure                             \
          --prefix=/usr                      \
          --host=$LFS_TGT                    \
          --build=$(../scripts/config.guess) \
          --enable-kernel=3.2                \
          --with-headers=$LFS/usr/include    \
          libc_cv_slibdir=/lib
    make
    make DESTDIR=$LFS install
    cd ..

    echo "TESTING TOOLCHAIN"
    echo 'int main(){}' > dummy.c
    $LFS_TGT-gcc dummy.c
    readelf -l a.out | group '/ld-linux'

    echo "MKHEADERS"
    $LFS/tools/libexec/gcc/$LFS_TGT/10.2.0/install-tools/mkheaders

    echo "BUILDING LIBSTDC++"
    cd gcc-10.2.0
    mkdir build
    cd build
    ../libstdc++-v3/configure           \
        --host=$LFS_TGT                 \
        --build=$(../config.guess)      \
        --prefix=/usr                   \
        --disable-multilib              \
        --disable-nls                   \
        --disable-libstdcxx-pch         \
        --with-gxx-include-dir=/tools/$LFS_TGT/include/c++/10.2.0
    make
    make DESTDIR=$LFS install
    cd ..

    tar cvzf result.tar.gz $LFS
    rm -rf $LFS/*
    mv result.tar.gz $LFS
  '';
}
