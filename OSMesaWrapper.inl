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

        m_makeCurrent();

		// Initialize GLEW entry points for our new context
        glewExperimental=GL_TRUE;
        auto status = glewInit();
        if (status != GLEW_OK) {
            std::cerr << "GLEW Error: " << glewGetErrorString(status) << std::endl;
            throw std::runtime_error("glewInit failure");
        }
    }

    ~OSMesaWrapper() {
        OSMesaDestroyContext(m_ctx);
    }

private:
    virtual void m_makeCurrent() override {
        if (!OSMesaMakeCurrent(m_ctx, m_buffer.data(), GL_UNSIGNED_BYTE, m_width, m_height))
            throw std::runtime_error("OSMesaMakeCurrent failed!");
        std::cout << glGetString(GL_VERSION) << std::endl;
    }

    OSMesaContext m_ctx;
};

#endif /* end of include guard: OSMESAWRAPPER_HH */
