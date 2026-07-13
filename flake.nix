{
  description = "A Nix-flake-based development environment";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-25.05";
  };

  outputs = { self , nixpkgs ,... }: let
    # system should match the system you are running on
    system = "x86_64-linux";
  in {
    devShells."${system}".default = let
      pkgs = import nixpkgs {
        inherit system;
        config.allowUnfree = true;
      };

      # flip-evaluator is not packaged in nixpkgs; install the manylinux wheel
      # from PyPI and patch its ELF interpreter for NixOS.
      flip-evaluator = pkgs.python3.pkgs.buildPythonPackage {
        pname = "flip_evaluator";
        version = "1.7";
        format = "wheel";
        src = pkgs.fetchurl {
          url = "https://files.pythonhosted.org/packages/ef/13/f6cd04b63d30bed8befeec3c5f424f698d80f71a530f6fc6f86e7aca0fe6/flip_evaluator-1.7-cp312-cp312-manylinux_2_17_x86_64.manylinux2014_x86_64.whl";
          sha256 = "afd53e4ea606e6b6518e4cbbdce416a1b3b018bcb4c370bed0328a7f65f1308f";
        };
        nativeBuildInputs = [ pkgs.autoPatchelfHook ];
        buildInputs = [ pkgs.stdenv.cc.cc.lib ];
        propagatedBuildInputs = with pkgs.python3.pkgs; [
          numpy
          matplotlib
          pillow
        ];
      };
    in pkgs.mkShell {
      packages = with pkgs; [
        meson
        cmake
        ninja
        glslang # or shaderc
        vulkan-headers
        vulkan-loader
        vulkan-tools
        vulkan-validation-layers
        pkg-config
        doxygen
        python3
        (pkgs.python3.withPackages (python-pkgs: with python-pkgs; [
            # select Python packages here
            pillow
            numpy
            pyyaml
            gurobipy
            scipy
            matplotlib
            flip-evaluator
        ]))
      ];

      buildInputs = with pkgs; [
        glfw
        glm
        spdlog
        # imgui # no backend vulkan implementation in the nixpkgs version, let it build over meson
        stb
        assimp
        yaml-cpp
        openvdb
        tbb

        R
        rPackages.tidyverse
        rPackages.scales
        rPackages.ggplot2

      ];

      shellHook = ''
        echo "Entered env"
      '';
    };
  };
}
