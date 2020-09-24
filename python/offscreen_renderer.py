import _offscreen_renderer
from _offscreen_renderer import *

class OpenGLContext(_offscreen_renderer.OpenGLContext):
    def image(self):
        from PIL import Image
        self.finish() # Copy image to internal buffer
        # Reshape flat buffer into (height, width, components) 3D array
        # and flip vertically (since OpenGL's image coordinate system is vertically flipped
        # with respect to PIL/png/etc.
        return Image.fromarray(self.buffer().reshape((self.height, self.width, 4))[::-1,:,:])
