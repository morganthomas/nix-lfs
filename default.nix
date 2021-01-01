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
    # Note: LFS specifies that /usr/bin/awk should be a symlink to gawk and /usr/bin/yacc should be a symlink to bison, but we are not doing that
    echo $(which bash)
    echo $(which sh)
    echo $(which awk)
    echo $(which yacc)
    
  '';
  installPhase = ''
    mkdir -p $out
    echo install phase >$out/hello.txt
  '';
}
