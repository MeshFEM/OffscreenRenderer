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

#include <glew.h>

struct OpenGLContext {
    void resize(int width, int height) {
        m_width = width;
        m_height = height;
        m_buffer.resize(width * height * 4);
        m_resizeImpl(width, height);
    }

    template<class F> void render(F &&f) {
        std::cout << "render called" << std::endl;
        m_makeCurrent();
        f();
        glFinish();
        m_readImage();
    }

    const std::vector<unsigned char> &buffer() { return m_buffer; }

    void write_ppm(const std::string &path) const {
        std::ofstream outFile(path, std::ofstream::binary);
        if (!outFile.is_open())
            throw std::runtime_error("Failed to open " + path);

        outFile << "P6\n";
        outFile << m_width << " " << m_height << "\n";
        outFile << "255\n";

        // Write the RGB components of the image in PPM format.
        // Due to the opposite vertical axis conventions of OpenGL and PPM, the
        // buffer holds a vertically flipped image; we flip it while writing.
        const size_t npixels = m_buffer.size() / 4;
        for (size_t row = 0; row < m_height; ++row) {
            for (size_t col = 0; col < m_width; ++col) {
                size_t pixel = (m_height - row - 1) * m_width + col;
                outFile.write((const char *) &m_buffer[4 * pixel], 3);
            }
        }
    }

    virtual ~OpenGLContext()  { }

protected:
    int m_width, m_height;
    std::vector<unsigned char> m_buffer;

    virtual void m_makeCurrent() = 0;

    virtual void m_readImage() { }
    virtual void m_resizeImpl(int /* width */, int /* height */) { }
};

#endif /* end of include guard: OPENGLCONTEXT_HH */
