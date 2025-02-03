project('Raytracing',
        ['cpp', 'c'],
        default_options : [
            'warning_level=3',
            'cpp_std=c++20',
        ],
)

incdirs = [
    include_directories('include'),
    include_directories('include/builders'),
    include_directories('include/rendering/camera'),
    include_directories('include/rendering/engine'),
    include_directories('include/rendering/options'),
    include_directories('include/rendering/ressources'),
    include_directories('include/rendering/ressources/materials'),
    include_directories('include/rendering/scene_graph'),
    include_directories('include/rendering/ui'),
    include_directories('include/rendering'),
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
assimp_dep = dependency('assimp')

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

src = []
subdir('src')

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
        assimp_dep,
    ],
    include_directories: incdirs,
    install : true
)