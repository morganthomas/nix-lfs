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

    cd bash-5.0
    ./configure --prefix=$out
    make
    make install
  '';
}
