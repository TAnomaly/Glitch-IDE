cmake_minimum_required(VERSION 3.10)
project(ModernTextEditor)

# C++ standart
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows specific settings
if(WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUNICODE -D_UNICODE")
endif()

# Source files
set(SOURCES
    main.cpp
    TextEditor.cpp
    # TextEditor_Part2.cpp içeriğini TextEditor.cpp'ye ekleyin
)

# Header files
set(HEADERS
    EditorPane.h
    TextEditor.h
)

# Executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Windows libraries
if(WIN32)
    target_link_libraries(${PROJECT_NAME} 
        gdi32 
        user32 
        kernel32 
        comdlg32
    )
endif()

# Compiler specific options
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Debug/Release configurations
set_target_properties(${PROJECT_NAME} PROPERTIES
    DEBUG_POSTFIX "_d"
)

# Install
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
) 