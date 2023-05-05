{
  description = "Port of OpenAI's Whisper model in C/C++";

  inputs = {
    flake-utils.url = "github:numtide/flake-utils";
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-22.11";
  };

  outputs = { self, flake-utils, nixpkgs }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs { inherit system; };
      in
      {
        formatter = pkgs.nixpkgs-fmt;
        packages.default = with pkgs; stdenv.mkDerivation rec {
          name = "whisper-cpp";
          src = ./.;
          nativeBuildInputs = [ makeWrapper ];
          buildInputs = [ SDL2 ] ++ lib.optionals stdenv.isDarwin [ Accelerate CoreGraphics CoreVideo ];

          makeFlags = [ "main" "stream" ];

          installPhase = ''
            runHook preInstall

            mkdir -p $out/bin
            cp ./main $out/bin/whisper-cpp
            cp ./stream $out/bin/whisper-cpp-stream

            cp models/download-ggml-model.sh $out/bin/whisper-cpp-download-ggml-model

            wrapProgram $out/bin/whisper-cpp-download-ggml-model \
              --prefix PATH : ${lib.makeBinPath [wget]}

            runHook postInstall
          '';

          meta = with lib; {
            description = "Port of OpenAI's Whisper model in C/C++";
            longDescription = ''
              To download the models as described in the project's readme, you may
              use the `whisper-cpp-download-ggml-model` binary from this package.
            '';
            homepage = "https://github.com/ggerganov/whisper.cpp";
            license = licenses.mit;
            platforms = platforms.all;
          };
        };
        apps.default = {
          type = "app";
          program = "${self.packages.${system}.default}/bin/whisper-cpp";
        };
        devShells.default = pkgs.mkShell {
          packages = with pkgs; [ cmake ] ++ lib.optionals stdenv.isDarwin [
            darwin.apple_sdk.frameworks.Accelerate
          ];
        };
      });
}
