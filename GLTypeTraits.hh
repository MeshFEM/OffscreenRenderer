////////////////////////////////////////////////////////////////////////////////
// GLTypeTraits.hh
////////////////////////////////////////////////////////////////////////////////
/*! @file
//  Template metaprogramming for converting C/Eigen types to GLSL types.
//  Currently we only convert some of the basic integer and single precision
//  floating point types from:
//      https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glGetActiveUniform.xhtml
*/
//  Author:  Julian Panetta (jpanetta), julian.panetta@gmail.com
//  Created:  09/22/2020 11:48:27
////////////////////////////////////////////////////////////////////////////////
#ifndef GLTYPES_HH
#define GLTYPES_HH

#include <Eigen/Dense>
#include <GL/glew.h>
#include <type_traits>
#include <map>

inline const char *getGLTypeName(GLenum type) {
    static const std::map<GLenum, const char *> lut {
        {GL_FLOAT       , "GL_FLOAT"       },
        {GL_INT         , "GL_INT"         },
        {GL_UNSIGNED_INT, "GL_UNSIGNED_INT"},
        {GL_BOOL        , "GL_BOOL"        },
        {GL_FLOAT_VEC2  , "GL_FLOAT_VEC2"  },
        {GL_FLOAT_VEC3  , "GL_FLOAT_VEC3"  },
        {GL_FLOAT_VEC4  , "GL_FLOAT_VEC4"  },
        {GL_FLOAT_MAT2  , "GL_FLOAT_MAT2"  },
        {GL_FLOAT_MAT3  , "GL_FLOAT_MAT3"  },
        {GL_FLOAT_MAT4  , "GL_FLOAT_MAT4"  }
    };
    auto it = lut.find(type);
    if (it == lut.end()) throw std::runtime_error("Unhandled type id: " + std::to_string(type));
    return it->second;
}

template<typename T>
struct GLTypeTraitsImpl;

template<> struct GLTypeTraitsImpl<GLfloat>         { static constexpr GLenum type = GL_FLOAT       ; static void uniformSetter(GLint loc,                  GLfloat f) { glUniform1f       (loc,           f);                   } };
template<> struct GLTypeTraitsImpl<GLint>           { static constexpr GLenum type = GL_INT         ; static void uniformSetter(GLint loc,                    GLint i) { glUniform1i       (loc,           i);                   } };
template<> struct GLTypeTraitsImpl<GLuint>          { static constexpr GLenum type = GL_UNSIGNED_INT; static void uniformSetter(GLint loc,                   GLuint u) { glUniform1ui      (loc,           u);                   } };
template<> struct GLTypeTraitsImpl<bool>            { static constexpr GLenum type = GL_BOOL        ; static void uniformSetter(GLint loc,                     bool b) { glUniform1i       (loc,           static_cast<int>(b)); } };
template<> struct GLTypeTraitsImpl<Eigen::Vector2f> { static constexpr GLenum type = GL_FLOAT_VEC2  ; static void uniformSetter(GLint loc, const Eigen::Vector2f &vec) { glUniform2fv      (loc, 1,        vec.data());          } };
template<> struct GLTypeTraitsImpl<Eigen::Vector3f> { static constexpr GLenum type = GL_FLOAT_VEC3  ; static void uniformSetter(GLint loc, const Eigen::Vector3f &vec) { glUniform3fv      (loc, 1,        vec.data());          } };
template<> struct GLTypeTraitsImpl<Eigen::Vector4f> { static constexpr GLenum type = GL_FLOAT_VEC4  ; static void uniformSetter(GLint loc, const Eigen::Vector4f &vec) { glUniform4fv      (loc, 1,        vec.data());          } };
template<> struct GLTypeTraitsImpl<Eigen::Matrix2f> { static constexpr GLenum type = GL_FLOAT_MAT2  ; static void uniformSetter(GLint loc, const Eigen::Matrix2f &mat) { glUniformMatrix2fv(loc, 1, false, mat.data());          } };
template<> struct GLTypeTraitsImpl<Eigen::Matrix3f> { static constexpr GLenum type = GL_FLOAT_MAT3  ; static void uniformSetter(GLint loc, const Eigen::Matrix3f &mat) { glUniformMatrix3fv(loc, 1, false, mat.data());          } };
template<> struct GLTypeTraitsImpl<Eigen::Matrix4f> { static constexpr GLenum type = GL_FLOAT_MAT4  ; static void uniformSetter(GLint loc, const Eigen::Matrix4f &mat) { glUniformMatrix4fv(loc, 1, false, mat.data());          } };

// "Decay" Eigen expression templates to their underlying evaluated type.
// Leave non-Eigen types unchanged.
template<typename T>
struct EigenDecay { using type = T; };
template<typename Derived>
struct EigenDecay<Eigen::MatrixBase<Derived>> { using type = decltype(std::declval<Derived>().eval()); };

template<typename T>
using GLTypeTraits = GLTypeTraitsImpl<typename EigenDecay<std::decay_t<T>>::type>;

#endif /* end of include guard: GLTYPES_HH */
