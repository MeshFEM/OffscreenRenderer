#include <pybind11/eigen.h>
#include <pybind11/pybind11.h>

#include "../Shader.hh"
#include "../OpenGLContext.hh"
#include "../Buffers.hh"

namespace py = pybind11;

template<typename... T>
struct addSetUniformBindings;

template<>
struct addSetUniformBindings<> {
    template<class PyShader>
    static void run(PyShader &/* pyShader */) { }
};

template<typename T, typename... Trest>
struct addSetUniformBindings<T, Trest...>
{
    template<class PyShader>
    static void run(PyShader &pyShader) {
        pyShader.def("setUniform", &Shader::setUniform<T>, py::arg("name"), py::arg("val"));
        addSetUniformBindings<Trest...>::run(pyShader);
    }
};

#include "py_gl_enum.inl"

PYBIND11_MODULE(py_offscreen_renderer, m) {
    py::class_<OpenGLContext>(m, "OpenGLContext")
        .def(py::init(&OpenGLContext::construct), py::arg("width"), py::arg("height"))
        .def("resize", &OpenGLContext::resize, py::arg("width"), py::arg("height"))
        .def("makeCurrent", &OpenGLContext::makeCurrent)
        .def("finish", &OpenGLContext::finish)
        .def("buffer", &OpenGLContext::buffer)
        ;

    bindGLEnum(m);

    py::class_<Uniform>(m, "Uniform")
        .def_readonly("index", &Uniform::index)
        .def_readonly("name",  &Uniform::name)
        .def_property_readonly("type", [](const Uniform &u) { return wrapGLenum(u.type); })
        ;

    py::class_<Shader> pyShader(m, "Shader");
    pyShader
        .def(py::init<const std::string &,
                      const std::string &,
                      const std::string &>(), py::arg("vtx"), py::arg("frag"), py::arg("geo") = std::string())
        .def("use", &Shader::use)
        .def("getUniforms", &Shader::getUniforms, py::return_value_policy::reference)
        ;

    addSetUniformBindings<int, float, Eigen::Vector2f, Eigen::Vector3f, Eigen::Vector4f,
                                      Eigen::Matrix2f, Eigen::Matrix3f, Eigen::Matrix4f>::run(pyShader);

    py::class_<VertexArrayObject>(m, "VertexArrayObject")
        .def(py::init<>())
        .def("addAttribute", &VertexArrayObject::addAttribute, py::arg("A"))
        .def("addIndexBuffer", &VertexArrayObject::addIndexBuffer, py::arg("A"))
        .def("bind", &VertexArrayObject::bind)
        .def("draw", &VertexArrayObject::draw)
        .def_property_readonly("buffers", &VertexArrayObject::buffers)
        ;
}
