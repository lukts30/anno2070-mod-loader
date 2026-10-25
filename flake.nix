{
  description = "Anno 2070 modloader: MSVC-ABI Clang development tools";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/d2f67949798825fe853f7c5d0492b8bf016d3f88";

  outputs =
    { self, nixpkgs }:
    let
      systems = [ "x86_64-linux" ];
      eachSystem = nixpkgs.lib.genAttrs systems;
      packagesFor = system: import nixpkgs { inherit system; };
    in
    {
      devShells = eachSystem (
        system:
        let
          pkgs = packagesFor system;
        in
        {
          default = pkgs.mkShellNoCC {
            packages = with pkgs; [
              llvmPackages_21.clang-unwrapped
              llvmPackages_21.lld
              llvmPackages_21.llvm
              xwin
              cmake
              ninja
              python3
              git
              gh
              treefmt
              nixfmt
              actionlint
              ruff
            ];
            shellHook = ''
              export XWIN_ROOT="$PWD/.xwin"
            '';
          };
        }
      );
      formatter = eachSystem (system: (packagesFor system).nixfmt);
      checks = eachSystem (
        system:
        let
          pkgs = packagesFor system;
        in
        {
          tooling =
            pkgs.runCommand "anno2070-tooling-check"
              {
                nativeBuildInputs = with pkgs; [
                  actionlint
                  ruff
                ];
              }
              ''
                cd ${self}
                actionlint .github/workflows/*.yml
                ruff check --no-cache scripts
                ruff format --no-cache --check scripts
                touch $out
              '';
        }
      );
    };
}
