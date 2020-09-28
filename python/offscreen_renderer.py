import _offscreen_renderer
from _offscreen_renderer import *
import os
import numpy as np

SHADER_DIR = os.path.dirname(__file__) + '/../shaders'

class ShaderLibrary:
    """
    Load GLSL shaders from files, with caching.
    """
    def __init__(self):
        self.shaders = {}
    def load(self, vtxFile, fragFile, geoFile = ""):
        files = [vtxFile, fragFile]
        if geoFile != "": files.append(geoFile)
        files = tuple(files)
        if files not in self.shaders:
            self.shaders[files] = Shader(*[open(f).read() for f in files])
        return self.shaders[files]

class OpenGLContext(_offscreen_renderer.OpenGLContext):
    def array(self, unpremultiply = True):
        self.finish() # Copy image to internal buffer
        # Reshape flat buffer into (height, width, components) 3D array
        # and flip vertically (since OpenGL's image coordinate system is vertically flipped
        # with respect to PIL/png/etc.)
        buf = self.unpremultipliedBuffer() if unpremultiply else self.buffer()
        return buf.reshape((self.height, self.width, 4))[::-1,:,:]

    def image(self, unpremultiply = True):
        from PIL import Image
        return Image.fromarray(self.array(unpremultiply))

    def save(self, path):
        ext = os.path.splitext(path)[-1][1:].lower()
        if (ext not in ['png', 'ppm']): raise Exception('Output file extension not supported')
        self.finish() # Copy image to internal buffer
        if ext == 'png': self.writePNG(path)
        if ext == 'ppm': self.writePPM(path)

    def renderMeshNonIndexed(self, V, N, *args, **kwargs):
        self.renderMesh(V, None, N, *args, **kwargs)

    def shaderLibrary(self):
        if not hasattr(self, '_shaderLib'):
            self._shaderLib = ShaderLibrary()
        return self._shaderLib

    def renderMesh(self, V, F, N, color, matProjection, matModelView, lightEyePos, diffuseIntensity, ambientIntensity, specularIntensity=np.zeros(3), shininess=10.0, lineWidth = 0.0, alpha = 1.0, wireframeColor = [0.0, 0.0, 0.0, 1.0]):
        wireframe = lineWidth != 0
        constColor = len(color.ravel()) in [3, 4]

        if len(N) != len(V): raise Exception('Unexpected normal array size (must be per-vertex)')
        if not constColor and (len(color) != len(V)): raise Exception('Unexpected `color` size (must be per-vertex or constant)')

        if wireframe and F is not None:
            # Our wireframe rendering approach needs a copy of each vertex per
            # triangle.
            V = V[F.ravel()]
            N = N[F.ravel()]
            if not constColor:
                color = color[F.ravel()]
            F = None

        if (alpha != 1.0):
            # Sort the triangles in increasing order by their barycenters'
            # z coordinates in eye space. Note, we discard the irrelevant
            # translation part of `matModelView` to avoid working with
            # homogeneous coordinates.
            if F is not None: triPts = V[F].reshape((-1, 3, 3))
            else:             triPts = V   .reshape((-1, 3, 3))
            order = np.argsort((triPts.mean(axis=1) @ matModelView[0:3, 0:3].T)[:, 2])
            if F is not None: F = F.reshape((-1, 3))[order]
            else:
                # We must reorder vertex normal and color data...
                V = triPts[order].reshape((-1, 3))
                N = N.reshape((-1, 3, 3))[order].reshape((-1, 3))
                if not constColor:
                    color = color.reshape((-1, 3, 3))[order].reshape((-1, 3))

        self.makeCurrent()

        vao = VertexArrayObject()
        vao.setAttribute(0, V)
        vao.setAttribute(1, N)

        if constColor: vao.setConstantAttribute(2, color)
        else:          vao.setAttribute        (2, color)

        if F is not None:
            vao.setIndexBuffer(F)

        self.enable(GLenum.GL_DEPTH_TEST)
        self.disable(GLenum.GL_CULL_FACE)

        # Set blending mode to obtain a correct (but alpha-premultiplied)
        # RGBA image when rendering atop a cleared ([0, 0, 0, 0]) buffer.
        # In particular, we need to be careful with how the alpha components
        # are blended.
        self.enable(GLenum.GL_BLEND)
        self.blendFunc(GLenum.GL_SRC_ALPHA, GLenum.GL_ONE_MINUS_SRC_ALPHA,
                       GLenum.GL_ONE,       GLenum.GL_ONE_MINUS_SRC_ALPHA)

        s = self.shaderLibrary().load(SHADER_DIR + '/phong_with_wireframe.vert',
                                      SHADER_DIR + '/phong_with_wireframe.frag')

        s.setUniform('modelViewMatrix',   matModelView)
        s.setUniform('projectionMatrix',  matProjection)
        s.setUniform('normalMatrix',      np.linalg.inv(matModelView[0:3, 0:3]).T)

        s.setUniform('lightEyePos',       lightEyePos)
        s.setUniform('diffuseIntensity',  diffuseIntensity)
        s.setUniform('ambientIntensity',  ambientIntensity)
        s.setUniform('specularIntensity', specularIntensity)
        s.setUniform('shininess',         shininess)
        s.setUniform('alpha',             alpha)

        s.setUniform('lineWidth',         lineWidth)
        s.setUniform('wireframeColor',    wireframeColor)

        vao.draw(s)

    def __del__(self):
        if (hasattr(self, '_shaderLib')):
            self.makeCurrent() # Make sure shader deletion is acting on this context's shaders!!!
            del self._shaderLib
