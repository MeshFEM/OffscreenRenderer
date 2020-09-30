try:    import _offscreen_renderer
except: raise Exception('Could not load compiled module; is OffscreenRenderer missing a dependency?')

from _offscreen_renderer import *
import os
import numpy as np
import scipy.spatial
import video_writer

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

    def save(self, path, unpremultiply = True):
        ext = os.path.splitext(path)[-1][1:].lower()
        if (ext not in ['png', 'ppm']): raise Exception('Output file extension not supported')
        self.finish() # Copy image to internal buffer
        if ext == 'png': self.writePNG(path, unpremultiply)
        if ext == 'ppm': self.writePPM(path, unpremultiply)

    def shaderLibrary(self):
        if not hasattr(self, '_shaderLib'):
            self._shaderLib = ShaderLibrary()
        return self._shaderLib

    def __del__(self):
        if (hasattr(self, '_shaderLib')):
            self.makeCurrent() # Make sure shader deletion is acting on this context's shaders!!!
            del self._shaderLib

def hexColorToFloat(c):
    """
    Pythreejs likes using hex color strings like #FFFFFF; convert
    them to OpenGL-compatible float arrays.
    """
    if not isinstance(c, str) or (len(c) != 7): raise Exception('Invalid hex color')
    return np.array([int(c[i:i+2], 16) / 255 for i in range(1, len(c), 2)])

class MeshRenderer:
    def __init__(self, width, height):
        self.ctx = OpenGLContext(width, height)
        self.shader = self.ctx.shaderLibrary().load(SHADER_DIR + '/phong_with_wireframe.vert',
                                                    SHADER_DIR + '/phong_with_wireframe.frag')
        self.alpha = 1.0 # Global opacity of the mesh
        self._meshColorOpaque = True # to be determined from the user-passed color
        self._sorted = False

        self.setWireframe(0.0)
        self.matModel      = np.identity(4)
        self.matView       = np.identity(4)
        self.matProjection = np.identity(4)

        white = np.ones(3)
        self.lightEyePos       = [0, 0, 5]
        self. diffuseIntensity = 0.6 * white
        self. ambientIntensity = 0.5 * white
        self.specularIntensity = 1.0 * white
        self.shininess         = 20.0

        self.transparentBackground = True

        # The triangle index array in active use to replicate vertex data to
        # per-corner data.
        self._activeReplicationIndices = None

    def _validateSizes(self, V, F, N, color):
        """ Side effect: update constColor """
        ccolor = isinstance(color, str) or len(np.ravel(color)) in [3, 4] # `str` case probably corresponds to RGB hex code '#FFFFFF'
        self.constColor = ccolor
        nv = self.numVertices
        if len(V) != nv:                      raise Exception(f'Unexpected vertex array size {len(V)} vs {nv} (use setMesh if changing connectivity)')
        if len(N) != nv:                      raise Exception(f'Unexpected normal array size {len(N)} vs {nv} (must be per-vertex)')
        if not ccolor and (len(color) != nv): raise Exception(f'Unexpected `color` size {len(N)} vs {nv} (must be per-vertex or constant)')
        if F is not None and F.max() >= nv:   raise Exception('Corner index out of bounds')

    def setMesh(self, V, F, N, color):
        """
        Initialize/update the mesh data and connectivity.
        `F` can be `None` to disable indexed face set representation
        (i.e., to use glDrawArrays instead of glDrawElements)
        """
        self.numVertices = len(V)
        self._validateSizes(V, F, N, color)
        self.F = F
        self._activeReplicationIndices = None

        self.ctx.makeCurrent()

        self.vao = VertexArrayObject()
        if self.F is not None:
            self.vao.setIndexBuffer(self.F)

        self.updateMeshData(V, N, color)

    def needsDepthSort(self):
        return not self._sorted and ((self.alpha != 1.0) or not self._meshColorOpaque)

    def depthSort(self):
        """
        If the mesh has translucency, sort the triangles from back to front.
        To avoid shuffling vertex attribute data when the viewpoint changes, we
        always use an index buffer when depth sorting.
        """
        if not self.needsDepthSort(): return
        if self.F is None:
            self.F = np.arange(self.numVertices)

        # Sort the triangles in increasing order by their barycenters'
        # z coordinates in eye space. Note, we discard the irrelevant
        # translation part of `matModelView` to avoid working with
        # homogeneous coordinates.
        order = np.argsort((self.V[self.F].reshape((-1, 3, 3)).mean(axis=1) @ (self.matView @ self.matModel)[0:3, 0:3].T)[:, 2])
        self.F = self.F.reshape((-1, 3))[order]
        self.vao.setIndexBuffer(self.F)

    def updateMeshData(self, V, N, color = None):
        """
        Update the mesh's data without changing its connectivity.
        """
        if (color is None): color = self.color
        self._validateSizes(V, None, N, color) # updates `self.constColor`

        if isinstance(color, str) and len(color) == 7:
            color = hexColorToFloat(color)

        # Replicate the data to per-corner if in replication mode.
        if self._activeReplicationIndices is not None:
            V = V[self._activeReplicationIndices]
            N = N[self._activeReplicationIndices]
            if not self.constColor: color = color[self._activeReplicationIndices]

        # Keep track of the vertex data in case it we need to switch to replicating per
        # incident triangle (e.g., for wireframe rendering)
        self.V = V
        self.N = N
        self.color = color

        self.vao.setAttribute(0, V)
        self.vao.setAttribute(1, N)

        if self.constColor:
            self.vao.setConstantAttribute(2, color)
            self._meshColorOpaque = (len(color) == 3) or (color[3] == 1.0)
        else:
            self.vao.setAttribute(2, color)
            self._meshColorOpaque = (color.shape[1] == 3) or (color[:, 3].min() == 1.0)

    def lookAt(self, position, target, up):
        self.cam_position = position
        self.cam_target   = target
        self.cam_up       = up
        viewDir = np.array(target) - np.array(position)
        viewDir /= np.linalg.norm(viewDir)
        right   = np.cross(viewDir, up)
        upPerp  = np.cross(right, viewDir)
        self.matView[0, 0:3] = right
        self.matView[1, 0:3] = upPerp
        self.matView[2, 0:3] = -viewDir
        self.matView[0:3, 3] = -self.matView[0:3, 0:3] @ position
        self.matView[3, 0:4] = [0, 0, 0, 1]

        self._sorted = False # Changing the viewpoint invalidates the depth sort

    def orbitedLookAt(self, position, target, up, angle):
        """
        View looking at the target from a camera position rotated by `angle` around the up vector.
        """
        v = np.array(position) - np.array(target)
        self.lookAt(target + scipy.spatial.transform.Rotation.from_rotvec(angle * np.array(up) / np.linalg.norm(up)).apply(v),
                    target, up)

    def perspective(self, fovy, aspect, near, far):
        f = 1.0 / np.tan(0.5 * np.deg2rad(fovy))
        self.matProjection = np.zeros((4, 4))
        self.matProjection[0, 0] = f / aspect
        self.matProjection[1, 1] = f
        self.matProjection[2, 2] = (near + far) / (near - far)
        self.matProjection[2, 3] = 2.0 * (near * far) / (near - far)
        self.matProjection[3, 2] = -1.0

    def modelMatrix(self, position, scale, quaternion):
        self.matModel[0:3, 0:3] = scale * scipy.spatial.transform.Rotation.from_quat(quaternion).as_matrix()
        self.matModel[0:3,   3] = position
        self.matModel[  3, 0:4] = [0, 0, 0, 1]

        self._sorted = False # Changing the mesh orientation invalidates its depth sort

    def getCameraParams(self):
        """
        Emulate with MeshFEM's TriMeshViewer.getCameraParams. As an unfortunate
        consequence, the `up` and `target` vectors are swapped wrt the
        arguments of  `lookAt`
        """
        return (self.cam_position, self.cam_up, self.cam_target)

    def setCameraParams(self, params):
        """
        Emulate with MeshFEM's TriMeshViewer.setCameraParams. As an unfortunate
        consequence, the `up` and `target` vectors are swapped wrt the
        arguments of  `lookAt`
        """
        self.lookAt(params[0], params[2], params[1])

    def hasPerCornerVtxData(self):
        return (self.F is None) or (len(self.F.ravel()) == self.numVertices)

    def setWireframe(self, lineWidth = 1.0, color = [0.0, 0.0, 0.0, 1.0]):
        self.lineWidth = lineWidth
        self.lineColor = color

    def replicatePerCorner(self):
        # Note: this operation preserves depth sorting!
        # (And it is more efficient to perform depth sorting first...)
        if self.hasPerCornerVtxData(): return

        # Switch to replication mode
        self._activeReplicationIndices = self.F.ravel()
        self.updateMeshData(self.V, self.N, self.color)

        # Disable the index buffers
        self.F = None
        self.vao.unsetIndexBuffer()

    def render(self, clear=True):
        self.ctx.makeCurrent()

        # Sort triangles if needed
        self.depthSort()

        # Our wireframe rendering technique needs distinct copies of vertices
        # for each incident triangle.
        if self.lineWidth != 0: self.replicatePerCorner()

        if clear:
            self.ctx.clear(np.zeros(4) if self.transparentBackground else np.ones(4))

        self.ctx. enable(GLenum.GL_DEPTH_TEST)
        self.ctx.disable(GLenum.GL_CULL_FACE)

        # Set blending mode to obtain a correct (but alpha-premultiplied)
        # RGBA image when rendering atop a cleared ([0, 0, 0, 0]) buffer.
        # In particular, we need to be careful with how the alpha components
        # are blended.
        self.ctx.enable(GLenum.GL_BLEND)
        self.ctx.blendFunc(GLenum.GL_SRC_ALPHA, GLenum.GL_ONE_MINUS_SRC_ALPHA,
                           GLenum.GL_ONE,       GLenum.GL_ONE_MINUS_SRC_ALPHA)

        self.shader.use()

        modelViewMatrix = self.matView @ self.matModel
        self.shader.setUniform('modelViewMatrix',   modelViewMatrix)
        self.shader.setUniform('projectionMatrix',  self.matProjection)
        self.shader.setUniform('normalMatrix',      np.linalg.inv(modelViewMatrix[0:3, 0:3]).T)

        self.shader.setUniform('lightEyePos',       self.lightEyePos)
        self.shader.setUniform('diffuseIntensity',  self.diffuseIntensity)
        self.shader.setUniform('ambientIntensity',  self.ambientIntensity)
        self.shader.setUniform('specularIntensity', self.specularIntensity)
        self.shader.setUniform('shininess',         self.shininess)
        self.shader.setUniform('alpha',             self.alpha)

        self.shader.setUniform('lineWidth',         self.lineWidth)
        self.shader.setUniform('wireframeColor',    self.lineColor)
        self.vao.draw(self.shader)

    def array(self      ): return self.ctx.array(     unpremultiply=self.transparentBackground)
    def image(self      ): return self.ctx.image(     unpremultiply=self.transparentBackground)
    def  save(self, path): return self.ctx.save(path, unpremultiply=self.transparentBackground)

    def renderAnimation(self, outPath, nframes, frameCallback, *videoWriterArgs, **videoWriterKWargs):
        """
        Write an animation out as a video/image sequence, where each frame is set up by `frameCallback`
        """
        vw = video_writer.MeshRendererVideoWriter(outPath, self, *videoWriterArgs, **videoWriterKWargs)
        for frame in range(nframes):
            frameCallback(self, frame)
            vw.writeFrame()

    def orbitAnimation(self, outPath, nframes, *videoWriterArgs, **videoWriterKWargs):
        """
        Render an animation of the camera making a full orbit around the up axis centered at its target.
        """
        def c(r, i): r.orbitedLookAt(self.cam_position, self.cam_target, self.cam_up, 2  * np.pi / nframes * i)
        self.renderAnimation(outPath, nframes, c, *videoWriterArgs, **videoWriterKWargs)

    def __del__(self):
        self.shader = None # Must happen first!
        del self.ctx