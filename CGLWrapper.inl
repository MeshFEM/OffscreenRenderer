////////////////////////////////////////////////////////////////////////////////
// CGLWrapper.hh
////////////////////////////////////////////////////////////////////////////////
/*! @file
//  Wrapper for RAII CGL context creation, rendering, and destruction.
//  Adapted from
//      https://stackoverflow.com/questions/37077935/osx-offscreen-rendering-coregl-framebuffers-shaders-headache
//      http://renderingpipeline.com/2012/05/windowless-opengl-on-macos-x/
*/
//  Author:  Julian Panetta (jpanetta), julian.panetta@gmail.com
//  Created:  09/22/2020 05:15:43
////////////////////////////////////////////////////////////////////////////////
#ifndef EGLWRAPPER_HH
#define EGLWRAPPER_HH

#include <OpenGL/CGLTypes.h>
#include <OpenGL/CGLCurrent.h>
#include <OpenGL/OpenGL.h>

struct CGLWrapper : public OpenGLContext {
    CGLWrapper(int width, int height, GLenum /* format */ = GL_RGBA,
               GLint depthBits = 24, GLint /* stencilBits */ = 0, GLint /* accumBits */ = 0)
    {
		std::cout << "Initialize CGL" << std::endl;
        // Initialize CGL
        CGLPixelFormatAttribute attributes[4] = {
            kCGLPFAAccelerated,   // no software rendering
            kCGLPFAOpenGLProfile, // core profile with the version stated below
            (CGLPixelFormatAttribute) kCGLOGLPVersion_3_2_Core,
            (CGLPixelFormatAttribute) 0
        };

        CGLPixelFormatObj pix;
        GLint num;
        CGLError errorCode = CGLChoosePixelFormat(attributes, &pix, &num);
        if (errorCode != kCGLNoError)
            throw std::runtime_error("CGLChoosePixelFormat failure");

        errorCode = CGLCreateContext(pix, NULL, &m_ctx);
        if (errorCode != kCGLNoError)
            throw std::runtime_error("CGLCreateContext failure");

        CGLDestroyPixelFormat(pix);

        errorCode = CGLSetCurrentContext(m_ctx);
        if (errorCode != kCGLNoError)
            throw std::runtime_error("CGLSetCurrentContext failure");

        std::cout << glGetString(GL_VERSION) << std::endl;

		// Initialize GLEW entry points for our new context
		m_glewInit();

        glGenFramebuffers(1,  &m_frameBufferID);
        glGenRenderbuffers(1, &m_renderBufferID);
        glGenRenderbuffers(1, &m_depthBufferID);

        resize(width, height);

        glBindFramebuffer(GL_FRAMEBUFFER, m_frameBufferID);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_renderBufferID);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT , GL_RENDERBUFFER,  m_depthBufferID);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            throw std::runtime_error("framebuffer is not complete!");

        resize(width, height);
    }

    ~CGLWrapper() {
        glDeleteRenderbuffers(1, &m_renderBufferID);
        glDeleteRenderbuffers(1, &m_depthBufferID);
        glDeleteFramebuffers (1, &m_frameBufferID);

        CGLSetCurrentContext(nullptr);
        CGLDestroyContext(m_ctx);
    }

private:
    virtual void m_makeCurrent() override {
        CGLSetCurrentContext(m_ctx);
    }

    virtual void m_readImage() override {
        glBindFramebuffer(GL_FRAMEBUFFER, m_frameBufferID);
        glBindRenderbuffer(GL_RENDERBUFFER, m_renderBufferID);
        glReadPixels(0, 0, m_width, m_height, GL_RGBA, GL_UNSIGNED_BYTE, m_buffer.data());
    }

    virtual void m_resizeImpl(int width, int height) override {
        glBindFramebuffer(GL_FRAMEBUFFER, m_frameBufferID);
        glBindRenderbuffer(GL_RENDERBUFFER, m_renderBufferID);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_RGB8, width, height);

        glBindRenderbuffer(GL_RENDERBUFFER, m_depthBufferID);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
    }

    CGLContextObj m_ctx = nullptr;
    GLuint m_frameBufferID = 0,
           m_renderBufferID = 0,
           m_depthBufferID = 0;
};

#endif /* end of include guard: EGLWRAPPER_HH */
