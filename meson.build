project('Raytracing',
        ['cpp', 'c'],
        default_options : [
            'warning_level=3',
            'cpp_std=c++20'
        ],
)

incdirs = [
    include_directories('include'),
    include_directories('include/engine'),
    include_directories('include/engine/rendering'),
    include_directories('include/engine/rendering/builders'),
    include_directories('include/engine/rendering/context'),
    include_directories('include/engine/rendering/renderer'),
    include_directories('include/engine/rendering/resources'),
    include_directories('include/engine/rendering/resources/materials'),
    include_directories('include/engine/rendering/resources/pipelines'),
    include_directories('include/engine/rendering/vulkan_scene_representation'),
    include_directories('include/engine/repositories'),
    include_directories('include/engine/runner'),
    include_directories('include/engine/scene_graph'),
    include_directories('include/engine/scene_graph/components'),
    include_directories('include/engine/ui'),
    include_directories('include/io'),
    include_directories('include/properties'),
    include_directories('include/util'),
]

vulkan = dependency('vulkan', version: ['>=1.3.0'], required: false)
if not vulkan.found()
    vulkan = dependency('vulkan')
endif
glfw = dependency('glfw3', version: ['>=3.4.0'], fallback: ['glfw', 'glfw_dep'])
glm = dependency('glm', fallback : ['glm', 'glm_dep'], version: ['>=0.9.9.8'])
imgui = dependency('imgui', fallback: ['imgui', 'imgui_dep'])
stb = dependency('stb', fallback: ['stb', 'stb_dep'], default_options : ['stb_libraries=[\'stb_image\', \'stb_image_write\']'])
spdlog = dependency('spdlog')
assimp = dependency('assimp')
yaml = dependency('yaml-cpp')
openmp = dependency('openmp', required: true)

project_root = meson.current_source_dir()

glslc = find_program('glslangValidator', required: false)
if glslc.found()
    glslc_path = glslc.full_path()
    shader_generator_deps = []
else
    # :( in this case we have to build glslangValidator ourselves
    glslang_options = cmake.subproject_options()
    glslang_options.add_cmake_defines({'GLSLANG_TESTS': false})
    glslang_options.add_cmake_defines({'GLSLANG_ENABLE_INSTALL': false})
    glslang_options.set_install(false)
    # installs as glslang_standalone
    # glslang_options.set_install(true, target: 'glslang-standalone')
    glslang_subp = cmake.subproject('glslang', options: glslang_options)
    glslc_tgt = glslang_subp.target('glslang-standalone')
    glslc_path = glslc_tgt.full_path()
    shader_generator_deps = [glslc_tgt]
endif

spv_compile_script = [meson.project_source_root() + '/scripts/compile_shader.py', '--glslc_path', glslc_path]
glslc_args = ['--target-env', 'vulkan1.3']
glslc_args += ['-I' + meson.project_source_root()]
glslc_args += ['-I' + join_paths(project_root, 'src')]
glslc_args += ['-I' + join_paths(project_root, 'include')]
glslc_args += ['--depfile', '@DEPFILE@']
if get_option('buildtype').startswith('debug')
    glslc_args += ['-g']
endif

py = import('python').find_installation('python3')
shader_generator = generator(
    py,
    output: ['@PLAINNAME@.spv.h', '@PLAINNAME@.spv.c'],
    depfile: '@PLAINNAME@.d',
    arguments: spv_compile_script + ['@INPUT@', '@OUTPUT0@', '@OUTPUT1@'] + glslc_args + ['@EXTRA_ARGS@'],
    depends: shader_generator_deps
)

run_command(py, 'scripts/generate_meson_files.py', 'src', check: true)

src = []
subdir('src')
subdir('shaders')

exe = executable(
    'renderer',
    sources: src,
    dependencies: [
        glfw,
        glm,
        imgui,
        stb,
        spdlog,
        vulkan,
        assimp,
        yaml,
        openmp
    ],
    include_directories: incdirs,
    install : true
)