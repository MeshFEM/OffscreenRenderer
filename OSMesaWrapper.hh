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

#include "OpenGLContext.hh"
#include "GL/osmesa.h"

struct OSMesaWrapper : public OpenGLContext {
    OSMesaWrapper(int width, int height, GLenum format = OSMESA_RGBA,
                  GLint depthBits = 24, GLint stencilBits = 0, GLint accumBits = 0)
    {
        m_ctx = OSMesaCreateContextExt(format, depthBits, stencilBits, accumBits, /* sharelist = */ NULL);
        if (!m_ctx) throw std::runtime_error("OSMesaCreateContext failed!");
        resize(width, height);
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
