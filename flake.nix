{
  description = "A basic flake with a shell";
  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
  inputs.systems.url = "github:nix-systems/default";
  inputs.netmonpkgs.url = "github:jaroslavpesek/netmonpkgs";
  inputs.flake-utils = {
    url = "github:numtide/flake-utils";
    inputs.systems.follows = "systems";
  };

  outputs =
    { nixpkgs, flake-utils, netmonpkgs, ... }:
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
      in
      {
        devShells.default = pkgs.mkShell {
          packages = [
            pkgs.bashInteractive
            pkgs.nixd
            pkgs.nixpkgs-fmt
            pkgs.cmake
            pkgs.gcc
            pkgs.pkg-config
            pkgs.ncurses
            pkgs.fuse3
            pkgs.rpm
            pkgs.clang-tools
            pkgs.editorconfig-checker
            netmonpkgs.packages.x86_64-linux.nemea-modules-meta
            netmonpkgs.packages.x86_64-linux.nemea-framework
            netmonpkgs.packages.x86_64-linux.telemetry
          ];
        };
      }
    );
}
