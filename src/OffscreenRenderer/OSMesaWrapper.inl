////////////////////////////////////////////////////////////////////////////////
// OSMesaWrapper.hh
////////////////////////////////////////////////////////////////////////////////
/*! @file
//  A simple C++ wrapper for RAII OSMesa context creation, rendering, and
//  destruction.
*/
//  Author:  Julian Panetta (jpanetta), julian.panetta@gmail.com
//  Created:  09/22/2020 02:47:20
////////////////////////////////////////////////////////////////////////////////
#ifndef OSMESAWRAPPER_HH
#define OSMESAWRAPPER_HH

#include <GL/gl.h>
#define GLAPI extern // Annoyingly, GLEW undefs this macro, breaking osmesa...
#include <GL/osmesa.h>

struct OSMesaWrapper : public OpenGLContext {
    OSMesaWrapper(int width, int height, GLenum format = OSMESA_RGBA,
                  GLint depthBits = 24, GLint stencilBits = 0, GLint accumBits = 0)
    {
        const GLint ctxAttribs[] = {
            OSMESA_FORMAT,                GLint(format),
            OSMESA_DEPTH_BITS,            depthBits,
            OSMESA_STENCIL_BITS,          stencilBits,
            OSMESA_ACCUM_BITS,            accumBits,
            OSMESA_CONTEXT_MAJOR_VERSION, 3,
            OSMESA_CONTEXT_MINOR_VERSION, 1,
            0
        };
        m_ctx = OSMesaCreateContextAttribs(ctxAttribs, /* sharelist = */ NULL);
        if (!m_ctx) throw std::runtime_error("OSMesaCreateContext failed!");

        resize(width, height);

        // Initialize GLEW entry points for our new context
        // (The call to glViewport made by `resize` above should be OK as it is
        //  part of OpenGL 1.0 and therefore not managed by GLEW.)
        m_glewInit();
        std::cout << "gl version: " << glGetString(GL_VERSION) << std::endl;
    }

    virtual ~OSMesaWrapper() {
        OSMesaDestroyContext(m_ctx);
    }

private:
    virtual void m_resizeImpl(int /* width */, int /* height */) override {
         // This is called after the buffer, m_width, and m_height are
         // initialized/updated but before the glViewport call--the exact time
         // we can and need to make this context current.
         makeCurrent();
    }

    virtual void m_makeCurrent() override {
        if (!OSMesaMakeCurrent(m_ctx, m_buffer.data(), GL_UNSIGNED_BYTE, m_width, m_height))
            throw std::runtime_error("OSMesaMakeCurrent failed!");
    }

    OSMesaContext m_ctx;
};

#endif /* end of include guard: OSMESAWRAPPER_HH */
