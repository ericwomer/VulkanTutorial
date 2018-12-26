project(
    'Vulkan Tutorial',
    'cpp',
    version : '0.0.0', # This is oveverridden in the conf_data section
    default_options:
    [
        'cpp_std=c++17',
        'warning_level=3',
    ],
    meson_version: '>=0.48.0'
)

executable_name = 'vktut'

pkgconfig = import('pkgconfig')

cpp = meson.get_compiler('cpp')

conf_data = configuration_data()
conf_data.set('name', meson.project_name())

conf_data.set('compiler_name', cpp.get_id())
conf_data.set('compiler_version', cpp.version())
conf_data.set('host_machine', host_machine.system())
conf_data.set('cpu_family', build_machine.cpu_family())

conf_data.set('major', '0')
conf_data.set('minor', '0')
conf_data.set('patch', '1')
conf_data.set('compile', '0')

configure_file(input : 'src/config.h.in',
              output : 'config.h',
              configuration : conf_data)

vktutorial_src = [
    'src/vulkan/VulkanFactories.cpp',
    'src/vulkan/VulkanFunctions.cpp',
    'src/vulkan/VulkanCore.cpp',
    'src/vktutorialapp.cpp',
    'src/main.cpp',
    'src/vulkan/VulkanUtilities.cpp'
]

vktutorial_include_directories = [
    include_directories('src/'),
    include_directories('include/'),
    include_directories('thirdparty/include/'),
    include_directories('/usr/include/c++/v1/'),
    include_directories('/usr/include/')
    ]

vktutorial_cflags = [
    '-stdlib=libc++'
    ]

if get_option('debug') == true or get_option('buildtype') == 'debug'
  vktutorial_cflags += ['-g']
else
  vktutorial_cflags += ['-O2', '-DNDEBUG']
endif

vktutorial_ldflags = [
    '-lm',
    '-lc++',
    '-lc++abi',
    '-ldl'
    ]

xcb = dependency('xcb')
glm = dependency('glm')

#sdl = dependency('sdl2')

vktutorial_deps = [xcb, glm]#, sdl]

vktutorial = executable(
    executable_name,
    vktutorial_src,
    install : true,
    dependencies: vktutorial_deps,
    include_directories: vktutorial_include_directories,
    cpp_args: vktutorial_cflags,
    link_args: vktutorial_ldflags
)

install_data('LICENSE', install_dir: join_paths('share/doc', executable_name))

if get_option('build-docs')
    subdir('doc')
endif

subdir('data/shaders/')

