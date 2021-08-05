// Copyright 2021 The Tapirus-Team Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "universal.inl"
#include <io.h>
#include <fcntl.h>
#include <wchar.h>


namespace base::console
{
    void SetConsoleCodePage(_In_opt_ uint32_t CodePage, _In_opt_ const char* FontName)
    {
        auto handle = GetStdHandle(STD_OUTPUT_HANDLE);
        if (handle == INVALID_HANDLE_VALUE)
        {
            return;
        }

        SetConsoleCP(CodePage);
        SetConsoleOutputCP(CodePage);

        if (FontName)
        {
            CONSOLE_FONT_INFOEX font = { sizeof(CONSOLE_FONT_INFOEX) };

            if (GetCurrentConsoleFontEx(handle, FALSE, &font))
            {
                auto state = std::mbstate_t();

                font.FontWeight = FW_NORMAL;
                font.FontFamily = FF_DONTCARE;
                mbsrtowcs_s(nullptr, font.FaceName, _countof(font.FaceName), &FontName, _countof(font.FaceName), &state);

                SetCurrentConsoleFontEx(handle, FALSE, &font);
            }
        }
    }

    // Reference: https://github.com/chromium/chromium/blob/master/base/process/launch_win.cc#L138
    void RedirectIOToConsole(_In_opt_ short MaxConsoleLines)
    {
        // Don't change anything if stdout or stderr already point to a
        // valid stream.
        //
        // If we are running under Buildbot or under Cygwin's default
        // terminal (mintty), stderr and stderr will be pipe handles.  In
        // that case, we don't want to open CONOUT$, because its output
        // likely does not go anywhere.
        //
        // We don't use GetStdHandle() to check stdout/stderr here because
        // it can return dangling IDs of handles that were never inherited
        // by this process.  These IDs could have been reused by the time
        // this function is called.  The CRT checks the validity of
        // stdout/stderr on startup (before the handle IDs can be reused).
        // _fileno(stdout) will return -2 (_NO_CONSOLE_FILENO) if stdout was
        // invalid.

        if (_fileno(stdout) >= 0 || _fileno(stderr) >= 0) {
            // _fileno was broken for SUBSYSTEM:WINDOWS from VS2010 to VS2012/2013.
            // http://crbug.com/358267. Confirm that the underlying HANDLE is valid
            // before aborting.

            intptr_t stdout_handle = _get_osfhandle(_fileno(stdout));
            intptr_t stderr_handle = _get_osfhandle(_fileno(stderr));
            if (stdout_handle >= 0 || stderr_handle >= 0)
            {
                return;
            }
        }

        // allocate a console for this app

        if (!AllocConsole())
        {
            return;
        }

        FILE* fp = nullptr;

        freopen_s(&fp, "CONIN$" , "r", stdin);
        freopen_s(&fp, "CONOUT$", "w", stdout);
        freopen_s(&fp, "CONOUT$", "w", stderr);

        setvbuf(stdin,  nullptr, _IONBF, 0);
        setvbuf(stdout, nullptr, _IONBF, 0);
        setvbuf(stderr, nullptr, _IONBF, 0);

        // make cout, wcout, cin, wcin, wcerr, cerr, wclog and clog
        // point to console as well

        std::ios::sync_with_stdio();

        // set the screen buffer to be big enough to let us scroll text

        CONSOLE_SCREEN_BUFFER_INFO coninfo{};
        GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &coninfo);

        coninfo.dwSize.Y = MaxConsoleLines;

        SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), coninfo.dwSize);
    }

}