////////////////////////////////////////////////////////////////////////////////
// RAIIGLResource.hh
////////////////////////////////////////////////////////////////////////////////
/*! @file
//  CRTP class providing a safe wrapper around an OpenGL resource (shader,
//  buffer, etc.) guarding against leaks, dangling ids, and double frees.
*/
//  Author:  Julian Panetta (jpanetta), julian.panetta@gmail.com
//  Created:  09/23/2020 17:37:35
////////////////////////////////////////////////////////////////////////////////
#ifndef RAIIGLRESOURCE_HH
#define RAIIGLRESOURCE_HH

template<class Derived>
struct RAIIGLResource {
    RAIIGLResource() { }
    RAIIGLResource(GLuint _id) : id(_id) {
        m_validateConstruction();
    }

    // Eliminate dangerous copy constructor/assignment;
    // provide move constructor/assignment instead.
    RAIIGLResource(const RAIIGLResource &) = delete;
    RAIIGLResource(RAIIGLResource &&b) : id(b.id) { b.id = 0; }

    RAIIGLResource &operator=(const RAIIGLResource &  ) = delete;
    RAIIGLResource &operator=(      RAIIGLResource &&b) { id = b.id; b.id = 0; return *this; }

    bool allocated() const { return id != 0; }

    ~RAIIGLResource() {
        if (allocated()) {
            // std::cout << "Deleting resource " << id << std::endl;
            static_cast<Derived *>(this)->m_delete();
        }
    }

    GLuint id = 0;
protected:
    void m_validateConstruction() {
        glCheckError("resource creation");
        if (id == 0) throw std::runtime_error("Resource creation failed");
    }
};

#endif /* end of include guard: RAIIGLRESOURCE_HH */