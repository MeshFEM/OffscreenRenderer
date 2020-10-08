#include <GL/gl.h>
#include <GL/osmesa.h>
#include <stdlib.h>
#include <malloc.h>

int main(int iargs, const char *argv[])
{
     void *pBuf0Ptr, *pBuf1Ptr, *pBuf2Ptr;
     GLsizei iBuf0Width = 720, iBuf0Height = 480;
     GLsizei iBuf1Width = 720, iBuf1Height = 480;
     GLsizei iBuf2Width = 120, iBuf2Height = 120;
     OSMesaContext ctx0, ctx1;
     GLboolean bOk;

     pBuf0Ptr = memalign(iBuf0Width*iBuf0Height*4, 4);
     pBuf1Ptr = memalign(iBuf1Width*iBuf1Height*4, 4);
     pBuf2Ptr = memalign(iBuf2Width*iBuf2Height*4, 4);

     ctx0 = OSMesaCreateContext(GL_RGBA, NULL);
     ctx1 = OSMesaCreateContext(GL_RGBA, NULL);

     bOk = OSMesaMakeCurrent(ctx0, pBuf0Ptr, GL_UNSIGNED_BYTE, iBuf0Width, iBuf0Height);
     glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
     glFlush();

     bOk = OSMesaMakeCurrent(ctx1, pBuf1Ptr, GL_UNSIGNED_BYTE, iBuf1Width, iBuf1Height);
     glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
     glFlush();

     // bOk = OSMesaMakeCurrent(ctx1, pBuf2Ptr, GL_UNSIGNED_BYTE, iBuf2Width, iBuf2Height);
     // glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
     // glFlush();

     // bOk = OSMesaMakeCurrent(ctx0, pBuf0Ptr, GL_UNSIGNED_BYTE, iBuf0Width, iBuf0Height);
     // glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
     // glFlush();

     exit(0);
}
