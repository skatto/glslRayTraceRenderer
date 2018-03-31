//
//  ppm.h
//  NewGlslRenderer
//
//  Created by Shun on 2018/03/31.
//  Copyright © 2018年 Shun. All rights reserved.
//

#ifndef ppm_h0331
#define ppm_h0331

#include <fstream>
#include "common.h"

static bool SaveImageAsPPM(const std::string& filename,
                           const std::vector<GLfloat>& pixels, const int& width,
                           const int& height) {
  std::ofstream file(filename);
  if (!file) {
    LOG_INFO("failed to open a file : ", filename);
    return false;
  }

  file << "P3" << std::endl;
  file << width << " " << height << std::endl;
  file << 255 << std::endl;

  const size_t length = size_t(width) * size_t(height);
  for (size_t i = 0; i < length; i++) {
    int r = std::max(0, std::min(255, int(255.f * pixels[i * 3 + 0])));
    int g = std::max(0, std::min(255, int(255.f * pixels[i * 3 + 1])));
    int b = std::max(0, std::min(255, int(255.f * pixels[i * 3 + 2])));

    file << r << " " << g << " " << b << std::endl;
  }

  file.close();

  return true;
}

#endif /* ppm_h0331 */
