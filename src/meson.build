shaders = [
    'shader.frag',
    'shader.vert',
]

foreach s : shaders
    src += shader_generator.process(s)
endforeach

src += files(
    'main.cpp',
    'DescriptorAllocator.cpp',
    'PipelineBuilder.cpp',
)