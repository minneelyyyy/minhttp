let
  pkgs = import <nixpkgs> {};
in
  pkgs.mkShell {
    packages = with pkgs; [
      gcc
      gnumake
      gdb
    ];
  }
