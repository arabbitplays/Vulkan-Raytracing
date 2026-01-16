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
      ];

      buildInputs = with pkgs; [
        glfw
        glm
        spdlog
        # imgui # no backend vulkan implementation in the nixpkgs version, let it build over meson
        stb
        assimp
        yaml-cpp
      ];

      shellHook = ''
        echo "Entered env"
      '';
    };
  };
}