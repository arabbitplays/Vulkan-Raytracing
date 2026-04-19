shaders = [
    'miss/miss.rmiss',
    'miss/environment_miss.rmiss',
    'miss/shadow_miss.rmiss',
    'phong/phong_closesthit.rchit',
    'metalRough/metal_rough_closesthit.rchit',
    'metalRough/metal_rough_miss.rmiss',
    'phong/phong_raygen.rgen',
    'metalRough/metal_rough_raygen.rgen',
    'compute/glitch.comp',
]

foreach s : shaders
    src += shader_generator.process(s)
endforeach