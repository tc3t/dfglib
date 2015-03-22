#pragma once

#ifdef _WIN32

#include <iostream>
#include <cstdlib>

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(console) {

inline void pause() {std::system("pause");}
inline void pause(const char* const psz) {std::cout << psz << std::endl; std::system("pause");}

}} // module console

#endif // _WIN32

