glslc = find_program('glslc', requried: true)

shaders_args = ['--target-env=vulkan1.1', '-c', '@INPUT@']
shaders_input = ['triangle.vert', 'triangle.frag']
shaders_output = ['triangle.vert.spv', 'triangle.frag.spv']

if get_option('debug') == true
  shaders_args += ['-O0', '-g']
else
  shaders_args += ['-O', '-w']
endif

shaders = custom_target('shaders',
  build_by_default: true,
  build_always_stale: false,
  command: [glslc, shaders_args],
  input: shaders_input,
  output: shaders_output
  )

