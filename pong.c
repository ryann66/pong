#ifdef __APPLE_CC__
#include <GLUT/gl.h>
#include <GLUT/glu.h>
#include <GLUT/glut.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#endif

#include <stdio.h>
#include <stdbool.h>

#include "utils.h"

#define FRAME_RATE 60
#define SEC_PER_FRAME 1000 / (FRAME_RATE)

#define PADDLE_SPEED 4.

int windowWidth = 800, windowHeight = 600;

#define MIN_WINDOW_WIDTH 500
#define MIN_WINDOW_HEIGHT (WINDOW_WIDTHF * 9. / 16.)
#define MAX_WINDOW_HEIGHT (WINDOW_WIDTHF * 3. / 4.)

#define WINDOW_WIDTHF ((float) windowWidth)
#define WINDOW_HEIGHTF ((float) windowHeight)

#define PADDLE_HEIGHT (WINDOW_HEIGHTF / 6)
#define PADDLE_WIDTH (WINDOW_WIDTHF / 100)
#define MAX_PADDLE_Y (WINDOW_HEIGHTF - PADDLE_HEIGHT)
#define MIN_PADDLE_Y (0)
#define LEFT_PADDLE_X (50)
#define RIGHT_PADDLE_X (WINDOW_WIDTHF - LEFT_PADDLE_X)

#define Xpos(x) (((x) * 2 / WINDOW_WIDTHF) - 1)
#define Ypos(y) (((y) * 2 / WINDOW_HEIGHTF) - 1)

float ballX = 100., ballY = 100., leftPaddleY = 100., rightPaddleY = 100.;
float ballVelocityX = 10., ballVelocityY = 0.;

bool upButton = false, specialUpButton = false, downButton = false, specialDownButton = false;

// prints the screen
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glColor3f(1., 1., 1.);
    glRectf(Xpos(LEFT_PADDLE_X - PADDLE_WIDTH), Ypos(leftPaddleY), Xpos(LEFT_PADDLE_X), Ypos(leftPaddleY + PADDLE_HEIGHT));

    glutSwapBuffers();
}

void reshape(int width, int height) {
    bool flag = false;
    if (width < MIN_WINDOW_WIDTH) {
        width = MIN_WINDOW_WIDTH;
        flag = true;
    }
    if (height < MIN_WINDOW_HEIGHT) {
        height = MIN_WINDOW_HEIGHT;
        flag = true;
    }
    else if (height > MAX_WINDOW_HEIGHT) {
        height = MAX_WINDOW_HEIGHT;
        flag = true;
    }

    // TODO calculate new position of all elements

    windowWidth = width;
    windowHeight = height;
    if (flag) {
        glutReshapeWindow(width, height);
    }
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
            leftPaddleY = max(leftPaddleY - windowHeight / 512. * PADDLE_SPEED, MIN_PADDLE_Y);
        } else if (upButton || specialUpButton) {
            // up, TODO
            leftPaddleY = min(leftPaddleY + windowHeight / 512. * PADDLE_SPEED, MAX_PADDLE_Y);
        }
    }

    // TODO move computer (right paddle)

    // TODO move ball
    // TODO check collision

    glutPostRedisplay();
    glutTimerFunc(SEC_PER_FRAME, fixedUpdate, 0);
}

int main(int argc, char* argv) {
    glutInit(&argc, &argv);
    glutInitWindowSize(windowWidth, windowHeight);
    glutInitWindowPosition(100, 100);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);

    glClearColor(0., 0., 0., 1.);

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