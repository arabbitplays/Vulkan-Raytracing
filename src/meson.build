subdir('builders')
subdir('rendering/engine')
subdir('rendering/nodes')

shaders = [
    'shaders/shader.frag',
    'shaders/shader.vert',
    'shaders/miss.rmiss',
    'shaders/closesthit.rchit',
    'shaders/raygen.rgen',
]

foreach s : shaders
    src += shader_generator.process(s)
endforeach

src += files(
    'main.cpp',
)