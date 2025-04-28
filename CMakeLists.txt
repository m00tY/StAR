cmake_minimum_required(VERSION 3.10)
project(star)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Ask R for its CFLAGS and LDFLAGS
execute_process(COMMAND R CMD config --cppflags OUTPUT_VARIABLE R_CPPFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND R CMD config --ldflags OUTPUT_VARIABLE R_LDFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)

# Apply R flags
add_compile_options(${R_CPPFLAGS})
link_libraries(${R_LDFLAGS})

# Add source files
add_executable(star
    src/main.cpp
    src/sourceparser.cpp
	src/contractparser.cpp
)


