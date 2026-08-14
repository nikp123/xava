{
  description = "X11 Audio Visualizer for ALSA";

  outputs = {
    self,
    nixpkgs,
  }: let
    systems = ["x86_64-linux" "aarch64-linux"];
    # Need to validate before pushing "x86_64-darwin" "aarch64-darwin"

    forAllSystems = f: nixpkgs.lib.genAttrs systems (system: f system);

    runtime_libraries = pkgs: with pkgs; [
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
      wayland-scanner
      wayland-utils
      xorg.libX11
      xorg.libXdmcp
      xorg.libXfixes
      xorg.libXrandr

      # Graphics API
      cairo
      glew
      libGL

      # Mesa ships the actual EGL/OpenGL drivers (libEGL_mesa etc.) plus the
      # EGL vendor descriptor that libglvnd needs to find a working driver.
      # Bundling it keeps the runtime self-consistent regardless of the
      # host's Mesa/NVIDIA setup (and host glibc differs from flake glibc).
      mesa

      # Misc
      curl
      dbus
      expat # Might be a nixpkgs bug idfk
      taglib
      zlib
    ];

    nixpkgsFor = forAllSystems (system:
      import nixpkgs {
        inherit system;
        overlays = [self.overlay];
      });
  in rec {
    overlay = final: prev: let
      libPath = final.lib.makeLibraryPath (runtime_libraries final);
    in {
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
          clang
          cmake
          git # Not sure if it's needed
          pkg-config

          # For building icons
          imagemagick
          librsvg

          # For debugging
          clang-tools

          # Patching
          makeWrapper
        ];

        buildInputs = runtime_libraries final;

        #outputs = [ "out" "lib" ];

        postInstall = ''
          wrapProgram "$out/bin/xava" \
            --prefix LD_LIBRARY_PATH : "${libPath}" \
            --set __EGL_VENDOR_LIBRARY_DIRS "${final.mesa}/share/glvnd/egl_vendor.d" \
            --set LIBGL_DRIVERS_PATH "${final.mesa}/lib/dri"
        '';

        CARGO_FEATURE_USE_SYSTEM_LIBS = "1";
      };
    };

    packages =
      forAllSystems (system: {inherit (nixpkgsFor.${system}) xava;});

    defaultPackage = forAllSystems (system: self.packages.${system}.xava);

    apps = forAllSystems (system: {
      xava = {
        type = "app";
        program = "${self.packages.${system}.xava}/bin/xava";
      };
    });

    defaultApp = forAllSystems (system: self.apps.${system}.xava);

    devShell = forAllSystems (system: nixpkgs.legacyPackages.${system}.mkShell {
      inputsFrom = builtins.attrValues (packages.${system});

      # Make EGL find the bundled Mesa driver when running the binary straight
      # out of build/, instead of the flake's libglvnd shadowing the host GL.
      # (mkShell already puts buildInputs on LD_LIBRARY_PATH.)
      shellHook = ''
        export __EGL_VENDOR_LIBRARY_DIRS="${nixpkgsFor.${system}.mesa}/share/glvnd/egl_vendor.d"
        export LIBGL_DRIVERS_PATH="${nixpkgsFor.${system}.mesa}/lib/dri"
      '';
    });
  };
}
