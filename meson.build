project('Raytracing', 'cpp',
        default_options : [
            'warning_level=3',
            'cpp_std=c++20',
        ],
)

incdir = include_directories('include')

vulkan = dependency('vulkan', version: ['>=1.3.0'], required: false)
if not vulkan.found()
    vulkan = dependency('vulkan')
endif

glfw = dependency('glfw3', version: ['>=3.4.0'], fallback: ['glfw', 'glfw_dep'])
glm = dependency('glm', fallback : ['glm', 'glm_dep'], version: ['>=0.9.9.8'])
stb = dependency('stb', fallback: ['stb', 'stb_dep'], default_options : ['stb_libraries=[\'stb_image\', \'stb_image_write\']'])
tol = dependency('tinyobjloader', fallback : ['tinyobjloader', 'tinyobjloader_dep'])

src = []
subdir('src')

exe = executable(
    'renderer',
    src,
    dependencies: [
        glfw,
        glm,
        stb,
        tol,
        vulkan,
    ],
    include_directories: incdir,
    install : true
)