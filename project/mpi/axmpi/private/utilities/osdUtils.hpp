#pragma once

#include "../../axpi.hpp"

namespace axpi {
typedef struct {
    int           width;
    int           height;
    int           channel;
    unsigned char *data;
} axosd_img_t;

void releaseImage(axosd_img_t *img);
void createImage(int charlen, float fontscale, int thickness, axosd_img_t *out);

int putText(const char *text, float fontscale, int thickness, axosd_img_t *base, axosd_img_t *out);

void drawResults(axosd_img_t *out, float fontscale, int thickness, axpi_results_t *results, int offset_x, int offset_y);
int releaseObjects(axpi_results_t *results);
}
