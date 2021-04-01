// Copyright 2021 The Tapirus-Team Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "universal.inl"
#include <io.h>
#include <fcntl.h>


namespace base::console
{
    void SetConsoleUTF8()
    {
        SetConsoleCP(65001);
        SetConsoleOutputCP(65001);

        if (auto Handle = GetStdHandle(STD_OUTPUT_HANDLE))
        {
            CONSOLE_FONT_INFOEX Font{ sizeof(CONSOLE_FONT_INFOEX) };

            if (GetCurrentConsoleFontEx(Handle, FALSE, &Font))
            {
                Font.FontWeight = FW_NORMAL;
                Font.FontFamily = FF_DONTCARE;
                wcscpy_s(Font.FaceName, L"Lucida Console");

                SetCurrentConsoleFontEx(Handle, FALSE, &Font);
            }
        }
    }

    void RedirectIOToConsole(_In_ short MaxConsoleLines)
    {
        int hConHandle = 0;
        intptr_t lStdHandle = 0;

        FILE* fp = nullptr;
        CONSOLE_SCREEN_BUFFER_INFO coninfo{};

        // allocate a console for this app

        AllocConsole();

        // set the screen buffer to be big enough to let us scroll text

        GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &coninfo);

        coninfo.dwSize.Y = MaxConsoleLines;

        SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), coninfo.dwSize);

        // redirect unbuffered STDOUT to the console

        lStdHandle = (intptr_t)GetStdHandle(STD_OUTPUT_HANDLE);

        if (lStdHandle == -1 /*INVALID_HANDLE_VALUE*/)
        {
            return;
        }

        hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);

        fp = _fdopen(hConHandle, "w");

        *stdout = *fp;

        setvbuf(stdout, NULL, _IONBF, 0);

        // redirect unbuffered STDIN to the console

        lStdHandle = (intptr_t)GetStdHandle(STD_INPUT_HANDLE);

        hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);

        fp = _fdopen(hConHandle, "r");

        *stdin = *fp;

        setvbuf(stdin, NULL, _IONBF, 0);

        // redirect unbuffered STDERR to the console

        lStdHandle = (intptr_t)GetStdHandle(STD_ERROR_HANDLE);

        hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);

        fp = _fdopen(hConHandle, "w");

        *stderr = *fp;

        setvbuf(stderr, NULL, _IONBF, 0);

        // make cout, wcout, cin, wcin, wcerr, cerr, wclog and clog

        freopen("CON", "w", stdout);
        freopen("CON", "r", stdin);
        freopen("CON", "w", stderr);

        // point to console as well

        std::ios::sync_with_stdio();
    }

}