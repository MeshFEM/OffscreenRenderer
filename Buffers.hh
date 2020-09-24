////////////////////////////////////////////////////////////////////////////////
// Buffers.hh
////////////////////////////////////////////////////////////////////////////////
/*! @file
//  Manage VAOs, VBOs
*/
//  Author:  Julian Panetta (jpanetta), julian.panetta@gmail.com
//  Created:  09/23/2020 17:40:22
////////////////////////////////////////////////////////////////////////////////
#ifndef BUFFERS_HH
#define BUFFERS_HH

#include "GLTypeTraits.hh"
#include "RAIIGLResource.hh"

// We need to use row-major types in order to interpret each row as giving a
// vertex's attributes (following libigl conventions).
using MXfR  = Eigen::Matrix<float       , Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;
using MXuiR = Eigen::Matrix<unsigned int, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;

struct BufferObject : RAIIGLResource<BufferObject> {
    using Base = RAIIGLResource<BufferObject>;
    using Base::id;

    BufferObject(Eigen::Ref<const MXfR > A) { glGenBuffers(1, &id); this->m_validateConstruction(); setData(A); }
    BufferObject(Eigen::Ref<const MXuiR> A) { glGenBuffers(1, &id); this->m_validateConstruction(); setData(A); }

    void bind(GLenum target) const { glBindBuffer(target, id); }

    template<class Derived>
    void setData(const Eigen::MatrixBase<Derived> &A,
                 GLenum usage = GL_DYNAMIC_DRAW) { // In our typical use cases, buffers may change every frame...
        bind(GL_ARRAY_BUFFER);
        glBufferData(GL_ARRAY_BUFFER,
                     A.size() * sizeof(typename Derived::Scalar),
                     A.derived().data(),
                     usage); // We assume buffers change rarely
    }

private:
    friend class RAIIGLResource<BufferObject>;
    void m_delete() { glDeleteBuffers(1, &id); }
};

struct VertexArrayObject : RAIIGLResource<VertexArrayObject> {
    using Base = RAIIGLResource<VertexArrayObject>;
    using Base::id;

    VertexArrayObject() {
        glGenVertexArrays(1, &id);
        this->m_validateConstruction();
    }

    // Each row of A is interpreted as a vertex attribute,
    // so A's column size determines the attribute size.
    void addAttribute(Eigen::Ref<const MXfR> A) {
        bind(); // Bind this VAO to modify it
        m_buffers.emplace_back(A);
        auto &buf = m_buffers.back();
        buf.bind(GL_ARRAY_BUFFER);
        size_t attribIdx = m_buffers.size() - 1;
        glVertexAttribPointer(attribIdx,
                              A.cols(), GL_FLOAT, // # cols floats per vertex
                              GL_FALSE,  // Don't normalize
                              0, 0); // No gap between vertex data, and no offset from array beginning.
        glCheckError();
        glEnableVertexAttribArray(attribIdx);
        glCheckError();
    }

    void addIndexBuffer(Eigen::Ref<const MXuiR> A) {
        bind(); // Bind this VAO to modify it
        m_buffers.emplace_back(A);
        auto &buf = m_buffers.back();
        buf.bind(GL_ELEMENT_ARRAY_BUFFER);
        glCheckError();
        m_size = A.size();
    }

    void bind() const { glBindVertexArray(id); }

    void draw() const {
        bind();
        if (m_size == 0) { throw std::runtime_error("Unknown size; did you forget to add an index buffer?"); }
        glDrawElements(GL_TRIANGLES, m_size, GL_UNSIGNED_INT, NULL);
        glCheckError();
    }

    const std::vector<BufferObject> &buffers() const { return m_buffers; }

private:
    std::vector<BufferObject> m_buffers;
    friend class RAIIGLResource<VertexArrayObject>;
    void m_delete() { glDeleteVertexArrays(1, &id); }
    GLsizei m_size = 0;
};

#endif /* end of include guard: BUFFERS_HH */
