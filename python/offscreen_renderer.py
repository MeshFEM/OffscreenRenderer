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
    def array(self):
        self.finish() # Copy image to internal buffer
        # Reshape flat buffer into (height, width, components) 3D array
        # and flip vertically (since OpenGL's image coordinate system is vertically flipped
        # with respect to PIL/png/etc.)
        return self.buffer().reshape((self.height, self.width, 4))[::-1,:,:]

    def image(self):
        from PIL import Image
        return Image.fromarray(self.array())

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

    def renderMesh(self, V, F, N, color, matProjection, matModelView, lightEyePos, diffuseIntensity, ambientIntensity, specularIntensity=np.zeros(3), shininess=10.0, lineWidth = 0.0, alpha = 1.0):
        wireframe = lineWidth != 0
        if wireframe and F is not None:
            # Our wireframe rendering approach needs a copy of each vertex per
            # triangle.
            color = color[F] if len(color.shape) == 2 else color
            V = V[F]
            N = N[F]
            F = None

        self.makeCurrent()

        vao = VertexArrayObject()
        vao.setAttribute(0, V)
        vao.setAttribute(1, N)

        if (len(color.shape) == 1):
            vao.setConstantAttribute(2, color)
        else:
            if (len(color.shape) != 2) or (color.shape[0] != V.shape[0]):
                raise Exception('Unexpected `color` size (must be per-vertex or constant)')
            vao.setAttribute(2, color)

        if F is not None:
            vao.setIndexBuffer(F)

        if (alpha != 1.0):
            # TODO depth sorting?
            pass

        self.enable(GLenum.GL_DEPTH_TEST)

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

        vao.draw(s)

    def __del__(self):
        if (hasattr(self, '_shaderLib')):
            self.makeCurrent() # Make sure shader deletion is acting on this context's shaders!!!
            del self._shaderLib
