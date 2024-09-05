// vector .c


#include "vector.h"
#include "math.h"


Vec3D orthogonalProjection(Vec3D a, Vec3D b) {
    float bLengthSquared = b.x * b.x + b.y * b.y + b.z * b.z;
    float dotProduct = a.x * b.x + a.y * b.y + a.z * b.z;
    Vec3D projection = {
        (dotProduct / bLengthSquared) * b.x,
        (dotProduct / bLengthSquared) * b.y,
        (dotProduct / bLengthSquared) * b.z
    };
    return projection;
}

// Function to subtract vector 'b' from vector 'a'
Vec3D subVector(Vec3D a, Vec3D b) {
    Vec3D result = { a.x - b.x, a.y - b.y, a.z - b.z };
    return result;
}

// Function to add  vector 'b' to  vector 'a'
Vec3D addVector(Vec3D a, Vec3D b) {
    Vec3D result = { a.x + b.x, a.y +  b.y, a.z + b.z };
    return result;
}
// Function to apply rotation based on yaw and pitch
Vec3D rotate(Vec3D point, float pitch, float yaw) {
    Vec3D rotated;

    // Rotate around y-axis (yaw)
    rotated.x = cosf(yaw) * point.x - sinf(yaw) * point.z;
    rotated.z = sinf(yaw) * point.x + cosf(yaw) * point.z;
    rotated.y = point.y;

    // Rotate around x-axis (pitch)
    float tempY = rotated.y;
    rotated.y = cosf(pitch) * tempY - sinf(pitch) * rotated.z;
    rotated.z = sinf(pitch) * tempY + cosf(pitch) * rotated.z;

    return rotated;
}

float magnitudeVec3D(Vec3D v) {
    return sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}
