import _offscreen_renderer
from _offscreen_renderer import *
import os

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
        if files not in self.shaders:
            self.shaders[files] = Shader(*[open(f).read() for f in files])
        return self.shaders[files]

class OpenGLContext(_offscreen_renderer.OpenGLContext):
    def __init__(self, width, height):
        _offscreen_renderer.OpenGLContext.__init__(self, width, height)
        self.shaderLibrary = ShaderLibrary()
    def image(self):
        from PIL import Image
        self.finish() # Copy image to internal buffer
        # Reshape flat buffer into (height, width, components) 3D array
        # and flip vertically (since OpenGL's image coordinate system is vertically flipped
        # with respect to PIL/png/etc.
        return Image.fromarray(self.buffer().reshape((self.height, self.width, 4))[::-1,:,:])

    def renderMesh(V, F, N, color, matProj, matModelView, wireframe = False, alpha = 1.0):
        s = self.shaderLibrary.load(SHADER_DIR + '/demo.vert', SHADER_DIR + '/demo.frag')
        vao = VertexArrayObject()
        vao.setAttribute(0, V)
        vao.setAttribute(1, N)

        if (len(color.shape) == 1):
            vao.setConstantAttribute(2, color)
        else:
            if (len(color.shape) != 2) or (color.shape[0] != V.shape[0]):
                raise Exception('Unexpected `color` size (must be per-vertex or constant)')

        vao.setIndexBuffer(F)

        if (alpha != 1.0):
            # TODO depth sorting
            pass

        s.setUniform('mv',        matModelView)
        s.setUniform('mvp',       matMVP)
        s.setUniform('wireframe', wireframe)
        s.setUniform('alpha',     alpha)
