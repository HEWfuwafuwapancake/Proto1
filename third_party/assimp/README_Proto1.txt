Assimp 5.2.4（Open Asset Import Library。https://github.com/assimp/assimp）のビルド済みファイル。授業で配られたもの（Visual Studio 2022 / x64 / マルチスレッド DLL 版）をそのまま入れた。
FBX などのモデルを読む（src/render/model.cpp。.obj は自作の読み込みのまま）。中身は書き換えない。

- include/assimp/ … ヘッダー（ライセンス（3条項 BSD）の文は各ヘッダーの先頭にある）
- lib/x64/assimp-vc143-mt.lib … リンク用（Proto1.vcxproj の x64 の設定で使う。Git LFS）
- bin/x64/assimp-vc143-mt.dll … 実行に要る DLL。ビルドすると exe の隣（x64/Debug・x64/Release）にコピーされる（Git LFS）。
  exe をほかの PC に渡すときは、この DLL も一緒に渡す。

Win32（x86）用のファイルはないので、ゲームは x64 でだけビルドできる。
版を上げるときは、同じ3つ（ヘッダー・lib・dll）を丸ごと置き換え、この文の版の番号も直す。
