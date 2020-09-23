////////////////////////////////////////////////////////////////////////////////
// EGLWrapper.hh
////////////////////////////////////////////////////////////////////////////////
/*! @file
//  Wrapper for RAII EGL context creation, rendering, and destruction.
//  Adapted from https://developer.nvidia.com/blog/egl-eye-opengl-visualization-without-x-server/
*/
//  Author:  Julian Panetta (jpanetta), julian.panetta@gmail.com
//  Created:  09/22/2020 05:15:43
////////////////////////////////////////////////////////////////////////////////
#ifndef EGLWRAPPER_HH
#define EGLWRAPPER_HH

#include "OpenGLContext.hh"
#include <EGL/egl.h>

struct EGLWrapper : public OpenGLContext {
    EGLWrapper(int width, int height, GLenum /* format */ = GL_RGBA,
               GLint depthBits = 24, GLint /* stencilBits */ = 0, GLint /* accumBits */ = 0)
    {
        // Initialize EGL
        m_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

        EGLint major, minor;
        eglInitialize(m_display, &major, &minor);
        std::cout << major << ", " << minor << std::endl;

        // Bind the API
        eglBindAPI(EGL_OPENGL_API);

        // Select an appropriate configuration
        EGLint numConfigs;

        const EGLint configAttribs[] = {
            EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
            EGL_BLUE_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_RED_SIZE, 8,
            EGL_DEPTH_SIZE, depthBits,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
            EGL_NONE
        };

        eglChooseConfig(m_display, configAttribs, &m_config, 1, &numConfigs);

        resize(width, height);
    }

    ~EGLWrapper() {
        m_destroy_size_specific();
        eglTerminate(m_display);
    }

private:
    virtual void m_makeCurrent() override {
        eglMakeCurrent(m_display, m_surf, m_surf, m_ctx);
        std::cout << glGetString(GL_VERSION) << std::endl;
    }

    void m_destroy_size_specific() {
        if (m_ctx  != nullptr) { eglDestroyContext(m_display, m_ctx ); m_ctx  = nullptr; }
        if (m_surf != nullptr) { eglDestroySurface(m_display, m_surf); m_surf = nullptr; }
    }

    virtual void m_readImage() override {
        glReadPixels(0, 0, m_width, m_height, GL_RGBA, GL_UNSIGNED_BYTE, m_buffer.data());
    }

    virtual void m_resizeImpl(int width, int height) override {
        m_destroy_size_specific();

        static const EGLint pbufferAttribs[] = {
            EGL_WIDTH, width,
            EGL_HEIGHT, height,
            EGL_NONE,
        };

        // Create a surface
        m_surf = eglCreatePbufferSurface(m_display, m_config, pbufferAttribs);
        if (!m_surf) throw std::runtime_error("eglCreatePbufferSurface failed");

        // Create a context and make it current
        m_ctx = eglCreateContext(m_display, m_config, EGL_NO_CONTEXT, NULL);
        if (!m_surf) throw std::runtime_error("eglCreateContext failed");
    }

    EGLDisplay m_display = nullptr;
    EGLConfig  m_config = nullptr;
    EGLSurface m_surf = nullptr;
    EGLContext m_ctx = nullptr;
};

#endif /* end of include guard: EGLWRAPPER_HH */
