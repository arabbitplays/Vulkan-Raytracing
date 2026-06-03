shaders = [
    'miss/environment_miss.rmiss',
    'phong/phong_closesthit.rchit',
    'metalRough/metal_rough_closesthit.rchit',
    'metalRough/metal_rough_miss.rmiss',
    'phong/phong_raygen.rgen',
    'metalRough/metal_rough_raygen.rgen',
    'nee/shadow_clostesthit.rchit',
    'nee/shadow_miss.rmiss',
    'compute/glitch.comp',
]

foreach s : shaders
    src += shader_generator.process(s)
endforeach