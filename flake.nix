{
  description = "X11 Audio Visualizer for ALSA";

  outputs = { self, nixpkgs }:
    let
      systems =
        [ "x86_64-linux" "aarch64-linux" ];
	# Need to validate before pushing "x86_64-darwin" "aarch64-darwin" 

      forAllSystems = f: nixpkgs.lib.genAttrs systems (system: f system);

      nixpkgsFor = forAllSystems (system:
        import nixpkgs {
          inherit system;
          overlays = [ self.overlay ];
        });
    in rec {
      overlay = final: prev: {
        xava = final.stdenv.mkDerivation {
          name = "xava";
          version = "unstable";
          description = "X11 Audio Visualizer for ALSA";
         
          src = self;
         
          cmakeFlags = [
            (final.lib.cmakeBool "CMAKE_SKIP_BUILD_RPATH" true)
            "-DNIX_BUILDER=ON"
            "-DCMAKE_INSTALL_PREFIX=${placeholder "out"}"
          ];
         
          nativeBuildInputs = with final; [ 
            # Compiling and fetching dependencies
            gcc
            cmake
            pkg-config
            git # Not sure if it's needed
        
            # For building icons
            imagemagick
	    librsvg
          ];
        
          buildInputs = with final; [
            # Basic
            fftwFloat
            iniparser
        
            # Input
            alsa-lib
            libpulseaudio
            pipewire
            portaudio
            sndio
        
            # Output
            SDL2
            wayland
            wayland-protocols
            wayland-utils
            xorg.libX11
	    xorg.libXdmcp
	    xorg.libXfixes
            xorg.libXrandr
        
            # Graphics API
            cairo
            glew
        
            # Misc
            curl
	    dbus
            expat # Might be a nixpkgs bug idfk
            taglib
            zlib
          ];
        
          #outputs = [ "out" "lib" ];
        
          #postInstall = ''
          #  moveToOutput "lib" "$lib"
          #'';
        
          CARGO_FEATURE_USE_SYSTEM_LIBS = "1";
        };
      };

      packages =
        forAllSystems (system: { inherit (nixpkgsFor.${system}) xava; });

      defaultPackage = forAllSystems (system: self.packages.${system}.xava);

      apps = forAllSystems (system: {
        xava = {
          type = "app";
          program = "${self.packages.${system}.xava}/bin/xava";
        };
      });

      defaultApp = forAllSystems (system: self.apps.${system}.xava);

      devShell = forAllSystems (system:
        nixpkgs.legacyPackages.${system}.mkShell {
          inputsFrom = builtins.attrValues (packages.${system});
        });
    };
}
