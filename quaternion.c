#include "quaternion.h"
#include <math.h>

void quaternion_normalise(vector4d *q) {
  double mag = sqrt(q->w * q->w + q->x * q->x + q->y * q->y + q->z * q->z);
  q->w /= mag;
  q->x /= mag;
  q->y /= mag;
  q->z /= mag;
}

void quaternion_multiply(vector4d *r, vector4d *a, vector4d *b) {
  r->x = + a->x * b->w + a->y * b->z - a->z * b->y + a->w * b->x;
  r->y = - a->x * b->z + a->y * b->w + a->z * b->x + a->w * b->y;
  r->z = + a->x * b->y - a->y * b->x + a->z * b->w + a->w * b->z;
  r->w = - a->x * b->x - a->y * b->y - a->z * b->z + a->w * b->w;
}

void quaternion_from_axisangle(vector4d *q, vector3d *axis, double angle) {
  double sin_a_2 = sin(angle / 2);
  double cos_a_2 = cos(angle / 2);
  q->x = axis->x * sin_a_2;
  q->y = axis->y * sin_a_2;
  q->z = axis->z * sin_a_2;
  q->w = cos_a_2;
  quaternion_normalise(q);
}

void quaternion_from_euler(vector4d *q, double rx, double ry, double rz) {
  vector3d vx = {1, 0, 0}, vy = {0, 1, 0}, vz = {0, 0, 1};
  vector4d qx, qy, qz, qt;
  quaternion_from_axisangle(&qx, &vx, rx);
  quaternion_from_axisangle(&qy, &vy, ry);
  quaternion_from_axisangle(&qz, &vz, rz);
  quaternion_multiply(&qt, &qx, &qy);
  quaternion_multiply(q,  &qt, &qz);
}

