#if __ARM_NEON
#include <arm_neon.h>
#endif
#include <math.h>
#include <limits.h>

#include <vector>

#include "matPixelAffine.hpp"

namespace axpi {
void get_rotation_matrix(float angle, float scale, float dx, float dy, float *tm)
{
    angle *= (float)(3.14159265358979323846 / 180);

    float beta = sin(angle) * scale;
    float alpha = cos(angle) * scale;

    tm[0] = alpha;
    tm[1] = beta;
    tm[2] = (1.f - alpha) * dx - beta * dy;
    tm[3] = -beta;
    tm[4] = alpha;
    tm[5] = beta * dx + (1.f - alpha) * dy;
}

void get_affine_transform(const float *points_from, const float *points_to, int num_point, float *tm)
{
    float mm[4];
    float mb[4] = {0.f};
    float ma[4][4] = {{0.f}};

    for (int i = 0; i < num_point; i++)
    {
        ma[0][0] += points_from[0] * points_from[0] + points_from[1] * points_from[1];
        ma[0][2] += points_from[0];
        ma[0][3] += points_from[1];

        mb[0] += points_from[0] * points_to[0] + points_from[1] * points_to[1];
        mb[1] += points_from[0] * points_to[1] - points_from[1] * points_to[0];
        mb[2] += points_to[0];
        mb[3] += points_to[1];

        points_from += 2;
        points_to += 2;
    }

    ma[1][1] = ma[0][0];
    ma[2][1] = ma[1][2] = -ma[0][3];
    ma[3][1] = ma[1][3] = ma[2][0] = ma[0][2];
    ma[2][2] = ma[3][3] = (float)num_point;
    ma[3][0] = ma[0][3];

    float det;
    float mai[4][4];

    {
        float A2323 = ma[2][2] * ma[3][3] - ma[2][3] * ma[3][2];
        float A1323 = ma[2][1] * ma[3][3] - ma[2][3] * ma[3][1];
        float A1223 = ma[2][1] * ma[3][2] - ma[2][2] * ma[3][1];
        float A0323 = ma[2][0] * ma[3][3] - ma[2][3] * ma[3][0];
        float A0223 = ma[2][0] * ma[3][2] - ma[2][2] * ma[3][0];
        float A0123 = ma[2][0] * ma[3][1] - ma[2][1] * ma[3][0];
        float A2313 = ma[1][2] * ma[3][3] - ma[1][3] * ma[3][2];
        float A1313 = ma[1][1] * ma[3][3] - ma[1][3] * ma[3][1];
        float A1213 = ma[1][1] * ma[3][2] - ma[1][2] * ma[3][1];
        float A2312 = ma[1][2] * ma[2][3] - ma[1][3] * ma[2][2];
        float A1312 = ma[1][1] * ma[2][3] - ma[1][3] * ma[2][1];
        float A1212 = ma[1][1] * ma[2][2] - ma[1][2] * ma[2][1];
        float A0313 = ma[1][0] * ma[3][3] - ma[1][3] * ma[3][0];
        float A0213 = ma[1][0] * ma[3][2] - ma[1][2] * ma[3][0];
        float A0312 = ma[1][0] * ma[2][3] - ma[1][3] * ma[2][0];
        float A0212 = ma[1][0] * ma[2][2] - ma[1][2] * ma[2][0];
        float A0113 = ma[1][0] * ma[3][1] - ma[1][1] * ma[3][0];
        float A0112 = ma[1][0] * ma[2][1] - ma[1][1] * ma[2][0];

        det = ma[0][0] * (ma[1][1] * A2323 - ma[1][2] * A1323 + ma[1][3] * A1223)
            - ma[0][1] * (ma[1][0] * A2323 - ma[1][2] * A0323 + ma[1][3] * A0223)
            + ma[0][2] * (ma[1][0] * A1323 - ma[1][1] * A0323 + ma[1][3] * A0123)
            - ma[0][3] * (ma[1][0] * A1223 - ma[1][1] * A0223 + ma[1][2] * A0123);

        det = 1.f / det;

        mai[0][0] =   (ma[1][1] * A2323 - ma[1][2] * A1323 + ma[1][3] * A1223);
        mai[0][1] = - (ma[0][1] * A2323 - ma[0][2] * A1323 + ma[0][3] * A1223);
        mai[0][2] =   (ma[0][1] * A2313 - ma[0][2] * A1313 + ma[0][3] * A1213);
        mai[0][3] = - (ma[0][1] * A2312 - ma[0][2] * A1312 + ma[0][3] * A1212);
        mai[1][0] = - (ma[1][0] * A2323 - ma[1][2] * A0323 + ma[1][3] * A0223);
        mai[1][1] =   (ma[0][0] * A2323 - ma[0][2] * A0323 + ma[0][3] * A0223);
        mai[1][2] = - (ma[0][0] * A2313 - ma[0][2] * A0313 + ma[0][3] * A0213);
        mai[1][3] =   (ma[0][0] * A2312 - ma[0][2] * A0312 + ma[0][3] * A0212);
        mai[2][0] =   (ma[1][0] * A1323 - ma[1][1] * A0323 + ma[1][3] * A0123);
        mai[2][1] = - (ma[0][0] * A1323 - ma[0][1] * A0323 + ma[0][3] * A0123);
        mai[2][2] =   (ma[0][0] * A1313 - ma[0][1] * A0313 + ma[0][3] * A0113);
        mai[2][3] = - (ma[0][0] * A1312 - ma[0][1] * A0312 + ma[0][3] * A0112);
        mai[3][0] = - (ma[1][0] * A1223 - ma[1][1] * A0223 + ma[1][2] * A0123);
        mai[3][1] =   (ma[0][0] * A1223 - ma[0][1] * A0223 + ma[0][2] * A0123);
        mai[3][2] = - (ma[0][0] * A1213 - ma[0][1] * A0213 + ma[0][2] * A0113);
        mai[3][3] =   (ma[0][0] * A1212 - ma[0][1] * A0212 + ma[0][2] * A0112);
    }

    mm[0] = det * (mai[0][0] * mb[0] + mai[0][1] * mb[1] + mai[0][2] * mb[2] + mai[0][3] * mb[3]);
    mm[1] = det * (mai[1][0] * mb[0] + mai[1][1] * mb[1] + mai[1][2] * mb[2] + mai[1][3] * mb[3]);
    mm[2] = det * (mai[2][0] * mb[0] + mai[2][1] * mb[1] + mai[2][2] * mb[2] + mai[2][3] * mb[3]);
    mm[3] = det * (mai[3][0] * mb[0] + mai[3][1] * mb[1] + mai[3][2] * mb[2] + mai[3][3] * mb[3]);

    tm[0] = tm[4] = mm[0];
    tm[1] = -mm[1];
    tm[3] = mm[1];
    tm[2] = mm[2];
    tm[5] = mm[3];
}

void invert_affine_transform(const float *tm, float *tm_inv)
{
    float D = tm[0] * tm[4] - tm[1] * tm[3];
    D = D != 0.f ? 1.f / D : 0.f;

    float A11 = tm[4] * D;
    float A22 = tm[0] * D;
    float A12 = -tm[1] * D;
    float A21 = -tm[3] * D;
    float b1 = -A11 * tm[2] - A12 * tm[5];
    float b2 = -A21 * tm[2] - A22 * tm[5];

    tm_inv[0] = A11;
    tm_inv[1] = A12;
    tm_inv[2] = b1;
    tm_inv[3] = A21;
    tm_inv[4] = A22;
    tm_inv[5] = b2;
}
}
