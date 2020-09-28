////////////////////////////////////////////////////////////////////////////////
// OpenGLContext.hh
////////////////////////////////////////////////////////////////////////////////
/*! @file
//  Simple platform-agnostic interface for RAII OpenGL context creation,
//  rendering and destruction.
*/
//  Author:  Julian Panetta (jpanetta), julian.panetta@gmail.com
//  Created:  09/22/2020 05:05:01
////////////////////////////////////////////////////////////////////////////////
#ifndef OPENGLCONTEXT_HH
#define OPENGLCONTEXT_HH

#include <fstream>
#include <iostream>
#include <vector>
#include <stdexcept>
#include <string>
#include <memory>

#include "write_png.h"
#include "GLErrors.hh"

#include <GL/glew.h>

struct OpenGLContext {
    using ImageBuffer = Eigen::Array<unsigned char, Eigen::Dynamic, 1>;
    // Convenience types for accessing a single component of each RGBA pixel or
    // for accessing the three RGB components.
    using MapCmpnt      = Eigen::Map<      ImageBuffer, 0, Eigen::InnerStride<4>>;
    using MapConstCmpnt = Eigen::Map<const ImageBuffer, 0, Eigen::InnerStride<4>>;
    using MapColor      = Eigen::Map<      Eigen::Array<unsigned char, Eigen::Dynamic, 3, Eigen::RowMajor>, 0, Eigen::OuterStride<4>>;
    using MapConstColor = Eigen::Map<const Eigen::Array<unsigned char, Eigen::Dynamic, 3, Eigen::RowMajor>, 0, Eigen::OuterStride<4>>;

    // Factory method for getting the right platform-specific library
    static std::unique_ptr<OpenGLContext> construct(int width, int height);

    void resize(int width, int height) {
        m_width = width;
        m_height = height;
        m_buffer.resize(width * height * 4);
        m_resizeImpl(width, height);

        glViewport(0, 0, width, height);
    }

    int getWidth()  const { return m_width;  }
    int getHeight() const { return m_height; }

    void makeCurrent() { m_makeCurrent(); }

    template<class F> void render(F &&f) {
        std::cout << "render called" << std::endl;
        makeCurrent();
        f();
    }

    void clear(Eigen::VectorXf color = Eigen::Vector3f::Zero()) {
        makeCurrent();
        if ((color.size() < 3) || (color.size() > 4))
            throw std::runtime_error("Unexpected color size");
        glClearColor(color[0], color[1], color[2], (color.size() == 3)  ? 1.0 : color[3]);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void enable(GLenum capability) {
        makeCurrent();
        glEnable(capability);
        glCheckError("glEnable");
    }

    void disable(GLenum capability) {
        makeCurrent();
        glDisable(capability);
        glCheckError("glDisable");
    }

    void cullFace(GLenum face) {
        makeCurrent();
        glEnable(GL_CULL_FACE);
        glCullFace(face);
        glCheckError("cull face");
    }

	void finish() {
        glFinish();
        m_readImage();
	}

    void blendFunc(GLenum sfactor, GLenum dfactor) { blendFunc(sfactor, dfactor, sfactor, dfactor); }
    void blendFunc(GLenum sfactor, GLenum dfactor, GLenum alpha_sfactor, GLenum alpha_dfactor) {
        makeCurrent();
        glBlendFuncSeparate(sfactor, dfactor, alpha_sfactor, alpha_dfactor);
        glCheckError("blend func");
    }

    const ImageBuffer &buffer() const { return m_buffer; }

    ImageBuffer unpremultipliedBuffer() const {
        // For transparent images, the render output has a "premultiplied alpha"
        // (i.e., the color components are scaled by the alpha component, and
        // the image has already effectively been composited against a black background).
        // We must divide by the alpha channel before saving.
        const int numPixels = m_width * m_height;
        Eigen::ArrayXf colorScale = MapConstCmpnt(m_buffer.data() + 3, numPixels).cast<float>() / 255.0;
        colorScale = (colorScale != 0.0).select(1.0 / colorScale, colorScale);

        ImageBuffer result(m_buffer.size());
        MapColor(result.data(),     numPixels, 3) = ((MapConstColor(m_buffer.data(), numPixels, 3).cast<float>().colwise() * colorScale).round()).cast<unsigned char>();
        MapCmpnt(result.data() + 3, numPixels, 1) = MapConstCmpnt(m_buffer.data() + 3, numPixels, 1);

        // MapColor(result.data()    , numPixels, 3) = MapConstColor(m_buffer.data()    , numPixels, 3);
        // MapCmpnt(result.data() + 3, numPixels, 1) = MapConstCmpnt(m_buffer.data() + 3, numPixels, 1);

        // std::cout << "unpremultipliedBuffer: " << result.head(10).transpose().cast<int>() << std::endl;
        // std::cout << "Result size: " << result.size() << std::endl;

        return result;
    }

    void writePPM(const std::string &path, bool unpremultiply = true) const {
        std::ofstream outFile(path, std::ofstream::binary);
        if (!outFile.is_open())
            throw std::runtime_error("Failed to open " + path);

        outFile << "P6\n";
        outFile << m_width << " " << m_height << "\n";
        outFile << "255\n";

        const ImageBuffer &buf = unpremultiply ? unpremultipliedBuffer() : buffer();

        // Write the RGB components of the image in PPM format.
        // Due to the opposite vertical axis conventions of OpenGL and PPM, the
        // buffer holds a vertically flipped image; we flip it while writing.
        const size_t npixels = buf.size() / 4;
        for (size_t row = 0; row < m_height; ++row) {
            for (size_t col = 0; col < m_width; ++col) {
                size_t pixel = (m_height - row - 1) * m_width + col;
                outFile.write((const char *) &buf[4 * pixel], 3);
            }
        }
    }

    void writePNG(const std::string &path, bool unpremultiply = true) const {
        const ImageBuffer &buf = unpremultiply ? unpremultipliedBuffer() : buffer();
        write_png_RGBA(path, m_width, m_height, buf.data(), /* verticalFlip = */ true);
    }

    virtual ~OpenGLContext()  { }

protected:
    int m_width, m_height;
    ImageBuffer m_buffer;

    virtual void m_makeCurrent() = 0;

    virtual void m_readImage() { }
    virtual void m_resizeImpl(int /* width */, int /* height */) { }

    void m_glewInit() {
        GLenum status = glewInit();
        // Silence spurious error with headless EGL
        // https://github.com/nigels-com/glew/issues/273
        if ((status != GLEW_OK) && (status != GLEW_ERROR_NO_GLX_DISPLAY)) {
            std::cerr << "GLEW Error: " << glewGetErrorString(status) << std::endl;
            throw std::runtime_error("glewInit failure");
        }
    }
};

#if USE_EGL
#include "EGLWrapper.inl"
#elif USE_OSMESA
#include "OSMesaWrapper.inl"
#elif USE_CGL
#include "CGLWrapper.inl"
#else
static_assert(false, "No context wrapper available");
#endif

// Factory method for getting the right platform-specific library
inline std::unique_ptr<OpenGLContext> OpenGLContext::construct(int width, int height) {
    #if USE_EGL
    return std::make_unique<EGLWrapper>(width, height);
    #elif USE_OSMESA
    return std::make_unique<OSMesaWrapper>(width, height);
    #elif USE_CGL
    return std::make_unique<CGLWrapper>(width, height);
    #else
    static_assert(false, "No context wrapper available");
    #endif
}

#endif /* end of include guard: OPENGLCONTEXT_HH */
