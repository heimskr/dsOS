#pragma once

typedef void (*ctor_t)(void);
extern ctor_t _ctors_start[0];
extern ctor_t _ctors_end[0];
extern "C" void _init();
