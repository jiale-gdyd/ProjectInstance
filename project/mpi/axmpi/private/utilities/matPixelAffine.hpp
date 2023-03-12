#pragma once

namespace axpi {
void invert_affine_transform(const float *tm, float *tm_inv);

void get_rotation_matrix(float angle, float scale, float dx, float dy, float *tm);
void get_affine_transform(const float *points_from, const float *points_to, int num_point, float *tm);
}
