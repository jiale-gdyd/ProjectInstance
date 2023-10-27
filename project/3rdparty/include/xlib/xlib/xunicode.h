#ifndef __X_UNICODE_H__
#define __X_UNICODE_H__

#include "xerror.h"
#include "xtypes.h"

X_BEGIN_DECLS

typedef xuint32 xunichar;
typedef xuint16 xunichar2;

typedef enum {
    X_UNICODE_CONTROL,
    X_UNICODE_FORMAT,
    X_UNICODE_UNASSIGNED,
    X_UNICODE_PRIVATE_USE,
    X_UNICODE_SURROGATE,
    X_UNICODE_LOWERCASE_LETTER,
    X_UNICODE_MODIFIER_LETTER,
    X_UNICODE_OTHER_LETTER,
    X_UNICODE_TITLECASE_LETTER,
    X_UNICODE_UPPERCASE_LETTER,
    X_UNICODE_SPACING_MARK,
    X_UNICODE_ENCLOSING_MARK,
    X_UNICODE_NON_SPACING_MARK,
    X_UNICODE_DECIMAL_NUMBER,
    X_UNICODE_LETTER_NUMBER,
    X_UNICODE_OTHER_NUMBER,
    X_UNICODE_CONNECT_PUNCTUATION,
    X_UNICODE_DASH_PUNCTUATION,
    X_UNICODE_CLOSE_PUNCTUATION,
    X_UNICODE_FINAL_PUNCTUATION,
    X_UNICODE_INITIAL_PUNCTUATION,
    X_UNICODE_OTHER_PUNCTUATION,
    X_UNICODE_OPEN_PUNCTUATION,
    X_UNICODE_CURRENCY_SYMBOL,
    X_UNICODE_MODIFIER_SYMBOL,
    X_UNICODE_MATH_SYMBOL,
    X_UNICODE_OTHER_SYMBOL,
    X_UNICODE_LINE_SEPARATOR,
    X_UNICODE_PARAGRAPH_SEPARATOR,
    X_UNICODE_SPACE_SEPARATOR
} XUnicodeType;

#define X_UNICODE_COMBINING_MARK                    X_UNICODE_SPACING_MARK XLIB_DEPRECATED_MACRO_IN_2_30_FOR(X_UNICODE_SPACING_MARK)

typedef enum {
    X_UNICODE_BREAK_MANDATORY,
    X_UNICODE_BREAK_CARRIAGE_RETURN,
    X_UNICODE_BREAK_LINE_FEED,
    X_UNICODE_BREAK_COMBINING_MARK,
    X_UNICODE_BREAK_SURROGATE,
    X_UNICODE_BREAK_ZERO_WIDTH_SPACE,
    X_UNICODE_BREAK_INSEPARABLE,
    X_UNICODE_BREAK_NON_BREAKING_GLUE,
    X_UNICODE_BREAK_CONTINGENT,
    X_UNICODE_BREAK_SPACE,
    X_UNICODE_BREAK_AFTER,
    X_UNICODE_BREAK_BEFORE,
    X_UNICODE_BREAK_BEFORE_AND_AFTER,
    X_UNICODE_BREAK_HYPHEN,
    X_UNICODE_BREAK_NON_STARTER,
    X_UNICODE_BREAK_OPEN_PUNCTUATION,
    X_UNICODE_BREAK_CLOSE_PUNCTUATION,
    X_UNICODE_BREAK_QUOTATION,
    X_UNICODE_BREAK_EXCLAMATION,
    X_UNICODE_BREAK_IDEOGRAPHIC,
    X_UNICODE_BREAK_NUMERIC,
    X_UNICODE_BREAK_INFIX_SEPARATOR,
    X_UNICODE_BREAK_SYMBOL,
    X_UNICODE_BREAK_ALPHABETIC,
    X_UNICODE_BREAK_PREFIX,
    X_UNICODE_BREAK_POSTFIX,
    X_UNICODE_BREAK_COMPLEX_CONTEXT,
    X_UNICODE_BREAK_AMBIGUOUS,
    X_UNICODE_BREAK_UNKNOWN,
    X_UNICODE_BREAK_NEXT_LINE,
    X_UNICODE_BREAK_WORD_JOINER,
    X_UNICODE_BREAK_HANGUL_L_JAMO,
    X_UNICODE_BREAK_HANGUL_V_JAMO,
    X_UNICODE_BREAK_HANGUL_T_JAMO,
    X_UNICODE_BREAK_HANGUL_LV_SYLLABLE,
    X_UNICODE_BREAK_HANGUL_LVT_SYLLABLE,
    X_UNICODE_BREAK_CLOSE_PARANTHESIS,
    X_UNICODE_BREAK_CLOSE_PARENTHESIS XLIB_AVAILABLE_ENUMERATOR_IN_2_70 = X_UNICODE_BREAK_CLOSE_PARANTHESIS,
    X_UNICODE_BREAK_CONDITIONAL_JAPANESE_STARTER,
    X_UNICODE_BREAK_HEBREW_LETTER,
    X_UNICODE_BREAK_REGIONAL_INDICATOR,
    X_UNICODE_BREAK_EMOJI_BASE,
    X_UNICODE_BREAK_EMOJI_MODIFIER,
    X_UNICODE_BREAK_ZERO_WIDTH_JOINER,
    X_UNICODE_BREAK_AKSARA,
    X_UNICODE_BREAK_AKSARA_PRE_BASE,
    X_UNICODE_BREAK_AKSARA_START,
    X_UNICODE_BREAK_VIRAMA_FINAL,
    X_UNICODE_BREAK_VIRAMA
} XUnicodeBreakType;

typedef enum {
    X_UNICODE_SCRIPT_INVALID_CODE = -1,
    X_UNICODE_SCRIPT_COMMON       = 0,   /* Zyyy */
    X_UNICODE_SCRIPT_INHERITED,          /* Zinh (Qaai) */
    X_UNICODE_SCRIPT_ARABIC,             /* Arab */
    X_UNICODE_SCRIPT_ARMENIAN,           /* Armn */
    X_UNICODE_SCRIPT_BENGALI,            /* Beng */
    X_UNICODE_SCRIPT_BOPOMOFO,           /* Bopo */
    X_UNICODE_SCRIPT_CHEROKEE,           /* Cher */
    X_UNICODE_SCRIPT_COPTIC,             /* Copt (Qaac) */
    X_UNICODE_SCRIPT_CYRILLIC,           /* Cyrl (Cyrs) */
    X_UNICODE_SCRIPT_DESERET,            /* Dsrt */
    X_UNICODE_SCRIPT_DEVANAGARI,         /* Deva */
    X_UNICODE_SCRIPT_ETHIOPIC,           /* Ethi */
    X_UNICODE_SCRIPT_GEORGIAN,           /* Geor (Geon, Geoa) */
    X_UNICODE_SCRIPT_GOTHIC,             /* Goth */
    X_UNICODE_SCRIPT_GREEK,              /* Grek */
    X_UNICODE_SCRIPT_GUJARATI,           /* Gujr */
    X_UNICODE_SCRIPT_GURMUKHI,           /* Guru */
    X_UNICODE_SCRIPT_HAN,                /* Hani */
    X_UNICODE_SCRIPT_HANGUL,             /* Hang */
    X_UNICODE_SCRIPT_HEBREW,             /* Hebr */
    X_UNICODE_SCRIPT_HIRAGANA,           /* Hira */
    X_UNICODE_SCRIPT_KANNADA,            /* Knda */
    X_UNICODE_SCRIPT_KATAKANA,           /* Kana */
    X_UNICODE_SCRIPT_KHMER,              /* Khmr */
    X_UNICODE_SCRIPT_LAO,                /* Laoo */
    X_UNICODE_SCRIPT_LATIN,              /* Latn (Latf, Latg) */
    X_UNICODE_SCRIPT_MALAYALAM,          /* Mlym */
    X_UNICODE_SCRIPT_MONGOLIAN,          /* Mong */
    X_UNICODE_SCRIPT_MYANMAR,            /* Mymr */
    X_UNICODE_SCRIPT_OGHAM,              /* Ogam */
    X_UNICODE_SCRIPT_OLD_ITALIC,         /* Ital */
    X_UNICODE_SCRIPT_ORIYA,              /* Orya */
    X_UNICODE_SCRIPT_RUNIC,              /* Runr */
    X_UNICODE_SCRIPT_SINHALA,            /* Sinh */
    X_UNICODE_SCRIPT_SYRIAC,             /* Syrc (Syrj, Syrn, Syre) */
    X_UNICODE_SCRIPT_TAMIL,              /* Taml */
    X_UNICODE_SCRIPT_TELUGU,             /* Telu */
    X_UNICODE_SCRIPT_THAANA,             /* Thaa */
    X_UNICODE_SCRIPT_THAI,               /* Thai */
    X_UNICODE_SCRIPT_TIBETAN,            /* Tibt */
    X_UNICODE_SCRIPT_CANADIAN_ABORIGINAL, /* Cans */
    X_UNICODE_SCRIPT_YI,                 /* Yiii */
    X_UNICODE_SCRIPT_TAGALOG,            /* Tglg */
    X_UNICODE_SCRIPT_HANUNOO,            /* Hano */
    X_UNICODE_SCRIPT_BUHID,              /* Buhd */
    X_UNICODE_SCRIPT_TAGBANWA,           /* Tagb */

    /* Unicode-4.0 additions */
    X_UNICODE_SCRIPT_BRAILLE,            /* Brai */
    X_UNICODE_SCRIPT_CYPRIOT,            /* Cprt */
    X_UNICODE_SCRIPT_LIMBU,              /* Limb */
    X_UNICODE_SCRIPT_OSMANYA,            /* Osma */
    X_UNICODE_SCRIPT_SHAVIAN,            /* Shaw */
    X_UNICODE_SCRIPT_LINEAR_B,           /* Linb */
    X_UNICODE_SCRIPT_TAI_LE,             /* Tale */
    X_UNICODE_SCRIPT_UGARITIC,           /* Ugar */

    /* Unicode-4.1 additions */
    X_UNICODE_SCRIPT_NEW_TAI_LUE,        /* Talu */
    X_UNICODE_SCRIPT_BUGINESE,           /* Bugi */
    X_UNICODE_SCRIPT_GLAGOLITIC,         /* Glag */
    X_UNICODE_SCRIPT_TIFINAGH,           /* Tfng */
    X_UNICODE_SCRIPT_SYLOTI_NAGRI,       /* Sylo */
    X_UNICODE_SCRIPT_OLD_PERSIAN,        /* Xpeo */
    X_UNICODE_SCRIPT_KHAROSHTHI,         /* Khar */

    /* Unicode-5.0 additions */
    X_UNICODE_SCRIPT_UNKNOWN,            /* Zzzz */
    X_UNICODE_SCRIPT_BALINESE,           /* Bali */
    X_UNICODE_SCRIPT_CUNEIFORM,          /* Xsux */
    X_UNICODE_SCRIPT_PHOENICIAN,         /* Phnx */
    X_UNICODE_SCRIPT_PHAGS_PA,           /* Phag */
    X_UNICODE_SCRIPT_NKO,                /* Nkoo */

    /* Unicode-5.1 additions */
    X_UNICODE_SCRIPT_KAYAH_LI,           /* Kali */
    X_UNICODE_SCRIPT_LEPCHA,             /* Lepc */
    X_UNICODE_SCRIPT_REJANG,             /* Rjng */
    X_UNICODE_SCRIPT_SUNDANESE,          /* Sund */
    X_UNICODE_SCRIPT_SAURASHTRA,         /* Saur */
    X_UNICODE_SCRIPT_CHAM,               /* Cham */
    X_UNICODE_SCRIPT_OL_CHIKI,           /* Olck */
    X_UNICODE_SCRIPT_VAI,                /* Vaii */
    X_UNICODE_SCRIPT_CARIAN,             /* Cari */
    X_UNICODE_SCRIPT_LYCIAN,             /* Lyci */
    X_UNICODE_SCRIPT_LYDIAN,             /* Lydi */

    /* Unicode-5.2 additions */
    X_UNICODE_SCRIPT_AVESTAN,                /* Avst */
    X_UNICODE_SCRIPT_BAMUM,                  /* Bamu */
    X_UNICODE_SCRIPT_EGYPTIAN_HIEROGLYPHS,   /* Egyp */
    X_UNICODE_SCRIPT_IMPERIAL_ARAMAIC,       /* Armi */
    X_UNICODE_SCRIPT_INSCRIPTIONAL_PAHLAVI,  /* Phli */
    X_UNICODE_SCRIPT_INSCRIPTIONAL_PARTHIAN, /* Prti */
    X_UNICODE_SCRIPT_JAVANESE,               /* Java */
    X_UNICODE_SCRIPT_KAITHI,                 /* Kthi */
    X_UNICODE_SCRIPT_LISU,                   /* Lisu */
    X_UNICODE_SCRIPT_MEETEI_MAYEK,           /* Mtei */
    X_UNICODE_SCRIPT_OLD_SOUTH_ARABIAN,      /* Sarb */
    X_UNICODE_SCRIPT_OLD_TURKIC,             /* Orkh */
    X_UNICODE_SCRIPT_SAMARITAN,              /* Samr */
    X_UNICODE_SCRIPT_TAI_THAM,               /* Lana */
    X_UNICODE_SCRIPT_TAI_VIET,               /* Tavt */

    /* Unicode-6.0 additions */
    X_UNICODE_SCRIPT_BATAK,                  /* Batk */
    X_UNICODE_SCRIPT_BRAHMI,                 /* Brah */
    X_UNICODE_SCRIPT_MANDAIC,                /* Mand */

    /* Unicode-6.1 additions */
    X_UNICODE_SCRIPT_CHAKMA,                 /* Cakm */
    X_UNICODE_SCRIPT_MEROITIC_CURSIVE,       /* Merc */
    X_UNICODE_SCRIPT_MEROITIC_HIEROGLYPHS,   /* Mero */
    X_UNICODE_SCRIPT_MIAO,                   /* Plrd */
    X_UNICODE_SCRIPT_SHARADA,                /* Shrd */
    X_UNICODE_SCRIPT_SORA_SOMPENG,           /* Sora */
    X_UNICODE_SCRIPT_TAKRI,                  /* Takr */

    /* Unicode 7.0 additions */
    X_UNICODE_SCRIPT_BASSA_VAH,              /* Bass */
    X_UNICODE_SCRIPT_CAUCASIAN_ALBANIAN,     /* Aghb */
    X_UNICODE_SCRIPT_DUPLOYAN,               /* Dupl */
    X_UNICODE_SCRIPT_ELBASAN,                /* Elba */
    X_UNICODE_SCRIPT_GRANTHA,                /* Gran */
    X_UNICODE_SCRIPT_KHOJKI,                 /* Khoj */
    X_UNICODE_SCRIPT_KHUDAWADI,              /* Sind */
    X_UNICODE_SCRIPT_LINEAR_A,               /* Lina */
    X_UNICODE_SCRIPT_MAHAJANI,               /* Mahj */
    X_UNICODE_SCRIPT_MANICHAEAN,             /* Mani */
    X_UNICODE_SCRIPT_MENDE_KIKAKUI,          /* Mend */
    X_UNICODE_SCRIPT_MODI,                   /* Modi */
    X_UNICODE_SCRIPT_MRO,                    /* Mroo */
    X_UNICODE_SCRIPT_NABATAEAN,              /* Nbat */
    X_UNICODE_SCRIPT_OLD_NORTH_ARABIAN,      /* Narb */
    X_UNICODE_SCRIPT_OLD_PERMIC,             /* Perm */
    X_UNICODE_SCRIPT_PAHAWH_HMONG,           /* Hmng */
    X_UNICODE_SCRIPT_PALMYRENE,              /* Palm */
    X_UNICODE_SCRIPT_PAU_CIN_HAU,            /* Pauc */
    X_UNICODE_SCRIPT_PSALTER_PAHLAVI,        /* Phlp */
    X_UNICODE_SCRIPT_SIDDHAM,                /* Sidd */
    X_UNICODE_SCRIPT_TIRHUTA,                /* Tirh */
    X_UNICODE_SCRIPT_WARANG_CITI,            /* Wara */

    /* Unicode 8.0 additions */
    X_UNICODE_SCRIPT_AHOM,                   /* Ahom */
    X_UNICODE_SCRIPT_ANATOLIAN_HIEROGLYPHS,  /* Hluw */
    X_UNICODE_SCRIPT_HATRAN,                 /* Hatr */
    X_UNICODE_SCRIPT_MULTANI,                /* Mult */
    X_UNICODE_SCRIPT_OLD_HUNGARIAN,          /* Hung */
    X_UNICODE_SCRIPT_SIGNWRITING,            /* Sgnw */

    /* Unicode 9.0 additions */
    X_UNICODE_SCRIPT_ADLAM,                  /* Adlm */
    X_UNICODE_SCRIPT_BHAIKSUKI,              /* Bhks */
    X_UNICODE_SCRIPT_MARCHEN,                /* Marc */
    X_UNICODE_SCRIPT_NEWA,                   /* Newa */
    X_UNICODE_SCRIPT_OSAGE,                  /* Osge */
    X_UNICODE_SCRIPT_TANGUT,                 /* Tang */

    /* Unicode 10.0 additions */
    X_UNICODE_SCRIPT_MASARAM_GONDI,          /* Gonm */
    X_UNICODE_SCRIPT_NUSHU,                  /* Nshu */
    X_UNICODE_SCRIPT_SOYOMBO,                /* Soyo */
    X_UNICODE_SCRIPT_ZANABAZAR_SQUARE,       /* Zanb */

    /* Unicode 11.0 additions */
    X_UNICODE_SCRIPT_DOGRA,                  /* Dogr */
    X_UNICODE_SCRIPT_GUNJALA_GONDI,          /* Gong */
    X_UNICODE_SCRIPT_HANIFI_ROHINGYA,        /* Rohg */
    X_UNICODE_SCRIPT_MAKASAR,                /* Maka */
    X_UNICODE_SCRIPT_MEDEFAIDRIN,            /* Medf */
    X_UNICODE_SCRIPT_OLD_SOGDIAN,            /* Sogo */
    X_UNICODE_SCRIPT_SOGDIAN,                /* Sogd */

    /* Unicode 12.0 additions */
    X_UNICODE_SCRIPT_ELYMAIC,                /* Elym */
    X_UNICODE_SCRIPT_NANDINAGARI,            /* Nand */
    X_UNICODE_SCRIPT_NYIAKENG_PUACHUE_HMONG, /* Rohg */
    X_UNICODE_SCRIPT_WANCHO,                 /* Wcho */

    /* Unicode 13.0 additions */
    X_UNICODE_SCRIPT_CHORASMIAN,             /* Chrs */
    X_UNICODE_SCRIPT_DIVES_AKURU,            /* Diak */
    X_UNICODE_SCRIPT_KHITAN_SMALL_SCRIPT,    /* Kits */
    X_UNICODE_SCRIPT_YEZIDI,                 /* Yezi */

    /* Unicode 14.0 additions */
    X_UNICODE_SCRIPT_CYPRO_MINOAN,           /* Cpmn */
    X_UNICODE_SCRIPT_OLD_UYGHUR,             /* Ougr */
    X_UNICODE_SCRIPT_TANGSA,                 /* Tnsa */
    X_UNICODE_SCRIPT_TOTO,                   /* Toto */
    X_UNICODE_SCRIPT_VITHKUQI,               /* Vith */

    /* not really a Unicode script, but part of ISO 15924 */
    X_UNICODE_SCRIPT_MATH,                   /* Zmth */

    /* Unicode 15.0 additions */
    X_UNICODE_SCRIPT_KAWI XLIB_AVAILABLE_ENUMERATOR_IN_2_74,          /* Kawi */
    X_UNICODE_SCRIPT_NAG_MUNDARI XLIB_AVAILABLE_ENUMERATOR_IN_2_74,   /* Nag Mundari */
} XUnicodeScript;

XLIB_AVAILABLE_IN_ALL
xuint32 x_unicode_script_to_iso15924(XUnicodeScript script);

XLIB_AVAILABLE_IN_ALL
XUnicodeScript x_unicode_script_from_iso15924(xuint32 iso15924);

XLIB_AVAILABLE_IN_ALL
xboolean x_unichar_isalnum(xunichar c) X_GNUC_CONST;

XLIB_AVAILABLE_IN_ALL
xboolean x_unichar_isalpha(xunichar c) X_GNUC_CONST;

XLIB_AVAILABLE_IN_ALL
xboolean x_unichar_iscntrl(xunichar c) X_GNUC_CONST;

XLIB_AVAILABLE_IN_ALL
xboolean x_unichar_isdigit(xunichar c) X_GNUC_CONST;

XLIB_AVAILABLE_IN_ALL
xboolean x_unichar_isgraph(xunichar c) X_GNUC_CONST;

XLIB_AVAILABLE_IN_ALL
xboolean x_unichar_islower(xunichar c) X_GNUC_CONST;

XLIB_AVAILABLE_IN_ALL
xboolean x_unichar_isprint(xunichar c) X_GNUC_CONST;

XLIB_AVAILABLE_IN_ALL
xboolean x_unichar_ispunct(xunichar c) X_GNUC_CONST;

XLIB_AVAILABLE_IN_ALL
xboolean x_unichar_isspace(xunichar c) X_GNUC_CONST;

XLIB_AVAILABLE_IN_ALL
xboolean x_unichar_isupper(xunichar c) X_GNUC_CONST;

XLIB_AVAILABLE_IN_ALL
xboolean x_unichar_isxdigit(xunichar c) X_GNUC_CONST;

XLIB_AVAILABLE_IN_ALL
xboolean x_unichar_istitle(xunichar c) X_GNUC_CONST;

XLIB_AVAILABLE_IN_ALL
xboolean x_unichar_isdefined(xunichar c) X_GNUC_CONST;

XLIB_AVAILABLE_IN_ALL
xboolean x_unichar_iswide(xunichar c) X_GNUC_CONST;

XLIB_AVAILABLE_IN_ALL
xboolean x_unichar_iswide_cjk(xunichar c) X_GNUC_CONST;

XLIB_AVAILABLE_IN_ALL
xboolean x_unichar_iszerowidth(xunichar c) X_GNUC_CONST;

XLIB_AVAILABLE_IN_ALL
xboolean x_unichar_ismark(xunichar c) X_GNUC_CONST;

XLIB_AVAILABLE_IN_ALL
xunichar x_unichar_toupper (xunichar c) X_GNUC_CONST;

XLIB_AVAILABLE_IN_ALL
xunichar x_unichar_tolower (xunichar c) X_GNUC_CONST;

XLIB_AVAILABLE_IN_ALL
xunichar x_unichar_totitle (xunichar c) X_GNUC_CONST;

XLIB_AVAILABLE_IN_ALL
xint x_unichar_digit_value(xunichar c) X_GNUC_CONST;

XLIB_AVAILABLE_IN_ALL
xint x_unichar_xdigit_value(xunichar c) X_GNUC_CONST;

XLIB_AVAILABLE_IN_ALL
XUnicodeType x_unichar_type(xunichar c) X_GNUC_CONST;

XLIB_AVAILABLE_IN_ALL
XUnicodeBreakType x_unichar_break_type(xunichar c) X_GNUC_CONST;

XLIB_AVAILABLE_IN_ALL
xint x_unichar_combining_class(xunichar uc) X_GNUC_CONST;

XLIB_AVAILABLE_IN_ALL
xboolean x_unichar_get_mirror_char(xunichar ch, xunichar *mirrored_ch);

XLIB_AVAILABLE_IN_ALL
XUnicodeScript x_unichar_get_script(xunichar ch) X_GNUC_CONST;

XLIB_AVAILABLE_IN_ALL
xboolean x_unichar_validate(xunichar ch) X_GNUC_CONST;

XLIB_AVAILABLE_IN_ALL
xboolean x_unichar_compose(xunichar a, xunichar b, xunichar *ch);

XLIB_AVAILABLE_IN_ALL
xboolean x_unichar_decompose (xunichar ch, xunichar *a, xunichar *b);

XLIB_AVAILABLE_IN_ALL
xsize x_unichar_fully_decompose(xunichar ch, xboolean compat, xunichar *result, xsize result_len);

#define X_UNICHAR_MAX_DECOMPOSITION_LENGTH              18

XLIB_AVAILABLE_IN_ALL
void x_unicode_canonical_ordering(xunichar *string, xsize len);

XLIB_DEPRECATED_IN_2_30
xunichar *x_unicode_canonical_decomposition(xunichar ch, xsize *result_len) X_GNUC_MALLOC;

XLIB_VAR const xchar *const x_utf8_skip;

#define x_utf8_next_char(p)                             (char *)((p) + x_utf8_skip[*(const xuchar *)(p)])

XLIB_AVAILABLE_IN_ALL
xunichar x_utf8_get_char(const xchar *p) X_GNUC_PURE;

XLIB_AVAILABLE_IN_ALL
xunichar x_utf8_get_char_validated(const xchar *p, xssize max_len) X_GNUC_PURE;

XLIB_AVAILABLE_IN_ALL
xchar *x_utf8_offset_to_pointer(const xchar *str, xlong offset) X_GNUC_PURE;

XLIB_AVAILABLE_IN_ALL
xlong x_utf8_pointer_to_offset(const xchar *str, const xchar *pos) X_GNUC_PURE;

XLIB_AVAILABLE_IN_ALL
xchar *x_utf8_prev_char(const xchar *p) X_GNUC_PURE;

XLIB_AVAILABLE_IN_ALL
xchar *x_utf8_find_next_char(const xchar *p, const xchar *end) X_GNUC_PURE;

XLIB_AVAILABLE_IN_ALL
xchar *x_utf8_find_prev_char(const xchar *str, const xchar *p) X_GNUC_PURE;

XLIB_AVAILABLE_IN_ALL
xlong x_utf8_strlen(const xchar *p, xssize max) X_GNUC_PURE;

XLIB_AVAILABLE_IN_2_30
xchar *x_utf8_substring(const xchar *str, xlong start_pos, xlong end_pos) X_GNUC_MALLOC;

XLIB_AVAILABLE_IN_ALL
xchar *x_utf8_strncpy(xchar *dest, const xchar *src, xsize n);

XLIB_AVAILABLE_IN_2_78
xchar *x_utf8_truncate_middle(const xchar *string, xsize truncate_length);

XLIB_AVAILABLE_IN_ALL
xchar *x_utf8_strchr(const xchar *p, xssize len, xunichar c);

XLIB_AVAILABLE_IN_ALL
xchar *x_utf8_strrchr(const xchar *p, xssize len, xunichar c);

XLIB_AVAILABLE_IN_ALL
xchar *x_utf8_strreverse(const xchar *str, xssize len);

XLIB_AVAILABLE_IN_ALL
xunichar2 *x_utf8_to_utf16(const xchar *str, xlong len, xlong *items_read, xlong *items_written, XError **error) X_GNUC_MALLOC;

XLIB_AVAILABLE_IN_ALL
xunichar *x_utf8_to_ucs4(const xchar *str, xlong len, xlong *items_read, xlong *items_written, XError **error) X_GNUC_MALLOC;

XLIB_AVAILABLE_IN_ALL
xunichar *x_utf8_to_ucs4_fast(const xchar *str, xlong len, xlong *items_written) X_GNUC_MALLOC;

XLIB_AVAILABLE_IN_ALL
xunichar *x_utf16_to_ucs4(const xunichar2 *str, xlong len, xlong *items_read, xlong *items_written, XError **error) X_GNUC_MALLOC;

XLIB_AVAILABLE_IN_ALL
xchar *x_utf16_to_utf8(const xunichar2 *str, xlong len, xlong *items_read, xlong *items_written, XError **error) X_GNUC_MALLOC;

XLIB_AVAILABLE_IN_ALL
xunichar2 *x_ucs4_to_utf16(const xunichar *str, xlong len, xlong *items_read, xlong *items_written, XError **error) X_GNUC_MALLOC;

XLIB_AVAILABLE_IN_ALL
xchar *x_ucs4_to_utf8(const xunichar *str, xlong len, xlong *items_read, xlong *items_written, XError **error) X_GNUC_MALLOC;

XLIB_AVAILABLE_IN_ALL
xint x_unichar_to_utf8(xunichar c, xchar *outbuf);

XLIB_AVAILABLE_IN_ALL
xboolean x_utf8_validate(const xchar *str, xssize max_len, const xchar **end);

XLIB_AVAILABLE_IN_2_60
xboolean x_utf8_validate_len(const xchar *str, xsize max_len, const xchar **end);

XLIB_AVAILABLE_IN_ALL
xchar *x_utf8_strup(const xchar *str, xssize len) X_GNUC_MALLOC;

XLIB_AVAILABLE_IN_ALL
xchar *x_utf8_strdown(const xchar *str, xssize len) X_GNUC_MALLOC;

XLIB_AVAILABLE_IN_ALL
xchar *x_utf8_casefold(const xchar *str, xssize len) X_GNUC_MALLOC;

typedef enum {
    X_NORMALIZE_DEFAULT,
    X_NORMALIZE_NFD = X_NORMALIZE_DEFAULT,
    X_NORMALIZE_DEFAULT_COMPOSE,
    X_NORMALIZE_NFC = X_NORMALIZE_DEFAULT_COMPOSE,
    X_NORMALIZE_ALL,
    X_NORMALIZE_NFKD = X_NORMALIZE_ALL,
    X_NORMALIZE_ALL_COMPOSE,
    X_NORMALIZE_NFKC = X_NORMALIZE_ALL_COMPOSE
} XNormalizeMode;

XLIB_AVAILABLE_IN_ALL
xchar *x_utf8_normalize(const xchar *str, xssize len, XNormalizeMode mode) X_GNUC_MALLOC;

XLIB_AVAILABLE_IN_ALL
xint x_utf8_collate(const xchar *str1, const xchar *str2) X_GNUC_PURE;

XLIB_AVAILABLE_IN_ALL
xchar *x_utf8_collate_key(const xchar *str, xssize len) X_GNUC_MALLOC;

XLIB_AVAILABLE_IN_ALL
xchar *x_utf8_collate_key_for_filename(const xchar *str, xssize len) X_GNUC_MALLOC;

XLIB_AVAILABLE_IN_2_52
xchar *x_utf8_make_valid(const xchar *str, xssize len) X_GNUC_MALLOC;

X_END_DECLS

#endif
