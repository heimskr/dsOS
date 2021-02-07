#pragma once

typedef void (*ctor_t)(void);
extern ctor_t _ctors_start[];
extern ctor_t _ctors_end[];
extern "C" void _init();
