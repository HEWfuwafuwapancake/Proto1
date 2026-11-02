# Switch 2 Pro 初期化シーケンスの出典

`src/input/padcalc.cpp` の `PadCalcBuildSwitch2UsbCommand` は、
[SDL の SDL_hidapi_switch2.c](https://github.com/libsdl-org/SDL/blob/main/src/joystick/hidapi/SDL_hidapi_switch2.c)
の USB 初期化シーケンスを使っています（2026-11-02 参照）。
Windows に依存しないコマンド生成関数に変更し、転送は独自の WinUSB 処理にしました。
ボタンとスティックの配置も同じファイルを参照しています。SDL 自体のリンクはしません。

以下は参照元のライセンス表示です。

```text
Simple DirectMedia Layer
Copyright (C) 1997-2026 Sam Lantinga <slouken@libsdl.org>

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.

Code and logic contributed by Valve Corporation under the SDL zlib license.
```
