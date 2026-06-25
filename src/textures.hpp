#pragma once

#include <GL/glew.h>

#include <algorithm>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <ImageIO/ImageIO.h>
#endif

inline GLuint createFallbackTexture() {
  unsigned char pixels[4] = {255u, 255u, 255u, 255u};
  GLuint texture = 0;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE,
               pixels);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glBindTexture(GL_TEXTURE_2D, 0);
  return texture;
}

#ifdef __APPLE__
inline GLuint loadTextureFromFile(const std::filesystem::path &texturePath) {
  if (!std::filesystem::exists(texturePath)) {
    return 0;
  }

  // Cache by absolute path: many meshes share the same texture file
  // (fishes.obj has 10 meshes all using the same diffuse). Caching avoids
  // re-decoding + re-uploading + leaking GL memory per mesh.
  static std::unordered_map<std::string, GLuint> cache;
  const std::string key = std::filesystem::absolute(texturePath).string();
  auto found = cache.find(key);
  if (found != cache.end() && found->second != 0) {
    return found->second;
  }

  CFStringRef pathString = CFStringCreateWithCString(
      kCFAllocatorDefault, texturePath.string().c_str(), kCFStringEncodingUTF8);
  if (pathString == nullptr) {
    return 0;
  }

  CFURLRef url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, pathString,
                                               kCFURLPOSIXPathStyle, false);
  CFRelease(pathString);
  if (url == nullptr) {
    return 0;
  }

  CGImageSourceRef source = CGImageSourceCreateWithURL(url, nullptr);
  CFRelease(url);
  if (source == nullptr) {
    return 0;
  }

  CGImageRef image = CGImageSourceCreateImageAtIndex(source, 0, nullptr);
  CFRelease(source);
  if (image == nullptr) {
    return 0;
  }

  const size_t width = CGImageGetWidth(image);
  const size_t height = CGImageGetHeight(image);
  if (width == 0 || height == 0) {
    CGImageRelease(image);
    return 0;
  }

  // Cap texture resolution at 1024 px on the longest side. Source assets are
  // 4K, but in a foggy underwater scene anything above 1K is invisible at
  // viewing distance. This drops decoded texture memory by ~16x.
  constexpr size_t kMaxTexDim = 1024;
  size_t outW = width;
  size_t outH = height;
  if (std::max(width, height) > kMaxTexDim) {
    const float scale =
        static_cast<float>(kMaxTexDim) / static_cast<float>(std::max(width, height));
    outW = std::max<size_t>(1, static_cast<size_t>(width * scale));
    outH = std::max<size_t>(1, static_cast<size_t>(height * scale));
  }

  std::vector<unsigned char> pixels(outW * outH * 4u, 0u);
  CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
  CGContextRef context = CGBitmapContextCreate(
      pixels.data(), outW, outH, 8, outW * 4u, colorSpace,
      kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big);
  CGColorSpaceRelease(colorSpace);
  if (context == nullptr) {
    CGImageRelease(image);
    return 0;
  }

  CGContextTranslateCTM(context, 0.0f, static_cast<CGFloat>(outH));
  CGContextScaleCTM(context, 1.0f, -1.0f);
  // Draw into the smaller buffer; CG scales the image down for us.
  CGContextDrawImage(context, CGRectMake(0, 0, outW, outH), image);
  CGContextRelease(context);
  CGImageRelease(image);

  GLuint texture = 0;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, static_cast<GLsizei>(outW),
               static_cast<GLsizei>(outH), 0, GL_RGBA, GL_UNSIGNED_BYTE,
               pixels.data());
  glGenerateMipmap(GL_TEXTURE_2D);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glBindTexture(GL_TEXTURE_2D, 0);

  cache[key] = texture;
  return texture;
}

inline float sampleDiffuseHeight(const std::vector<unsigned char> &pixels,
                                 int width, int height, int x, int y) {
  x = std::clamp(x, 0, width - 1);
  y = std::clamp(y, 0, height - 1);
  const std::size_t index =
      (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) +
       static_cast<std::size_t>(x)) *
      4u;
  const float red = static_cast<float>(pixels[index]) / 255.0f;
  const float green = static_cast<float>(pixels[index + 1u]) / 255.0f;
  const float blue = static_cast<float>(pixels[index + 2u]) / 255.0f;
  return std::max(red, std::max(green, blue));
}

inline bool decodeDiffusePixels(const std::filesystem::path &texturePath,
                                std::vector<unsigned char> &pixels, int &outW,
                                int &outH) {
  if (!std::filesystem::exists(texturePath)) {
    return false;
  }

  CFStringRef pathString = CFStringCreateWithCString(
      kCFAllocatorDefault, texturePath.string().c_str(), kCFStringEncodingUTF8);
  if (pathString == nullptr) {
    return false;
  }

  CFURLRef url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, pathString,
                                               kCFURLPOSIXPathStyle, false);
  CFRelease(pathString);
  if (url == nullptr) {
    return false;
  }

  CGImageSourceRef source = CGImageSourceCreateWithURL(url, nullptr);
  CFRelease(url);
  if (source == nullptr) {
    return false;
  }

  CGImageRef image = CGImageSourceCreateImageAtIndex(source, 0, nullptr);
  CFRelease(source);
  if (image == nullptr) {
    return false;
  }

  const size_t width = CGImageGetWidth(image);
  const size_t height = CGImageGetHeight(image);
  if (width == 0 || height == 0) {
    CGImageRelease(image);
    return false;
  }

  constexpr size_t kMaxTexDim = 1024;
  size_t scaledW = width;
  size_t scaledH = height;
  if (std::max(width, height) > kMaxTexDim) {
    const float scale =
        static_cast<float>(kMaxTexDim) / static_cast<float>(std::max(width, height));
    scaledW = std::max<size_t>(1, static_cast<size_t>(width * scale));
    scaledH = std::max<size_t>(1, static_cast<size_t>(height * scale));
  }

  pixels.assign(scaledW * scaledH * 4u, 0u);
  CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
  CGContextRef context = CGBitmapContextCreate(
      pixels.data(), scaledW, scaledH, 8, scaledW * 4u, colorSpace,
      kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big);
  CGColorSpaceRelease(colorSpace);
  if (context == nullptr) {
    CGImageRelease(image);
    return false;
  }

  CGContextTranslateCTM(context, 0.0f, static_cast<CGFloat>(scaledH));
  CGContextScaleCTM(context, 1.0f, -1.0f);
  CGContextDrawImage(context, CGRectMake(0, 0, scaledW, scaledH), image);
  CGContextRelease(context);
  CGImageRelease(image);

  outW = static_cast<int>(scaledW);
  outH = static_cast<int>(scaledH);
  return true;
}

inline GLuint uploadRgbaTexture(const std::vector<unsigned char> &pixels,
                                int width, int height, bool generateMipmaps) {
  if (pixels.empty() || width <= 0 || height <= 0) {
    return 0;
  }

  GLuint texture = 0;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, pixels.data());
  if (generateMipmaps) {
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR_MIPMAP_LINEAR);
  } else {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  }
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glBindTexture(GL_TEXTURE_2D, 0);
  return texture;
}

// Builds a tangent-space normal map from diffuse luminance (for cutout foliage).
inline GLuint createNormalMapFromDiffuseFile(
    const std::filesystem::path &diffusePath, float strength = 3.0f) {
  if (!std::filesystem::exists(diffusePath)) {
    return 0;
  }

  static std::unordered_map<std::string, GLuint> cache;
  const std::string key =
      std::filesystem::absolute(diffusePath).string() + "|normal|" +
      std::to_string(strength);
  auto found = cache.find(key);
  if (found != cache.end() && found->second != 0) {
    return found->second;
  }

  std::vector<unsigned char> diffusePixels;
  int width = 0;
  int height = 0;
  if (!decodeDiffusePixels(diffusePath, diffusePixels, width, height)) {
    return 0;
  }

  std::vector<unsigned char> normalPixels(
      static_cast<std::size_t>(width * height * 4), 255u);
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const float left = sampleDiffuseHeight(diffusePixels, width, height, x - 1, y);
      const float right = sampleDiffuseHeight(diffusePixels, width, height, x + 1, y);
      const float down = sampleDiffuseHeight(diffusePixels, width, height, x, y - 1);
      const float up = sampleDiffuseHeight(diffusePixels, width, height, x, y + 1);
      const float dx = (right - left) * strength;
      const float dy = (up - down) * strength;
      const float nx = -dx;
      const float ny = -dy;
      const float nz = 1.0f;
      const float invLen = 1.0f / std::sqrt(nx * nx + ny * ny + nz * nz);

      const std::size_t index =
          (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) +
           static_cast<std::size_t>(x)) *
          4u;
      normalPixels[index] =
          static_cast<unsigned char>(std::clamp((nx * invLen * 0.5f + 0.5f) * 255.0f,
                                                0.0f, 255.0f));
      normalPixels[index + 1u] =
          static_cast<unsigned char>(std::clamp((ny * invLen * 0.5f + 0.5f) * 255.0f,
                                                0.0f, 255.0f));
      normalPixels[index + 2u] =
          static_cast<unsigned char>(std::clamp((nz * invLen * 0.5f + 0.5f) * 255.0f,
                                                0.0f, 255.0f));
      normalPixels[index + 3u] = 255u;
    }
  }

  const GLuint texture =
      uploadRgbaTexture(normalPixels, width, height, true);
  if (texture != 0) {
    cache[key] = texture;
  }
  return texture;
}
#else
inline GLuint loadTextureFromFile(const std::filesystem::path &) { return 0; }
inline GLuint createNormalMapFromDiffuseFile(const std::filesystem::path &,
                                             float = 3.0f) {
  return 0;
}
#endif
