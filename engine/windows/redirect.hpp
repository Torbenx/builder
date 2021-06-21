#pragma once

#include <fcntl.h>
#include <fstream>
#include <io.h>
#include <iostream>
#include <stdio.h>
#include <windows.h>

//based on http://dslweb.nwnexus.com/~ast/dload/guicon.htm
static const long MAX_CONSOLE_LINES = 5000;
void redirectToConsole() {
    int hConHandle;
    intptr_t lStdHandle;
    CONSOLE_SCREEN_BUFFER_INFO coninfo;
    FILE* fp;
    // allocate a console for this app
    AllocConsole();
    // set the screen buffer to be big enough to let us scroll text
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &coninfo);
    coninfo.dwSize.Y = MAX_CONSOLE_LINES;
    SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), coninfo.dwSize);

    // redirect unbuffered STDOUT to the console
    freopen("CONOUT$", "w", stdout);

    // redirect unbuffered STDIN to the console
    freopen("CONIN$", "r", stdin);

    // redirect unbuffered STDERR to the console
    freopen("CONERR$", "w", stderr);

    // make cout, wcout, cin, wcin, wcerr, cerr, wclog and clog
    // point to console as well
    std::ios::sync_with_stdio();
}
