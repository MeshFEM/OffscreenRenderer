from enum import Enum
import os
import subprocess as sp

# Enum class whose values hold the corresponding FFmpeg command line options
class Codec(Enum):
    ImgSeq = 0
    H264   = ['h264']
    HEVC   = ['hevc',  '-tag:v', 'hvc1'] # We need to set the hvc1 tag for Quicktime compatibility

class VideoWriter:
    """
    Writes a raw image stream to an image sequence or a compressed video
    """
    def __init__(self, outPath, inWidth, inHeight, codec=Codec.H264, framerate=30, quality='-crf 23', outWidth=None, outHeight=None):
        self. outPath     = outPath
        self. inWidth     = inWidth
        self. inHeight    = inHeight
        self.outWidth     = inWidth  if outWidth  is None else outWidth
        self.outHeight    = inHeight if outHeight is None else outHeight
        self.codec        = codec
        self.framerate    = framerate
        self.frameCounter = 0

        self.ffmpegProc = None

        if codec == Codec.ImgSeq:
            os.makedirs(outPath, exist_ok=True)
        else:
            ffmpegCommand = ['ffmpeg', '-y']
            # Input settings (read raw pixel data from stdin)
            ffmpegCommand += [
                    '-f', 'rawvideo', '-pixel_format', 'rgba',
                    '-video_size', f'{inWidth}x{inHeight}',
                    '-framerate', str(framerate),
                    '-i', '-']
            # Output settings
            ffmpegCommand += ['-pix_fmt', 'yuv420p'] # Quicktime-compatible YUV pixel format
            if self.isResizing():
                ffmpegCommand += ['-vf', f'scale={self.outWidth}:{self.outHeight}']
            ffmpegCommand += ['-vcodec'] + codec.value # Codec CLI settings are stored in enum
            ffmpegCommand += quality.split()
            ffmpegCommand.append(outPath)

            self.ffmpegProc = sp.Popen(ffmpegCommand, stdin=sp.PIPE)

    def frameDataSize(self):
        return self.inWidth * self.inHeight * 4 # We assume RGBA input

    def isResizing(self):
        return (self.outWidth != self.inWidth) or (self.outHeight != self.inHeight)

    def writeFrame(self, frameData):
        """
        Write frame data held in a numpy array (in scaline order from top-left to bottom-right
        """
        frameData = frameData.ravel()
        if len(frameData) != self.frameDataSize(): raise Exception('Unexpected frame data size')

        if self.ffmpegProc is not None:
            self.ffmpegProc.stdin.write(frameData)
        else:
            from PIL import Image
            img = Image.fromarray(frameData.reshape((self.inHeight, self.inWidth, 4)))
            if self.isResizing(): img = img.resize((self.outWidth, self.outHeight))
            img.save(f'{self.outPath}/frame_{self.frameCounter:06d}.png')

        self.frameCounter += 1

    def finish(self):
        if self.ffmpegProc is not None:
            self.ffmpegProc.stdin.close()

    def __del__(self):
        self.finish()

class ContextVideoWriter(VideoWriter):
    """
    Renders an OpenGL context to an image sequence or a compressed video
    """
    def __init__(self, outPath, ctx, codec=Codec.H264, framerate=30, outWidth=None, outHeight=None):
        super().__init__(outPath, ctx.width, ctx.height, codec=codec, framerate=framerate, outWidth=outWidth, outHeight=outHeight)
        self.ctx = ctx

    def writeFrame(self):
        super().writeFrame(self.ctx.array())

class MeshRendererVideoWriter(VideoWriter):
    """
    Creates an image sequence or a compressed video from  a MeshRenderer
    """
    def __init__(self, outPath, mrenderer, codec=Codec.H264, framerate=30, outWidth=None, outHeight=None):
        super().__init__(outPath, mrenderer.ctx.width, mrenderer.ctx.height, codec=codec, framerate=framerate, outWidth=outWidth, outHeight=outHeight)
        self.mrenderer = mrenderer
        if codec != Codec.ImgSeq:
            # FFmpeg doesn't support transparent H264/HEVC output--it just
            # composites over a black background :(
            self.mrenderer.transparentBackground = False

    def writeFrame(self):
        self.mrenderer.render(True)
        super().writeFrame(self.mrenderer.array())
