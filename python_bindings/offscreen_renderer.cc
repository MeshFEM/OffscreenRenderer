#include <pybind11/pybind11.h>
#include <pybind11/eigen.h>
#include <pybind11/stl.h>

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

PYBIND11_MODULE(_offscreen_renderer, m) {
    py::class_<OpenGLContext>(m, "OpenGLContext")
        .def(py::init(&OpenGLContext::construct), py::arg("width"), py::arg("height"))
        .def("resize", &OpenGLContext::resize,    py::arg("width"), py::arg("height"))
        .def("makeCurrent", &OpenGLContext::makeCurrent)
        .def("finish",      &OpenGLContext::finish)
        .def("buffer",      &OpenGLContext::buffer)
        .def("enable",      &OpenGLContext::enable,   py::arg("capability"))
        .def("clear",       &OpenGLContext::clear,    py::arg("color") = Eigen::Vector3f::Zero())
        .def("writePPM",    &OpenGLContext::writePPM, py::arg("path"))
        .def_property_readonly("width",  &OpenGLContext::getWidth)
        .def_property_readonly("height", &OpenGLContext::getHeight)
        ;

    bindGLEnum(m);

    py::class_<Uniform>(m, "Uniform")
        .def_readonly("index", &Uniform::index)
        .def_readonly("size",  &Uniform::size)
        .def_readonly("name",  &Uniform::name)
        .def_property_readonly("type", [](const Uniform &u) { return wrapGLenum(u.type); })
        .def("__repr__", [](const Uniform &u) { return "Uniform '" + u.name + "': " + getGLenumRepr(u.type); })
        ;

    py::class_<Attribute>(m, "Attribute")
        .def_readonly("index", &Attribute::index)
        .def_readonly("size",  &Attribute::size)
        .def_readonly("name",  &Attribute::name)
        .def_property_readonly("type", [](const Attribute &a) { return wrapGLenum(a.type); })
        .def("__repr__", [](const Attribute &a) { return "Attribute " + std::to_string(a.index) + " ('" + a.name + "'): " + getGLenumRepr(a.type); })
        ;

    py::class_<Shader> pyShader(m, "Shader");
    pyShader
        .def(py::init<const std::string &, const std::string &>(), py::arg("vtx"), py::arg("frag"))
        .def(py::init<const std::string &, const std::string &, const std::string &>(), py::arg("vtx"), py::arg("frag"), py::arg("geo"))
        .def("use", &Shader::use)
        .def_property_readonly("uniforms",   &Shader::getUniforms,   py::return_value_policy::reference)
        .def_property_readonly("attributes", &Shader::getAttributes, py::return_value_policy::reference)
        ;

    addSetUniformBindings<int, float, Eigen::Vector2f, Eigen::Vector3f, Eigen::Vector4f,
                                      Eigen::Matrix2f, Eigen::Matrix3f, Eigen::Matrix4f>::run(pyShader);

    py::class_<BufferObject>(m, "BufferObject")
        .def("bind", &BufferObject::bind)
        .def("setData", [](BufferObject &b, Eigen::Ref<const MXfR > data, GLenumWrapper usage) { b.setData(data, unwrapGLenum(usage)); }, py::arg("data"), py::arg("usage") = GLenumWrapper::wGL_DYNAMIC_DRAW)
        .def("setData", [](BufferObject &b, Eigen::Ref<const MXuiR> data, GLenumWrapper usage) { b.setData(data, unwrapGLenum(usage)); }, py::arg("data"), py::arg("usage") = GLenumWrapper::wGL_DYNAMIC_DRAW)
        ;

    py::class_<VertexArrayObject>(m, "VertexArrayObject")
        .def(py::init<>())
        .def("setAttribute",   &VertexArrayObject::setAttribute,   py::arg("index"), py::arg("A"))
        .def("setIndexBuffer", &VertexArrayObject::setIndexBuffer, py::arg("A"))
        .def("bind", &VertexArrayObject::bind)
        .def("draw", &VertexArrayObject::draw, py::arg("shader"))
        .def_property_readonly("attributeBuffers", &VertexArrayObject::attributeBuffers, py::return_value_policy::reference)
        .def_property_readonly("indexBuffer",      &VertexArrayObject::indexBuffer,      py::return_value_policy::reference)
        ;
}
