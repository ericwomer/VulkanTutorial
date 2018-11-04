#include <iostream>

#include "vktutorialapp.h"

rake::vlkn::vkTutorialApp vktutorialapp;

int main(int argv, char* argc[])
{
    if (argv == 1) {
        vktutorialapp.main();
    } else {
        vktutorialapp.main(argv, argc);
    }
    return 0;
}
