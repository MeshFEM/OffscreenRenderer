#include <stdio.h>
#include <stdlib.h>

#include <Eigen/Dense>
#include <stdexcept>
#include "OpenGLContext.hh"
#include "Shader.hh"

int main(int argc, char *argv[])
{
    int width = 400;
    int height = 400;
    char *filename = NULL;

    if (argc < 2) {
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "  osdemo filename [width height]\n");
        return 0;
    }

    filename = argv[1];
    if (argc == 4) {
        width = atoi(argv[2]);
        height = atoi(argv[3]);
    }

    auto ctx = OpenGLContext::construct(width, height);
    ctx->makeCurrent();

    auto shader = Shader::fromFiles(SHADER_PATH "/demo.vert", SHADER_PATH "/demo.frag");
    for (const auto &u : shader->getUniforms()) {
        std::cout << "Uniform " << u.index << ": " << u.name << std::endl;
    }

    ctx->render([&]() {
        glEnable(GL_DEPTH_TEST);

        glViewport(0, 0, width, height);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(-2.5, 2.5, -2.5, 2.5, -10.0, 10.0);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

		glClearColor(0.0, 0.0, 1.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBegin(GL_TRIANGLES);
            glColor3f(1.0, 0.0, 0.0);
            glVertex3f(0, 0, 0);
            glColor3f(0.0, 1.0, 0.0);
            glVertex3f(1, 0, 0);
            glColor3f(0.0, 0.0, 0.1);
            glVertex3f(0, 1, 0);
        glEnd();
    });

    ctx->finish();

    if (filename != NULL) {
        ctx->write_ppm(filename);
    }
    else {
        printf("Specify a filename if you want to make an image file\n");
    }

    return 0;
}
