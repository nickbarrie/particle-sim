// vector.h

#ifndef VECTOR_H
#define VECTOR_H

typedef struct {
    float x, y, z;
} Vec3D;


// Function prototypes for vector operations
Vec3D addVector(Vec3D a, Vec3D b);
Vec3D subVector(Vec3D a, Vec3D b);
Vec3D rotate(Vec3D v, float pitch, float yaw);
Vec3D orthogonalProjection(Vec3D v, Vec3D direction);

#endif // VECTOR_H

