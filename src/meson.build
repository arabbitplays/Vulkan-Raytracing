subdir('builders')
subdir('rendering')

shaders = [
    'shaders/miss/miss.rmiss',
    'shaders/miss/environment_miss.rmiss',
    'shaders/miss/shadow_miss.rmiss',
    'shaders/phong/phong_closesthit.rchit',
    'shaders/metalRough/metal_rough_closesthit.rchit',
    'shaders/common/raygen.rgen',
]

foreach s : shaders
    src += shader_generator.process(s)
endforeach

src += files(
    'main.cpp',
)