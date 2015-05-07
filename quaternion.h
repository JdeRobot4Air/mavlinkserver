#ifndef _QUATERNION_H
#define _QUATERNION_H

typedef struct {
  double x, y, z;
} vector3d;

typedef struct {
  double x, y, z, w;
} vector4d;

#ifdef __cplusplus
extern "C" {
#endif

void quaternion_normalise(vector4d *q);
void quaternion_multiply(vector4d *r, vector4d *a, vector4d *b);
void quaternion_from_axisangle(vector4d *q, vector3d *axis, double angle);
void quaternion_from_euler( vector4d *q, double rx, double ry, double rz);

#ifdef __cplusplus
}
#endif

#endif // _QUATERNION_H
