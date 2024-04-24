#ifdef __APPLE_CC__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include <stdio.h>

#include "utils.h"

#define MIN_WINDOW_WIDTH 500
#define MIN_WINDOW_HEIGHT 400

int windowWidth = 800, windowHeight = 600;
float ballX = 100., ballY = 100., leftPaddleY = 100., rightPaddleY = 100.;
float ballVelocityX = 10., ballVelocityY = 0.;

// prints the screen
void display() {

}

void reshape(int width, int height) {
    if (width < MIN_WINDOW_WIDTH || height < MIN_WINDOW_HEIGHT) {
        glutReshapeWindow(max(MIN_WINDOW_WIDTH, width), max(MIN_WINDOW_HEIGHT, height));
        return;
    }

    // TODO

    glutPostRedisplay();
}

void keypress(unsigned char key, int mouseX, int mouseY) {
    // TODO
    printf("%c\n", key);
}

void specialKeypress(int key, int mouseX, int mouseY) {
    // TODO
}

void fixedUpdate(int value) {
    // TODO
}

int main(int argc, char* argv) {
    glutInit(&argc, &argv);
    glutInitWindowSize(windowWidth, windowHeight);
    glutInitWindowPosition(100, 100);
    glutInitDisplayMode(GLUT_RGBA);

    glutCreateWindow("Pong");
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keypress);
    glutSpecialFunc(specialKeypress);
    glutTimerFunc(500 /*millisecs between ball updates*/, fixedUpdate, 0);
    
    glutMainLoop();
}