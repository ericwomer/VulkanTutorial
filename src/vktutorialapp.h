#if !defined(SKELETONAPP_H)
#define SKELETONAPP_H

#include <string>
#include <iostream>

// #include <SDL.h>

#include "skeleton/skeleton.h"
#include "vulkan/VulkanCommon.h"

// clang-format off
// clang-format on

namespace Rake { namespace Application {

class vkTutorialApp : public Rake::Base::Skeleton {
  public:
  // Core Application Public Interface Methods
  vkTutorialApp();
  virtual ~vkTutorialApp(){};
  virtual int               main() override;
  virtual int               main(std::vector<std::string>& params) override;
  virtual int               size() override { return sizeof(this); };
  virtual const std::string name() override { return app_name; };
  virtual void              help() override;
  virtual std::string       name() const override { return app_name; };
  virtual void              name(const std::string& name) override { app_name = name; };
  virtual void              version() const override;
  virtual void              version(const Version_t& version) override { version_number = version; };

  private:
  // Core Application Private Member Variables
  std::vector<std::string> actions;

  // Vulkan Private Member Variables
  uint32_t width  = 640;
  uint32_t height = 480;

  // SDL_Window* window;
  void*             window;  // Eric: void for now until I get the windowing part up and runnint
  xcb_connection_t* connection = nullptr;
  xcb_window_t      handle     = 0;

  Graphics::Core vkcore;

  bool rendering_loop();
  void init_window();
  void init_input();
  void cleanup()
  {
    vkcore.cleanup();
    if (connection != nullptr) {
      xcb_destroy_window(connection, handle);
      xcb_disconnect(connection);
    }
  }
};

}}      // namespace Rake::Application
#endif  // SKELETONAPP_H
