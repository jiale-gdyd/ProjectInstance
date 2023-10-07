#include <xlib/xlib/config.h>
#include <xlib/xlib/xlib-object.h>
#include <xlib/xobj/xlib-enumtypes.h>

X_GNUC_BEGIN_IGNORE_DEPRECATIONS

XType x_unicode_type_get_type(void)
{
    static XType static_x_define_type_id = 0;

    if (x_once_init_enter_pointer(&static_x_define_type_id)) {
        static const XEnumValue values[] = {
            { X_UNICODE_CONTROL, "X_UNICODE_CONTROL", "control" },
            { X_UNICODE_FORMAT, "X_UNICODE_FORMAT", "format" },
            { X_UNICODE_UNASSIGNED, "X_UNICODE_UNASSIGNED", "unassigned" },
            { X_UNICODE_PRIVATE_USE, "X_UNICODE_PRIVATE_USE", "private-use" },
            { X_UNICODE_SURROGATE, "X_UNICODE_SURROGATE", "surrogate" },
            { X_UNICODE_LOWERCASE_LETTER, "X_UNICODE_LOWERCASE_LETTER", "lowercase-letter" },
            { X_UNICODE_MODIFIER_LETTER, "X_UNICODE_MODIFIER_LETTER", "modifier-letter" },
            { X_UNICODE_OTHER_LETTER, "X_UNICODE_OTHER_LETTER", "other-letter" },
            { X_UNICODE_TITLECASE_LETTER, "X_UNICODE_TITLECASE_LETTER", "titlecase-letter" },
            { X_UNICODE_UPPERCASE_LETTER, "X_UNICODE_UPPERCASE_LETTER", "uppercase-letter" },
            { X_UNICODE_SPACING_MARK, "X_UNICODE_SPACING_MARK", "spacing-mark" },
            { X_UNICODE_ENCLOSING_MARK, "X_UNICODE_ENCLOSING_MARK", "enclosing-mark" },
            { X_UNICODE_NON_SPACING_MARK, "X_UNICODE_NON_SPACING_MARK", "non-spacing-mark" },
            { X_UNICODE_DECIMAL_NUMBER, "X_UNICODE_DECIMAL_NUMBER", "decimal-number" },
            { X_UNICODE_LETTER_NUMBER, "X_UNICODE_LETTER_NUMBER", "letter-number" },
            { X_UNICODE_OTHER_NUMBER, "X_UNICODE_OTHER_NUMBER", "other-number" },
            { X_UNICODE_CONNECT_PUNCTUATION, "X_UNICODE_CONNECT_PUNCTUATION", "connect-punctuation" },
            { X_UNICODE_DASH_PUNCTUATION, "X_UNICODE_DASH_PUNCTUATION", "dash-punctuation" },
            { X_UNICODE_CLOSE_PUNCTUATION, "X_UNICODE_CLOSE_PUNCTUATION", "close-punctuation" },
            { X_UNICODE_FINAL_PUNCTUATION, "X_UNICODE_FINAL_PUNCTUATION", "final-punctuation" },
            { X_UNICODE_INITIAL_PUNCTUATION, "X_UNICODE_INITIAL_PUNCTUATION", "initial-punctuation" },
            { X_UNICODE_OTHER_PUNCTUATION, "X_UNICODE_OTHER_PUNCTUATION", "other-punctuation" },
            { X_UNICODE_OPEN_PUNCTUATION, "X_UNICODE_OPEN_PUNCTUATION", "open-punctuation" },
            { X_UNICODE_CURRENCY_SYMBOL, "X_UNICODE_CURRENCY_SYMBOL", "currency-symbol" },
            { X_UNICODE_MODIFIER_SYMBOL, "X_UNICODE_MODIFIER_SYMBOL", "modifier-symbol" },
            { X_UNICODE_MATH_SYMBOL, "X_UNICODE_MATH_SYMBOL", "math-symbol" },
            { X_UNICODE_OTHER_SYMBOL, "X_UNICODE_OTHER_SYMBOL", "other-symbol" },
            { X_UNICODE_LINE_SEPARATOR, "X_UNICODE_LINE_SEPARATOR", "line-separator" },
            { X_UNICODE_PARAGRAPH_SEPARATOR, "X_UNICODE_PARAGRAPH_SEPARATOR", "paragraph-separator" },
            { X_UNICODE_SPACE_SEPARATOR, "X_UNICODE_SPACE_SEPARATOR", "space-separator" },
            { 0, NULL, NULL }
        };

        XType x_define_type_id = x_enum_register_static(x_intern_static_string("XUnicodeType"), values);
        x_once_init_leave_pointer (&static_x_define_type_id, x_define_type_id);
    }

    return static_x_define_type_id;
}

XType x_unicode_break_type_get_type(void)
{
    static XType static_x_define_type_id = 0;

    if (x_once_init_enter_pointer(&static_x_define_type_id)) {
        static const XEnumValue values[] = {
            { X_UNICODE_BREAK_MANDATORY, "X_UNICODE_BREAK_MANDATORY", "mandatory" },
            { X_UNICODE_BREAK_CARRIAGE_RETURN, "X_UNICODE_BREAK_CARRIAGE_RETURN", "carriage-return" },
            { X_UNICODE_BREAK_LINE_FEED, "X_UNICODE_BREAK_LINE_FEED", "line-feed" },
            { X_UNICODE_BREAK_COMBINING_MARK, "X_UNICODE_BREAK_COMBINING_MARK", "combining-mark" },
            { X_UNICODE_BREAK_SURROGATE, "X_UNICODE_BREAK_SURROGATE", "surrogate" },
            { X_UNICODE_BREAK_ZERO_WIDTH_SPACE, "X_UNICODE_BREAK_ZERO_WIDTH_SPACE", "zero-width-space" },
            { X_UNICODE_BREAK_INSEPARABLE, "X_UNICODE_BREAK_INSEPARABLE", "inseparable" },
            { X_UNICODE_BREAK_NON_BREAKING_GLUE, "X_UNICODE_BREAK_NON_BREAKING_GLUE", "non-breaking-glue" },
            { X_UNICODE_BREAK_CONTINGENT, "X_UNICODE_BREAK_CONTINGENT", "contingent" },
            { X_UNICODE_BREAK_SPACE, "X_UNICODE_BREAK_SPACE", "space" },
            { X_UNICODE_BREAK_AFTER, "X_UNICODE_BREAK_AFTER", "after" },
            { X_UNICODE_BREAK_BEFORE, "X_UNICODE_BREAK_BEFORE", "before" },
            { X_UNICODE_BREAK_BEFORE_AND_AFTER, "X_UNICODE_BREAK_BEFORE_AND_AFTER", "before-and-after" },
            { X_UNICODE_BREAK_HYPHEN, "X_UNICODE_BREAK_HYPHEN", "hyphen" },
            { X_UNICODE_BREAK_NON_STARTER, "X_UNICODE_BREAK_NON_STARTER", "non-starter" },
            { X_UNICODE_BREAK_OPEN_PUNCTUATION, "X_UNICODE_BREAK_OPEN_PUNCTUATION", "open-punctuation" },
            { X_UNICODE_BREAK_CLOSE_PUNCTUATION, "X_UNICODE_BREAK_CLOSE_PUNCTUATION", "close-punctuation" },
            { X_UNICODE_BREAK_QUOTATION, "X_UNICODE_BREAK_QUOTATION", "quotation" },
            { X_UNICODE_BREAK_EXCLAMATION, "X_UNICODE_BREAK_EXCLAMATION", "exclamation" },
            { X_UNICODE_BREAK_IDEOGRAPHIC, "X_UNICODE_BREAK_IDEOGRAPHIC", "ideographic" },
            { X_UNICODE_BREAK_NUMERIC, "X_UNICODE_BREAK_NUMERIC", "numeric" },
            { X_UNICODE_BREAK_INFIX_SEPARATOR, "X_UNICODE_BREAK_INFIX_SEPARATOR", "infix-separator" },
            { X_UNICODE_BREAK_SYMBOL, "X_UNICODE_BREAK_SYMBOL", "symbol" },
            { X_UNICODE_BREAK_ALPHABETIC, "X_UNICODE_BREAK_ALPHABETIC", "alphabetic" },
            { X_UNICODE_BREAK_PREFIX, "X_UNICODE_BREAK_PREFIX", "prefix" },
            { X_UNICODE_BREAK_POSTFIX, "X_UNICODE_BREAK_POSTFIX", "postfix" },
            { X_UNICODE_BREAK_COMPLEX_CONTEXT, "X_UNICODE_BREAK_COMPLEX_CONTEXT", "complex-context" },
            { X_UNICODE_BREAK_AMBIGUOUS, "X_UNICODE_BREAK_AMBIGUOUS", "ambiguous" },
            { X_UNICODE_BREAK_UNKNOWN, "X_UNICODE_BREAK_UNKNOWN", "unknown" },
            { X_UNICODE_BREAK_NEXT_LINE, "X_UNICODE_BREAK_NEXT_LINE", "next-line" },
            { X_UNICODE_BREAK_WORD_JOINER, "X_UNICODE_BREAK_WORD_JOINER", "word-joiner" },
            { X_UNICODE_BREAK_HANGUL_L_JAMO, "X_UNICODE_BREAK_HANGUL_L_JAMO", "hangul-l-jamo" },
            { X_UNICODE_BREAK_HANGUL_V_JAMO, "X_UNICODE_BREAK_HANGUL_V_JAMO", "hangul-v-jamo" },
            { X_UNICODE_BREAK_HANGUL_T_JAMO, "X_UNICODE_BREAK_HANGUL_T_JAMO", "hangul-t-jamo" },
            { X_UNICODE_BREAK_HANGUL_LV_SYLLABLE, "X_UNICODE_BREAK_HANGUL_LV_SYLLABLE", "hangul-lv-syllable" },
            { X_UNICODE_BREAK_HANGUL_LVT_SYLLABLE, "X_UNICODE_BREAK_HANGUL_LVT_SYLLABLE", "hangul-lvt-syllable" },
            { X_UNICODE_BREAK_CLOSE_PARANTHESIS, "X_UNICODE_BREAK_CLOSE_PARANTHESIS", "close-paranthesis" },
            { X_UNICODE_BREAK_CLOSE_PARENTHESIS, "X_UNICODE_BREAK_CLOSE_PARENTHESIS", "close-parenthesis" },
            { X_UNICODE_BREAK_CONDITIONAL_JAPANESE_STARTER, "X_UNICODE_BREAK_CONDITIONAL_JAPANESE_STARTER", "conditional-japanese-starter" },
            { X_UNICODE_BREAK_HEBREW_LETTER, "X_UNICODE_BREAK_HEBREW_LETTER", "hebrew-letter" },
            { X_UNICODE_BREAK_REGIONAL_INDICATOR, "X_UNICODE_BREAK_REGIONAL_INDICATOR", "regional-indicator" },
            { X_UNICODE_BREAK_EMOJI_BASE, "X_UNICODE_BREAK_EMOJI_BASE", "emoji-base" },
            { X_UNICODE_BREAK_EMOJI_MODIFIER, "X_UNICODE_BREAK_EMOJI_MODIFIER", "emoji-modifier" },
            { X_UNICODE_BREAK_ZERO_WIDTH_JOINER, "X_UNICODE_BREAK_ZERO_WIDTH_JOINER", "zero-width-joiner" },
            { 0, NULL, NULL }
        };
    
        XType x_define_type_id = x_enum_register_static(x_intern_static_string("XUnicodeBreakType"), values);
        x_once_init_leave_pointer(&static_x_define_type_id, x_define_type_id);
    }

    return static_x_define_type_id;
}

XType x_unicode_script_get_type(void)
{
    static XType static_x_define_type_id = 0;

    if (x_once_init_enter_pointer(&static_x_define_type_id)) {
        static const XEnumValue values[] = {
            { X_UNICODE_SCRIPT_INVALID_CODE, "X_UNICODE_SCRIPT_INVALID_CODE", "invalid-code" },
            { X_UNICODE_SCRIPT_COMMON, "X_UNICODE_SCRIPT_COMMON", "common" },
            { X_UNICODE_SCRIPT_INHERITED, "X_UNICODE_SCRIPT_INHERITED", "inherited" },
            { X_UNICODE_SCRIPT_ARABIC, "X_UNICODE_SCRIPT_ARABIC", "arabic" },
            { X_UNICODE_SCRIPT_ARMENIAN, "X_UNICODE_SCRIPT_ARMENIAN", "armenian" },
            { X_UNICODE_SCRIPT_BENGALI, "X_UNICODE_SCRIPT_BENGALI", "bengali" },
            { X_UNICODE_SCRIPT_BOPOMOFO, "X_UNICODE_SCRIPT_BOPOMOFO", "bopomofo" },
            { X_UNICODE_SCRIPT_CHEROKEE, "X_UNICODE_SCRIPT_CHEROKEE", "cherokee" },
            { X_UNICODE_SCRIPT_COPTIC, "X_UNICODE_SCRIPT_COPTIC", "coptic" },
            { X_UNICODE_SCRIPT_CYRILLIC, "X_UNICODE_SCRIPT_CYRILLIC", "cyrillic" },
            { X_UNICODE_SCRIPT_DESERET, "X_UNICODE_SCRIPT_DESERET", "deseret" },
            { X_UNICODE_SCRIPT_DEVANAGARI, "X_UNICODE_SCRIPT_DEVANAGARI", "devanagari" },
            { X_UNICODE_SCRIPT_ETHIOPIC, "X_UNICODE_SCRIPT_ETHIOPIC", "ethiopic" },
            { X_UNICODE_SCRIPT_GEORGIAN, "X_UNICODE_SCRIPT_GEORGIAN", "georgian" },
            { X_UNICODE_SCRIPT_GOTHIC, "X_UNICODE_SCRIPT_GOTHIC", "gothic" },
            { X_UNICODE_SCRIPT_GREEK, "X_UNICODE_SCRIPT_GREEK", "greek" },
            { X_UNICODE_SCRIPT_GUJARATI, "X_UNICODE_SCRIPT_GUJARATI", "gujarati" },
            { X_UNICODE_SCRIPT_GURMUKHI, "X_UNICODE_SCRIPT_GURMUKHI", "gurmukhi" },
            { X_UNICODE_SCRIPT_HAN, "X_UNICODE_SCRIPT_HAN", "han" },
            { X_UNICODE_SCRIPT_HANGUL, "X_UNICODE_SCRIPT_HANGUL", "hangul" },
            { X_UNICODE_SCRIPT_HEBREW, "X_UNICODE_SCRIPT_HEBREW", "hebrew" },
            { X_UNICODE_SCRIPT_HIRAGANA, "X_UNICODE_SCRIPT_HIRAGANA", "hiragana" },
            { X_UNICODE_SCRIPT_KANNADA, "X_UNICODE_SCRIPT_KANNADA", "kannada" },
            { X_UNICODE_SCRIPT_KATAKANA, "X_UNICODE_SCRIPT_KATAKANA", "katakana" },
            { X_UNICODE_SCRIPT_KHMER, "X_UNICODE_SCRIPT_KHMER", "khmer" },
            { X_UNICODE_SCRIPT_LAO, "X_UNICODE_SCRIPT_LAO", "lao" },
            { X_UNICODE_SCRIPT_LATIN, "X_UNICODE_SCRIPT_LATIN", "latin" },
            { X_UNICODE_SCRIPT_MALAYALAM, "X_UNICODE_SCRIPT_MALAYALAM", "malayalam" },
            { X_UNICODE_SCRIPT_MONGOLIAN, "X_UNICODE_SCRIPT_MONGOLIAN", "mongolian" },
            { X_UNICODE_SCRIPT_MYANMAR, "X_UNICODE_SCRIPT_MYANMAR", "myanmar" },
            { X_UNICODE_SCRIPT_OGHAM, "X_UNICODE_SCRIPT_OGHAM", "ogham" },
            { X_UNICODE_SCRIPT_OLD_ITALIC, "X_UNICODE_SCRIPT_OLD_ITALIC", "old-italic" },
            { X_UNICODE_SCRIPT_ORIYA, "X_UNICODE_SCRIPT_ORIYA", "oriya" },
            { X_UNICODE_SCRIPT_RUNIC, "X_UNICODE_SCRIPT_RUNIC", "runic" },
            { X_UNICODE_SCRIPT_SINHALA, "X_UNICODE_SCRIPT_SINHALA", "sinhala" },
            { X_UNICODE_SCRIPT_SYRIAC, "X_UNICODE_SCRIPT_SYRIAC", "syriac" },
            { X_UNICODE_SCRIPT_TAMIL, "X_UNICODE_SCRIPT_TAMIL", "tamil" },
            { X_UNICODE_SCRIPT_TELUGU, "X_UNICODE_SCRIPT_TELUGU", "telugu" },
            { X_UNICODE_SCRIPT_THAANA, "X_UNICODE_SCRIPT_THAANA", "thaana" },
            { X_UNICODE_SCRIPT_THAI, "X_UNICODE_SCRIPT_THAI", "thai" },
            { X_UNICODE_SCRIPT_TIBETAN, "X_UNICODE_SCRIPT_TIBETAN", "tibetan" },
            { X_UNICODE_SCRIPT_CANADIAN_ABORIGINAL, "X_UNICODE_SCRIPT_CANADIAN_ABORIGINAL", "canadian-aboriginal" },
            { X_UNICODE_SCRIPT_YI, "X_UNICODE_SCRIPT_YI", "yi" },
            { X_UNICODE_SCRIPT_TAGALOG, "X_UNICODE_SCRIPT_TAGALOG", "tagalog" },
            { X_UNICODE_SCRIPT_HANUNOO, "X_UNICODE_SCRIPT_HANUNOO", "hanunoo" },
            { X_UNICODE_SCRIPT_BUHID, "X_UNICODE_SCRIPT_BUHID", "buhid" },
            { X_UNICODE_SCRIPT_TAGBANWA, "X_UNICODE_SCRIPT_TAGBANWA", "tagbanwa" },
            { X_UNICODE_SCRIPT_BRAILLE, "X_UNICODE_SCRIPT_BRAILLE", "braille" },
            { X_UNICODE_SCRIPT_CYPRIOT, "X_UNICODE_SCRIPT_CYPRIOT", "cypriot" },
            { X_UNICODE_SCRIPT_LIMBU, "X_UNICODE_SCRIPT_LIMBU", "limbu" },
            { X_UNICODE_SCRIPT_OSMANYA, "X_UNICODE_SCRIPT_OSMANYA", "osmanya" },
            { X_UNICODE_SCRIPT_SHAVIAN, "X_UNICODE_SCRIPT_SHAVIAN", "shavian" },
            { X_UNICODE_SCRIPT_LINEAR_B, "X_UNICODE_SCRIPT_LINEAR_B", "linear-b" },
            { X_UNICODE_SCRIPT_TAI_LE, "X_UNICODE_SCRIPT_TAI_LE", "tai-le" },
            { X_UNICODE_SCRIPT_UGARITIC, "X_UNICODE_SCRIPT_UGARITIC", "ugaritic" },
            { X_UNICODE_SCRIPT_NEW_TAI_LUE, "X_UNICODE_SCRIPT_NEW_TAI_LUE", "new-tai-lue" },
            { X_UNICODE_SCRIPT_BUGINESE, "X_UNICODE_SCRIPT_BUGINESE", "buginese" },
            { X_UNICODE_SCRIPT_GLAGOLITIC, "X_UNICODE_SCRIPT_GLAGOLITIC", "glagolitic" },
            { X_UNICODE_SCRIPT_TIFINAGH, "X_UNICODE_SCRIPT_TIFINAGH", "tifinagh" },
            { X_UNICODE_SCRIPT_SYLOTI_NAGRI, "X_UNICODE_SCRIPT_SYLOTI_NAGRI", "syloti-nagri" },
            { X_UNICODE_SCRIPT_OLD_PERSIAN, "X_UNICODE_SCRIPT_OLD_PERSIAN", "old-persian" },
            { X_UNICODE_SCRIPT_KHAROSHTHI, "X_UNICODE_SCRIPT_KHAROSHTHI", "kharoshthi" },
            { X_UNICODE_SCRIPT_UNKNOWN, "X_UNICODE_SCRIPT_UNKNOWN", "unknown" },
            { X_UNICODE_SCRIPT_BALINESE, "X_UNICODE_SCRIPT_BALINESE", "balinese" },
            { X_UNICODE_SCRIPT_CUNEIFORM, "X_UNICODE_SCRIPT_CUNEIFORM", "cuneiform" },
            { X_UNICODE_SCRIPT_PHOENICIAN, "X_UNICODE_SCRIPT_PHOENICIAN", "phoenician" },
            { X_UNICODE_SCRIPT_PHAGS_PA, "X_UNICODE_SCRIPT_PHAGS_PA", "phags-pa" },
            { X_UNICODE_SCRIPT_NKO, "X_UNICODE_SCRIPT_NKO", "nko" },
            { X_UNICODE_SCRIPT_KAYAH_LI, "X_UNICODE_SCRIPT_KAYAH_LI", "kayah-li" },
            { X_UNICODE_SCRIPT_LEPCHA, "X_UNICODE_SCRIPT_LEPCHA", "lepcha" },
            { X_UNICODE_SCRIPT_REJANG, "X_UNICODE_SCRIPT_REJANG", "rejang" },
            { X_UNICODE_SCRIPT_SUNDANESE, "X_UNICODE_SCRIPT_SUNDANESE", "sundanese" },
            { X_UNICODE_SCRIPT_SAURASHTRA, "X_UNICODE_SCRIPT_SAURASHTRA", "saurashtra" },
            { X_UNICODE_SCRIPT_CHAM, "X_UNICODE_SCRIPT_CHAM", "cham" },
            { X_UNICODE_SCRIPT_OL_CHIKI, "X_UNICODE_SCRIPT_OL_CHIKI", "ol-chiki" },
            { X_UNICODE_SCRIPT_VAI, "X_UNICODE_SCRIPT_VAI", "vai" },
            { X_UNICODE_SCRIPT_CARIAN, "X_UNICODE_SCRIPT_CARIAN", "carian" },
            { X_UNICODE_SCRIPT_LYCIAN, "X_UNICODE_SCRIPT_LYCIAN", "lycian" },
            { X_UNICODE_SCRIPT_LYDIAN, "X_UNICODE_SCRIPT_LYDIAN", "lydian" },
            { X_UNICODE_SCRIPT_AVESTAN, "X_UNICODE_SCRIPT_AVESTAN", "avestan" },
            { X_UNICODE_SCRIPT_BAMUM, "X_UNICODE_SCRIPT_BAMUM", "bamum" },
            { X_UNICODE_SCRIPT_EGYPTIAN_HIEROGLYPHS, "X_UNICODE_SCRIPT_EGYPTIAN_HIEROGLYPHS", "egyptian-hieroglyphs" },
            { X_UNICODE_SCRIPT_IMPERIAL_ARAMAIC, "X_UNICODE_SCRIPT_IMPERIAL_ARAMAIC", "imperial-aramaic" },
            { X_UNICODE_SCRIPT_INSCRIPTIONAL_PAHLAVI, "X_UNICODE_SCRIPT_INSCRIPTIONAL_PAHLAVI", "inscriptional-pahlavi" },
            { X_UNICODE_SCRIPT_INSCRIPTIONAL_PARTHIAN, "X_UNICODE_SCRIPT_INSCRIPTIONAL_PARTHIAN", "inscriptional-parthian" },
            { X_UNICODE_SCRIPT_JAVANESE, "X_UNICODE_SCRIPT_JAVANESE", "javanese" },
            { X_UNICODE_SCRIPT_KAITHI, "X_UNICODE_SCRIPT_KAITHI", "kaithi" },
            { X_UNICODE_SCRIPT_LISU, "X_UNICODE_SCRIPT_LISU", "lisu" },
            { X_UNICODE_SCRIPT_MEETEI_MAYEK, "X_UNICODE_SCRIPT_MEETEI_MAYEK", "meetei-mayek" },
            { X_UNICODE_SCRIPT_OLD_SOUTH_ARABIAN, "X_UNICODE_SCRIPT_OLD_SOUTH_ARABIAN", "old-south-arabian" },
            { X_UNICODE_SCRIPT_OLD_TURKIC, "X_UNICODE_SCRIPT_OLD_TURKIC", "old-turkic" },
            { X_UNICODE_SCRIPT_SAMARITAN, "X_UNICODE_SCRIPT_SAMARITAN", "samaritan" },
            { X_UNICODE_SCRIPT_TAI_THAM, "X_UNICODE_SCRIPT_TAI_THAM", "tai-tham" },
            { X_UNICODE_SCRIPT_TAI_VIET, "X_UNICODE_SCRIPT_TAI_VIET", "tai-viet" },
            { X_UNICODE_SCRIPT_BATAK, "X_UNICODE_SCRIPT_BATAK", "batak" },
            { X_UNICODE_SCRIPT_BRAHMI, "X_UNICODE_SCRIPT_BRAHMI", "brahmi" },
            { X_UNICODE_SCRIPT_MANDAIC, "X_UNICODE_SCRIPT_MANDAIC", "mandaic" },
            { X_UNICODE_SCRIPT_CHAKMA, "X_UNICODE_SCRIPT_CHAKMA", "chakma" },
            { X_UNICODE_SCRIPT_MEROITIC_CURSIVE, "X_UNICODE_SCRIPT_MEROITIC_CURSIVE", "meroitic-cursive" },
            { X_UNICODE_SCRIPT_MEROITIC_HIEROGLYPHS, "X_UNICODE_SCRIPT_MEROITIC_HIEROGLYPHS", "meroitic-hieroglyphs" },
            { X_UNICODE_SCRIPT_MIAO, "X_UNICODE_SCRIPT_MIAO", "miao" },
            { X_UNICODE_SCRIPT_SHARADA, "X_UNICODE_SCRIPT_SHARADA", "sharada" },
            { X_UNICODE_SCRIPT_SORA_SOMPENG, "X_UNICODE_SCRIPT_SORA_SOMPENG", "sora-sompeng" },
            { X_UNICODE_SCRIPT_TAKRI, "G_UNICODE_SCRIPT_TAKRI", "takri" },
            { X_UNICODE_SCRIPT_BASSA_VAH, "X_UNICODE_SCRIPT_BASSA_VAH", "bassa-vah" },
            { X_UNICODE_SCRIPT_CAUCASIAN_ALBANIAN, "G_UNICODE_SCRIPT_CAUCASIAN_ALBANIAN", "caucasian-albanian" },
            { X_UNICODE_SCRIPT_DUPLOYAN, "X_UNICODE_SCRIPT_DUPLOYAN", "duployan" },
            { X_UNICODE_SCRIPT_ELBASAN, "X_UNICODE_SCRIPT_ELBASAN", "elbasan" },
            { X_UNICODE_SCRIPT_GRANTHA, "X_UNICODE_SCRIPT_GRANTHA", "grantha" },
            { X_UNICODE_SCRIPT_KHOJKI, "X_UNICODE_SCRIPT_KHOJKI", "khojki" },
            { X_UNICODE_SCRIPT_KHUDAWADI, "X_UNICODE_SCRIPT_KHUDAWADI", "khudawadi" },
            { X_UNICODE_SCRIPT_LINEAR_A, "X_UNICODE_SCRIPT_LINEAR_A", "linear-a" },
            { X_UNICODE_SCRIPT_MAHAJANI, "X_UNICODE_SCRIPT_MAHAJANI", "mahajani" },
            { X_UNICODE_SCRIPT_MANICHAEAN, "X_UNICODE_SCRIPT_MANICHAEAN", "manichaean" },
            { X_UNICODE_SCRIPT_MENDE_KIKAKUI, "X_UNICODE_SCRIPT_MENDE_KIKAKUI", "mende-kikakui" },
            { X_UNICODE_SCRIPT_MODI, "X_UNICODE_SCRIPT_MODI", "modi" },
            { X_UNICODE_SCRIPT_MRO, "X_UNICODE_SCRIPT_MRO", "mro" },
            { X_UNICODE_SCRIPT_NABATAEAN, "X_UNICODE_SCRIPT_NABATAEAN", "nabataean" },
            { X_UNICODE_SCRIPT_OLD_NORTH_ARABIAN, "X_UNICODE_SCRIPT_OLD_NORTH_ARABIAN", "old-north-arabian" },
            { X_UNICODE_SCRIPT_OLD_PERMIC, "X_UNICODE_SCRIPT_OLD_PERMIC", "old-permic" },
            { X_UNICODE_SCRIPT_PAHAWH_HMONG, "X_UNICODE_SCRIPT_PAHAWH_HMONG", "pahawh-hmong" },
            { X_UNICODE_SCRIPT_PALMYRENE, "X_UNICODE_SCRIPT_PALMYRENE", "palmyrene" },
            { X_UNICODE_SCRIPT_PAU_CIN_HAU, "X_UNICODE_SCRIPT_PAU_CIN_HAU", "pau-cin-hau" },
            { X_UNICODE_SCRIPT_PSALTER_PAHLAVI, "X_UNICODE_SCRIPT_PSALTER_PAHLAVI", "psalter-pahlavi" },
            { X_UNICODE_SCRIPT_SIDDHAM, "X_UNICODE_SCRIPT_SIDDHAM", "siddham" },
            { X_UNICODE_SCRIPT_TIRHUTA, "X_UNICODE_SCRIPT_TIRHUTA", "tirhuta" },
            { X_UNICODE_SCRIPT_WARANG_CITI, "X_UNICODE_SCRIPT_WARANG_CITI", "warang-citi" },
            { X_UNICODE_SCRIPT_AHOM, "X_UNICODE_SCRIPT_AHOM", "ahom" },
            { X_UNICODE_SCRIPT_ANATOLIAN_HIEROGLYPHS, "X_UNICODE_SCRIPT_ANATOLIAN_HIEROGLYPHS", "anatolian-hieroglyphs" },
            { X_UNICODE_SCRIPT_HATRAN, "X_UNICODE_SCRIPT_HATRAN", "hatran" },
            { X_UNICODE_SCRIPT_MULTANI, "X_UNICODE_SCRIPT_MULTANI", "multani" },
            { X_UNICODE_SCRIPT_OLD_HUNGARIAN, "X_UNICODE_SCRIPT_OLD_HUNGARIAN", "old-hungarian" },
            { X_UNICODE_SCRIPT_SIGNWRITING, "X_UNICODE_SCRIPT_SIGNWRITING", "signwriting" },
            { X_UNICODE_SCRIPT_ADLAM, "X_UNICODE_SCRIPT_ADLAM", "adlam" },
            { X_UNICODE_SCRIPT_BHAIKSUKI, "X_UNICODE_SCRIPT_BHAIKSUKI", "bhaiksuki" },
            { X_UNICODE_SCRIPT_MARCHEN, "X_UNICODE_SCRIPT_MARCHEN", "marchen" },
            { X_UNICODE_SCRIPT_NEWA, "X_UNICODE_SCRIPT_NEWA", "newa" },
            { X_UNICODE_SCRIPT_OSAGE, "X_UNICODE_SCRIPT_OSAGE", "osage" },
            { X_UNICODE_SCRIPT_TANGUT, "X_UNICODE_SCRIPT_TANGUT", "tangut" },
            { X_UNICODE_SCRIPT_MASARAM_GONDI, "X_UNICODE_SCRIPT_MASARAM_GONDI", "masaram-gondi" },
            { X_UNICODE_SCRIPT_NUSHU, "X_UNICODE_SCRIPT_NUSHU", "nushu" },
            { X_UNICODE_SCRIPT_SOYOMBO, "X_UNICODE_SCRIPT_SOYOMBO", "soyombo" },
            { X_UNICODE_SCRIPT_ZANABAZAR_SQUARE, "X_UNICODE_SCRIPT_ZANABAZAR_SQUARE", "zanabazar-square" },
            { X_UNICODE_SCRIPT_DOGRA, "X_UNICODE_SCRIPT_DOGRA", "dogra" },
            { X_UNICODE_SCRIPT_GUNJALA_GONDI, "X_UNICODE_SCRIPT_GUNJALA_GONDI", "gunjala-gondi" },
            { X_UNICODE_SCRIPT_HANIFI_ROHINGYA, "X_UNICODE_SCRIPT_HANIFI_ROHINGYA", "hanifi-rohingya" },
            { X_UNICODE_SCRIPT_MAKASAR, "X_UNICODE_SCRIPT_MAKASAR", "makasar" },
            { X_UNICODE_SCRIPT_MEDEFAIDRIN, "X_UNICODE_SCRIPT_MEDEFAIDRIN", "medefaidrin" },
            { X_UNICODE_SCRIPT_OLD_SOGDIAN, "X_UNICODE_SCRIPT_OLD_SOGDIAN", "old-sogdian" },
            { X_UNICODE_SCRIPT_SOGDIAN, "X_UNICODE_SCRIPT_SOGDIAN", "sogdian" },
            { X_UNICODE_SCRIPT_ELYMAIC, "X_UNICODE_SCRIPT_ELYMAIC", "elymaic" },
            { X_UNICODE_SCRIPT_NANDINAGARI, "X_UNICODE_SCRIPT_NANDINAGARI", "nandinagari" },
            { X_UNICODE_SCRIPT_NYIAKENG_PUACHUE_HMONG, "X_UNICODE_SCRIPT_NYIAKENG_PUACHUE_HMONG", "nyiakeng-puachue-hmong" },
            { X_UNICODE_SCRIPT_WANCHO, "X_UNICODE_SCRIPT_WANCHO", "wancho" },
            { X_UNICODE_SCRIPT_CHORASMIAN, "X_UNICODE_SCRIPT_CHORASMIAN", "chorasmian" },
            { X_UNICODE_SCRIPT_DIVES_AKURU, "X_UNICODE_SCRIPT_DIVES_AKURU", "dives-akuru" },
            { X_UNICODE_SCRIPT_KHITAN_SMALL_SCRIPT, "X_UNICODE_SCRIPT_KHITAN_SMALL_SCRIPT", "khitan-small-script" },
            { X_UNICODE_SCRIPT_YEZIDI, "X_UNICODE_SCRIPT_YEZIDI", "yezidi" },
            { X_UNICODE_SCRIPT_CYPRO_MINOAN, "X_UNICODE_SCRIPT_CYPRO_MINOAN", "cypro-minoan" },
            { X_UNICODE_SCRIPT_OLD_UYGHUR, "X_UNICODE_SCRIPT_OLD_UYGHUR", "old-uyghur" },
            { X_UNICODE_SCRIPT_TANGSA, "X_UNICODE_SCRIPT_TANGSA", "tangsa" },
            { X_UNICODE_SCRIPT_TOTO, "X_UNICODE_SCRIPT_TOTO", "toto" },
            { X_UNICODE_SCRIPT_VITHKUQI, "X_UNICODE_SCRIPT_VITHKUQI", "vithkuqi" },
            { X_UNICODE_SCRIPT_MATH, "X_UNICODE_SCRIPT_MATH", "math" },
            { 0, NULL, NULL }
        };

        XType x_define_type_id = x_enum_register_static(x_intern_static_string("XUnicodeScript"), values);
        x_once_init_leave_pointer(&static_x_define_type_id, x_define_type_id);
    }

    return static_x_define_type_id;
}

XType x_normalize_mode_get_type(void)
{
    static XType static_x_define_type_id = 0;

    if (x_once_init_enter_pointer(&static_x_define_type_id)) {
        static const XEnumValue values[] = {
            { X_NORMALIZE_DEFAULT, "X_NORMALIZE_DEFAULT", "default" },
            { X_NORMALIZE_NFD, "X_NORMALIZE_NFD", "nfd" },
            { X_NORMALIZE_DEFAULT_COMPOSE, "X_NORMALIZE_DEFAULT_COMPOSE", "default-compose" },
            { X_NORMALIZE_NFC, "X_NORMALIZE_NFC", "nfc" },
            { X_NORMALIZE_ALL, "X_NORMALIZE_ALL", "all" },
            { X_NORMALIZE_NFKD, "X_NORMALIZE_NFKD", "nfkd" },
            { X_NORMALIZE_ALL_COMPOSE, "X_NORMALIZE_ALL_COMPOSE", "all-compose" },
            { X_NORMALIZE_NFKC, "X_NORMALIZE_NFKC", "nfkc" },
            { 0, NULL, NULL }
        };

        XType x_define_type_id = x_enum_register_static(x_intern_static_string("XNormalizeMode"), values);
        x_once_init_leave_pointer(&static_x_define_type_id, x_define_type_id);
    }

    return static_x_define_type_id;
}

X_GNUC_END_IGNORE_DEPRECATIONS
