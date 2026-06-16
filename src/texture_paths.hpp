#pragma once

#include <algorithm>
#include <filesystem>
#include <string>

inline std::string toLowerCopy(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return value;
}

inline std::filesystem::path firstExistingPath(
    std::initializer_list<std::filesystem::path> candidates) {
  for (const auto &candidate : candidates) {
    if (std::filesystem::exists(candidate)) {
      return candidate;
    }
  }
  return {};
}

inline std::filesystem::path modelImageRoot(const std::filesystem::path &modelPath) {
  return modelPath.parent_path().parent_path() / "img";
}

inline std::filesystem::path resolveDiffuseTexturePath(
    const std::filesystem::path &modelPath) {
  const std::string stem = toLowerCopy(modelPath.stem().string());
  const std::filesystem::path imgRoot = modelImageRoot(modelPath);

  if (stem == "coral1") {
    return firstExistingPath({imgRoot / "coral_1" / "coral1Basecolor.jpg",
                              imgRoot / "coral_1" / "coral1Diffuse.jpg"});
  }
  if (stem == "coral2") {
    return firstExistingPath({imgRoot / "coral_2" / "Coral2Base.jpg",
                              imgRoot / "coral_2" / "Coral2Diffuse.jpg"});
  }
  if (stem == "coral3") {
    return firstExistingPath({imgRoot / "coral_3" / "CoralBase.jpg",
                              imgRoot / "coral_3" / "Coral3Diffuse.jpg"});
  }
  if (stem == "coral4") {
    return firstExistingPath({imgRoot / "coral_4" / "Coral4Base.jpg",
                              imgRoot / "coral_4" / "Coral4Diffuse.jpg"});
  }
  if (stem == "coral5") {
    return firstExistingPath({imgRoot / "coral_5" / "Coral5Base.jpg",
                              imgRoot / "coral_5" / "Coral5Diffuse.jpg"});
  }
  if (stem == "coral6") {
    return firstExistingPath({imgRoot / "coral_6" / "Coral6Base.jpg",
                              imgRoot / "coral_6" / "Coral6Diffuse.jpg"});
  }
  if (stem == "coral7") {
    return firstExistingPath({imgRoot / "coral_7" / "Coral7Base.jpg",
                              imgRoot / "coral_7" / "Coral7Diffuse.jpg"});
  }
  if (stem == "coral8") {
    return firstExistingPath({imgRoot / "coral_8" / "Coral8Base.jpg",
                              imgRoot / "coral_8" / "Coral8Diffuse.jpg"});
  }
  if (stem == "coral9") {
    return firstExistingPath({imgRoot / "coral_9" / "Coral9Base.jpg",
                              imgRoot / "coral_9" / "Coral9Diffuse.jpg"});
  }
  if (stem == "coral10") {
    return firstExistingPath({imgRoot / "coral_10" / "Coral10Base.jpg",
                              imgRoot / "coral_10" / "Coral10Diffuse.jpg"});
  }
  if (stem == "coral11") {
    return firstExistingPath({imgRoot / "coral_11" / "Coral11Base.jpg",
                              imgRoot / "coral_11" / "Coral11Diffuse.jpg"});
  }
  if (stem == "ship") {
    return firstExistingPath({imgRoot / "ship" / "ship.png",
                              imgRoot / "ship" / "texture.png"});
  }
  if (stem == "shark") {
    return firstExistingPath({imgRoot / "shark" / "Nurse_Shark_Quad_Diffuse.png",
                              imgRoot / "shark" / "Nurse_Shark_Tris_Diffuse.png"});
  }
  if (stem == "fish") {
    return firstExistingPath(
        {imgRoot / "fish" / "Tailor_low_DefaultMaterial_BaseColor.png",
         imgRoot / "fish" / "texture.png"});
  }
  if (stem == "fishes") {
    return firstExistingPath(
        {imgRoot / "fishes" / "Striped-Dottyback-Pseudochromis-sankeyi.jpg",
         imgRoot / "fishes" / "fishes.jpg"});
  }
  return {};
}

inline std::filesystem::path resolveNormalTexturePath(
    const std::filesystem::path &modelPath) {
  const std::string stem = toLowerCopy(modelPath.stem().string());
  const std::filesystem::path imgRoot = modelImageRoot(modelPath);

  if (stem == "coral1") {
    return firstExistingPath({imgRoot / "coral_1" / "coral1Normals.jpg"});
  }
  if (stem == "coral2") {
    return firstExistingPath({imgRoot / "coral_2" / "Coral2Normals.jpg"});
  }
  if (stem == "coral3") {
    return firstExistingPath({imgRoot / "coral_3" / "Coral3Normals.jpg"});
  }
  if (stem == "coral4") {
    return firstExistingPath({imgRoot / "coral_4" / "Coral4Normals.jpg"});
  }
  if (stem == "coral5") {
    return firstExistingPath({imgRoot / "coral_5" / "Coral5Normals.jpg"});
  }
  if (stem == "coral6") {
    return firstExistingPath({imgRoot / "coral_6" / "Coral6Normals.jpg"});
  }
  if (stem == "coral7") {
    return firstExistingPath({imgRoot / "coral_7" / "Coral7Normals.jpg"});
  }
  if (stem == "coral8") {
    return firstExistingPath({imgRoot / "coral_8" / "coral8Normals.jpg"});
  }
  if (stem == "coral9") {
    return firstExistingPath({imgRoot / "coral_9" / "coral9Normals.jpg"});
  }
  if (stem == "coral10") {
    return firstExistingPath({imgRoot / "coral_10" / "coral10Normals.jpg"});
  }
  if (stem == "coral11") {
    return firstExistingPath({imgRoot / "coral_11" / "coral12Normals.jpg",
                              imgRoot / "coral_11" / "coral11Normals.jpg"});
  }
  if (stem == "fish") {
    return firstExistingPath(
        {imgRoot / "fish" / "Tailor_low_DefaultMaterial_Normal.png"});
  }
  if (stem == "shark") {
    return firstExistingPath({imgRoot / "shark" / "Nurse_Shark_Quad_Normal.png",
                              imgRoot / "shark" / "Nurse_Shark_Tris_Normal.png"});
  }
  return {};
}

inline std::filesystem::path resolveMetallicTexturePath(
    const std::filesystem::path &modelPath) {
  const std::string stem = toLowerCopy(modelPath.stem().string());
  const std::filesystem::path imgRoot = modelImageRoot(modelPath);

  if (stem == "coral1") {
    return firstExistingPath({imgRoot / "coral_1" / "coral1Metallic.jpg"});
  }
  if (stem == "coral2") {
    return firstExistingPath({imgRoot / "coral_2" / "Coral2Metalic.jpg",
                              imgRoot / "coral_2" / "Coral2Metallic.jpg"});
  }
  if (stem == "coral3") {
    return firstExistingPath({imgRoot / "coral_3" / "CoralMetallic.jpg"});
  }
  if (stem == "coral4") {
    return firstExistingPath({imgRoot / "coral_4" / "Coral4Metallic.jpg"});
  }
  if (stem == "coral5") {
    return firstExistingPath({imgRoot / "coral_5" / "Coral5Metallic.jpg"});
  }
  if (stem == "coral6") {
    return firstExistingPath({imgRoot / "coral_6" / "Coral6Metallic.jpg"});
  }
  if (stem == "coral7") {
    return firstExistingPath({imgRoot / "coral_7" / "Coral7Metallic.jpg"});
  }
  if (stem == "coral8") {
    return firstExistingPath({imgRoot / "coral_8" / "coral8Metallic.jpg"});
  }
  if (stem == "coral9") {
    return firstExistingPath({imgRoot / "coral_9" / "coral9Metallic.jpg"});
  }
  if (stem == "coral10") {
    return firstExistingPath({imgRoot / "coral_10" / "coral10Metallic.jpg"});
  }
  if (stem == "coral11") {
    return firstExistingPath({imgRoot / "coral_11" / "coral12Metallic.jpg",
                              imgRoot / "coral_11" / "coral11Metallic.jpg"});
  }
  if (stem == "fish") {
    return firstExistingPath(
        {imgRoot / "fish" / "Tailor_low_DefaultMaterial_Metallic.png"});
  }
  return {};
}

inline std::filesystem::path resolveRoughnessTexturePath(
    const std::filesystem::path &modelPath) {
  const std::string stem = toLowerCopy(modelPath.stem().string());
  const std::filesystem::path imgRoot = modelImageRoot(modelPath);

  if (stem == "fish") {
    return firstExistingPath(
        {imgRoot / "fish" / "Tailor_low_DefaultMaterial_Roughness.png"});
  }
  if (stem == "shark") {
    return firstExistingPath({imgRoot / "shark" / "Nurse_Shark_Quad_Roughness.png",
                              imgRoot / "shark" / "Nurse_Shark_Tris_Roughness.png"});
  }
  return {};
}
