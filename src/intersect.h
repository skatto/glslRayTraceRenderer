//
//  intersect.hpp
//  NewGlslRenderer
//
//  Created by Shun on 2018/03/31.
//  Copyright © 2018年 Shun. All rights reserved.
//

#ifndef intersect_hpp0331
#define intersect_hpp0331

#include "common.h"

Intersection intersectBVH(const Ray& ray, const BVH& bvh);
bool intersectBVH(const Ray& ray, const BVH& bvh, Intersection* isect);

#endif /* intersect_hpp0331 */
