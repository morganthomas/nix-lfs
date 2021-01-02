with (import <nixpkgs> {});
stdenv.mkDerivation {
  name = "lfs";
  src = ./src;
  buildInputs = [
    gcc bison python3
  ];
  configurePhase = ''
  '';
  buildPhase = ''
  '';
  installPhase = ''
    cd glibc-2.32
    mkdir build
    cd build
    ../configure --prefix=$out
    make
    make DESTDIR=$out install
    cd ../..

    tar czvf result.tar.gz $out
    rm -rf $out/*
    mv result.tar.gz $out
  '';

}
