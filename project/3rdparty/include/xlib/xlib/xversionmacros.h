#ifndef __X_VERSION_MACROS_H__
#define __X_VERSION_MACROS_H__

#define X_ENCODE_VERSION(major, minor)                          ((major) << 16 | (minor) << 8)

#define XLIB_VERSION_2_26                                       (X_ENCODE_VERSION(2, 26))
#define XLIB_VERSION_2_28                                       (X_ENCODE_VERSION(2, 28))
#define XLIB_VERSION_2_30                                       (X_ENCODE_VERSION(2, 30))
#define XLIB_VERSION_2_32                                       (X_ENCODE_VERSION(2, 32))
#define XLIB_VERSION_2_34                                       (X_ENCODE_VERSION(2, 34))
#define XLIB_VERSION_2_36                                       (X_ENCODE_VERSION(2, 36))
#define XLIB_VERSION_2_38                                       (X_ENCODE_VERSION(2, 38))
#define XLIB_VERSION_2_40                                       (X_ENCODE_VERSION(2, 40))
#define XLIB_VERSION_2_42                                       (X_ENCODE_VERSION(2, 42))
#define XLIB_VERSION_2_44                                       (X_ENCODE_VERSION(2, 44))
#define XLIB_VERSION_2_46                                       (X_ENCODE_VERSION(2, 46))
#define XLIB_VERSION_2_48                                       (X_ENCODE_VERSION(2, 48))
#define XLIB_VERSION_2_50                                       (X_ENCODE_VERSION(2, 50))
#define XLIB_VERSION_2_52                                       (X_ENCODE_VERSION(2, 52))
#define XLIB_VERSION_2_54                                       (X_ENCODE_VERSION(2, 54))
#define XLIB_VERSION_2_56                                       (X_ENCODE_VERSION(2, 56))
#define XLIB_VERSION_2_58                                       (X_ENCODE_VERSION(2, 58))
#define XLIB_VERSION_2_60                                       (X_ENCODE_VERSION(2, 60))
#define XLIB_VERSION_2_62                                       (X_ENCODE_VERSION(2, 62))
#define XLIB_VERSION_2_64                                       (X_ENCODE_VERSION(2, 64))
#define XLIB_VERSION_2_66                                       (X_ENCODE_VERSION(2, 66))
#define XLIB_VERSION_2_68                                       (X_ENCODE_VERSION(2, 68))
#define XLIB_VERSION_2_70                                       (X_ENCODE_VERSION(2, 70))
#define XLIB_VERSION_2_72                                       (X_ENCODE_VERSION(2, 72))
#define XLIB_VERSION_2_74                                       (X_ENCODE_VERSION(2, 74))
#define XLIB_VERSION_2_76                                       (X_ENCODE_VERSION(2, 76))
#define XLIB_VERSION_2_78                                       (X_ENCODE_VERSION(2, 78))
#define XLIB_VERSION_2_80                                       (X_ENCODE_VERSION(2, 80))

#if (XLIB_MINOR_VERSION % 2)
#define XLIB_VERSION_CUR_STABLE                                 (X_ENCODE_VERSION(XLIB_MAJOR_VERSION, XLIB_MINOR_VERSION + 1))
#else
#define XLIB_VERSION_CUR_STABLE                                 (X_ENCODE_VERSION(XLIB_MAJOR_VERSION, XLIB_MINOR_VERSION))
#endif

#if (XLIB_MINOR_VERSION % 2)
#define XLIB_VERSION_PREV_STABLE                                (X_ENCODE_VERSION(XLIB_MAJOR_VERSION, XLIB_MINOR_VERSION - 1))
#else
#define XLIB_VERSION_PREV_STABLE                                (X_ENCODE_VERSION(XLIB_MAJOR_VERSION, XLIB_MINOR_VERSION - 2))
#endif

#ifndef XLIB_VERSION_MIN_REQUIRED
#define XLIB_VERSION_MIN_REQUIRED                               (XLIB_VERSION_CUR_STABLE)
#elif XLIB_VERSION_MIN_REQUIRED == 0
#undef  XLIB_VERSION_MIN_REQUIRED
#define XLIB_VERSION_MIN_REQUIRED                               (XLIB_VERSION_CUR_STABLE + 2)
#endif

#if !defined(XLIB_VERSION_MAX_ALLOWED) || (XLIB_VERSION_MAX_ALLOWED == 0)
#undef XLIB_VERSION_MAX_ALLOWED
#define XLIB_VERSION_MAX_ALLOWED                                (XLIB_VERSION_CUR_STABLE)
#endif

#if XLIB_VERSION_MIN_REQUIRED > XLIB_VERSION_CUR_STABLE
#error "XLIB_VERSION_MIN_REQUIRED must be <= XLIB_VERSION_CUR_STABLE"
#endif
#if XLIB_VERSION_MAX_ALLOWED < XLIB_VERSION_MIN_REQUIRED
#error "XLIB_VERSION_MAX_ALLOWED must be >= XLIB_VERSION_MIN_REQUIRED"
#endif
#if XLIB_VERSION_MIN_REQUIRED < XLIB_VERSION_2_26
#error "XLIB_VERSION_MIN_REQUIRED must be >= XLIB_VERSION_2_26"
#endif

#define XLIB_AVAILABLE_IN_ALL                                       _XLIB_EXTERN

#if XLIB_VERSION_MIN_REQUIRED >= XLIB_VERSION_2_26
#define XLIB_DEPRECATED_IN_2_26                                     XLIB_DEPRECATED
#define XLIB_DEPRECATED_IN_2_26_FOR(f)                              XLIB_DEPRECATED_FOR(f)
#define XLIB_DEPRECATED_MACRO_IN_2_26                               XLIB_DEPRECATED_MACRO
#define XLIB_DEPRECATED_MACRO_IN_2_26_FOR(f)                        XLIB_DEPRECATED_MACRO_FOR(f)
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_26                          XLIB_DEPRECATED_ENUMERATOR
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_26_FOR(f)                   XLIB_DEPRECATED_ENUMERATOR_FOR(f)
#define XLIB_DEPRECATED_TYPE_IN_2_26                                XLIB_DEPRECATED_TYPE
#define XLIB_DEPRECATED_TYPE_IN_2_26_FOR(f)                         XLIB_DEPRECATED_TYPE_FOR(f)
#else
#define XLIB_DEPRECATED_IN_2_26                                     _XLIB_EXTERN
#define XLIB_DEPRECATED_IN_2_26_FOR(f)                              _XLIB_EXTERN
#define XLIB_DEPRECATED_MACRO_IN_2_26
#define XLIB_DEPRECATED_MACRO_IN_2_26_FOR(f)
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_26
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_26_FOR(f)
#define XLIB_DEPRECATED_TYPE_IN_2_26
#define XLIB_DEPRECATED_TYPE_IN_2_26_FOR(f)
#endif

#if XLIB_VERSION_MAX_ALLOWED < XLIB_VERSION_2_26
#define XLIB_AVAILABLE_IN_2_26                                      XLIB_UNAVAILABLE(2, 26)
#define XLIB_AVAILABLE_MACRO_IN_2_26                                XLIB_UNAVAILABLE_MACRO(2, 26)
#define XLIB_AVAILABLE_ENUMERATOR_IN_2_26                           XLIB_UNAVAILABLE_ENUMERATOR(2, 26)
#define XLIB_AVAILABLE_TYPE_IN_2_26                                 XLIB_UNAVAILABLE_TYPE(2, 26)
#else
#define XLIB_AVAILABLE_IN_2_26                                      _XLIB_EXTERN
#define XLIB_AVAILABLE_MACRO_IN_2_26
#define XLIB_AVAILABLE_ENUMERATOR_IN_2_26
#define XLIB_AVAILABLE_TYPE_IN_2_26
#endif

#if XLIB_VERSION_MIN_REQUIRED >= XLIB_VERSION_2_28
#define XLIB_DEPRECATED_IN_2_28                                     XLIB_DEPRECATED
#define XLIB_DEPRECATED_IN_2_28_FOR(f)                              XLIB_DEPRECATED_FOR(f)
#define XLIB_DEPRECATED_MACRO_IN_2_28                               XLIB_DEPRECATED_MACRO
#define XLIB_DEPRECATED_MACRO_IN_2_28_FOR(f)                        XLIB_DEPRECATED_MACRO_FOR(f)
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_28                          XLIB_DEPRECATED_ENUMERATOR
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_28_FOR(f)                   XLIB_DEPRECATED_ENUMERATOR_FOR(f)
#define XLIB_DEPRECATED_TYPE_IN_2_28                                XLIB_DEPRECATED_TYPE
#define XLIB_DEPRECATED_TYPE_IN_2_28_FOR(f)                         XLIB_DEPRECATED_TYPE_FOR(f)
#else
#define XLIB_DEPRECATED_IN_2_28                                     _XLIB_EXTERN
#define XLIB_DEPRECATED_IN_2_28_FOR(f)                              _XLIB_EXTERN
#define XLIB_DEPRECATED_MACRO_IN_2_28
#define XLIB_DEPRECATED_MACRO_IN_2_28_FOR(f)
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_28
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_28_FOR(f)
#define XLIB_DEPRECATED_TYPE_IN_2_28
#define XLIB_DEPRECATED_TYPE_IN_2_28_FOR(f)
#endif

#if XLIB_VERSION_MAX_ALLOWED < XLIB_VERSION_2_28
#define XLIB_AVAILABLE_IN_2_28                                      XLIB_UNAVAILABLE(2, 28)
#define XLIB_AVAILABLE_MACRO_IN_2_28                                XLIB_UNAVAILABLE_MACRO(2, 28)
#define XLIB_AVAILABLE_ENUMERATOR_IN_2_28                           XLIB_UNAVAILABLE_ENUMERATOR(2, 28)
#define XLIB_AVAILABLE_TYPE_IN_2_28                                 XLIB_UNAVAILABLE_TYPE(2, 28)
#else
#define XLIB_AVAILABLE_IN_2_28                                      _XLIB_EXTERN
#define XLIB_AVAILABLE_MACRO_IN_2_28
#define XLIB_AVAILABLE_ENUMERATOR_IN_2_28
#define XLIB_AVAILABLE_TYPE_IN_2_28
#endif

#if XLIB_VERSION_MIN_REQUIRED >= XLIB_VERSION_2_30
#define XLIB_DEPRECATED_IN_2_30                                     XLIB_DEPRECATED
#define XLIB_DEPRECATED_IN_2_30_FOR(f)                              XLIB_DEPRECATED_FOR(f)
#define XLIB_DEPRECATED_MACRO_IN_2_30                               XLIB_DEPRECATED_MACRO
#define XLIB_DEPRECATED_MACRO_IN_2_30_FOR(f)                        XLIB_DEPRECATED_MACRO_FOR(f)
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_30                          XLIB_DEPRECATED_ENUMERATOR
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_30_FOR(f)                   XLIB_DEPRECATED_ENUMERATOR_FOR(f)
#define XLIB_DEPRECATED_TYPE_IN_2_30                                XLIB_DEPRECATED_TYPE
#define XLIB_DEPRECATED_TYPE_IN_2_30_FOR(f)                         XLIB_DEPRECATED_TYPE_FOR(f)
#else
#define XLIB_DEPRECATED_IN_2_30                                     _XLIB_EXTERN
#define XLIB_DEPRECATED_IN_2_30_FOR(f)                              _XLIB_EXTERN
#define XLIB_DEPRECATED_MACRO_IN_2_30
#define XLIB_DEPRECATED_MACRO_IN_2_30_FOR(f)
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_30
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_30_FOR(f)
#define XLIB_DEPRECATED_TYPE_IN_2_30
#define XLIB_DEPRECATED_TYPE_IN_2_30_FOR(f)
#endif

#if XLIB_VERSION_MAX_ALLOWED < XLIB_VERSION_2_30
#define XLIB_AVAILABLE_IN_2_30                                      XLIB_UNAVAILABLE(2, 30)
#define XLIB_AVAILABLE_MACRO_IN_2_30                                XLIB_UNAVAILABLE_MACRO(2, 30)
#define XLIB_AVAILABLE_ENUMERATOR_IN_2_30                           XLIB_UNAVAILABLE_ENUMERATOR(2, 30)
#define XLIB_AVAILABLE_TYPE_IN_2_30                                 XLIB_UNAVAILABLE_TYPE(2, 30)
#else
#define XLIB_AVAILABLE_IN_2_30                                      _XLIB_EXTERN
#define XLIB_AVAILABLE_MACRO_IN_2_30
#define XLIB_AVAILABLE_ENUMERATOR_IN_2_30
#define XLIB_AVAILABLE_TYPE_IN_2_30
#endif

#if XLIB_VERSION_MIN_REQUIRED >= XLIB_VERSION_2_32
#define XLIB_DEPRECATED_IN_2_32                                     XLIB_DEPRECATED
#define XLIB_DEPRECATED_IN_2_32_FOR(f)                              XLIB_DEPRECATED_FOR(f)
#define XLIB_DEPRECATED_MACRO_IN_2_32                               XLIB_DEPRECATED_MACRO
#define XLIB_DEPRECATED_MACRO_IN_2_32_FOR(f)                        XLIB_DEPRECATED_MACRO_FOR(f)
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_32                          XLIB_DEPRECATED_ENUMERATOR
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_32_FOR(f)                   XLIB_DEPRECATED_ENUMERATOR_FOR(f)
#define XLIB_DEPRECATED_TYPE_IN_2_32                                XLIB_DEPRECATED_TYPE
#define XLIB_DEPRECATED_TYPE_IN_2_32_FOR(f)                         XLIB_DEPRECATED_TYPE_FOR(f)
#else
#define XLIB_DEPRECATED_IN_2_32                                     _XLIB_EXTERN
#define XLIB_DEPRECATED_IN_2_32_FOR(f)                              _XLIB_EXTERN
#define XLIB_DEPRECATED_MACRO_IN_2_32
#define XLIB_DEPRECATED_MACRO_IN_2_32_FOR(f)
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_32
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_32_FOR(f)
#define XLIB_DEPRECATED_TYPE_IN_2_32
#define XLIB_DEPRECATED_TYPE_IN_2_32_FOR(f)
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_32
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_32_FOR(f)
#define XLIB_DEPRECATED_TYPE_IN_2_32
#define XLIB_DEPRECATED_TYPE_IN_2_32_FOR(f)
#endif

#if XLIB_VERSION_MAX_ALLOWED < XLIB_VERSION_2_32
#define XLIB_AVAILABLE_IN_2_32                                      XLIB_UNAVAILABLE(2, 32)
#define XLIB_AVAILABLE_MACRO_IN_2_32                                XLIB_UNAVAILABLE_MACRO(2, 32)
#define XLIB_AVAILABLE_ENUMERATOR_IN_2_32                           XLIB_UNAVAILABLE_ENUMERATOR(2, 32)
#define XLIB_AVAILABLE_TYPE_IN_2_32                                 XLIB_UNAVAILABLE_TYPE(2, 32)
#else
#define XLIB_AVAILABLE_IN_2_32                                      _XLIB_EXTERN
#define XLIB_AVAILABLE_MACRO_IN_2_32
#define XLIB_AVAILABLE_ENUMERATOR_IN_2_32
#define XLIB_AVAILABLE_TYPE_IN_2_32
#endif

#if XLIB_VERSION_MIN_REQUIRED >= XLIB_VERSION_2_34
#define XLIB_DEPRECATED_IN_2_34                                     XLIB_DEPRECATED
#define XLIB_DEPRECATED_IN_2_34_FOR(f)                              XLIB_DEPRECATED_FOR(f)
#define XLIB_DEPRECATED_MACRO_IN_2_34                               XLIB_DEPRECATED_MACRO
#define XLIB_DEPRECATED_MACRO_IN_2_34_FOR(f)                        XLIB_DEPRECATED_MACRO_FOR(f)
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_34                          XLIB_DEPRECATED_ENUMERATOR
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_34_FOR(f)                   XLIB_DEPRECATED_ENUMERATOR_FOR(f)
#define XLIB_DEPRECATED_TYPE_IN_2_34                                XLIB_DEPRECATED_TYPE
#define XLIB_DEPRECATED_TYPE_IN_2_34_FOR(f)                         XLIB_DEPRECATED_TYPE_FOR(f)
#else
#define XLIB_DEPRECATED_IN_2_34                                     _XLIB_EXTERN
#define XLIB_DEPRECATED_IN_2_34_FOR(f)                              _XLIB_EXTERN
#define XLIB_DEPRECATED_MACRO_IN_2_34
#define XLIB_DEPRECATED_MACRO_IN_2_34_FOR(f)
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_34
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_34_FOR(f)
#define XLIB_DEPRECATED_TYPE_IN_2_34
#define XLIB_DEPRECATED_TYPE_IN_2_34_FOR(f)
#endif

#if XLIB_VERSION_MAX_ALLOWED < XLIB_VERSION_2_34
#define XLIB_AVAILABLE_IN_2_34                                      XLIB_UNAVAILABLE(2, 34)
#define XLIB_AVAILABLE_MACRO_IN_2_34                                XLIB_UNAVAILABLE_MACRO(2, 34)
#define XLIB_AVAILABLE_ENUMERATOR_IN_2_34                           XLIB_UNAVAILABLE_ENUMERATOR(2, 34)
#define XLIB_AVAILABLE_TYPE_IN_2_34                                 XLIB_UNAVAILABLE_TYPE(2, 34)
#else
#define XLIB_AVAILABLE_IN_2_34                                      _XLIB_EXTERN
#define XLIB_AVAILABLE_MACRO_IN_2_34
#define XLIB_AVAILABLE_ENUMERATOR_IN_2_34
#define XLIB_AVAILABLE_TYPE_IN_2_34
#endif

#if XLIB_VERSION_MIN_REQUIRED >= XLIB_VERSION_2_36
#define XLIB_DEPRECATED_IN_2_36                                     XLIB_DEPRECATED
#define XLIB_DEPRECATED_IN_2_36_FOR(f)                              XLIB_DEPRECATED_FOR(f)
#define XLIB_DEPRECATED_MACRO_IN_2_36                               XLIB_DEPRECATED_MACRO
#define XLIB_DEPRECATED_MACRO_IN_2_36_FOR(f)                        XLIB_DEPRECATED_MACRO_FOR(f)
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_36                          XLIB_DEPRECATED_ENUMERATOR
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_36_FOR(f)                   XLIB_DEPRECATED_ENUMERATOR_FOR(f)
#define XLIB_DEPRECATED_TYPE_IN_2_36                                XLIB_DEPRECATED_TYPE
#define XLIB_DEPRECATED_TYPE_IN_2_36_FOR(f)                         XLIB_DEPRECATED_TYPE_FOR(f)
#else
#define XLIB_DEPRECATED_IN_2_36                                     _XLIB_EXTERN
#define XLIB_DEPRECATED_IN_2_36_FOR(f)                              _XLIB_EXTERN
#define XLIB_DEPRECATED_MACRO_IN_2_36
#define XLIB_DEPRECATED_MACRO_IN_2_36_FOR(f)
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_36
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_36_FOR(f)
#define XLIB_DEPRECATED_TYPE_IN_2_36
#define XLIB_DEPRECATED_TYPE_IN_2_36_FOR(f)
#endif

#if XLIB_VERSION_MAX_ALLOWED < XLIB_VERSION_2_36
#define XLIB_AVAILABLE_IN_2_36                                      XLIB_UNAVAILABLE(2, 36)
#define XLIB_AVAILABLE_MACRO_IN_2_36                                XLIB_UNAVAILABLE_MACRO(2, 36)
#define XLIB_AVAILABLE_ENUMERATOR_IN_2_36                           XLIB_UNAVAILABLE_ENUMERATOR(2, 36)
#define XLIB_AVAILABLE_TYPE_IN_2_36                                 XLIB_UNAVAILABLE_TYPE(2, 36)
#else
#define XLIB_AVAILABLE_IN_2_36                                      _XLIB_EXTERN
#define XLIB_AVAILABLE_MACRO_IN_2_36
#define XLIB_AVAILABLE_ENUMERATOR_IN_2_36
#define XLIB_AVAILABLE_TYPE_IN_2_36
#endif

#if XLIB_VERSION_MIN_REQUIRED >= XLIB_VERSION_2_38
#define XLIB_DEPRECATED_IN_2_38                                     XLIB_DEPRECATED
#define XLIB_DEPRECATED_IN_2_38_FOR(f)                              XLIB_DEPRECATED_FOR(f)
#define XLIB_DEPRECATED_MACRO_IN_2_38                               XLIB_DEPRECATED_MACRO
#define XLIB_DEPRECATED_MACRO_IN_2_38_FOR(f)                        XLIB_DEPRECATED_MACRO_FOR(f)
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_38                          XLIB_DEPRECATED_ENUMERATOR
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_38_FOR(f)                   XLIB_DEPRECATED_ENUMERATOR_FOR(f)
#define XLIB_DEPRECATED_TYPE_IN_2_38                                XLIB_DEPRECATED_TYPE
#define XLIB_DEPRECATED_TYPE_IN_2_38_FOR(f)                         XLIB_DEPRECATED_TYPE_FOR(f)
#else
#define XLIB_DEPRECATED_IN_2_38                                     _XLIB_EXTERN
#define XLIB_DEPRECATED_IN_2_38_FOR(f)                              _XLIB_EXTERN
#define XLIB_DEPRECATED_MACRO_IN_2_38
#define XLIB_DEPRECATED_MACRO_IN_2_38_FOR(f)
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_38
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_38_FOR(f)
#define XLIB_DEPRECATED_TYPE_IN_2_38
#define XLIB_DEPRECATED_TYPE_IN_2_38_FOR(f)
#endif

#if XLIB_VERSION_MAX_ALLOWED < XLIB_VERSION_2_38
#define XLIB_AVAILABLE_IN_2_38                                      XLIB_UNAVAILABLE(2, 38)
#define XLIB_AVAILABLE_MACRO_IN_2_38                                XLIB_UNAVAILABLE_MACRO(2, 38)
#define XLIB_AVAILABLE_ENUMERATOR_IN_2_38                           XLIB_UNAVAILABLE_ENUMERATOR(2, 38)
#define XLIB_AVAILABLE_TYPE_IN_2_38                                 XLIB_UNAVAILABLE_TYPE(2, 38)
#else
#define XLIB_AVAILABLE_IN_2_38                                      _XLIB_EXTERN
#define XLIB_AVAILABLE_MACRO_IN_2_38
#define XLIB_AVAILABLE_ENUMERATOR_IN_2_38
#define XLIB_AVAILABLE_TYPE_IN_2_38
#endif

#if XLIB_VERSION_MIN_REQUIRED >= XLIB_VERSION_2_40
#define XLIB_DEPRECATED_IN_2_40                                     XLIB_DEPRECATED
#define XLIB_DEPRECATED_IN_2_40_FOR(f)                              XLIB_DEPRECATED_FOR(f)
#define XLIB_DEPRECATED_MACRO_IN_2_40                               XLIB_DEPRECATED_MACRO
#define XLIB_DEPRECATED_MACRO_IN_2_40_FOR(f)                        XLIB_DEPRECATED_MACRO_FOR(f)
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_40                          XLIB_DEPRECATED_ENUMERATOR
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_40_FOR(f)                   XLIB_DEPRECATED_ENUMERATOR_FOR(f)
#define XLIB_DEPRECATED_TYPE_IN_2_40                                XLIB_DEPRECATED_TYPE
#define XLIB_DEPRECATED_TYPE_IN_2_40_FOR(f)                         XLIB_DEPRECATED_TYPE_FOR(f)
#else
#define XLIB_DEPRECATED_IN_2_40                                     _XLIB_EXTERN
#define XLIB_DEPRECATED_IN_2_40_FOR(f)                              _XLIB_EXTERN
#define XLIB_DEPRECATED_MACRO_IN_2_40
#define XLIB_DEPRECATED_MACRO_IN_2_40_FOR(f)
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_40
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_40_FOR(f)
#define XLIB_DEPRECATED_TYPE_IN_2_40
#define XLIB_DEPRECATED_TYPE_IN_2_40_FOR(f)
#endif

#if XLIB_VERSION_MAX_ALLOWED < XLIB_VERSION_2_40
#define XLIB_AVAILABLE_IN_2_40                                      XLIB_UNAVAILABLE(2, 40)
#define XLIB_AVAILABLE_MACRO_IN_2_40                                XLIB_UNAVAILABLE_MACRO(2, 40)
#define XLIB_AVAILABLE_ENUMERATOR_IN_2_40                           XLIB_UNAVAILABLE_ENUMERATOR(2, 40)
#define XLIB_AVAILABLE_TYPE_IN_2_40                                 XLIB_UNAVAILABLE_TYPE(2, 40)
#else
#define XLIB_AVAILABLE_IN_2_40                                      _XLIB_EXTERN
#define XLIB_AVAILABLE_MACRO_IN_2_40
#define XLIB_AVAILABLE_ENUMERATOR_IN_2_40
#define XLIB_AVAILABLE_TYPE_IN_2_40
#endif

#if XLIB_VERSION_MIN_REQUIRED >= XLIB_VERSION_2_42
#define XLIB_DEPRECATED_IN_2_42                                     XLIB_DEPRECATED
#define XLIB_DEPRECATED_IN_2_42_FOR(f)                              XLIB_DEPRECATED_FOR(f)
#define XLIB_DEPRECATED_MACRO_IN_2_42                               XLIB_DEPRECATED_MACRO
#define XLIB_DEPRECATED_MACRO_IN_2_42_FOR(f)                        XLIB_DEPRECATED_MACRO_FOR(f)
#else
#define XLIB_DEPRECATED_IN_2_42                                     _XLIB_EXTERN
#define XLIB_DEPRECATED_IN_2_42_FOR(f)                              _XLIB_EXTERN
#define XLIB_DEPRECATED_MACRO_IN_2_42
#define XLIB_DEPRECATED_MACRO_IN_2_42_FOR(f)
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_42
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_42_FOR(f)
#define XLIB_DEPRECATED_TYPE_IN_2_42
#define XLIB_DEPRECATED_TYPE_IN_2_42_FOR(f)
#endif

#if XLIB_VERSION_MAX_ALLOWED < XLIB_VERSION_2_42
#define XLIB_AVAILABLE_IN_2_42                                      XLIB_UNAVAILABLE(2, 42)
#define XLIB_AVAILABLE_MACRO_IN_2_42                                XLIB_UNAVAILABLE_MACRO(2, 42)
#define XLIB_AVAILABLE_ENUMERATOR_IN_2_42                           XLIB_UNAVAILABLE_ENUMERATOR(2, 42)
#define XLIB_AVAILABLE_TYPE_IN_2_42                                 XLIB_UNAVAILABLE_TYPE(2, 42)
#else
#define XLIB_AVAILABLE_IN_2_42                                      _XLIB_EXTERN
#define XLIB_AVAILABLE_MACRO_IN_2_42
#define XLIB_AVAILABLE_ENUMERATOR_IN_2_42
#define XLIB_AVAILABLE_TYPE_IN_2_42
#endif

#if XLIB_VERSION_MIN_REQUIRED >= XLIB_VERSION_2_44
#define XLIB_DEPRECATED_IN_2_44                                     XLIB_DEPRECATED
#define XLIB_DEPRECATED_IN_2_44_FOR(f)                              XLIB_DEPRECATED_FOR(f)
#define XLIB_DEPRECATED_MACRO_IN_2_44                               XLIB_DEPRECATED_MACRO
#define XLIB_DEPRECATED_MACRO_IN_2_44_FOR(f)                        XLIB_DEPRECATED_MACRO_FOR(f)
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_44                          XLIB_DEPRECATED_ENUMERATOR
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_44_FOR(f)                   XLIB_DEPRECATED_ENUMERATOR_FOR(f)
#define XLIB_DEPRECATED_TYPE_IN_2_44                                XLIB_DEPRECATED_TYPE
#define XLIB_DEPRECATED_TYPE_IN_2_44_FOR(f)                         XLIB_DEPRECATED_TYPE_FOR(f)
#else
#define XLIB_DEPRECATED_IN_2_44                                     _XLIB_EXTERN
#define XLIB_DEPRECATED_IN_2_44_FOR(f)                              _XLIB_EXTERN
#define XLIB_DEPRECATED_MACRO_IN_2_44
#define XLIB_DEPRECATED_MACRO_IN_2_44_FOR(f)
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_44
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_44_FOR(f)
#define XLIB_DEPRECATED_TYPE_IN_2_44
#define XLIB_DEPRECATED_TYPE_IN_2_44_FOR(f)
#endif

#if XLIB_VERSION_MAX_ALLOWED < XLIB_VERSION_2_44
#define XLIB_AVAILABLE_IN_2_44                                      XLIB_UNAVAILABLE(2, 44)
#define XLIB_AVAILABLE_STATIC_INLINE_IN_2_44                        XLIB_UNAVAILABLE_STATIC_INLINE(2, 44)
#define XLIB_AVAILABLE_MACRO_IN_2_44                                XLIB_UNAVAILABLE_MACRO(2, 44)
#define XLIB_AVAILABLE_ENUMERATOR_IN_2_44                           XLIB_UNAVAILABLE_ENUMERATOR(2, 44)
#define XLIB_AVAILABLE_TYPE_IN_2_44                                 XLIB_UNAVAILABLE_TYPE(2, 44)
#else
#define XLIB_AVAILABLE_IN_2_44                                      _XLIB_EXTERN
#define XLIB_AVAILABLE_STATIC_INLINE_IN_2_44
#define XLIB_AVAILABLE_MACRO_IN_2_44
#define XLIB_AVAILABLE_ENUMERATOR_IN_2_44
#define XLIB_AVAILABLE_TYPE_IN_2_44
#endif

#if XLIB_VERSION_MIN_REQUIRED >= XLIB_VERSION_2_46
#define XLIB_DEPRECATED_IN_2_46                                     XLIB_DEPRECATED
#define XLIB_DEPRECATED_IN_2_46_FOR(f)                              XLIB_DEPRECATED_FOR(f)
#define XLIB_DEPRECATED_MACRO_IN_2_46                               XLIB_DEPRECATED_MACRO
#define XLIB_DEPRECATED_MACRO_IN_2_46_FOR(f)                        XLIB_DEPRECATED_MACRO_FOR(f)
#else
#define XLIB_DEPRECATED_IN_2_46                                     _XLIB_EXTERN
#define XLIB_DEPRECATED_IN_2_46_FOR(f)                              _XLIB_EXTERN
#define XLIB_DEPRECATED_MACRO_IN_2_46
#define XLIB_DEPRECATED_MACRO_IN_2_46_FOR(f)
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_46
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_46_FOR(f)
#define XLIB_DEPRECATED_TYPE_IN_2_46
#define XLIB_DEPRECATED_TYPE_IN_2_46_FOR(f)
#endif

#if XLIB_VERSION_MAX_ALLOWED < XLIB_VERSION_2_46
#define XLIB_AVAILABLE_IN_2_46                                      XLIB_UNAVAILABLE(2, 46)
#define XLIB_AVAILABLE_MACRO_IN_2_46                                XLIB_UNAVAILABLE_MACRO(2, 46)
#define XLIB_AVAILABLE_ENUMERATOR_IN_2_46                           XLIB_UNAVAILABLE_ENUMERATOR(2, 46)
#define XLIB_AVAILABLE_TYPE_IN_2_46                                 XLIB_UNAVAILABLE_TYPE(2, 46)
#else
#define XLIB_AVAILABLE_IN_2_46                                      _XLIB_EXTERN
#define XLIB_AVAILABLE_MACRO_IN_2_46
#define XLIB_AVAILABLE_ENUMERATOR_IN_2_46
#define XLIB_AVAILABLE_TYPE_IN_2_46
#endif

#if XLIB_VERSION_MIN_REQUIRED >= XLIB_VERSION_2_48
#define XLIB_DEPRECATED_IN_2_48                                     XLIB_DEPRECATED
#define XLIB_DEPRECATED_IN_2_48_FOR(f)                              XLIB_DEPRECATED_FOR(f)
#define XLIB_DEPRECATED_MACRO_IN_2_48                               XLIB_DEPRECATED_MACRO
#define XLIB_DEPRECATED_MACRO_IN_2_48_FOR(f)                        XLIB_DEPRECATED_MACRO_FOR(f)
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_48                          XLIB_DEPRECATED_ENUMERATOR
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_48_FOR(f)                   XLIB_DEPRECATED_ENUMERATOR_FOR(f)
#define XLIB_DEPRECATED_TYPE_IN_2_48                                XLIB_DEPRECATED_TYPE
#define XLIB_DEPRECATED_TYPE_IN_2_48_FOR(f)                         XLIB_DEPRECATED_TYPE_FOR(f)
#else
#define XLIB_DEPRECATED_IN_2_48                                     _XLIB_EXTERN
#define XLIB_DEPRECATED_IN_2_48_FOR(f)                              _XLIB_EXTERN
#define XLIB_DEPRECATED_MACRO_IN_2_48
#define XLIB_DEPRECATED_MACRO_IN_2_48_FOR(f)
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_48
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_48_FOR(f)
#define XLIB_DEPRECATED_TYPE_IN_2_48
#define XLIB_DEPRECATED_TYPE_IN_2_48_FOR(f)
#endif

#if XLIB_VERSION_MAX_ALLOWED < XLIB_VERSION_2_48
#define XLIB_AVAILABLE_IN_2_48                                      XLIB_UNAVAILABLE(2, 48)
#define XLIB_AVAILABLE_MACRO_IN_2_48                                XLIB_UNAVAILABLE_MACRO(2, 48)
#define XLIB_AVAILABLE_ENUMERATOR_IN_2_48                           XLIB_UNAVAILABLE_ENUMERATOR(2, 48)
#define XLIB_AVAILABLE_TYPE_IN_2_48                                 XLIB_UNAVAILABLE_TYPE(2, 48)
#else
#define XLIB_AVAILABLE_IN_2_48                                      _XLIB_EXTERN
#define XLIB_AVAILABLE_MACRO_IN_2_48
#define XLIB_AVAILABLE_ENUMERATOR_IN_2_48
#define XLIB_AVAILABLE_TYPE_IN_2_48
#endif

#if XLIB_VERSION_MIN_REQUIRED >= XLIB_VERSION_2_50
#define XLIB_DEPRECATED_IN_2_50                                     XLIB_DEPRECATED
#define XLIB_DEPRECATED_IN_2_50_FOR(f)                              XLIB_DEPRECATED_FOR(f)
#define XLIB_DEPRECATED_MACRO_IN_2_50                               XLIB_DEPRECATED_MACRO
#define XLIB_DEPRECATED_MACRO_IN_2_50_FOR(f)                        XLIB_DEPRECATED_MACRO_FOR(f)
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_50                          XLIB_DEPRECATED_ENUMERATOR
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_50_FOR(f)                   XLIB_DEPRECATED_ENUMERATOR_FOR(f)
#define XLIB_DEPRECATED_TYPE_IN_2_50                                XLIB_DEPRECATED_TYPE
#define XLIB_DEPRECATED_TYPE_IN_2_50_FOR(f)                         XLIB_DEPRECATED_TYPE_FOR(f)
#else
#define XLIB_DEPRECATED_IN_2_50                                     _XLIB_EXTERN
#define XLIB_DEPRECATED_IN_2_50_FOR(f)                              _XLIB_EXTERN
#define XLIB_DEPRECATED_MACRO_IN_2_50
#define XLIB_DEPRECATED_MACRO_IN_2_50_FOR(f)
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_50
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_50_FOR(f)
#define XLIB_DEPRECATED_TYPE_IN_2_50
#define XLIB_DEPRECATED_TYPE_IN_2_50_FOR(f)
#endif

#if XLIB_VERSION_MAX_ALLOWED < XLIB_VERSION_2_50
#define XLIB_AVAILABLE_IN_2_50                                      XLIB_UNAVAILABLE(2, 50)
#define XLIB_AVAILABLE_MACRO_IN_2_50                                XLIB_UNAVAILABLE_MACRO(2, 50)
#define XLIB_AVAILABLE_ENUMERATOR_IN_2_50                           XLIB_UNAVAILABLE_ENUMERATOR(2, 50)
#define XLIB_AVAILABLE_TYPE_IN_2_50                                 XLIB_UNAVAILABLE_TYPE(2, 50)
#else
#define XLIB_AVAILABLE_IN_2_50                                      _XLIB_EXTERN
#define XLIB_AVAILABLE_MACRO_IN_2_50
#define XLIB_AVAILABLE_ENUMERATOR_IN_2_50
#define XLIB_AVAILABLE_TYPE_IN_2_50
#endif

#if XLIB_VERSION_MIN_REQUIRED >= XLIB_VERSION_2_52
#define XLIB_DEPRECATED_IN_2_52                                     XLIB_DEPRECATED
#define XLIB_DEPRECATED_IN_2_52_FOR(f)                              XLIB_DEPRECATED_FOR(f)
#define XLIB_DEPRECATED_MACRO_IN_2_52                               XLIB_DEPRECATED_MACRO
#define XLIB_DEPRECATED_MACRO_IN_2_52_FOR(f)                        XLIB_DEPRECATED_MACRO_FOR(f)
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_52                          XLIB_DEPRECATED_ENUMERATOR
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_52_FOR(f)                   XLIB_DEPRECATED_ENUMERATOR_FOR(f)
#define XLIB_DEPRECATED_TYPE_IN_2_52                                XLIB_DEPRECATED_TYPE
#define XLIB_DEPRECATED_TYPE_IN_2_52_FOR(f)                         XLIB_DEPRECATED_TYPE_FOR(f)
#else
#define XLIB_DEPRECATED_IN_2_52                                     _XLIB_EXTERN
#define XLIB_DEPRECATED_IN_2_52_FOR(f)                              _XLIB_EXTERN
#define XLIB_DEPRECATED_MACRO_IN_2_52
#define XLIB_DEPRECATED_MACRO_IN_2_52_FOR(f)
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_52
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_52_FOR(f)
#define XLIB_DEPRECATED_TYPE_IN_2_52
#define XLIB_DEPRECATED_TYPE_IN_2_52_FOR(f)
#endif

#if XLIB_VERSION_MAX_ALLOWED < XLIB_VERSION_2_52
#define XLIB_AVAILABLE_IN_2_52                                      XLIB_UNAVAILABLE(2, 52)
#define XLIB_AVAILABLE_MACRO_IN_2_52                                XLIB_UNAVAILABLE_MACRO(2, 52)
#define XLIB_AVAILABLE_ENUMERATOR_IN_2_52                           XLIB_UNAVAILABLE_ENUMERATOR(2, 52)
#define XLIB_AVAILABLE_TYPE_IN_2_52                                 XLIB_UNAVAILABLE_TYPE(2, 52)
#else
#define XLIB_AVAILABLE_IN_2_52                                      _XLIB_EXTERN
#define XLIB_AVAILABLE_MACRO_IN_2_52
#define XLIB_AVAILABLE_ENUMERATOR_IN_2_52
#define XLIB_AVAILABLE_TYPE_IN_2_52
#endif

#if XLIB_VERSION_MIN_REQUIRED >= XLIB_VERSION_2_54
#define XLIB_DEPRECATED_IN_2_54                                     XLIB_DEPRECATED
#define XLIB_DEPRECATED_IN_2_54_FOR(f)                              XLIB_DEPRECATED_FOR(f)
#define XLIB_DEPRECATED_MACRO_IN_2_54                               XLIB_DEPRECATED_MACRO
#define XLIB_DEPRECATED_MACRO_IN_2_54_FOR(f)                        XLIB_DEPRECATED_MACRO_FOR(f)
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_54                          XLIB_DEPRECATED_ENUMERATOR
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_54_FOR(f)                   XLIB_DEPRECATED_ENUMERATOR_FOR(f)
#define XLIB_DEPRECATED_TYPE_IN_2_54                                XLIB_DEPRECATED_TYPE
#define XLIB_DEPRECATED_TYPE_IN_2_54_FOR(f)                         XLIB_DEPRECATED_TYPE_FOR(f)
#else
#define XLIB_DEPRECATED_IN_2_54                                     _XLIB_EXTERN
#define XLIB_DEPRECATED_IN_2_54_FOR(f)                              _XLIB_EXTERN
#define XLIB_DEPRECATED_MACRO_IN_2_54
#define XLIB_DEPRECATED_MACRO_IN_2_54_FOR(f)
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_54
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_54_FOR(f)
#define XLIB_DEPRECATED_TYPE_IN_2_54
#define XLIB_DEPRECATED_TYPE_IN_2_54_FOR(f)
#endif

#if XLIB_VERSION_MAX_ALLOWED < XLIB_VERSION_2_54
#define XLIB_AVAILABLE_IN_2_54                                      XLIB_UNAVAILABLE(2, 54)
#define XLIB_AVAILABLE_MACRO_IN_2_54                                XLIB_UNAVAILABLE_MACRO(2, 54)
#define XLIB_AVAILABLE_ENUMERATOR_IN_2_54                           XLIB_UNAVAILABLE_ENUMERATOR(2, 54)
#define XLIB_AVAILABLE_TYPE_IN_2_54                                 XLIB_UNAVAILABLE_TYPE(2, 54)
#else
#define XLIB_AVAILABLE_IN_2_54                                      _XLIB_EXTERN
#define XLIB_AVAILABLE_MACRO_IN_2_54
#define XLIB_AVAILABLE_ENUMERATOR_IN_2_54
#define XLIB_AVAILABLE_TYPE_IN_2_54
#endif

#if XLIB_VERSION_MIN_REQUIRED >= XLIB_VERSION_2_56
#define XLIB_DEPRECATED_IN_2_56                                     XLIB_DEPRECATED
#define XLIB_DEPRECATED_IN_2_56_FOR(f)                              XLIB_DEPRECATED_FOR(f)
#define XLIB_DEPRECATED_MACRO_IN_2_56                               XLIB_DEPRECATED_MACRO
#define XLIB_DEPRECATED_MACRO_IN_2_56_FOR(f)                        XLIB_DEPRECATED_MACRO_FOR(f)
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_56                          XLIB_DEPRECATED_ENUMERATOR
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_56_FOR(f)                   XLIB_DEPRECATED_ENUMERATOR_FOR(f)
#define XLIB_DEPRECATED_TYPE_IN_2_56                                XLIB_DEPRECATED_TYPE
#define XLIB_DEPRECATED_TYPE_IN_2_56_FOR(f)                         XLIB_DEPRECATED_TYPE_FOR(f)
#else
#define XLIB_DEPRECATED_IN_2_56                                     _XLIB_EXTERN
#define XLIB_DEPRECATED_IN_2_56_FOR(f)                              _XLIB_EXTERN
#define XLIB_DEPRECATED_MACRO_IN_2_56
#define XLIB_DEPRECATED_MACRO_IN_2_56_FOR(f)
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_56
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_56_FOR(f)
#define XLIB_DEPRECATED_TYPE_IN_2_56
#define XLIB_DEPRECATED_TYPE_IN_2_56_FOR(f)
#endif

#if XLIB_VERSION_MAX_ALLOWED < XLIB_VERSION_2_56
#define XLIB_AVAILABLE_IN_2_56                                      XLIB_UNAVAILABLE(2, 56)
#define XLIB_AVAILABLE_MACRO_IN_2_56                                XLIB_UNAVAILABLE_MACRO(2, 56)
#define XLIB_AVAILABLE_ENUMERATOR_IN_2_56                           XLIB_UNAVAILABLE_ENUMERATOR(2, 56)
#define XLIB_AVAILABLE_TYPE_IN_2_56                                 XLIB_UNAVAILABLE_TYPE(2, 56)
#else
#define XLIB_AVAILABLE_IN_2_56                                      _XLIB_EXTERN
#define XLIB_AVAILABLE_MACRO_IN_2_56
#define XLIB_AVAILABLE_ENUMERATOR_IN_2_56
#define XLIB_AVAILABLE_TYPE_IN_2_56
#endif

#if XLIB_VERSION_MIN_REQUIRED >= XLIB_VERSION_2_58
#define XLIB_DEPRECATED_IN_2_58                                     XLIB_DEPRECATED
#define XLIB_DEPRECATED_IN_2_58_FOR(f)                              XLIB_DEPRECATED_FOR(f)
#define XLIB_DEPRECATED_MACRO_IN_2_58                               XLIB_DEPRECATED_MACRO
#define XLIB_DEPRECATED_MACRO_IN_2_58_FOR(f)                        XLIB_DEPRECATED_MACRO_FOR(f)
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_58                          XLIB_DEPRECATED_ENUMERATOR
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_58_FOR(f)                   XLIB_DEPRECATED_ENUMERATOR_FOR(f)
#define XLIB_DEPRECATED_TYPE_IN_2_58                                XLIB_DEPRECATED_TYPE
#define XLIB_DEPRECATED_TYPE_IN_2_58_FOR(f)                         XLIB_DEPRECATED_TYPE_FOR(f)
#else
#define XLIB_DEPRECATED_IN_2_58                                     _XLIB_EXTERN
#define XLIB_DEPRECATED_IN_2_58_FOR(f)                              _XLIB_EXTERN
#define XLIB_DEPRECATED_MACRO_IN_2_58
#define XLIB_DEPRECATED_MACRO_IN_2_58_FOR(f)
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_58
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_58_FOR(f)
#define XLIB_DEPRECATED_TYPE_IN_2_58
#define XLIB_DEPRECATED_TYPE_IN_2_58_FOR(f)
#endif

#if XLIB_VERSION_MAX_ALLOWED < XLIB_VERSION_2_58
#define XLIB_AVAILABLE_IN_2_58                                      XLIB_UNAVAILABLE(2, 58)
#define XLIB_AVAILABLE_MACRO_IN_2_58                                XLIB_UNAVAILABLE_MACRO(2, 58)
#define XLIB_AVAILABLE_ENUMERATOR_IN_2_58                           XLIB_UNAVAILABLE_ENUMERATOR(2, 58)
#define XLIB_AVAILABLE_TYPE_IN_2_58                                 XLIB_UNAVAILABLE_TYPE(2, 58)
#else
#define XLIB_AVAILABLE_IN_2_58                                      _XLIB_EXTERN
#define XLIB_AVAILABLE_MACRO_IN_2_58
#define XLIB_AVAILABLE_ENUMERATOR_IN_2_58
#define XLIB_AVAILABLE_TYPE_IN_2_58
#endif

#if XLIB_VERSION_MIN_REQUIRED >= XLIB_VERSION_2_60
#define XLIB_DEPRECATED_IN_2_60                                     XLIB_DEPRECATED
#define XLIB_DEPRECATED_IN_2_60_FOR(f)                              XLIB_DEPRECATED_FOR(f)
#define XLIB_DEPRECATED_MACRO_IN_2_60                               XLIB_DEPRECATED_MACRO
#define XLIB_DEPRECATED_MACRO_IN_2_60_FOR(f)                        XLIB_DEPRECATED_MACRO_FOR(f)
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_60                          XLIB_DEPRECATED_ENUMERATOR
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_60_FOR(f)                   XLIB_DEPRECATED_ENUMERATOR_FOR(f)
#define XLIB_DEPRECATED_TYPE_IN_2_60                                XLIB_DEPRECATED_TYPE
#define XLIB_DEPRECATED_TYPE_IN_2_60_FOR(f)                         XLIB_DEPRECATED_TYPE_FOR(f)
#else
#define XLIB_DEPRECATED_IN_2_60                                     _XLIB_EXTERN
#define XLIB_DEPRECATED_IN_2_60_FOR(f)                              _XLIB_EXTERN
#define XLIB_DEPRECATED_MACRO_IN_2_60
#define XLIB_DEPRECATED_MACRO_IN_2_60_FOR(f)
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_60
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_60_FOR(f)
#define XLIB_DEPRECATED_TYPE_IN_2_60
#define XLIB_DEPRECATED_TYPE_IN_2_60_FOR(f)
#endif

#if XLIB_VERSION_MAX_ALLOWED < XLIB_VERSION_2_60
#define XLIB_AVAILABLE_IN_2_60                                      XLIB_UNAVAILABLE(2, 60)
#define XLIB_AVAILABLE_STATIC_INLINE_IN_2_60                        XLIB_UNAVAILABLE_STATIC_INLINE(2, 60)
#define XLIB_AVAILABLE_MACRO_IN_2_60                                XLIB_UNAVAILABLE_MACRO(2, 60)
#define XLIB_AVAILABLE_ENUMERATOR_IN_2_60                           XLIB_UNAVAILABLE_ENUMERATOR(2, 60)
#define XLIB_AVAILABLE_TYPE_IN_2_60                                 XLIB_UNAVAILABLE_TYPE(2, 60)
#else
#define XLIB_AVAILABLE_IN_2_60                                      _XLIB_EXTERN
#define XLIB_AVAILABLE_STATIC_INLINE_IN_2_60
#define XLIB_AVAILABLE_MACRO_IN_2_60
#define XLIB_AVAILABLE_ENUMERATOR_IN_2_60
#define XLIB_AVAILABLE_TYPE_IN_2_60
#endif

#if XLIB_VERSION_MIN_REQUIRED >= XLIB_VERSION_2_62
#define XLIB_DEPRECATED_IN_2_62                                     XLIB_DEPRECATED
#define XLIB_DEPRECATED_IN_2_62_FOR(f)                              XLIB_DEPRECATED_FOR(f)
#define XLIB_DEPRECATED_MACRO_IN_2_62                               XLIB_DEPRECATED_MACRO
#define XLIB_DEPRECATED_MACRO_IN_2_62_FOR(f)                        XLIB_DEPRECATED_MACRO_FOR(f)
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_62                          XLIB_DEPRECATED_ENUMERATOR
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_62_FOR(f)                   XLIB_DEPRECATED_ENUMERATOR_FOR(f)
#define XLIB_DEPRECATED_TYPE_IN_2_62                                XLIB_DEPRECATED_TYPE
#define XLIB_DEPRECATED_TYPE_IN_2_62_FOR(f)                         XLIB_DEPRECATED_TYPE_FOR(f)
#else
#define XLIB_DEPRECATED_IN_2_62                                     _XLIB_EXTERN
#define XLIB_DEPRECATED_IN_2_62_FOR(f)                              _XLIB_EXTERN
#define XLIB_DEPRECATED_MACRO_IN_2_62
#define XLIB_DEPRECATED_MACRO_IN_2_62_FOR(f)
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_62
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_62_FOR(f)
#define XLIB_DEPRECATED_TYPE_IN_2_62
#define XLIB_DEPRECATED_TYPE_IN_2_62_FOR(f)
#endif

#if XLIB_VERSION_MAX_ALLOWED < XLIB_VERSION_2_62
#define XLIB_AVAILABLE_IN_2_62                                      XLIB_UNAVAILABLE(2, 62)
#define XLIB_AVAILABLE_STATIC_INLINE_IN_2_62                        XLIB_UNAVAILABLE_STATIC_INLINE(2, 62)
#define XLIB_AVAILABLE_MACRO_IN_2_62                                XLIB_UNAVAILABLE_MACRO(2, 62)
#define XLIB_AVAILABLE_ENUMERATOR_IN_2_62                           XLIB_UNAVAILABLE_ENUMERATOR(2, 62)
#define XLIB_AVAILABLE_TYPE_IN_2_62                                 XLIB_UNAVAILABLE_TYPE(2, 62)
#else
#define XLIB_AVAILABLE_IN_2_62                                      _XLIB_EXTERN
#define XLIB_AVAILABLE_STATIC_INLINE_IN_2_62
#define XLIB_AVAILABLE_MACRO_IN_2_62
#define XLIB_AVAILABLE_ENUMERATOR_IN_2_62
#define XLIB_AVAILABLE_TYPE_IN_2_62
#endif

#if XLIB_VERSION_MIN_REQUIRED >= XLIB_VERSION_2_64
#define XLIB_DEPRECATED_IN_2_64                                     XLIB_DEPRECATED
#define XLIB_DEPRECATED_IN_2_64_FOR(f)                              XLIB_DEPRECATED_FOR(f)
#define XLIB_DEPRECATED_MACRO_IN_2_64                               XLIB_DEPRECATED_MACRO
#define XLIB_DEPRECATED_MACRO_IN_2_64_FOR(f)                        XLIB_DEPRECATED_MACRO_FOR(f)
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_64                          XLIB_DEPRECATED_ENUMERATOR
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_64_FOR(f)                   XLIB_DEPRECATED_ENUMERATOR_FOR(f)
#define XLIB_DEPRECATED_TYPE_IN_2_64                                XLIB_DEPRECATED_TYPE
#define XLIB_DEPRECATED_TYPE_IN_2_64_FOR(f)                         XLIB_DEPRECATED_TYPE_FOR(f)
#else
#define XLIB_DEPRECATED_IN_2_64                                     _XLIB_EXTERN
#define XLIB_DEPRECATED_IN_2_64_FOR(f)                              _XLIB_EXTERN
#define XLIB_DEPRECATED_MACRO_IN_2_64
#define XLIB_DEPRECATED_MACRO_IN_2_64_FOR(f)
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_64
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_64_FOR(f)
#define XLIB_DEPRECATED_TYPE_IN_2_64
#define XLIB_DEPRECATED_TYPE_IN_2_64_FOR(f)
#endif

#if XLIB_VERSION_MAX_ALLOWED < XLIB_VERSION_2_64
#define XLIB_AVAILABLE_IN_2_64                                      XLIB_UNAVAILABLE(2, 64)
#define XLIB_AVAILABLE_STATIC_INLINE_IN_2_64                        XLIB_UNAVAILABLE_STATIC_INLINE(2, 64)
#define XLIB_AVAILABLE_MACRO_IN_2_64                                XLIB_UNAVAILABLE_MACRO(2, 64)
#define XLIB_AVAILABLE_ENUMERATOR_IN_2_64                           XLIB_UNAVAILABLE_ENUMERATOR(2, 64)
#define XLIB_AVAILABLE_TYPE_IN_2_64                                 XLIB_UNAVAILABLE_TYPE(2, 64)
#else
#define XLIB_AVAILABLE_IN_2_64                                      _XLIB_EXTERN
#define XLIB_AVAILABLE_STATIC_INLINE_IN_2_64
#define XLIB_AVAILABLE_MACRO_IN_2_64
#define XLIB_AVAILABLE_ENUMERATOR_IN_2_64
#define XLIB_AVAILABLE_TYPE_IN_2_64
#endif

#if XLIB_VERSION_MIN_REQUIRED >= XLIB_VERSION_2_66
#define XLIB_DEPRECATED_IN_2_66                                     XLIB_DEPRECATED
#define XLIB_DEPRECATED_IN_2_66_FOR(f)                              XLIB_DEPRECATED_FOR(f)
#define XLIB_DEPRECATED_MACRO_IN_2_66                               XLIB_DEPRECATED_MACRO
#define XLIB_DEPRECATED_MACRO_IN_2_66_FOR(f)                        XLIB_DEPRECATED_MACRO_FOR(f)
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_66                          XLIB_DEPRECATED_ENUMERATOR
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_66_FOR(f)                   XLIB_DEPRECATED_ENUMERATOR_FOR(f)
#define XLIB_DEPRECATED_TYPE_IN_2_66                                XLIB_DEPRECATED_TYPE
#define XLIB_DEPRECATED_TYPE_IN_2_66_FOR(f)                         XLIB_DEPRECATED_TYPE_FOR(f)
#else
#define XLIB_DEPRECATED_IN_2_66                                     _XLIB_EXTERN
#define XLIB_DEPRECATED_IN_2_66_FOR(f)                              _XLIB_EXTERN
#define XLIB_DEPRECATED_MACRO_IN_2_66
#define XLIB_DEPRECATED_MACRO_IN_2_66_FOR(f)
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_66
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_66_FOR(f)
#define XLIB_DEPRECATED_TYPE_IN_2_66
#define XLIB_DEPRECATED_TYPE_IN_2_66_FOR(f)
#endif

#if XLIB_VERSION_MAX_ALLOWED < XLIB_VERSION_2_66
#define XLIB_AVAILABLE_IN_2_66                                      XLIB_UNAVAILABLE(2, 66)
#define XLIB_AVAILABLE_STATIC_INLINE_IN_2_66                        XLIB_UNAVAILABLE_STATIC_INLINE(2, 66)
#define XLIB_AVAILABLE_MACRO_IN_2_66                                XLIB_UNAVAILABLE_MACRO(2, 66)
#define XLIB_AVAILABLE_ENUMERATOR_IN_2_66                           XLIB_UNAVAILABLE_ENUMERATOR(2, 66)
#define XLIB_AVAILABLE_TYPE_IN_2_66                                 XLIB_UNAVAILABLE_TYPE(2, 66)
#else
#define XLIB_AVAILABLE_IN_2_66                                      _XLIB_EXTERN
#define XLIB_AVAILABLE_STATIC_INLINE_IN_2_66
#define XLIB_AVAILABLE_MACRO_IN_2_66
#define XLIB_AVAILABLE_ENUMERATOR_IN_2_66
#define XLIB_AVAILABLE_TYPE_IN_2_66
#endif

#if XLIB_VERSION_MIN_REQUIRED >= XLIB_VERSION_2_68
#define XLIB_DEPRECATED_IN_2_68                                     XLIB_DEPRECATED
#define XLIB_DEPRECATED_IN_2_68_FOR(f)                              XLIB_DEPRECATED_FOR(f)
#define XLIB_DEPRECATED_MACRO_IN_2_68                               XLIB_DEPRECATED_MACRO
#define XLIB_DEPRECATED_MACRO_IN_2_68_FOR(f)                        XLIB_DEPRECATED_MACRO_FOR(f)
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_68                          XLIB_DEPRECATED_ENUMERATOR
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_68_FOR(f)                   XLIB_DEPRECATED_ENUMERATOR_FOR(f)
#define XLIB_DEPRECATED_TYPE_IN_2_68                                XLIB_DEPRECATED_TYPE
#define XLIB_DEPRECATED_TYPE_IN_2_68_FOR(f)                         XLIB_DEPRECATED_TYPE_FOR(f)
#else
#define XLIB_DEPRECATED_IN_2_68                                     _XLIB_EXTERN
#define XLIB_DEPRECATED_IN_2_68_FOR(f)                              _XLIB_EXTERN
#define XLIB_DEPRECATED_MACRO_IN_2_68
#define XLIB_DEPRECATED_MACRO_IN_2_68_FOR(f)
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_68
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_68_FOR(f)
#define XLIB_DEPRECATED_TYPE_IN_2_68
#define XLIB_DEPRECATED_TYPE_IN_2_68_FOR(f)
#endif

#if XLIB_VERSION_MAX_ALLOWED < XLIB_VERSION_2_68
#define XLIB_AVAILABLE_IN_2_68                                      XLIB_UNAVAILABLE(2, 68)
#define XLIB_AVAILABLE_STATIC_INLINE_IN_2_68                        XLIB_UNAVAILABLE_STATIC_INLINE(2, 68)
#define XLIB_AVAILABLE_MACRO_IN_2_68                                XLIB_UNAVAILABLE_MACRO(2, 68)
#define XLIB_AVAILABLE_ENUMERATOR_IN_2_68                           XLIB_UNAVAILABLE_ENUMERATOR(2, 68)
#define XLIB_AVAILABLE_TYPE_IN_2_68                                 XLIB_UNAVAILABLE_TYPE(2, 68)
#else
#define XLIB_AVAILABLE_IN_2_68                                      _XLIB_EXTERN
#define XLIB_AVAILABLE_STATIC_INLINE_IN_2_68
#define XLIB_AVAILABLE_MACRO_IN_2_68
#define XLIB_AVAILABLE_ENUMERATOR_IN_2_68
#define XLIB_AVAILABLE_TYPE_IN_2_68
#endif

#if XLIB_VERSION_MIN_REQUIRED >= XLIB_VERSION_2_70
#define XLIB_DEPRECATED_IN_2_70                                     XLIB_DEPRECATED
#define XLIB_DEPRECATED_IN_2_70_FOR(f)                              XLIB_DEPRECATED_FOR(f)
#define XLIB_DEPRECATED_MACRO_IN_2_70                               XLIB_DEPRECATED_MACRO
#define XLIB_DEPRECATED_MACRO_IN_2_70_FOR(f)                        XLIB_DEPRECATED_MACRO_FOR(f)
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_70                          XLIB_DEPRECATED_ENUMERATOR
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_70_FOR(f)                   XLIB_DEPRECATED_ENUMERATOR_FOR(f)
#define XLIB_DEPRECATED_TYPE_IN_2_70                                XLIB_DEPRECATED_TYPE
#define XLIB_DEPRECATED_TYPE_IN_2_70_FOR(f)                         XLIB_DEPRECATED_TYPE_FOR(f)
#else
#define XLIB_DEPRECATED_IN_2_70                                     _XLIB_EXTERN
#define XLIB_DEPRECATED_IN_2_70_FOR(f)                              _XLIB_EXTERN
#define XLIB_DEPRECATED_MACRO_IN_2_70
#define XLIB_DEPRECATED_MACRO_IN_2_70_FOR(f)
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_70
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_70_FOR(f)
#define XLIB_DEPRECATED_TYPE_IN_2_70
#define XLIB_DEPRECATED_TYPE_IN_2_70_FOR(f)
#endif

#if XLIB_VERSION_MAX_ALLOWED < XLIB_VERSION_2_70
#define XLIB_AVAILABLE_IN_2_70                                      XLIB_UNAVAILABLE(2, 70)
#define XLIB_AVAILABLE_STATIC_INLINE_IN_2_70                        XLIB_UNAVAILABLE_STATIC_INLINE(2, 70)
#define XLIB_AVAILABLE_MACRO_IN_2_70                                XLIB_UNAVAILABLE_MACRO(2, 70)
#define XLIB_AVAILABLE_ENUMERATOR_IN_2_70                           XLIB_UNAVAILABLE_ENUMERATOR(2, 70)
#define XLIB_AVAILABLE_TYPE_IN_2_70                                 XLIB_UNAVAILABLE_TYPE(2, 70)
#else
#define XLIB_AVAILABLE_IN_2_70                                      _XLIB_EXTERN
#define XLIB_AVAILABLE_STATIC_INLINE_IN_2_70
#define XLIB_AVAILABLE_MACRO_IN_2_70
#define XLIB_AVAILABLE_ENUMERATOR_IN_2_70
#define XLIB_AVAILABLE_TYPE_IN_2_70
#endif

#if XLIB_VERSION_MIN_REQUIRED >= XLIB_VERSION_2_72
#define XLIB_DEPRECATED_IN_2_72                                     XLIB_DEPRECATED
#define XLIB_DEPRECATED_IN_2_72_FOR(f)                              XLIB_DEPRECATED_FOR(f)
#define XLIB_DEPRECATED_MACRO_IN_2_72                               XLIB_DEPRECATED_MACRO
#define XLIB_DEPRECATED_MACRO_IN_2_72_FOR(f)                        XLIB_DEPRECATED_MACRO_FOR(f)
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_72                          XLIB_DEPRECATED_ENUMERATOR
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_72_FOR(f)                   XLIB_DEPRECATED_ENUMERATOR_FOR(f)
#define XLIB_DEPRECATED_TYPE_IN_2_72                                XLIB_DEPRECATED_TYPE
#define XLIB_DEPRECATED_TYPE_IN_2_72_FOR(f)                         XLIB_DEPRECATED_TYPE_FOR(f)
#else
#define XLIB_DEPRECATED_IN_2_72                                     _XLIB_EXTERN
#define XLIB_DEPRECATED_IN_2_72_FOR(f)                              _XLIB_EXTERN
#define XLIB_DEPRECATED_MACRO_IN_2_72
#define XLIB_DEPRECATED_MACRO_IN_2_72_FOR(f)
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_72
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_72_FOR(f)
#define XLIB_DEPRECATED_TYPE_IN_2_72
#define XLIB_DEPRECATED_TYPE_IN_2_72_FOR(f)
#endif

#if XLIB_VERSION_MAX_ALLOWED < XLIB_VERSION_2_72
#define XLIB_AVAILABLE_IN_2_72                                      XLIB_UNAVAILABLE(2, 72)
#define XLIB_AVAILABLE_STATIC_INLINE_IN_2_72                        XLIB_UNAVAILABLE_STATIC_INLINE(2, 72)
#define XLIB_AVAILABLE_MACRO_IN_2_72                                XLIB_UNAVAILABLE_MACRO(2, 72)
#define XLIB_AVAILABLE_ENUMERATOR_IN_2_72                           XLIB_UNAVAILABLE_ENUMERATOR(2, 72)
#define XLIB_AVAILABLE_TYPE_IN_2_72                                 XLIB_UNAVAILABLE_TYPE(2, 72)
#else
#define XLIB_AVAILABLE_IN_2_72                                      _XLIB_EXTERN
#define XLIB_AVAILABLE_STATIC_INLINE_IN_2_72
#define XLIB_AVAILABLE_MACRO_IN_2_72
#define XLIB_AVAILABLE_ENUMERATOR_IN_2_72
#define XLIB_AVAILABLE_TYPE_IN_2_72
#endif

#if XLIB_VERSION_MIN_REQUIRED >= XLIB_VERSION_2_74
#define XLIB_DEPRECATED_IN_2_74                                     XLIB_DEPRECATED
#define XLIB_DEPRECATED_IN_2_74_FOR(f)                              XLIB_DEPRECATED_FOR(f)
#define XLIB_DEPRECATED_MACRO_IN_2_74                               XLIB_DEPRECATED_MACRO
#define XLIB_DEPRECATED_MACRO_IN_2_74_FOR(f)                        XLIB_DEPRECATED_MACRO_FOR(f)
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_74                          XLIB_DEPRECATED_ENUMERATOR
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_74_FOR(f)                   XLIB_DEPRECATED_ENUMERATOR_FOR(f)
#define XLIB_DEPRECATED_TYPE_IN_2_74                                XLIB_DEPRECATED_TYPE
#define XLIB_DEPRECATED_TYPE_IN_2_74_FOR(f)                         XLIB_DEPRECATED_TYPE_FOR(f)
#else
#define XLIB_DEPRECATED_IN_2_74                                     _XLIB_EXTERN
#define XLIB_DEPRECATED_IN_2_74_FOR(f)                              _XLIB_EXTERN
#define XLIB_DEPRECATED_MACRO_IN_2_74
#define XLIB_DEPRECATED_MACRO_IN_2_74_FOR(f)
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_74
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_74_FOR(f)
#define XLIB_DEPRECATED_TYPE_IN_2_74
#define XLIB_DEPRECATED_TYPE_IN_2_74_FOR(f)
#endif

#if XLIB_VERSION_MAX_ALLOWED < XLIB_VERSION_2_74
#define XLIB_AVAILABLE_IN_2_74                                      XLIB_UNAVAILABLE(2, 74)
#define XLIB_AVAILABLE_STATIC_INLINE_IN_2_74                        XLIB_UNAVAILABLE_STATIC_INLINE(2, 74)
#define XLIB_AVAILABLE_MACRO_IN_2_74                                XLIB_UNAVAILABLE_MACRO(2, 74)
#define XLIB_AVAILABLE_ENUMERATOR_IN_2_74                           XLIB_UNAVAILABLE_ENUMERATOR(2, 74)
#define XLIB_AVAILABLE_TYPE_IN_2_74                                 XLIB_UNAVAILABLE_TYPE(2, 74)
#else
#define XLIB_AVAILABLE_IN_2_74                                      _XLIB_EXTERN
#define XLIB_AVAILABLE_STATIC_INLINE_IN_2_74
#define XLIB_AVAILABLE_MACRO_IN_2_74
#define XLIB_AVAILABLE_ENUMERATOR_IN_2_74
#define XLIB_AVAILABLE_TYPE_IN_2_74
#endif

#if XLIB_VERSION_MIN_REQUIRED >= XLIB_VERSION_2_76
#define XLIB_DEPRECATED_IN_2_76                                     XLIB_DEPRECATED
#define XLIB_DEPRECATED_IN_2_76_FOR(f)                              XLIB_DEPRECATED_FOR(f)
#define XLIB_DEPRECATED_MACRO_IN_2_76                               XLIB_DEPRECATED_MACRO
#define XLIB_DEPRECATED_MACRO_IN_2_76_FOR(f)                        XLIB_DEPRECATED_MACRO_FOR(f)
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_76                          XLIB_DEPRECATED_ENUMERATOR
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_76_FOR(f)                   XLIB_DEPRECATED_ENUMERATOR_FOR(f)
#define XLIB_DEPRECATED_TYPE_IN_2_76                                XLIB_DEPRECATED_TYPE
#define XLIB_DEPRECATED_TYPE_IN_2_76_FOR(f)                         XLIB_DEPRECATED_TYPE_FOR(f)
#else
#define XLIB_DEPRECATED_IN_2_76                                     _XLIB_EXTERN
#define XLIB_DEPRECATED_IN_2_76_FOR(f)                              _XLIB_EXTERN
#define XLIB_DEPRECATED_MACRO_IN_2_76
#define XLIB_DEPRECATED_MACRO_IN_2_76_FOR(f)
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_76
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_76_FOR(f)
#define XLIB_DEPRECATED_TYPE_IN_2_76
#define XLIB_DEPRECATED_TYPE_IN_2_76_FOR(f)
#endif

#if XLIB_VERSION_MAX_ALLOWED < XLIB_VERSION_2_76
#define XLIB_AVAILABLE_IN_2_76                                      XLIB_UNAVAILABLE(2, 74)
#define XLIB_AVAILABLE_STATIC_INLINE_IN_2_76                        XLIB_UNAVAILABLE_STATIC_INLINE(2, 74)
#define XLIB_AVAILABLE_MACRO_IN_2_76                                XLIB_UNAVAILABLE_MACRO(2, 74)
#define XLIB_AVAILABLE_ENUMERATOR_IN_2_76                           XLIB_UNAVAILABLE_ENUMERATOR(2, 74)
#define XLIB_AVAILABLE_TYPE_IN_2_76                                 XLIB_UNAVAILABLE_TYPE(2, 74)
#else
#define XLIB_AVAILABLE_IN_2_76                                      _XLIB_EXTERN
#define XLIB_AVAILABLE_STATIC_INLINE_IN_2_76
#define XLIB_AVAILABLE_MACRO_IN_2_76
#define XLIB_AVAILABLE_ENUMERATOR_IN_2_76
#define XLIB_AVAILABLE_TYPE_IN_2_76
#endif

#if XLIB_VERSION_MIN_REQUIRED >= XLIB_VERSION_2_78
#define XLIB_DEPRECATED_IN_2_78                                     XLIB_DEPRECATED
#define XLIB_DEPRECATED_IN_2_78_FOR(f)                              XLIB_DEPRECATED_FOR(f)
#define XLIB_DEPRECATED_MACRO_IN_2_78                               XLIB_DEPRECATED_MACRO
#define XLIB_DEPRECATED_MACRO_IN_2_78_FOR(f)                        XLIB_DEPRECATED_MACRO_FOR(f)
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_78                          XLIB_DEPRECATED_ENUMERATOR
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_78_FOR(f)                   XLIB_DEPRECATED_ENUMERATOR_FOR(f)
#define XLIB_DEPRECATED_TYPE_IN_2_78                                XLIB_DEPRECATED_TYPE
#define XLIB_DEPRECATED_TYPE_IN_2_78_FOR(f)                         XLIB_DEPRECATED_TYPE_FOR(f)
#else
#define XLIB_DEPRECATED_IN_2_78                                     _XLIB_EXTERN
#define XLIB_DEPRECATED_IN_2_78_FOR(f)                              _XLIB_EXTERN
#define XLIB_DEPRECATED_MACRO_IN_2_78
#define XLIB_DEPRECATED_MACRO_IN_2_78_FOR(f)
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_78
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_78_FOR(f)
#define XLIB_DEPRECATED_TYPE_IN_2_78
#define XLIB_DEPRECATED_TYPE_IN_2_78_FOR(f)
#endif

#if XLIB_VERSION_MAX_ALLOWED < XLIB_VERSION_2_78
#define XLIB_AVAILABLE_IN_2_78                                      XLIB_UNAVAILABLE(2, 78)
#define XLIB_AVAILABLE_STATIC_INLINE_IN_2_78                        XLIB_UNAVAILABLE_STATIC_INLINE(2, 78)
#define XLIB_AVAILABLE_MACRO_IN_2_78                                XLIB_UNAVAILABLE_MACRO(2, 78)
#define XLIB_AVAILABLE_ENUMERATOR_IN_2_78                           XLIB_UNAVAILABLE_ENUMERATOR(2, 78)
#define XLIB_AVAILABLE_TYPE_IN_2_78                                 XLIB_UNAVAILABLE_TYPE(2, 78)
#else
#define XLIB_AVAILABLE_IN_2_78                                      _XLIB_EXTERN
#define XLIB_AVAILABLE_STATIC_INLINE_IN_2_78
#define XLIB_AVAILABLE_MACRO_IN_2_78
#define XLIB_AVAILABLE_ENUMERATOR_IN_2_78
#define XLIB_AVAILABLE_TYPE_IN_2_78
#endif

#if XLIB_VERSION_MIN_REQUIRED >= XLIB_VERSION_2_80
#define XLIB_DEPRECATED_IN_2_80                                     XLIB_DEPRECATED
#define XLIB_DEPRECATED_IN_2_80_FOR(f)                              XLIB_DEPRECATED_FOR(f)
#define XLIB_DEPRECATED_MACRO_IN_2_80                               XLIB_DEPRECATED_MACRO
#define XLIB_DEPRECATED_MACRO_IN_2_80_FOR(f)                        XLIB_DEPRECATED_MACRO_FOR(f)
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_80                          XLIB_DEPRECATED_ENUMERATOR
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_80_FOR(f)                   XLIB_DEPRECATED_ENUMERATOR_FOR(f)
#define XLIB_DEPRECATED_TYPE_IN_2_80                                XLIB_DEPRECATED_TYPE
#define XLIB_DEPRECATED_TYPE_IN_2_80_FOR(f)                         XLIB_DEPRECATED_TYPE_FOR(f)
#else
#define XLIB_DEPRECATED_IN_2_80                                     _XLIB_EXTERN
#define XLIB_DEPRECATED_IN_2_80_FOR(f)                              _XLIB_EXTERN
#define XLIB_DEPRECATED_MACRO_IN_2_80
#define XLIB_DEPRECATED_MACRO_IN_2_80_FOR(f)
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_80
#define XLIB_DEPRECATED_ENUMERATOR_IN_2_80_FOR(f)
#define XLIB_DEPRECATED_TYPE_IN_2_80
#define XLIB_DEPRECATED_TYPE_IN_2_80_FOR(f)
#endif

#if XLIB_VERSION_MAX_ALLOWED < XLIB_VERSION_2_80
#define XLIB_AVAILABLE_IN_2_80                                      XLIB_UNAVAILABLE(2, 80)
#define XLIB_AVAILABLE_STATIC_INLINE_IN_2_80                        XLIB_UNAVAILABLE_STATIC_INLINE(2, 80)
#define XLIB_AVAILABLE_MACRO_IN_2_80                                XLIB_UNAVAILABLE_MACRO(2, 80)
#define XLIB_AVAILABLE_ENUMERATOR_IN_2_80                           XLIB_UNAVAILABLE_ENUMERATOR(2, 80)
#define XLIB_AVAILABLE_TYPE_IN_2_80                                 XLIB_UNAVAILABLE_TYPE(2, 80)
#else
#define XLIB_AVAILABLE_IN_2_80                                      _XLIB_EXTERN
#define XLIB_AVAILABLE_STATIC_INLINE_IN_2_80
#define XLIB_AVAILABLE_MACRO_IN_2_80
#define XLIB_AVAILABLE_ENUMERATOR_IN_2_80
#define XLIB_AVAILABLE_TYPE_IN_2_80
#endif

#endif
