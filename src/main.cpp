#include <iostream>
#include <stdexcept>

#include "vktutorialapp.h"

rake::vlkn::vkTutorialApp vktutorialapp;

int main(int argc, char* argv[])
{
    std::vector<std::string> params;
    for (int i = 0; i != argc; i++) {
        params.push_back(argv[i]);
    }
    try {
        vktutorialapp.main(params);
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
