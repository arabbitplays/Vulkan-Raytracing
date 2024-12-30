subdir('builders')
subdir('rendering')

shaders = [
    'shaders/miss/miss.rmiss',
    'shaders/miss/shadow_miss.rmiss',
    'shaders/phong/closesthit.rchit',
    'shaders/common/raygen.rgen',
]

foreach s : shaders
    src += shader_generator.process(s)
endforeach

src += files(
    'main.cpp',
)