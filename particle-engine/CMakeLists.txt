cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

if(NOT DEFINED PROJECT_VERSION)
    set(PROJECT_VERSION 1.0.0)
endif()

project(particle-engine VERSION ${PROJECT_VERSION})

enable_language(CXX)

configure_file(
    "${PROJECT_SOURCE_DIR}/config.hxx.in"
    "${PROJECT_SOURCE_DIR}/include/config.hxx"
)

find_package(OpenGL REQUIRED)

find_package(glm REQUIRED)
find_package(glfw3 3.3 REQUIRED)
find_package(GLEW 2.1 REQUIRED)
find_package(fmt 5.3 CONFIG REQUIRED)
find_package(Boost 1.69 REQUIRED)

if(NOT WIN32)
	find_package(X11 REQUIRED)
endif()


include_directories(SYSTEM include)
include_directories(src)


set(SOURCE_FILES
    src/math/math.hxx                               src/math/math.cxx

    src/gfx/context.hxx

    src/platform/input/input_data.hxx
    src/platform/input/input_manager.hxx            src/platform/input/input_manager.cxx
    src/platform/input/mouse.hxx                    src/platform/input/mouse.cxx

    src/platform/window.hxx                         src/platform/window.cxx

	src/utility/barrier.hxx                         src/utility/barrier.cxx
	src/utility/helpers.hxx
	src/utility/mpl.hxx

    src/particle_engine.hxx                         src/particle_engine.cxx

    src/main.hxx                                    src/main.cxx
)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

if (CMAKE_CXX_COMPILER_ID MATCHES GNU)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _GLIBCXX_USE_CXX11_ABI=1)
endif ()

if(NOT WIN32)
    set(EXTRA_LIBS ${EXTRA_LIBS}
        stdc++fs
        pthread
    )

    set(EXTRA_LIBS_LINK_OPTIONS ${EXTRA_LIBS}
        -Wl,-no-undefined
        -Wl,-no-allow-shlib-undefined
        -Wl,-unresolved-symbols=report-all
    )
endif()


set_target_properties(${PROJECT_NAME} PROPERTIES
    VERSION ${PROJECT_VERSION}

    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED NO
    CXX_EXTENSIONS OFF

    POSITION_INDEPENDENT_CODE ON

    DEBUG_POSTFIX .d
)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(${PROJECT_NAME} PRIVATE
        -fconcepts

        -Wpedantic
        -Wall
        -Wextra
		-Wold-style-cast
		-Wnon-virtual-dtor
		-Wcast-align
		-Wunused
		-Wconversion
		-Wsign-conversion
		-Wnull-dereference
		-Wdouble-promotion
		-Wformat=2
		-Wduplicated-cond
		-Wduplicated-branches
		-Wlogical-op
		-Wuseless-cast
        # -Wlifetime

        # -fsanitize=thread -fsanitize=address
)

elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wpedantic
        -Wall
        -Wextra
		-Wold-style-cast
		-Wnon-virtual-dtor
		-Wcast-align
		-Wunused
		-Wconversion
		-Wsign-conversion
		-Wnull-dereference
		-Wdouble-promotion
		-Wformat=2

        #-fsanitize=thread -fsanitize=address
)

elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(${PROJECT_NAME} PRIVATE
        /W3
)
endif()


target_include_directories(${PROJECT_NAME} PRIVATE
    OpenGL::GL
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${EXTRA_LIBS_LINK_OPTIONS}

    ${EXTRA_LIBS}
    
    OpenGL::GL

    glm

    glfw
    GLEW::GLEW

	fmt::fmt
)
