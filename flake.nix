{
  description = "Standalone C++20 OpenGL visualizer";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-26.05";
    nixGL = {
      url = "github:nix-community/nixGL";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs = { self, nixpkgs, nixGL, ... }:
    let
      system = "x86_64-linux";
      pkgs = import nixpkgs { inherit system; };
      llvm = pkgs.llvmPackages_18;
      gladSources = pkgs.runCommand "glad-opengl-3.3" {
        nativeBuildInputs = [ pkgs.python3Packages.glad2 ];
      } ''
        glad --api gl:core=3.3 --extensions "" --reproducible --out-path "$out" c
      '';
      graphicsRunner = pkgs.writeShellScriptBin "visualizer-test-opengl" ''
        unset WAYLAND_DISPLAY
        export XDG_SESSION_TYPE=x11
        export LIBGL_ALWAYS_SOFTWARE=1
        export MESA_SHADER_CACHE_DISABLE=true
        if [ "$#" -eq 0 ]; then
          set -- ctest --preset opengl
        fi
        exec ${nixGL.packages.${system}.nixGLIntel}/bin/nixGLIntel \
          ${pkgs.xvfb-run}/bin/xvfb-run -a -s "-screen 0 640x480x24" "$@"
      '';
      qualityTools = map (tool: pkgs.writeShellScriptBin "${tool}-18" ''
        exec ${llvm.clang-tools}/bin/${tool} "$@"
      '') [ "clang-format" "clang-tidy" ];
    in {
      devShells.${system}.default = (pkgs.mkShell.override {
        stdenv = llvm.stdenv;
      }) {
        packages = [
          pkgs.cmake pkgs.ninja pkgs.git llvm.clang-tools
          (pkgs.python3.withPackages (packages: [ packages.pyyaml ]))
          nixGL.packages.${system}.nixGLIntel
        ] ++ qualityTools ++ [ graphicsRunner ];
        buildInputs = [ pkgs.glfw pkgs.glm pkgs.gtest ];
        GLAD_SOURCE_DIR = "${gladSources}";
      };

      checks.${system} = {
        unitTests = llvm.stdenv.mkDerivation {
          pname = "3d-visualizer-tests";
          version = "0.1.0";
          src = self;
          nativeBuildInputs = [ pkgs.cmake pkgs.ninja ];
          buildInputs = [ pkgs.glfw pkgs.glm pkgs.gtest ];
          cmakeFlags = [
            "-DGLAD_SOURCE_DIR=${gladSources}"
            "-DVISUALIZER_BUILD_TESTS=ON"
            "-DVISUALIZER_BUILD_EXAMPLES=OFF"
          ];
          doCheck = true;
          checkPhase = "ctest --output-on-failure --no-tests=error";
          installPhase = "touch $out";
        };

        openGL = llvm.stdenv.mkDerivation {
          pname = "3d-visualizer-opengl-tests";
          version = "0.1.0";
          src = self;
          nativeBuildInputs = [ pkgs.cmake pkgs.ninja graphicsRunner ];
          buildInputs = [ pkgs.glfw pkgs.glm pkgs.gtest ];
          cmakeFlags = [
            "-DGLAD_SOURCE_DIR=${gladSources}"
            "-DVISUALIZER_BUILD_TESTS=OFF"
            "-DVISUALIZER_BUILD_OPENGL_TESTS=ON"
            "-DVISUALIZER_BUILD_EXAMPLES=OFF"
          ];
          doCheck = true;
          checkPhase = "visualizer-test-opengl ctest --output-on-failure -L opengl --no-tests=error";
          installPhase = "touch $out";
        };
      };

      packages.${system}.default = llvm.stdenv.mkDerivation {
        pname = "3d-visualizer";
        version = "0.1.0";
        src = self;
        nativeBuildInputs = [ pkgs.cmake pkgs.ninja ];
        propagatedBuildInputs = [ pkgs.glfw pkgs.glm ];
        cmakeFlags = [
          "-DGLAD_SOURCE_DIR=${gladSources}"
          "-DVISUALIZER_BUILD_TESTS=OFF"
          "-DVISUALIZER_BUILD_EXAMPLES=OFF"
        ];
      };
    };
}
