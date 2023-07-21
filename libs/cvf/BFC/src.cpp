
#include<string.h>
#include<stdlib.h>
#include<memory.h>

#ifdef _WIN32
#include"bfc/portable_win32.cpp"
//#include"BFC/portable_std.cpp"
#else
//#include"BFC/portable_wx.cpp"
//#include"BFC/portable_linux.cpp"
#include"BFC/portable_std.cpp"
#endif

//base
#include"BFC/err.cpp"
#include"BFC/stdf.cpp"

//utils
#include"BFC/argv.cpp"
#include"BFC/bfstream.cpp"


