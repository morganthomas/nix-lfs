let bootstrap2 = import ./bootstrap2.nix; in
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
    tar xzf ${bootstrap2.outPath}/result.tar.gz -C $LFS
    mv $LFS/${bootstrap2.outPath}/* $LFS

    #
    # Build cross toolchain part 3
    #

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
