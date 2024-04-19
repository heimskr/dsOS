#pragma once

void assert_(bool condition, const char *condstr, const char *file, int line);

#define assert(cond) assert_((cond), #cond, __FILE__, __LINE__)