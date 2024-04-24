#ifdef __APPLE_CC__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include <stdio.h>
#include <stdbool.h>

#include "utils.h"

#define MIN_WINDOW_WIDTH 500
#define MIN_WINDOW_HEIGHT 400
#define FRAME_RATE 60
#define SEC_PER_FRAME 1000 / (FRAME_RATE)

int windowWidth = 800, windowHeight = 600;
float ballX = 100., ballY = 100., leftPaddleY = 100., rightPaddleY = 100.;
float ballVelocityX = 10., ballVelocityY = 0.;

bool upButton = false, specialUpButton = false, downButton = false, specialDownButton = false;

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
    if (key == 'w') upButton = true;
    else if (key == 's') downButton = true;
}

void specialKeypress(int key, int mouseX, int mouseY) {
    if (key == GLUT_KEY_UP) specialUpButton = true;
    else if (key == GLUT_KEY_DOWN) specialDownButton = true;
}

void keyrelease(unsigned char key, int mouseX, int mouseY) {
    if (key == 'w') upButton = false;
    else if (key == 's') downButton = false;
}

void specialKeyrelease(int key, int mouseX, int mouseY) {
    if (key == GLUT_KEY_UP) specialUpButton = false;
    else if (key == GLUT_KEY_DOWN) specialDownButton = false;
}

void fixedUpdate(int value) {
    if ((downButton || specialDownButton) ^ (upButton || specialUpButton)) {
        if (downButton || specialDownButton) {
            // down, TODO
            printf("down\n");
        } else if (upButton || specialUpButton) {
            // up, TODO
            printf("up\n");
        }
    }

    glutTimerFunc(SEC_PER_FRAME, fixedUpdate, 0);
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
    glutKeyboardUpFunc(keyrelease);
    glutSpecialUpFunc(specialKeyrelease);
    glutTimerFunc(SEC_PER_FRAME, fixedUpdate, 0);
    
    glutMainLoop();
}