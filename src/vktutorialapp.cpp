#include <iostream>
#include <string>

#include <thread>
#include <chrono>

#include "vktutorialapp.h"
#include "config.h"

#include "vulkan/VulkanFunctions.h"

namespace Rake::Application {

/**
 * @brief Construct a new vk Tutorial App::vk Tutorial App object
 *
 */
vkTutorialApp::vkTutorialApp()
{
  version({Major, Minor, Patch, Compile});
  name(project_name);
  app_description.push_back(std::string("vkTutorialApp \n"));
  app_description.push_back(std::string("** nothing else follows ** \n"));
}

/**
 * @brief // All of the application starts here if any parameters
 * are used they are handled with member vars.
 *
 * @return int
 */
int vkTutorialApp::main()
{
  // SDL_Event event;
  // Start Main Application Here.
  init_window();
  vkcore.init_vulkan(connection, handle);

  /*   while (event.type != SDL_QUIT) {
        SDL_PollEvent(&event);
    }
*/
  if (!rendering_loop()) {
    cleanup();
    return EXIT_FAILURE;
  }
  cleanup();
  return EXIT_SUCCESS;
}

/**
* @brief This is the main that handles parameters
*
* @param params
* @return int
*/
int vkTutorialApp::main(std::vector<std::string>& params)
{
  // std::vector<std::string> actions;
  std::vector<std::string> dump;

  // iterate through params to remove the -- from the text
  for (std::vector<std::string>::const_iterator i = params.begin(); i != params.end(); ++i) {
    if (*i == "--help" || *i == "-h") {
      version();
      help();
      return EXIT_SUCCESS;  // exit the app
    } else if (*i == "--version" || *i == "-V") {
      version();
      return EXIT_SUCCESS;
    } else {  // catch all to make sure there are no invalid parameters
      dump.push_back(*i);
    }
  }

  return main();
}

/**
* @brief
*
*/
void vkTutorialApp::help(void)
{
  std::cout << "Usage: " << app_name << " [options] files...\n\n";
  std::cout << "Options: \n";
  std::cout << " -h, --help \t\t Print this help message and exit the program.\n";
  std::cout << " -V, --version \t\t Print the version and exit.\n";
}

/**
 * @brief
 */
void vkTutorialApp::version() const
{
  std::cout << app_name << " version 0.0.1.0\n";
  std::cout << "Compiler: " << compiler_name << " " << compiler_version << "\n";
  std::cout << "Operating System: " << operating_system << "\n";
  std::cout << "Architecture: " << cpu_family << std::endl;
}

/**
* @brief
*
*/
void vkTutorialApp::init_window()
{
  int screen_index = 0;
  connection       = xcb_connect(nullptr, &screen_index);

  if (!connection) {
    std::runtime_error("Error in XCB connection creation!");
    return;
  }

  const xcb_setup_t*    setup           = xcb_get_setup(connection);
  xcb_screen_iterator_t screen_iterator = xcb_setup_roots_iterator(setup);

  while (screen_index-- > 0) {
    xcb_screen_next(&screen_iterator);
  }

  xcb_screen_t* screen = screen_iterator.data;
  handle               = xcb_generate_id(connection);

  uint32_t value_list[] = {screen->white_pixel,
                           XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_STRUCTURE_NOTIFY};

  xcb_create_window(connection,
                    XCB_COPY_FROM_PARENT,
                    handle,
                    screen->root,
                    29,
                    29,
                    this->width,
                    this->height,
                    0,
                    XCB_WINDOW_CLASS_INPUT_OUTPUT,
                    screen->root_visual,
                    XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK,
                    value_list);

  xcb_flush(connection);
  xcb_change_property(connection,
                      XCB_PROP_MODE_REPLACE,
                      handle,
                      XCB_ATOM_WM_NAME,
                      XCB_ATOM_STRING,
                      8,
                      strlen(app_name.c_str()),
                      app_name.c_str());
}

/**
* @brief
*
*/
void vkTutorialApp::init_input()
{
  // SDL_Init(SDL_INIT_EVENTS);
}

/**
* @brief
*
* @return true
* @return false
*/
bool vkTutorialApp::rendering_loop()
{
  xcb_intern_atom_cookie_t protocols_cookie = xcb_intern_atom(connection, 1, 12, "WM_PROTOCOLS");
  xcb_intern_atom_reply_t* protocols_reply  = xcb_intern_atom_reply(connection, protocols_cookie, 0);
  xcb_intern_atom_cookie_t delete_cookie    = xcb_intern_atom(connection, 0, 16, "WM_DELETE_WINDOW");
  xcb_intern_atom_reply_t* delete_reply     = xcb_intern_atom_reply(connection, delete_cookie, 0);
  xcb_change_property(connection,
                      XCB_PROP_MODE_REPLACE,
                      handle,
                      (*protocols_reply).atom,
                      4,
                      32,
                      1,
                      &(*delete_reply).atom);
  free(protocols_reply);

  // Display window
  xcb_map_window(connection, handle);
  xcb_flush(connection);

  // Main message loop
  xcb_generic_event_t* xevent;
  bool                 loop   = true;
  bool                 resize = false;
  bool                 result = true;

  while (loop) {
    if (xevent = xcb_poll_for_event(connection); xevent) {
      // Process event
      switch (xevent->response_type & 0x7f) {
          // resize
        case XCB_CONFIGURE_NOTIFY: {
          xcb_configure_notify_event_t* configure_event = (xcb_configure_notify_event_t*)xevent;
          width                                         = configure_event->width;
          height                                        = configure_event->height;

          if (((configure_event->width > 0) && (width != configure_event->width)) ||
              ((configure_event->height > 0) && height != configure_event->height)) {
            resize = true;
            width  = configure_event->width;
            height = configure_event->height;
          }
        } break;
          //close
        case XCB_CLIENT_MESSAGE:
          if ((*(xcb_client_message_event_t*)xevent).data.data32[0] == (*delete_reply).atom) {
            loop = false;
            free(delete_reply);
          }
          break;
        case XCB_KEY_PRESS:
          loop = false;
          break;
      }
      free(xevent);
    } else {
      // Draw
      if (resize) {
        resize = false;
        if (!vkcore.on_window_size_changed()) {
          result = false;
          break;
        }
      }
      if (vkcore.ready_to_draw()) {
        if (!vkcore.draw()) {
          result = false;
          break;
        }
      } else {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
    }
  }
  // \todo
  // vkDeviceWaitIdle(device);
  return result;
}

}  // namespace Rake::Application
