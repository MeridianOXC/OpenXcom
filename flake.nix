{
  inputs = {
    systems.url = "github:nix-systems/default";
    nixpkgs.url = "github:nixos/nixpkgs/nixpkgs-unstable";
  };

  outputs =
    {
      self,
      nixpkgs,
      systems,
    }:
    let
      forEachSystem =
        f: nixpkgs.lib.genAttrs (import systems) (system: f { pkgs = import nixpkgs { inherit system; }; });
    in
    {
      devShells = forEachSystem (
        { pkgs }:
        {
          default =
            pkgs.mkShell.override
              {
                # override clang 16 runtime
                stdenv = pkgs.lowPrio pkgs.llvmPackages_16.stdenv;
              }
              {
                propagatedBuildInputs = [
                  pkgs.SDL_compat
                  pkgs.SDL_image
                  pkgs.libwebp
                ];

                buildInputs = with pkgs; [
                  git
                  pkg-config
                  apple-sdk_15

                  # Need to use brew cmake since nix use old one 3.30.5
                  # which has issues with copy libraries to macOs app
                  cmake
                  # use clang explicitly since don't use cmake
                  clang_16
                  # OpenXcom dependecies
                  rapidyaml
                  zlib
                  #SDL_mixer
                  # SDL_image
                  # SDL_gfx
                  # Try to build using SDL12_compat
                  (SDL_compat.overrideAttrs (old: {
                    postInstall = ''
                      ln -s $out/lib/pkgconfig/sdl12_compat.pc $out/lib/pkgconfig/sdl.pc
                    '';
                  }))
                  (SDL_mixer.override (old: {
                    SDL = SDL_compat;
                    smpeg = old.smpeg.override {
                      SDL = SDL_compat;
                    };
                  }))
                  (SDL_gfx.override (old: {
                    SDL = SDL_compat;
                  }))
                  (
                    (SDL_image.override (old: {
                      SDL = SDL_compat;
                    })).overrideAttrs
                    (old: {
                      propagatedBuildInputs = [ SDL_compat ];
                      src = pkgs.fetchurl {
                        url = "https://github.com/libsdl-org/SDL_image/archive/refs/heads/SDL-1.2.tar.gz";
                        hash = "sha256-ytt87YL2zrvLun7bi5Jd4m5N7/UR09Y3I1kRDAqf6nQ=";
                      };
                      patches = [ ];
                    })
                  )
                ];
              };
        }
      );
    };
}
