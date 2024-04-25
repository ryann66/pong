#ifdef __APPLE_CC__
#include <GLUT/gl.h>
#include <GLUT/glu.h>
#include <GLUT/glut.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#endif

#include <stdbool.h>

#define FRAME_RATE 60
#define SCORE_DELAY 1

#define PADDLE_SPEED 4.
#define BALL_SPEED 4.
#define BALL_ACCELERATION_FACTOR 1.1

#define SEC_PER_FRAME 1000 / (FRAME_RATE)
#define SCORE_DELAY_MS (1000 * SCORE_DELAY)

#define WINDOW_WIDTHF (800.)
#define WINDOW_HEIGHTF (600.)

#define MIN_WINDOW_WIDTH 500
#define MIN_WINDOW_HEIGHT (WINDOW_WIDTHF * 9. / 16.)
#define MAX_WINDOW_HEIGHT (WINDOW_WIDTHF * 3. / 4.)

#define PADDLE_HEIGHT (WINDOW_HEIGHTF / 8)
#define PADDLE_WIDTH (WINDOW_WIDTHF / 100)
#define BALL_DIM (WINDOW_WIDTHF / 120)

#define MAX_PADDLE_Y (WINDOW_HEIGHTF - PADDLE_HEIGHT)
#define MIN_PADDLE_Y (0)
#define LEFT_PADDLE_X (50)
#define RIGHT_PADDLE_X (WINDOW_WIDTHF - LEFT_PADDLE_X)

#define DIGIT_HEIGHT (WINDOW_HEIGHTF / 9)
#define DIGIT_STROKE_WEIGHT (WINDOW_WIDTHF / 100)
#define DIGIT_WIDTH ((DIGIT_HEIGHT + DIGIT_STROKE_WEIGHT) / 2)
#define DIGIT_OFFSET DIGIT_WIDTH

#define DASH_HEIGHT (WINDOW_HEIGHTF / 49)
#define DASH_WIDTH (WINDOW_WIDTHF / 200)
#define DASH_OFFSET (DASH_WIDTH / 2)

#define Xpos(x) (((x) * 2 / WINDOW_WIDTHF) - 1)
#define Ypos(y) (((y) * 2 / WINDOW_HEIGHTF) - 1)

#define max(A, B) ((A) > (B) ? (A) : (B))
#define min(A, B) ((A) < (B) ? (A) : (B))

unsigned char leftScore = 0, rightScore = 0;

float ballX = -BALL_DIM, ballY = -BALL_DIM, leftPaddleY = (WINDOW_HEIGHTF - PADDLE_HEIGHT) / 2, rightPaddleY = (WINDOW_HEIGHTF - PADDLE_HEIGHT) / 2;
float ballVelocityX = 0, ballVelocityY = 0;
bool leftStart = true;
bool inPlay = false;

bool upButton = false, specialUpButton = false, downButton = false, specialDownButton = false;

void fixedUpdate(int value);

void resetBall() {
    ballX = WINDOW_WIDTHF / 2;
    ballY = WINDOW_HEIGHTF;
    ballVelocityX = leftStart ? -5 : 5;
    ballVelocityY = -5;
    leftStart = !leftStart;
    inPlay = true;
}

void reset(int value) {
    resetBall();
    glutPostRedisplay();
}

// prints digit with x and y giving the position of the bottom left corner of the digit
void printDigit(int x, int y, unsigned char digit) {
    // bottom bar
    if (digit == 0 || digit == 2 || digit == 3 || digit == 5 || digit == 6 || digit == 8) {
        glRectf(Xpos(x), Ypos(y), Xpos(x + DIGIT_WIDTH), Ypos(y + DIGIT_STROKE_WEIGHT));
    }
    // middle bar
    if ((digit >= 2 && digit <= 6) || digit == 8 || digit == 9) {
        glRectf(Xpos(x), Ypos(y + DIGIT_WIDTH - DIGIT_STROKE_WEIGHT), Xpos(x + DIGIT_WIDTH), Ypos(y + DIGIT_WIDTH));
    }
    // top bar
    if (digit == 0 || digit == 2 || digit == 3 || (digit >= 5 && digit <= 9)) {
        glRectf(Xpos(x), Ypos(y + DIGIT_HEIGHT), Xpos(x + DIGIT_WIDTH), Ypos(y + DIGIT_HEIGHT - DIGIT_STROKE_WEIGHT));
    }
    // upper left side
    if (digit == 0 || digit == 4 || digit == 5 || digit == 6 || digit == 8 || digit == 9) {
        glRectf(Xpos(x + DIGIT_STROKE_WEIGHT), Ypos(y + DIGIT_WIDTH - DIGIT_STROKE_WEIGHT), Xpos(x), Ypos(y + DIGIT_HEIGHT));
    }
    // lower left side
    if (digit == 0 || digit == 2 || digit == 6 || digit == 8) {
        glRectf(Xpos(x), Ypos(y), Xpos(x + DIGIT_STROKE_WEIGHT), Ypos(y + DIGIT_WIDTH));
    }
    // upper right side
    if (!(digit == 5 || digit == 6)) {
        glRectf(Xpos(x + DIGIT_WIDTH - DIGIT_STROKE_WEIGHT), Ypos(y + DIGIT_HEIGHT), Xpos(x + DIGIT_WIDTH), Ypos(y + DIGIT_WIDTH - DIGIT_STROKE_WEIGHT));
    }
    // lower right side
    if (digit != 2) {
        glRectf(Xpos(x + DIGIT_WIDTH - DIGIT_STROKE_WEIGHT), Ypos(y), Xpos(x + DIGIT_WIDTH), Ypos(y + DIGIT_WIDTH));
    }
}

// prints the screen
void display() {
    glClearColor(0., 0., 0., 1.);
    glClear(GL_COLOR_BUFFER_BIT);
    
    glColor3f(1., 1., 1.);
    // left paddle
    glRectf(Xpos(LEFT_PADDLE_X - PADDLE_WIDTH), Ypos(leftPaddleY), Xpos(LEFT_PADDLE_X), Ypos(leftPaddleY + PADDLE_HEIGHT));
    // right paddle
    glRectf(Xpos(RIGHT_PADDLE_X), Ypos(rightPaddleY), Xpos(RIGHT_PADDLE_X + PADDLE_WIDTH), Ypos(rightPaddleY + PADDLE_HEIGHT));
    // ball
    glRectf(Xpos(ballX), Ypos(ballY), Xpos(ballX + BALL_DIM), Ypos(ballY + BALL_DIM));
    // scores (left, right)
    printDigit((WINDOW_WIDTHF / 2) - DIGIT_OFFSET - DIGIT_WIDTH, WINDOW_HEIGHTF - DIGIT_HEIGHT - DIGIT_OFFSET, leftScore);
    printDigit((WINDOW_WIDTHF / 2) + DIGIT_OFFSET, WINDOW_HEIGHTF - DIGIT_HEIGHT - DIGIT_OFFSET, rightScore);
    // dashes
    float xtmp = (WINDOW_WIDTHF / 2) - DASH_OFFSET;
    for (float ytmp = 0; ytmp < WINDOW_HEIGHTF; ytmp += 2 * DASH_HEIGHT) {
        glRectf(Xpos(xtmp), Ypos(ytmp), Xpos(xtmp + DASH_WIDTH), Ypos(ytmp + DASH_HEIGHT));
    }

    glFlush();
    glutSwapBuffers();
}

void reshape(int width, int height) {
    glutReshapeWindow((int)WINDOW_WIDTHF, (int)WINDOW_HEIGHTF);
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
    if (downButton ^ upButton) {
        if (downButton) {
            leftPaddleY = max(leftPaddleY - WINDOW_HEIGHTF / 512. * PADDLE_SPEED, MIN_PADDLE_Y);
        } else if (upButton) {
            leftPaddleY = min(leftPaddleY + WINDOW_HEIGHTF / 512. * PADDLE_SPEED, MAX_PADDLE_Y);
        }
    }
    if (specialDownButton ^ specialUpButton) {
        if (specialDownButton) {
            rightPaddleY = max(rightPaddleY - WINDOW_HEIGHTF / 512. * PADDLE_SPEED, MIN_PADDLE_Y);
        } else if (specialUpButton) {
            rightPaddleY = min(rightPaddleY + WINDOW_HEIGHTF / 512. * PADDLE_SPEED, MAX_PADDLE_Y);
        }
    }

    ballX += ballVelocityX;
    ballY += ballVelocityY;

    // only calculate collisions if ball is in play
    if (inPlay) {
        // Simple collision resolvers that rely on low speed and small BALL_DIM to be accurate
        // top and bottom
        if (ballY > WINDOW_HEIGHTF) {
            ballVelocityY = -ballVelocityY;
            ballY += ballVelocityY;
        } else if (ballY < 0) {
            ballVelocityY = -ballVelocityY;
            ballY += ballVelocityY;
        }
        // paddles
        if (ballX < LEFT_PADDLE_X && ballX > LEFT_PADDLE_X - PADDLE_WIDTH && ballY > leftPaddleY && ballY < leftPaddleY + PADDLE_HEIGHT) {
            ballVelocityX = -ballVelocityX;
            ballX += ballVelocityX;
            // corner collision
            if (ballY - ballVelocityY > leftPaddleY + PADDLE_HEIGHT || ballY - ballVelocityY < leftPaddleY) {
                ballVelocityY = -ballVelocityY;
                ballY += ballVelocityY;
            }
            ballVelocityX *= BALL_ACCELERATION_FACTOR;
            ballVelocityY *= BALL_ACCELERATION_FACTOR;
        } else if (ballX > RIGHT_PADDLE_X && ballX < RIGHT_PADDLE_X + PADDLE_WIDTH && ballY > rightPaddleY && ballY < rightPaddleY + PADDLE_HEIGHT) {
            ballVelocityX = -ballVelocityX;
            ballX += ballVelocityX;
            // corner collision
            if (ballY - ballVelocityY > rightPaddleY + PADDLE_HEIGHT || ballY - ballVelocityY < rightPaddleY) {
                ballVelocityY = -ballVelocityY;
                ballY += ballVelocityY;
            }
            ballVelocityX *= BALL_ACCELERATION_FACTOR;
            ballVelocityY *= BALL_ACCELERATION_FACTOR;
        }

        // score colliders
        if (ballX < 0) {
            rightScore++;
            if (rightScore == 10) {
                leftScore = rightScore = 0;
            }
            glutTimerFunc(SCORE_DELAY_MS, reset, 0);
            inPlay = false;
        } else if (ballX > WINDOW_WIDTHF) {
            leftScore++;
            if (leftScore == 10) {
                leftScore = rightScore = 0;
            }
            glutTimerFunc(SCORE_DELAY_MS, reset, 0);
            inPlay = false;
        }
    }
    
    glutPostRedisplay();
    glutTimerFunc(SEC_PER_FRAME, fixedUpdate, 0);
}

int main(int argc, char* argv) {
    glutInit(&argc, &argv);
    glutInitWindowSize((int)WINDOW_WIDTHF, (int)WINDOW_HEIGHTF);
    glutInitWindowPosition(100, 100);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);

    glutCreateWindow("Pong");
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keypress);
    glutSpecialFunc(specialKeypress);
    glutKeyboardUpFunc(keyrelease);
    glutSpecialUpFunc(specialKeyrelease);
    
    // set delay before starting
    glutTimerFunc(SCORE_DELAY_MS, resetBall, 0);
    glutTimerFunc(SEC_PER_FRAME, fixedUpdate, 0);
    
    glutMainLoop();
}