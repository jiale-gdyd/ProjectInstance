#ifndef __X_CONSTRUCTOR_H__
#define __X_CONSTRUCTOR_H__

#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 7)

#define X_HAS_CONSTRUCTORS              1

#define X_DEFINE_DESTRUCTOR(_func)      static void __attribute__((destructor)) _func(void);
#define X_DEFINE_CONSTRUCTOR(_func)     static void __attribute__((constructor)) _func(void);

#endif

#endif
