cmake_minimum_required(VERSION 3.16)
project(RivenCompiler VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Compiler flags ─────────────────────────────────────────────────────────
if (MSVC)
    add_compile_options(/W4 /WX-)
else()
    add_compile_options(
        -Wall -Wextra -Wpedantic
        -Wno-unused-parameter
        -Wno-unused-variable
    )
endif()

# ── Source files ───────────────────────────────────────────────────────────
set(RIVEN_SOURCES
    src/main.cpp
    src/Lexer.cpp
    src/Parser.cpp
    src/SemanticAnalyzer.cpp
    src/CodeGen.cpp
)

set(RIVEN_HEADERS
    include/Token.h
    include/Lexer.h
    include/AST.h
    include/Parser.h
    include/SemanticAnalyzer.h
    include/CodeGen.h
)

# ── Executable ────────────────────────────────────────────────────────────
add_executable(rivc ${RIVEN_SOURCES} ${RIVEN_HEADERS})
target_include_directories(rivc PRIVATE include)

# Link filesystem on GCC < 9
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_libraries(rivc stdc++fs)
endif()

# ── Install ───────────────────────────────────────────────────────────────
install(TARGETS rivc RUNTIME DESTINATION bin)

# ── Tests (optional) ──────────────────────────────────────────────────────
enable_testing()

# Run every .rn file in tests/ through the compiler
file(GLOB RIVEN_TESTS "tests/*.rn")
foreach(TEST_FILE ${RIVEN_TESTS})
    get_filename_component(TEST_NAME ${TEST_FILE} NAME_WE)
    add_test(
        NAME compile_${TEST_NAME}
        COMMAND rivc --emit-cpp ${TEST_FILE} -o /dev/null
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
endforeach()

# ── Summary ───────────────────────────────────────────────────────────────
message(STATUS "")
message(STATUS "══════════════════════════════════════════════")
message(STATUS "  Riven Compiler v${PROJECT_VERSION}")
message(STATUS "  Build type : ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++        : ${CMAKE_CXX_STANDARD}")
message(STATUS "  Compiler   : ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Output     : rivc")
message(STATUS "══════════════════════════════════════════════")
message(STATUS "")
