////////////////////////////////////////////////////////////////////////////////
// Shader.hh
////////////////////////////////////////////////////////////////////////////////
/*! @file
//  Basic RAII wrappers for shader programs.
*/
//  Author:  Julian Panetta (jpanetta), julian.panetta@gmail.com
//  Created:  09/22/2020 11:26:37
////////////////////////////////////////////////////////////////////////////////
#ifndef SHADER_HH
#define SHADER_HH

#include <vector>
#include <array>
#include <stdexcept>
#include <memory>
#include <fstream>
#include <sstream>

#include "GLTypeTraits.hh"
#include "GLErrors.hh"

// A vertex/fragment/geometry/etc shader object that can be compiled and linked
// into a program
struct ShaderObject {
    // Create shader and load in source (but do not compile yet!)
    ShaderObject(const std::string &source, GLenum type) {
        id = glCreateShader(type);
        const char *src = source.c_str();
        glShaderSource(id, /* one string */1, &src, /* source is null terminated */ NULL);
    }

    void compile() {
        glCompileShader(id);
        glCheckStatus(id, GL_COMPILE_STATUS);
    }

    ~ShaderObject() {
        glDeleteShader(id);
    }

    GLuint id;
};

struct Uniform {
    Uniform(GLuint prog, GLuint idx)
        : index(idx)
    {
        std::array<char, 512> buf;
        GLint size;
        glGetActiveUniform(prog, idx, 512, NULL, &size, &type, buf.data());
        name = std::string(buf.data());
    }

    template<typename T>
    void set(const T &val) {
        using Traits = GLTypeTraits<T>;
        if (Traits::type != type) throw std::runtime_error("Uniform type mismatch");
        Traits::uniformSetter(index, val);
    }

    GLuint index;
    std::string name;
    GLenum type;
};

struct Attribute {
    int index;
    std::string name;
    GLenum type;
};

// Wrapper for a shader program
struct Shader {
    struct ProgRAIIWrapper {
        ProgRAIIWrapper() {
            id = glCreateProgram();
            glCheckError();
        }

        ~ProgRAIIWrapper() {
            glDeleteProgram(id);
        }

        GLuint id;
    };

    using Sources = std::vector<std::string>;
    Shader(const Sources &vtxSources,
           const Sources &fragSources,
           const Sources &geoSources = Sources())
    {
        for (const auto &s :  vtxSources) m_objects.emplace_back(s, GL_VERTEX_SHADER);
        for (const auto &s : fragSources) m_objects.emplace_back(s, GL_FRAGMENT_SHADER);
        for (const auto &s :  geoSources) m_objects.emplace_back(s, GL_GEOMETRY_SHADER);

        // Compile and attach all shader objects
        for (auto &obj : m_objects) {
            obj.compile();
            glAttachShader(m_prog.id, obj.id);
        }

        glLinkProgram(m_prog.id);
        glCheckStatus(m_prog.id, GL_LINK_STATUS);

        // Get uniform information
        GLint numUniforms;
        glGetProgramiv(m_prog.id, GL_ACTIVE_UNIFORMS, &numUniforms);
        for (GLuint i = 0; i < GLuint(numUniforms); ++i)
            m_uniforms.emplace_back(m_prog.id, i);

        // TODO: Query attributes
    }

    Shader(const std::string &vtx, const std::string &frag, const std::string &geo)
        : Shader(Sources(1, vtx), Sources(1, frag), Sources(1, geo)) { }

    Shader(const std::string &vtx, const std::string &frag)
        : Shader(Sources(1, vtx), Sources(1, frag)) { }

    static std::string readFile(const std::string &path) {
        std::ifstream inFile(path);
        if (!inFile.is_open()) throw std::runtime_error("Couldn't open input file " + path);
        std::stringstream ss;
        ss << inFile.rdbuf();
        return ss.str();
    }

    static std::unique_ptr<Shader> fromFiles(const std::string &vtxFile, const std::string &fragFile, const std::string &geoFile = std::string()) {
        if (geoFile.size())
            return std::make_unique<Shader>(readFile(vtxFile), readFile(fragFile), readFile(geoFile));
        return std::make_unique<Shader>(readFile(vtxFile), readFile(fragFile));
    }

    void use() { glUseProgram(m_prog.id); }

    template<typename T>
    void setUniform(const std::string &name, const T &val) {
        for (Uniform &u : m_uniforms) {
            if (u.name == name) {
                u.set(val);
                return;
            }
        }
        throw std::runtime_error("Uniform not present: " + name);
    }

    const std::vector<Uniform>   &getUniforms  () { return m_uniforms; }
    const std::vector<Attribute> &getAttributes() { return m_attributes; }
private:
    ProgRAIIWrapper m_prog;
    std::vector<ShaderObject> m_objects;
    std::vector<Uniform>      m_uniforms;
    std::vector<Attribute>    m_attributes;
};

#endif /* end of include guard: SHADER_HH */
