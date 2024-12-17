subdir('builders')
subdir('rendering/engine')
subdir('rendering/nodes')
subdir('rendering/ressources')

shaders = [
    'shaders/miss.rmiss',
    'shaders/shadow_miss.rmiss',
    'shaders/closesthit.rchit',
    'shaders/raygen.rgen',
]

foreach s : shaders
    src += shader_generator.process(s)
endforeach

src += files(
    'main.cpp',
)