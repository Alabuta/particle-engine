#pragma once

#include <string>
using namespace std::string_literals;

#include <fmt/format.h>

//#define GL_GLEXT_PROTOTYPES 0

#include <GL/glew.h>
#include <GL/glcorearb.h>

//#ifdef _MSC_VER
//    #define WGL_WGLEXT_PROTOTYPES 0
//    #include <GL/wglext.h>
//
//#elif defined(__GNUG__)
//    #include <GL/glx.h>
//    #include <GL/glxext.h>
//#endif

#ifdef _MSC_VER
    #pragma comment(lib, "OpenGL32.lib")
    #ifdef _DEBUG
        #pragma comment(lib, "glew32d.lib")
    #else
        #pragma comment(lib, "glew32.lib")
    #endif
    #pragma comment(lib, "glfw3dll.lib")
#endif

#include "platform/window.hxx"


namespace gfx {
    struct context final {
        GLFWwindow *handle{nullptr};

        context(platform::window &window) : handle{window.handle()}
        {
            glfwMakeContextCurrent(handle);
            glfwSwapInterval(0);

            //glewExperimental = true;

            if (auto result = glewInit(); result != GLEW_OK)
                throw std::runtime_error(fmt::format("failed to init GLEW: {0:#x}\n"s, result));

            glPointSize(2.f);

            glClearColor(0.f, .254901f, .6f, 1.f);

            glDisable(GL_CULL_FACE);

            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glEnable(GL_BLEND);

            glEnable(GL_DEPTH_TEST);
            glEnable(GL_DEPTH_CLAMP);
            glDepthFunc(GL_LESS);

            //glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
        }
    };

    inline void draw_point(float x, float y, float r, float g, float b, float a)
    {
        glColor4f(r, g, b, a);
        glVertex2f(x, y);
    }
}
