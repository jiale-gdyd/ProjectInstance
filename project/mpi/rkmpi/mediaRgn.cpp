#include <fcntl.h>
#include <unistd.h>

#define __ROCKCHIP_MEDIABASE_HPP_INSIDE__
#include "rkmpi.h"
#include "mediaRgn.hpp"
#undef __ROCKCHIP_MEDIABASE_HPP_INSIDE__

API_BEGIN_NAMESPACE(media)

MediaRgn::MediaRgn() : mInitFin(false)
{

}

MediaRgn::~MediaRgn()
{
    mInitFin = false;
}

int MediaRgn::registerFontLibraries(std::unordered_map<int, std::string> fonts)
{
    if (fonts.size() == 0) {
        return 0;
    }

    int validFonts = 0;
    for (auto font : fonts) {
        if (access(font.second.c_str(), F_OK) == 0) {
            validFonts++;
            mFontLibs[font.first] = font.second;
            mFontHandler[font.first] = cv::freetype::createFreeType2();
            mFontHandler[font.first]->loadFontData(font.second, 0);
        }
    }

    if (validFonts > 0) {
        mInitFin = true;
    }

    return validFonts;
}

void MediaRgn::initFrame(media_buffer_t &mediaFrame, size_t width, size_t height, uint8_t channels)
{
    if (!mInitFin || !mediaFrame || (width == 0) || (height == 0) || ((channels != 1) && (channels != 2) && (channels != 3))) {
        rkmpi_error("font library not initialized or invalid parameters");
        return;
    }

    mImgFrame = cv::Mat(height, width, CV_8UC(channels), drm_mpi_mb_get_ptr(mediaFrame));
}

void MediaRgn::putText(int fontType, std::string text, cv::Point pos, uint16_t fontHeight, cv::Scalar color, int thickness, int lineStyle, bool bottomLeftOrigin)
{
    if (!mInitFin || !findFont(fontType) || mImgFrame.empty()) {
        return;
    }

    if (((pos.x >= mImgFrame.cols) || (pos.x < 0)) || ((pos.y >= mImgFrame.rows) || (pos.y < 0))) {
        return;
    }

    mFontHandler[fontType]->putText(mImgFrame, text, pos, fontHeight, color, thickness, lineStyle, bottomLeftOrigin);
}

void MediaRgn::drawImage(cv::Mat image, cv::Point startPos, size_t width, size_t height)
{
    if (!mInitFin || mImgFrame.empty() || image.empty()) {
        return;
    }

    if (((startPos.x >= mImgFrame.cols) || (startPos.x < 0)) || ((startPos.y >= mImgFrame.rows) || (startPos.y < 0))) {
        return;
    }

    bool bNeedResize = false;
    size_t lwidth = width, lheight = height;

    if ((startPos.x + width) > mImgFrame.cols) {
        bNeedResize = true;
        lwidth = mImgFrame.cols - 1 - startPos.x;
    }

    if ((startPos.y + height) > mImgFrame.rows) {
        bNeedResize = true;
        lheight = mImgFrame.rows - 1 - startPos.y;
    }

    if (bNeedResize) {
        cv::Mat dstImage;
        cv::resize(image, dstImage, cv::Size(lwidth, lheight), 0, 0, cv::INTER_LINEAR);
        dstImage.copyTo(mImgFrame(cv::Rect(startPos.x, startPos.y, lwidth, lheight)));
    } else {
        image.copyTo(mImgFrame(cv::Rect(startPos.x, startPos.y, lwidth, lheight)));
    }
}

void MediaRgn::drawLine(cv::Point pos1, cv::Point pos2, cv::Scalar color, int thickness, int lineType, int shift)
{
    if (!mInitFin || mImgFrame.empty()) {
        return;
    }

    if (((pos1.x >= mImgFrame.cols) || (pos1.x < 0)) || ((pos1.y >= mImgFrame.rows) || (pos1.y < 0))) {
        return;
    }

    if (((pos2.x >= mImgFrame.cols) || (pos2.x < 0)) || ((pos2.y >= mImgFrame.rows) || (pos2.y < 0))) {
        return;
    }

    cv::line(mImgFrame, pos1, pos2, color, thickness, mapLineType(lineType), shift);
}

void MediaRgn::drawArrowedLine(cv::Point pos1, cv::Point pos2, cv::Scalar color, int thickness, int lineType, int shift, double tipLength)
{
    if (!mInitFin || mImgFrame.empty()) {
        return;
    }

    if (((pos1.x >= mImgFrame.cols) || (pos1.x < 0)) || ((pos1.y >= mImgFrame.rows) || (pos1.y < 0))) {
        return;
    }

    if (((pos2.x >= mImgFrame.cols) || (pos2.x < 0)) || ((pos2.y >= mImgFrame.rows) || (pos2.y < 0))) {
        return;
    }

    cv::arrowedLine(mImgFrame, pos1, pos2, color, thickness, mapLineType(lineType), shift, tipLength);
}

void MediaRgn::drawRect(cv::Point leftTop, cv::Point rightBottom, cv::Scalar color, int thickness, int lineType, int shift)
{
    if (!mInitFin || mImgFrame.empty()) {
        return;
    }

    if (((leftTop.x >= mImgFrame.cols) || (leftTop.x < 0)) || ((leftTop.y >= mImgFrame.rows) || (leftTop.y < 0))) {
        return;
    }

    if (((rightBottom.x >= mImgFrame.cols) || (rightBottom.x < 0)) || ((rightBottom.y >= mImgFrame.rows) || (rightBottom.y < 0))) {
        return;
    }

    cv::rectangle(mImgFrame, leftTop, rightBottom, color, thickness, mapLineType(lineType), shift);
}

void MediaRgn::drawCircle(cv::Point center, int radius, cv::Scalar color, int thickness, int lineType, int shift)
{
    if (!mInitFin || mImgFrame.empty()) {
        return;
    }

    if (((center.x >= mImgFrame.cols) || (center.x < 0)) || ((center.y >= mImgFrame.rows) || (center.y < 0))) {
        return;
    }

    if (radius < 0) {
        return;
    }

    cv::circle(mImgFrame, center, radius, color, thickness, mapLineType(lineType), shift);
}

void MediaRgn::drawEllipse(cv::Point center, cv::Size axes, double angle, double startAngle, double endAngle, cv::Scalar color, int thickness, int lineType, int shift)
{
    if (!mInitFin || mImgFrame.empty()) {
        return;
    }

    if (((center.x >= mImgFrame.cols) || (center.x < 0)) || ((center.y >= mImgFrame.rows) || (center.y < 0))) {
        return;
    }

    cv::ellipse(mImgFrame, center, axes, angle, startAngle, endAngle, color, thickness, mapLineType(lineType), shift);
}

void MediaRgn::drawPolygon(const cv::Point **pts, int npts, int ncontours, cv::Scalar color, int lineType, int shift, cv::Point offset)
{
    if (!mInitFin || mImgFrame.empty()) {
        return;
    }

    cv::fillPoly(mImgFrame, pts, (const int *)&npts, ncontours, color, mapLineType(lineType), shift, offset);
}

void MediaRgn::drawMarker(cv::Point pos, cv::Scalar color, int markerType, int markerSize, int thickness, int lineType)
{
    if (!mInitFin || mImgFrame.empty()) {
        return;
    }

    if (((pos.x >= mImgFrame.cols) || (pos.x < 0)) || ((pos.y >= mImgFrame.rows) || (pos.y < 0))) {
        return;
    }

    cv::drawMarker(mImgFrame, pos, color, mapMarkerType(markerType), markerSize, thickness, mapLineType(lineType));
}

int MediaRgn::mapFontFace(int fontFace)
{
    switch (fontFace) {
        case FONT_FACE_SIMPLEX:
            return cv::FONT_HERSHEY_SIMPLEX;

        case FONT_FACE_PLAIN:
            return cv::FONT_HERSHEY_PLAIN;

        case FONT_FACE_DUPLEX:
            return cv::FONT_HERSHEY_DUPLEX;

        case FONT_FACE_COMPLEX:
            return cv::FONT_HERSHEY_COMPLEX;

        case FONT_FACE_TRIPLEX:
            return cv::FONT_HERSHEY_TRIPLEX;

        case FONT_FACE_COMPLEX_SMALL:
            return cv::FONT_HERSHEY_COMPLEX_SMALL;

        case FONT_FACE_SCRIPT_SIMPLEX:
            return cv::FONT_HERSHEY_SCRIPT_SIMPLEX;

        case FONT_FACE_SCRIPT_COMPLEX:
            return cv::FONT_HERSHEY_SCRIPT_COMPLEX;

        case FONT_FACE_ITALIC:
            return cv::FONT_ITALIC;

        default:
            return cv::FONT_HERSHEY_SIMPLEX;
    }
}

int MediaRgn::mapLineType(int lineStyle)
{
    switch (lineStyle) {
        case LINE_STYLE_FILLED:
            return cv::FILLED;

        case LINE_STYLE_4:
            return cv::LINE_4;

        case LINE_STYLE_8:
            return cv::LINE_8;

        case LINE_STYLE_AA:
            return cv::LINE_AA;

        default:
            return cv::LINE_8; 
    }
}

int MediaRgn::mapMarkerType(int markerType)
{
    switch (markerType) {
        case MARK_CROSS:
            return cv::MARKER_CROSS;

        case MARK_TILTED_CROSS:
            return cv::MARKER_TILTED_CROSS;

        case MARK_STAR:
            return cv::MARKER_STAR;

        case MARK_DIAMOND:
            return cv::MARKER_DIAMOND;

        case MARK_SQUARE:
            return cv::MARKER_SQUARE;

        case MARK_TRIANGLE_UP:
            return cv::MARKER_TRIANGLE_UP;

        case MARK_TRIANGLE_DOWN:
            return cv::MARKER_TRIANGLE_DOWN;

        default:
            return cv::MARKER_CROSS;
    }
}

bool MediaRgn::findFont(int fontType)
{
    return !(mFontLibs.find(fontType) == mFontLibs.end());
}

API_END_NAMESPACE(media)
