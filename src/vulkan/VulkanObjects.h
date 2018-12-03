#if !defined(VULKANOBJECTS_H)
#define VULKANOBJECTS_H

#include <array>
#include <string>
#include <vector>

#include <glm/glm.hpp>

namespace Rake::Graphics::Object {
/**
 * @class UniformBufferObject
 * @author Salamanderrake
 * @date 02/12/18
 * @file VulkanCommon.h
 * @brief
 */
struct UniformBufferObject {
  glm::mat4 model;
  glm::mat4 view;
  glm::mat4 proj;
};

/**
 * @class Vertex
 * @author Salamanderrake
 * @date 02/12/18
 * @file VulkanCommon.h
 * @brief
 */
struct Vertex {
  glm::vec3 pos;
  glm::vec3 color;
  glm::vec2 texCoord;

  static VkVertexInputBindingDescription getBindingDescription()
  {
    VkVertexInputBindingDescription bindingDescription = {};
    bindingDescription.binding                         = 0;
    bindingDescription.stride                          = sizeof(Vertex);
    bindingDescription.inputRate                       = VK_VERTEX_INPUT_RATE_VERTEX;

    return bindingDescription;
  }

  static std::array<VkVertexInputAttributeDescription, 3> getAttributesDescription()
  {
    std::array<VkVertexInputAttributeDescription, 3> attributesDescription = {};

    attributesDescription[0].binding  = 0;
    attributesDescription[0].location = 0;
    attributesDescription[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
    attributesDescription[0].offset   = offsetof(Vertex, pos);

    attributesDescription[1].binding  = 0;
    attributesDescription[1].location = 1;
    attributesDescription[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
    attributesDescription[1].offset   = offsetof(Vertex, color);

    attributesDescription[2].binding  = 0;
    attributesDescription[2].location = 2;
    attributesDescription[2].format   = VK_FORMAT_R32G32_SFLOAT;
    attributesDescription[2].offset   = offsetof(Vertex, texCoord);

    return attributesDescription;
  }

  bool operator==(const Vertex& other) const
  {
    return pos == other.pos && color == other.color && texCoord == other.texCoord;
  }
};

/**
 * @class Model
 * @author Salamanderrake
 * @date 02/12/18
 * @file VulkanCommon.h
 * @brief
 */
struct Model {
  int width;   // = 800;
  int height;  // = 600;

  std::string modelPath;    // = "data/models/chalet.obj"
  std::string texturePath;  // = "data/textures/chalet.jpg"

  std::vector<Vertex>   verticies;
  std::vector<uint32_t> indices;
  VkBuffer              vertexBuffer;
  VkDeviceMemory        vertexBufferMemory;
};
}  // namespace Rake::Graphics::Object
#endif  // VULKANOBJECTS_H
