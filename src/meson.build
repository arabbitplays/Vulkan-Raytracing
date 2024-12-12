subdir('builders')
subdir('rendering/engine')
subdir('rendering/nodes')

shaders = [
    'shaders/shader.frag',
    'shaders/shader.vert',
]

foreach s : shaders
    src += shader_generator.process(s)
endforeach

src += files(
    'main.cpp',
)