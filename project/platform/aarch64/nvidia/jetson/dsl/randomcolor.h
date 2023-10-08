#ifndef RANDOMCOLOR_H
#define RANDOMCOLOR_H

#include <mutex>
#include <vector>
#include <random>
#include <initializer_list>

/* RandomColor类表示一个随机颜色生成器。它使用HSB颜色空间和预定义的颜色映射来选择颜色 */
class RandomColor {
public:
    enum Color {
        Red,
        RedOrange,
        Orange,
        OrangeYellow,
        Yellow,
        YellowGreen,
        Green,
        GreenCyan,
        Cyan,
        CyanBlue,
        Blue,
        BlueMagenta,
        Magenta,
        MagentaPink,
        Pink,
        PinkRed,
        RandomHue,
        BlackAndWhite,
        Brown
    };

    enum Luminosity {
        Dark,
        Normal,
        Light,
        Bright,
        RandomLuminosity
    };

    typedef std::initializer_list<Color> ColorList;

    static const ColorList AnyRed;
    static const ColorList AnyOrange;
    static const ColorList AnyYellow;
    static const ColorList AnyGreen;
    static const ColorList AnyBlue;
    static const ColorList AnyMagenta;
    static const ColorList AnyPink;

    struct Range {
        Range(int value = 0);
        Range(int left, int right);

        int& operator [](int i);
        int operator [](int i) const;
        int size() const;

        int values[2];
    };

    /* 返回具有指定亮度的随机颜色，以RGB格式(0xRRGGBB) */
    int generate(Color color = RandomHue, Luminosity luminosity = Normal);

    /* 与generate(color, luminosity)相同，但只针对其中一种颜色。颜色是随机选择的——颜色的色调范围越广，它被选中的机会就越大。颜色可以通过{color1, color2，…, colorN} */
    int generate(ColorList colors, Luminosity luminosity = Normal);

    /* 返回hueRange内的随机颜色，具有指定的亮度，以RGB格式(0xRRGGBB)。范围可以是一对{left, right}或单个值(然后变为{value, value}) */
    int generate(const Range &hueRange, Luminosity luminosity = Normal);

    /* 设置随机生成器的种子 */
    void setSeed(int seed);

private:
    struct SBRange {
        SBRange(int s = 0, int bMin = 0, int bMax = 100);

        int s;
        int bMin;
        int bMax;
    };

    struct ColorInfo {
        Color                color;
        Range                hRange;
        std::vector<SBRange> sbRanges;
    };

    int generate(int h, const ColorInfo &, Luminosity);
    int pickSaturation(const ColorInfo &, Luminosity);
    int pickBrightness(int s, const ColorInfo &, Luminosity);

    Range getBrightnessRange(int s, const ColorInfo &) const;
    const ColorInfo &getColorInfo(int h) const;
    int randomWithin(const Range &);
    int HSBtoRGB(double h, double s, double b) const;

    static const std::vector<ColorInfo> colorMap;
    std::default_random_engine          randomEngine;
    std::mutex                          mutex;
};

inline RandomColor::Range::Range(int left, int right) : values{left, right}
{

}

inline RandomColor::Range::Range(int value) : values{value, value}
{

}

inline int& RandomColor::Range::operator [](int i)
{
    return values[i];
}

inline int RandomColor::Range::operator [](int i) const
{
    return values[i];
}

inline int RandomColor::Range::size() const
{
    return values[1] - values[0];
}

#endif
