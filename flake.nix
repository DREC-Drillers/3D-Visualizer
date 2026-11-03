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
        ] ++ qualityTools;
        buildInputs = [ pkgs.glfw pkgs.glm ];
        GLAD_SOURCE_DIR = "${gladSources}";
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
