with (import <nixpkgs> {});
let glibcTar = import ./glibc.nix; in
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
    tar xf ${glibcTar.outPath}/result.tar.gz
    mkdir -p $out
    mv ./${glibcTar.outPath}/${glibcTar.outPath}/* $out

    cd bash-5.0
    CFLAGS="-Xlinker -I${glibc.outPath}/lib/ld-linux-x86-64.so.2 -nodefaultlibs -nostdinc -I${glibc.outPath}/include --sysroot=${glibc.outPath} -L${glibc.outPath}/lib -lc" ./configure --prefix=$out --host=x86_64-pc-linux-gnu
    make
    make install
  '';
}
