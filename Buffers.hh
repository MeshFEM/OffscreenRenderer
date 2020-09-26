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
#include "UASetters.hh"

// We need to use row-major types in order to interpret each row as giving a
// vertex's attributes (following libigl conventions).
using MXfR  = Eigen::Matrix<float       , Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;
using MXuiR = Eigen::Matrix<unsigned int, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;

struct BufferObject : RAIIGLResource<BufferObject> {
    using Base = RAIIGLResource<BufferObject>;
    using Base::id;

    BufferObject() { }
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
        m_count = A.rows();
    }

    size_t count() const { return m_count; }

private:
    friend class RAIIGLResource<BufferObject>;
    void m_delete() { glDeleteBuffers(1, &id); }
    size_t m_count = 0;
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
    void setAttribute(int loc, Eigen::Ref<const MXfR> A) {
        bind(); // Bind this VAO to modify it
        m_attributes[loc] = BufferObject(A);
        auto &buf = m_attributes[loc];
        buf.bind(GL_ARRAY_BUFFER);
        glVertexAttribPointer(loc,
                              A.cols(), GL_FLOAT, // # cols floats per vertex
                              GL_FALSE,  // Don't normalize
                              0, 0); // No gap between vertex data, and no offset from array beginning.
        glCheckError();
        glEnableVertexAttribArray(loc);
        glCheckError();
    }

    template<typename T>
    void setConstantAttribute(int loc, const T &a) {
        bind();
        m_attributes[loc] = BufferObject(); // Empty dummy attribute -- we need to set this to pass the validation in `draw`

        glDisableVertexAttribArray(GLuint(loc));
        detail::setAttribute(GLuint(loc), a);

        glCheckError();
    }

    void setIndexBuffer(Eigen::Ref<const MXuiR> A) {
        bind(); // Bind this VAO to modify it
        m_indexBuffer = BufferObject(A);
        m_indexBuffer.bind(GL_ELEMENT_ARRAY_BUFFER);
        glCheckError();
    }

    void unsetIndexBuffer() {
        bind(); // Bind this VAO to modify it
        m_indexBuffer = BufferObject();
        m_indexBuffer.bind(GL_ELEMENT_ARRAY_BUFFER);
        glCheckError();
    }

    void bind() const { glBindVertexArray(id); }

    void draw(const Shader &s) const {
        size_t numChecked = 0;
        for (const auto &attr : s.getAttributes()) {
            if (m_attributes.count(attr.loc)) ++numChecked;
            else if (attr.name.substr(0, 3) != "gl_") { // Ignore auto-generated attributes like gl_VertexID
                throw std::runtime_error("Attribute " + std::to_string(attr.loc) + " (" + attr.name + ") is not set in VAO");
            }
        }
        if (numChecked != m_attributes.size()) throw std::runtime_error("Extraneous attributes found in VAO");
        if (!s.allUniformsSet()) {
            std::string msg = "Unset uniform(s):";
            for (const Uniform &u : s.getUniforms())
                msg += " " + u.name;
            throw std::runtime_error(msg);
        }

        s.use();
        bind();

        if (m_indexBuffer.allocated())
            glDrawElements(GL_TRIANGLES, m_indexBuffer.count(), GL_UNSIGNED_INT, NULL);
        else
            glDrawArrays(GL_TRIANGLES, 0, m_attributes.at(0).count());
        glCheckError();
    }

    const std::map<int, BufferObject> &attributeBuffers() const { return m_attributes;  }
    const BufferObject                &indexBuffer()      const { return m_indexBuffer; }

private:
    std::map<int, BufferObject> m_attributes;
    BufferObject m_indexBuffer;
    friend class RAIIGLResource<VertexArrayObject>;
    void m_delete() { glDeleteVertexArrays(1, &id); /* std::cout << "Delete vertex array " << id << std::endl; */ }
};

#endif /* end of include guard: BUFFERS_HH */
