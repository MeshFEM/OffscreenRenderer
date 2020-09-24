////////////////////////////////////////////////////////////////////////////////
// RAIIGLResource.hh
////////////////////////////////////////////////////////////////////////////////
/*! @file
//  CRTP class providing a safe wrapper around an OpenGL resource
//  (shader, program, etc.) that guards against leaks, dangling ids, and double
//  frees.
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
        glCheckError("Resource creation");
        if (id == 0) throw std::runtime_error("Resource creation failed");
    }

    // Eliminate dangerous copy constructor/assignment;
    // provide move constructor instead.
    RAIIGLResource(const RAIIGLResource &) = delete;
    RAIIGLResource(RAIIGLResource &&b) : id(b.id) { b.id = 0; }

    ~RAIIGLResource() {
        if (id != 0)
            static_cast<Derived *>(this)->m_delete();
    }

    GLuint id = 0;
};

#endif /* end of include guard: RAIIGLRESOURCE_HH */
