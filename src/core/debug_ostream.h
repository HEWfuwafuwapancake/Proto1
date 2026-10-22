/*==============================================================================
   debug ostreamを作って使おう [debug_ostream.h]
                                                         Author : Youhei Sato
                                                         Date   : 2025/4/21
    Shift_JISのみ対応
==============================================================================*/
#pragma once
#include <Windows.h>
#include <sstream>

/// @file debug_ostream.h
/// @brief Visual Studio の出力ウィンドウに文字を出すストリーム（授業のひな形）
/// @details 使い方：`hal::dout << "文字" << std::endl;`

namespace hal
{
    /// @brief ためた文字を OutputDebugStringA で出力ウィンドウに送るバッファ
    class debugbuf : public std::basic_stringbuf<char, std::char_traits<char>>
    {
    public:
        /// @brief 残っている文字を出力してから破棄する
        virtual ~debugbuf() {
            sync();
        }
    protected:
        /// @brief ためた文字を出力ウィンドウに送り、バッファを空にする
        /// @return 常に 0
        int sync() {
            OutputDebugStringA(str().c_str());
            str(std::basic_string<char>());
            return 0;
        }
    };
    /// @brief 出力ウィンドウに書き込む ostream（debugbuf を使う）
    class debug_ostream : public std::basic_ostream<char, std::char_traits<char>>
    {
    public:
        /// @brief debugbuf を作ってストリームに設定する
        debug_ostream() : std::basic_ostream<char, std::char_traits<char>>(new
            debugbuf()) {}
        /// @brief 作った debugbuf を解放する
        ~debug_ostream() { delete rdbuf(); }
    };
    /// @brief 出力ウィンドウ用のストリーム（`hal::dout << ...` で使う）
    extern debug_ostream dout;
}
