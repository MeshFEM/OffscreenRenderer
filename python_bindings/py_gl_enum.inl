enum class GLenumWrapper : GLenum {
    // Types
    wGL_FLOAT        = GL_FLOAT       ,
    wGL_INT          = GL_INT         ,
    wGL_UNSIGNED_INT = GL_UNSIGNED_INT,
    wGL_BOOL         = GL_BOOL        ,
    wGL_FLOAT_VEC2   = GL_FLOAT_VEC2  ,
    wGL_FLOAT_VEC3   = GL_FLOAT_VEC3  ,
    wGL_FLOAT_VEC4   = GL_FLOAT_VEC4  ,
    wGL_FLOAT_MAT2   = GL_FLOAT_MAT2  ,
    wGL_FLOAT_MAT3   = GL_FLOAT_MAT3  ,
    wGL_FLOAT_MAT4   = GL_FLOAT_MAT4  ,

    // Capabilities
    wGL_DEPTH_TEST = GL_DEPTH_TEST
};

GLenumWrapper wrapGLenum(GLenum val) {
    return static_cast<GLenumWrapper>(val);
}

GLenum unwrapGLenum(GLenumWrapper val) {
    return static_cast<GLenum>(val);
}

void bindGLEnum(py::module &m) {
    py::enum_<GLenumWrapper>(m, "GLenum")
        // Types
        .value("GL_FLOAT"       , GLenumWrapper::wGL_FLOAT)
        .value("GL_INT"         , GLenumWrapper::wGL_INT)
        .value("GL_UNSIGNED_INT", GLenumWrapper::wGL_UNSIGNED_INT)
        .value("GL_BOOL"        , GLenumWrapper::wGL_BOOL)
        .value("GL_FLOAT_VEC2"  , GLenumWrapper::wGL_FLOAT_VEC2)
        .value("GL_FLOAT_VEC3"  , GLenumWrapper::wGL_FLOAT_VEC3)
        .value("GL_FLOAT_VEC4"  , GLenumWrapper::wGL_FLOAT_VEC4)
        .value("GL_FLOAT_MAT2"  , GLenumWrapper::wGL_FLOAT_MAT2)
        .value("GL_FLOAT_MAT3"  , GLenumWrapper::wGL_FLOAT_MAT3)
        .value("GL_FLOAT_MAT4"  , GLenumWrapper::wGL_FLOAT_MAT4)

        // Capabilities
        .value("GL_DEPTH_TEST", GLenumWrapper::wGL_DEPTH_TEST)
        ;
}
