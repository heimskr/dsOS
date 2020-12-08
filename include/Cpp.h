#ifndef CPP_H_
#define CPP_H_

#ifndef MEMORY_OPERATORS_SET
inline void *operator new     (size_t, void *p) throw() { return p; }
inline void *operator new[]   (size_t, void *p) throw() { return p; }
inline void  operator delete  (void *, void *)  throw() { };
inline void  operator delete[](void *, void *)  throw() { };
inline void  operator delete  (void *, unsigned long x) throw() { };
inline void  operator delete[](void *, unsigned long x) throw() { };
#endif

#endif
