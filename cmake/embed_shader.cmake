file(READ "${INPUT}" SHADER_CONTENT)
file(WRITE "${OUTPUT}" "#pragma once\nconstexpr const char ${VAR_NAME}[] = R\"GLSL(${SHADER_CONTENT})GLSL\";\n")
