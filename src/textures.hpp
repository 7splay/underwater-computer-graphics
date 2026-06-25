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
#else
inline GLuint loadTextureFromFile(const std::filesystem::path &) { return 0; }
#endif
