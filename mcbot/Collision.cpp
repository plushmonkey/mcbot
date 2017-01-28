#include "Collision.h"

#include <iostream>

template <typename T>
inline T Sign(T val) {
    return std::signbit(val) ? static_cast<T>(-1) : static_cast<T>(1);
}

inline Vector3d BasisAxis(int basisIndex) {
    static const Vector3d axes[3] = { Vector3d(1, 0, 0), Vector3d(0, 1, 0), Vector3d(0, 0, 1) };
    return axes[basisIndex];
}

Vector3d GetClosestFaceNormal(const Vector3d& pos, AABB bounds) {
    Vector3d center = bounds.min + (bounds.max - bounds.min) / 2;
    Vector3d dim = bounds.max - bounds.min;
    Vector3d offset = pos - center;

    double minDist = std::numeric_limits<double>::max();
    Vector3d normal;

    for (int i = 0; i < 3; ++i) {
        double dist = dim[i] - std::abs(offset[i]);
        if (dist < minDist) {
            minDist = dist;
            normal = BasisAxis(i) * Sign(offset[i]);
        }
    }

    return normal;
}


bool CollisionDetector::DetectCollision(Vector3d from, Vector3d rayVector, Collision* collision) const {
    static const std::vector<Vector3d> directions = {
        Vector3d(0, 0, 0), Vector3d(1, 0, 0), Vector3d(-1, 0, 0), Vector3d(0, 1, 0), Vector3d(0, -1, 0), Vector3d(0, 0, 1), Vector3d(0, 0, -1)
    };

    Vector3d direction = Vector3Normalize(rayVector);
    double length = rayVector.Length();

    Ray ray(from, direction);

    if (collision)
        *collision = Collision();

    for (double i = 0; i < length; ++i) {
        Vector3d position = from + direction * i;

        // Look for collisions in any blocks surrounding the ray
        for (Vector3d checkDirection : directions) {
            Vector3d checkPos = position + checkDirection;
            Minecraft::BlockPtr block = m_World->GetBlock(checkPos).GetBlock();

            if (block && block->IsSolid()) {
                AABB bounds = block->GetBoundingBox(checkPos);
                double distance;

                if (bounds.Intersects(ray, &distance)) {
                    if (distance < 0 || distance > length) continue;

                    Vector3d collisionHit = from + direction * distance;
                    Vector3d normal = GetClosestFaceNormal(collisionHit, bounds);

                    //std::cout << "Collision from " << from << " at " << collisionHit << " vector " << rayVector << " normal " << normal << " dist: " << distance << " len: " << length << std::endl;
                    
                    if (collision)
                        *collision = Collision(collisionHit, normal);
                    
                    return true;
                }
            }
        }
    }

    return false;
}
