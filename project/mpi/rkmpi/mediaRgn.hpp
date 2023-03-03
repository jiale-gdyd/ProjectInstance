#ifndef ROCKCHIP_MEDIA_RGN_HPP
#define ROCKCHIP_MEDIA_RGN_HPP

#if !defined(__ROCKCHIP_MEDIABASE_HPP_INSIDE__)
#error "Only <mediaBase.hpp> can be included directly."
#endif

#include <vector>
#include <string>
#include <unordered_map>

#include <stdbool.h>
#include <utils/export.h>
#include <opencv2/opencv.hpp>
#include <opencv2/freetype.hpp>
#include <media/drm_media_api.h>

API_BEGIN_NAMESPACE(media)

enum {
    LINE_STYLE_FILLED  = -1,        // 填满
    LINE_STYLE_4       = 4,         // 四连接线路
    LINE_STYLE_8       = 8,         // 八连接线路
    LINE_STYLE_AA      = 16         // 抗锯齿线
};

enum {
    FONT_FACE_SIMPLEX        = 0,   // 正常大小的无衬线字体
    FONT_FACE_PLAIN          = 1,   // 小尺寸无衬线字体
    FONT_FACE_DUPLEX         = 2,   // 正常大小无衬线字体(比FONT_FACE_SIMPLEX更复杂)
    FONT_FACE_COMPLEX        = 3,   // 正常大小的衬线字体
    FONT_FACE_TRIPLEX        = 4,   // 正常大小衬线字体(比FONT_FACE_COMPLEX更复杂)
    FONT_FACE_COMPLEX_SMALL  = 5,   // FONT_FACE_COMPLEX的小版本
    FONT_FACE_SCRIPT_SIMPLEX = 6,   // 手写字体
    FONT_FACE_SCRIPT_COMPLEX = 7,   // FONT_FACE_SCRIPT_SIMPLEX的更复杂的变体
    FONT_FACE_ITALIC         = 16   // 标志为斜体字体
};

enum {
    MARK_CROSS         = 0,         // 十字准星标记形状
    MARK_TILTED_CROSS  = 1,         // 一个45度倾斜准星标记形状
    MARK_STAR          = 2,         // 星形标记，结合十字和倾斜十字
    MARK_DIAMOND       = 3,         // 菱形标记形状
    MARK_SQUARE        = 4,         // 方形标记形状
    MARK_TRIANGLE_UP   = 5,         // 一个向上指向的三角形标记形状
    MARK_TRIANGLE_DOWN = 6          // 一个向下指向的三角形标记形状
};

class API_HIDDEN MediaRgn {
public:
    MediaRgn();
    ~MediaRgn();

    int registerFontLibraries(std::unordered_map<int, std::string> fonts);

    void initFrame(media_buffer_t &mediaFrame, size_t width, size_t height, uint8_t channels);

    void drawImage(cv::Mat image, cv::Point startPos, size_t width, size_t height);

    void drawLine(cv::Point pos1, cv::Point pos2, cv::Scalar color, int thickness = -1, int lineType = LINE_STYLE_8, int shift = 0);
    void drawArrowedLine(cv::Point pos1, cv::Point pos2, cv::Scalar color, int thickness = -1, int lineType = LINE_STYLE_8, int shift = 0, double tipLength = 0.1);

    void drawRect(cv::Point leftTop, cv::Point rightBottom, cv::Scalar color, int thickness = -1, int lineType = LINE_STYLE_8, int shift = 0);

    void drawCircle(cv::Point center, int radius, cv::Scalar color, int thickness = -1, int lineType = LINE_STYLE_8, int shift = 0);
    void drawEllipse(cv::Point center, cv::Size axes, double angle, double startAngle, double endAngle, cv::Scalar color, int thickness = -1, int lineType = LINE_STYLE_8, int shift = 0);

    void drawPolygon(const cv::Point **pts, int npts, int ncontours, cv::Scalar color, int lineType = LINE_STYLE_8, int shift = 0, cv::Point offset = cv::Point());

    void drawMarker(cv::Point pos, cv::Scalar color, int markerType = MARK_CROSS, int markerSize = 20, int thickness = -1, int lineType = LINE_STYLE_8);

    void putText(int fontType, std::string text, cv::Point pos, uint16_t fontHeight, cv::Scalar color, int thickness = -1, int lineStyle = LINE_STYLE_8, bool bottomLeftOrigin = true);

private:
    bool findFont(int fontType);
    int mapFontFace(int fontFace);
    int mapLineType(int lineStyle);
    int mapMarkerType(int markerType);

private:
    bool                                                      mInitFin;
    cv::Mat                                                   mImgFrame;
    std::unordered_map<int, std::string>                      mFontLibs;
    std::unordered_map<int, cv::Ptr<cv::freetype::FreeType2>> mFontHandler;
};

API_END_NAMESPACE(media)

#endif
