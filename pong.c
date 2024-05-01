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
#include <time.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define WINDOW_WIDTHF (1200.)
#define WINDOW_HEIGHTF (900.)

// timing constants
#define FRAME_RATE (60.)
#define SCORE_DELAY (1.)
#define RESUME_DELAY (1.)

// game constants
#define PADDLE_SPEED (4.)
#define INITIAL_BALL_SPEED (10)
#define MAX_BALL_SPEED (24.)
#define BALL_SPEED_ACCELERATION (1.03)
#define MAX_BOUNCE_ANGLE (60.)
#define TARGET_SCORE (10.)
#define PADDLE_HEIGHT (WINDOW_HEIGHTF / 8.)
#define PADDLE_WIDTH (WINDOW_WIDTHF / 90.)
#define BALL_RADIUS (WINDOW_WIDTHF / 240.)

// computer aiming probabilities
#define PROB_TOP (23)
#define PROB_BOTTOM (23)
#define PROB_FLAT (15)
#define PROB_EASY (5)
#define PROB_AGGRESSIVE (24)
#define PROB_ERRATIC_UP (5)
#define PROB_ERRATIC_DOWN (5)
#define PROB_WEIGHT_SUM (PROB_TOP + PROB_BOTTOM + PROB_FLAT + PROB_EASY + PROB_AGGRESSIVE + PROB_ERRATIC_UP + PROB_ERRATIC_DOWN)

// game colors
#define BACKGROUND_COLOR 0.,0.,0.
#define LOGO_COLOR 1.,1.,1.
#define TEXT_COLOR 1.,1.,1.
#define BUTTON_COLOR 1.,1.,1.
#define HOVER_BUTTON_COLOR .8,.8,.8
#define BUTTON_TEXT_COLOR 0.,0.,0.
#define PADDLE_COLOR 1.,1.,1.
#define BALL_COLOR 1.,1.,1.
#define GAME_ENVIRONMENT_COLOR 0.8,0.8,0.8

// derived timings
#define SEC_PER_FRAME (1000. / (FRAME_RATE))
#define SCORE_DELAY_MS (1000. * SCORE_DELAY)
#define RESUME_DELAY_MS (1000. * RESUME_DELAY)

#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))

// derived game values
#define MAX_BOUNCE_ANGLE_RAD (M_PI * MAX_BOUNCE_ANGLE / 180.)
#define COMPUTER_AIMING_TOLERANCE (PADDLE_SPEED)
#define PADDLE_INVISIBLE_COLLIDER_WIDTH (max(0, MAX_BALL_SPEED - PADDLE_WIDTH))
#define BALL_DIM (2. * BALL_RADIUS)
#define MAX_PADDLE_Y (WINDOW_HEIGHTF - PADDLE_HEIGHT)
#define MIN_PADDLE_Y (0.)
#define MIDDLE_PADDLE_Y ((MAX_PADDLE_Y - MIN_PADDLE_Y) / 2.)
#define LEFT_PADDLE_X (50.)
#define RIGHT_PADDLE_X (WINDOW_WIDTHF - LEFT_PADDLE_X)
#define INIT_PADDLE_Y (MIDDLE_PADDLE_Y)

// score counter drawing constants
#define DIGIT_HEIGHT (WINDOW_HEIGHTF / 9.)
#define DIGIT_STROKE_WEIGHT (WINDOW_WIDTHF / 100.)
#define DIGIT_WIDTH ((DIGIT_HEIGHT + DIGIT_STROKE_WEIGHT) / 2.)
#define DIGIT_OFFSET (DIGIT_WIDTH / 2.)

// centerline drawing constants
#define DASH_HEIGHT (WINDOW_HEIGHTF / 49.)
#define DASH_WIDTH (WINDOW_WIDTHF / 200.)
#define DASH_OFFSET (DASH_WIDTH / 2.)

// button drawing constants
#define BUTTON_WIDTH (WINDOW_WIDTHF / 2.5)
#define BUTTON_HEIGHT (WINDOW_HEIGHTF / 9.)
#define BUTTON_OFFSET_Y (BUTTON_HEIGHT / 2.)
#define BUTTON_OFFSET_X (BUTTON_WIDTH / 2.)
#define BUTTON_SPACING (BUTTON_HEIGHT / 3.)
#define PAUSE_BUTTON_WIDTH (WINDOW_WIDTHF / 3.75)
#define PAUSE_BUTTON_OFFSET_X (PAUSE_BUTTON_WIDTH / 2.)

// button text drawing constants
#define BUTTON_FONT_WIDTH (BUTTON_WIDTH / 14.)
#define BUTTON_FONT_HEIGHT (BUTTON_HEIGHT * 2. / 3.)
#define BUTTON_FONT_SPACING (BUTTON_FONT_WIDTH / 3.)
#define BUTTON_FONT_STROKE (BUTTON_FONT_WIDTH / 4.)

// logo drawing constants
#define LOGO_FONT_WIDTH (BUTTON_WIDTH / 4.)
#define LOGO_FONT_HEIGHT (BUTTON_HEIGHT * 1.25)
#define LOGO_FONT_SPACING (BUTTON_FONT_WIDTH / 3.)
#define LOGO_FONT_STROKE (LOGO_FONT_WIDTH / 6.)

// conversion to gl coordinate system
#define Xpos(x) (((x) * 2. / WINDOW_WIDTHF) - 1.)
#define Ypos(y) (((y) * 2. / WINDOW_HEIGHTF) - 1.)

unsigned char leftScore = 0, rightScore = 0;

float ballX = 0, ballY = 0, leftPaddleY = 0, rightPaddleY = 0;
float ballVelocityX = 0, ballVelocityY = 0;
float ballSpeed = 0;

// ball starting direction
bool leftStart = true;

// menu management booleans
bool inPlay = false, menu = true, pauseMenu = false;

// menu button press management
bool playerNumberButtonHover = false, playButtonHover = false, resumeButtonHover = false, exitButtonHover = false;

// current keypresses
bool upButton = false, specialUpButton = false, downButton = false, specialDownButton = false;

// used to prevent multiple fixed_update timers being triggered on spamming menu
int menuInstance = 0;

// gamemode
enum {
    ONE_PLAYER = 0, TWO_PLAYER = 1, ZERO_PLAYER = 2
} gameType = ONE_PLAYER;

// paddle movement directives
typedef enum {
    UP, DOWN, STATIC
} direction;

// computer controlled shot types
typedef enum {
    FLAT, TOP, BOTTOM, AGGRESSIVE, EASY, ERRATIC_UP, ERRATIC_DOWN
} computerShot;

computerShot leftComputerShot = FLAT, rightComputerShot = FLAT;

/**
 * Generates a random shot type for the computer to aim for
*/
computerShot getRandomShot() {
    int r = rand() % PROB_WEIGHT_SUM;
    if (r < PROB_FLAT) return FLAT;
    r -= PROB_FLAT;
    if (r < PROB_TOP) return TOP;
    r -= PROB_TOP;
    if (r < PROB_BOTTOM) return BOTTOM;
    r -= PROB_BOTTOM;
    if (r < PROB_AGGRESSIVE) return AGGRESSIVE;
    r -= PROB_AGGRESSIVE;
    if (r < PROB_EASY) return EASY;
    r -= PROB_EASY;
    if (r < PROB_ERRATIC_UP) return ERRATIC_UP;
    r -= PROB_ERRATIC_UP;
    return ERRATIC_DOWN;
}

/**
 * Returns the y value of the next time the ball will intersect a paddle
 * pass the current ball position and y velocity
 * Does not need the x velocity
*/
float ballIntersectY(float tBallX, float tBallY, float tBallVelocityY) {
    float yBounceTime;
    if (tBallVelocityY > 0) {
        yBounceTime = (WINDOW_HEIGHTF - tBallY) / tBallVelocityY;
    } else {
        yBounceTime = tBallY / -tBallVelocityY;
    }
    float xBounceTime;
    if (ballVelocityX < 0) {
        xBounceTime = (LEFT_PADDLE_X - tBallX) / ballVelocityX;
    } else {
        xBounceTime = (RIGHT_PADDLE_X - tBallX) / ballVelocityX;
    }

    // if hits a paddle next, return height of collision
    if (xBounceTime < yBounceTime) {
        return tBallY + tBallVelocityY * xBounceTime;
    }

    return ballIntersectY(tBallX + ballVelocityX * yBounceTime, (tBallVelocityY > 0) ? WINDOW_HEIGHTF : 0, -tBallVelocityY);
}

/**
 * returns the amount to shift the paddle in order to send the ball a specified distance (y value)
 * up or down on the other side
*/
float targetAimingShift(float yChange) {
    float angle = M_PI / 2. - atan2f(RIGHT_PADDLE_X - LEFT_PADDLE_X, fabsf(yChange));
    float relY = angle / MAX_BOUNCE_ANGLE_RAD * PADDLE_HEIGHT / 2.;
    relY = min(relY, PADDLE_HEIGHT / 2 - COMPUTER_AIMING_TOLERANCE);
    float shift = -copysignf(relY, yChange);
    return shift;
}

/**
 * paddle controller for one player mode
*/
direction onePlayerController() {
    if ((downButton || specialDownButton) ^ (upButton || specialUpButton)) {
        if (downButton || specialDownButton) {
            return DOWN;
        } else {
            return UP;
        }
    }
    return STATIC;
}

/**
 * left paddle controller for two player mode
*/
direction wasdPlayerController() {
    if (downButton ^ upButton) {
        if (downButton) {
            return DOWN;
        } else {
            return UP;
        }
    }
    return STATIC;
}

/**
 * right paddle controller for two player mode
*/
direction arrowPlayerController() {
    if (specialDownButton ^ specialUpButton) {
        if (specialDownButton) {
            return DOWN;
        } else {
            return UP;
        }
    }
    return STATIC;
}

/**
 * computer controller for the left paddle
*/
direction leftComputerController() {
    float targetY;
    if (ballVelocityX > 0 || !inPlay) targetY = MIDDLE_PADDLE_Y;
    else {
        if (ballVelocityY == 0) targetY = ballY;
        else targetY = ballIntersectY(ballX + BALL_RADIUS, ballY + BALL_RADIUS, ballVelocityY);
        targetY -= PADDLE_HEIGHT / 2;
        switch (leftComputerShot) {
            case TOP:
                targetY += targetAimingShift(MAX_PADDLE_Y - leftPaddleY - PADDLE_HEIGHT / 2.);
                break;
            case BOTTOM:
                targetY += targetAimingShift(-leftPaddleY + PADDLE_HEIGHT / 2.);
                break;
            case FLAT:
                // dummy target
                break;
            case AGGRESSIVE:
                targetY += targetAimingShift(rightPaddleY > MIDDLE_PADDLE_Y ? -leftPaddleY + PADDLE_HEIGHT / 2. : MAX_PADDLE_Y - leftPaddleY - PADDLE_HEIGHT / 2.);
                break;
            case EASY:
                targetY += targetAimingShift(rightPaddleY - leftPaddleY);
                break;
            case ERRATIC_UP:
                targetY -= (PADDLE_HEIGHT / 2) - COMPUTER_AIMING_TOLERANCE;
                break;
            case ERRATIC_DOWN:
                targetY += (PADDLE_HEIGHT / 2) - COMPUTER_AIMING_TOLERANCE;
                break;
        }
    }
    
    if (leftPaddleY < targetY - COMPUTER_AIMING_TOLERANCE) {
        return UP;
    }
    if (leftPaddleY > targetY + COMPUTER_AIMING_TOLERANCE) {
        return DOWN;
    }
    return STATIC;
}

/**
 * computer controller for the right paddle
*/
direction rightComputerController() {
    float targetY;
    if (ballVelocityX < 0 || !inPlay) targetY = MIDDLE_PADDLE_Y;
    else {
        if (ballVelocityY == 0) targetY = ballY;
        else targetY = ballIntersectY(ballX + BALL_RADIUS, ballY + BALL_RADIUS, ballVelocityY);
        targetY -= PADDLE_HEIGHT / 2;
        switch (rightComputerShot) {
            case TOP:
                targetY += targetAimingShift(MAX_PADDLE_Y - rightPaddleY - PADDLE_HEIGHT / 2.);
                break;
            case BOTTOM:
                targetY += targetAimingShift(-rightPaddleY + PADDLE_HEIGHT / 2.);
                break;
            case FLAT:
                // dummy target
                break;
            case AGGRESSIVE:
                targetY += targetAimingShift(leftPaddleY > MIDDLE_PADDLE_Y ? -rightPaddleY + PADDLE_HEIGHT / 2. : MAX_PADDLE_Y - rightPaddleY - PADDLE_HEIGHT / 2.);
                break;
            case EASY:
                targetY += targetAimingShift(leftPaddleY - rightPaddleY);
                break;
            case ERRATIC_UP:
                targetY -= (PADDLE_HEIGHT / 2) - COMPUTER_AIMING_TOLERANCE;
                break;
            case ERRATIC_DOWN:
                targetY += (PADDLE_HEIGHT / 2) - COMPUTER_AIMING_TOLERANCE;
                break;
        }
    }
    if (rightPaddleY < targetY - COMPUTER_AIMING_TOLERANCE) {
        return UP;
    }
    if (rightPaddleY > targetY + COMPUTER_AIMING_TOLERANCE) {
        return DOWN;
    }
    return STATIC;
}

// paddle controller functions (called to determine direction to move)
direction (*leftPaddleController)() = onePlayerController;
direction (*rightPaddleController)() = rightComputerController;

/**
 * increases the speed of the ball
*/
void accelerateBall() {
    ballSpeed = min(ballSpeed * BALL_SPEED_ACCELERATION, MAX_BALL_SPEED);
    float vel = sqrtf(ballVelocityX * ballVelocityX + ballVelocityY * ballVelocityY);
    vel /= ballSpeed;
    ballVelocityX /= vel;
    if (ballVelocityY != 0) ballVelocityY /= vel;
}

/**
 * resets the ball position for a new round
*/
void reset(int value) {
    ballSpeed = INITIAL_BALL_SPEED;
    ballX = WINDOW_WIDTHF / 2;
    ballY = WINDOW_HEIGHTF / 2;
    ballVelocityX = leftStart ? -10. : 10.;
    ballVelocityY = sinf((float) rand());
    float vel = sqrtf(ballVelocityX * ballVelocityX + ballVelocityY * ballVelocityY);
    vel /= ballSpeed;
    ballVelocityX /= vel;
    if (ballVelocityY != 0) ballVelocityY /= vel;
    leftStart = !leftStart;
    inPlay = true;
    glutPostRedisplay();
}

/**
 * glut callback (see below for definition)
*/
void fixedUpdate(int);

/**
 * sets state variables for leaving the main menu and entering the game
*/
void exitMenu() {
    menu = false;
    // hide ball
    ballX = -BALL_DIM;
    ballY = -BALL_DIM;
    leftPaddleY = INIT_PADDLE_Y;
    rightPaddleY = INIT_PADDLE_Y;
    // set delay before starting
    glutTimerFunc(RESUME_DELAY_MS, reset, 0);
    glutTimerFunc(SEC_PER_FRAME, fixedUpdate, menuInstance);
    glutPostRedisplay();
}

/**
 * sets state variables for entering the main menu
*/
void startMenu() {
    menu = true;
    inPlay = false;
    menuInstance++;
    glutPostRedisplay();
}

/**
 * sets state variables for pause menu
*/
void startPauseMenu() {
    pauseMenu = true;
    menuInstance++;
    glutPostRedisplay();
}

/**
 * sets state variables for resuming from pause menu
*/
void resumeFromPause() {
    pauseMenu = false;
    glutTimerFunc(RESUME_DELAY_MS, fixedUpdate, menuInstance);
    glutPostRedisplay();
}

/**
 * sets state variables for exiting to main menu from pause
*/
void exitFromPause() {
    pauseMenu = false;
    // hide ball
    ballX = -BALL_DIM;
    ballY = -BALL_DIM;
    ballVelocityX = 0;
    ballVelocityY = 0;
    leftPaddleY = rightPaddleY = INIT_PADDLE_Y;
    leftScore = rightScore = 0;
    startMenu();
}

/**
 * main game loop
 * should execute at frame rate while game is in play
*/
void fixedUpdate(int value) {
    if (value != menuInstance) return;
    direction d = leftPaddleController();
    if (d == DOWN) {
        leftPaddleY = max(leftPaddleY - WINDOW_HEIGHTF / 512. * PADDLE_SPEED, MIN_PADDLE_Y);
    } else if (d == UP) {
        leftPaddleY = min(leftPaddleY + WINDOW_HEIGHTF / 512. * PADDLE_SPEED, MAX_PADDLE_Y);
    }
    d = rightPaddleController();
    if (d == DOWN) {
        rightPaddleY = max(rightPaddleY - WINDOW_HEIGHTF / 512. * PADDLE_SPEED, MIN_PADDLE_Y);
    } else if (d == UP) {
        rightPaddleY = min(rightPaddleY + WINDOW_HEIGHTF / 512. * PADDLE_SPEED, MAX_PADDLE_Y);
    }

    ballX += ballVelocityX;
    ballY += ballVelocityY;

    // only calculate collisions if ball is in play
    if (inPlay) {
        // Simple collision resolvers that rely on low speed and small BALL_DIM to be accurate
        // top and bottom
        if (ballY + BALL_DIM > WINDOW_HEIGHTF) {
            ballVelocityY = -ballVelocityY;
            ballY += ballVelocityY;
        } else if (ballY < 0) {
            ballVelocityY = -ballVelocityY;
            ballY += ballVelocityY;
        }
        
        // paddles
        if (ballX < LEFT_PADDLE_X && ballX > LEFT_PADDLE_X - PADDLE_INVISIBLE_COLLIDER_WIDTH - PADDLE_WIDTH && ballY + BALL_DIM > leftPaddleY && ballY < leftPaddleY + PADDLE_HEIGHT) {
            ballX -= ballVelocityX;
            float relY = ballY + BALL_RADIUS - leftPaddleY - (PADDLE_HEIGHT / 2);
            relY /= (PADDLE_HEIGHT / 2);
            float bounceAngle = relY * MAX_BOUNCE_ANGLE_RAD;
            ballVelocityX = ballSpeed * cosf(bounceAngle);
            ballVelocityY = ballSpeed * sinf(bounceAngle);
            ballY += ballVelocityY;
            leftComputerShot = getRandomShot();
            accelerateBall();
        } else if (ballX + BALL_DIM > RIGHT_PADDLE_X && ballX + BALL_DIM < RIGHT_PADDLE_X + PADDLE_WIDTH + PADDLE_INVISIBLE_COLLIDER_WIDTH && ballY + BALL_DIM > rightPaddleY && ballY < rightPaddleY + PADDLE_HEIGHT) {
            ballX -= ballVelocityX;
            float relY = ballY + BALL_RADIUS - rightPaddleY - (PADDLE_HEIGHT / 2);
            relY /= (PADDLE_HEIGHT / 2);
            float bounceAngle = relY * MAX_BOUNCE_ANGLE_RAD;
            ballVelocityX = -ballSpeed * cosf(bounceAngle);
            ballVelocityY = ballSpeed * sinf(bounceAngle);
            ballY += ballVelocityY;
            rightComputerShot = getRandomShot();
            accelerateBall();
        }

        // score colliders
        if (ballX < 0) {
            rightScore++;
            inPlay = false;
            if (rightScore == TARGET_SCORE) {
                leftScore = rightScore = 0;
                leftPaddleY = rightPaddleY = INIT_PADDLE_Y;
                startMenu();
                return;
            }
            glutTimerFunc(SCORE_DELAY_MS, reset, 0);
        } else if (ballX + BALL_DIM > WINDOW_WIDTHF) {
            leftScore++;
            inPlay = false;
            if (leftScore == TARGET_SCORE) {
                leftScore = rightScore = 0;
                leftPaddleY = rightPaddleY = INIT_PADDLE_Y;
                startMenu();
                return;
            }
            glutTimerFunc(SCORE_DELAY_MS, reset, 0);
        }
    }
    
    glutPostRedisplay();
    glutTimerFunc(SEC_PER_FRAME, fixedUpdate, value);
}

/**
 * helper function for bounds checking
*/
inline bool inRect(int x, int y, int lbX, int lbY, int ubX, int ubY) {
    return x >= lbX && x <= ubX && y >= lbY && y <= ubY;
}

void printLogo(int x, int y) {
    // P
    glRectf(Xpos(x), Ypos(y), Xpos(x + LOGO_FONT_STROKE), Ypos(y + LOGO_FONT_HEIGHT));
    glRectf(Xpos(x), Ypos(y + LOGO_FONT_HEIGHT - LOGO_FONT_STROKE), Xpos(x + LOGO_FONT_WIDTH), Ypos(y + LOGO_FONT_HEIGHT));
    glRectf(Xpos(x), Ypos(y + (LOGO_FONT_HEIGHT - LOGO_FONT_STROKE) / 2), Xpos(x + LOGO_FONT_WIDTH), Ypos(y + (LOGO_FONT_HEIGHT + LOGO_FONT_STROKE) / 2));
    glRectf(Xpos(x + LOGO_FONT_WIDTH - LOGO_FONT_STROKE), Ypos(y + (LOGO_FONT_HEIGHT - LOGO_FONT_STROKE) / 2), Xpos(x + LOGO_FONT_WIDTH), Ypos(y + LOGO_FONT_HEIGHT));
    // O
    x += LOGO_FONT_WIDTH + LOGO_FONT_SPACING;
    glRectf(Xpos(x), Ypos(y), Xpos(x + LOGO_FONT_STROKE), Ypos(y + LOGO_FONT_HEIGHT));
    glRectf(Xpos(x + LOGO_FONT_WIDTH - LOGO_FONT_STROKE), Ypos(y), Xpos(x + LOGO_FONT_WIDTH), Ypos(y + LOGO_FONT_HEIGHT));
    glRectf(Xpos(x), Ypos(y), Xpos(x + LOGO_FONT_WIDTH), Ypos(y + LOGO_FONT_STROKE));
    glRectf(Xpos(x), Ypos(y + LOGO_FONT_HEIGHT), Xpos(x + LOGO_FONT_WIDTH), Ypos(y + LOGO_FONT_HEIGHT - LOGO_FONT_STROKE));
    // N
    x += LOGO_FONT_WIDTH + LOGO_FONT_SPACING;
    glRectf(Xpos(x), Ypos(y), Xpos(x + LOGO_FONT_STROKE), Ypos(y + LOGO_FONT_HEIGHT));
    glRectf(Xpos(x + LOGO_FONT_WIDTH), Ypos(y), Xpos(x + LOGO_FONT_WIDTH - LOGO_FONT_STROKE), Ypos(y + LOGO_FONT_HEIGHT));
    glBegin(GL_QUADS);
        glVertex2f(Xpos(x + LOGO_FONT_STROKE), Ypos(y + LOGO_FONT_HEIGHT));
        glVertex2f(Xpos(x + LOGO_FONT_STROKE), Ypos(y + LOGO_FONT_HEIGHT - 1.5 * LOGO_FONT_STROKE));
        glVertex2f(Xpos(x + LOGO_FONT_WIDTH - LOGO_FONT_STROKE), Ypos(y));
        glVertex2f(Xpos(x + LOGO_FONT_WIDTH - LOGO_FONT_STROKE), Ypos(y + 1.5 * LOGO_FONT_STROKE));
    glEnd();
    // G
    x += LOGO_FONT_WIDTH + LOGO_FONT_SPACING;
    glRectf(Xpos(x), Ypos(y), Xpos(x + LOGO_FONT_STROKE), Ypos(y + LOGO_FONT_HEIGHT));
    glRectf(Xpos(x), Ypos(y + LOGO_FONT_HEIGHT - LOGO_FONT_STROKE), Xpos(x + LOGO_FONT_WIDTH), Ypos(y + LOGO_FONT_HEIGHT));
    glRectf(Xpos(x), Ypos(y), Xpos(x + LOGO_FONT_WIDTH), Ypos(y + LOGO_FONT_STROKE));
    glRectf(Xpos(x + LOGO_FONT_WIDTH - LOGO_FONT_STROKE), Ypos(y), Xpos(x + LOGO_FONT_WIDTH), Ypos(y + (LOGO_FONT_HEIGHT + LOGO_FONT_STROKE) / 2));
    glRectf(Xpos(x + LOGO_FONT_WIDTH / 2), Ypos(y + (LOGO_FONT_HEIGHT - LOGO_FONT_STROKE) / 2), Xpos(x + LOGO_FONT_WIDTH), Ypos(y + (LOGO_FONT_HEIGHT + LOGO_FONT_STROKE) / 2));
}

inline void printLogoCentered(int x, int y) {
    printLogo(x - ((4 * LOGO_FONT_WIDTH + 3 * LOGO_FONT_SPACING) / 2), y);
}

/**
 * prints the character c at the given coords
 * WARNING! partial implementation, check code to see if your character is supported
 * Note: prints only uppercase, requires a lowercase char c
*/
void printButtonChar(int x, int y, char c) {
    switch (c) {
        case 'a':
            glRectf(Xpos(x), Ypos(y), Xpos(x + BUTTON_FONT_STROKE), Ypos(y + BUTTON_FONT_HEIGHT));
            glRectf(Xpos(x + BUTTON_FONT_WIDTH), Ypos(y), Xpos(x + BUTTON_FONT_WIDTH - BUTTON_FONT_STROKE), Ypos(y + BUTTON_FONT_HEIGHT));
            glRectf(Xpos(x), Ypos(y + (BUTTON_FONT_HEIGHT - BUTTON_FONT_STROKE) / 2), Xpos(x + BUTTON_FONT_WIDTH), Ypos(y + (BUTTON_FONT_HEIGHT + BUTTON_FONT_STROKE) / 2));
            glRectf(Xpos(x), Ypos(y + BUTTON_FONT_HEIGHT - BUTTON_FONT_STROKE), Xpos(x + BUTTON_FONT_WIDTH), Ypos(y + BUTTON_FONT_HEIGHT));
            break;
        case 'e':
            glRectf(Xpos(x), Ypos(y + (BUTTON_FONT_HEIGHT - BUTTON_FONT_STROKE) / 2), Xpos(x + 0.75 * BUTTON_FONT_WIDTH), Ypos(y + (BUTTON_FONT_HEIGHT + BUTTON_FONT_STROKE) / 2));
        case 'c':
            glRectf(Xpos(x), Ypos(y + BUTTON_FONT_HEIGHT - BUTTON_FONT_STROKE), Xpos(x + BUTTON_FONT_WIDTH), Ypos(y + BUTTON_FONT_HEIGHT));
        case 'l':
            glRectf(Xpos(x), Ypos(y), Xpos(x + BUTTON_FONT_STROKE), Ypos(y + BUTTON_FONT_HEIGHT));
            glRectf(Xpos(x), Ypos(y), Xpos(x + BUTTON_FONT_WIDTH), Ypos(y + BUTTON_FONT_STROKE));
            break;
        case 'm':
            glRectf(Xpos(x + (BUTTON_FONT_WIDTH - BUTTON_FONT_STROKE) / 2), Ypos(y), Xpos(x + (BUTTON_FONT_WIDTH + BUTTON_FONT_STROKE) / 2), Ypos(y + BUTTON_FONT_HEIGHT));
        case 'n':
            glRectf(Xpos(x), Ypos(y), Xpos(x + BUTTON_FONT_STROKE), Ypos(y + BUTTON_FONT_HEIGHT));
            glRectf(Xpos(x), Ypos(y + BUTTON_FONT_HEIGHT - BUTTON_FONT_STROKE), Xpos(x + BUTTON_FONT_WIDTH), Ypos(y + BUTTON_FONT_HEIGHT));
            glRectf(Xpos(x + BUTTON_FONT_WIDTH), Ypos(y), Xpos(x + BUTTON_FONT_WIDTH - BUTTON_FONT_STROKE), Ypos(y + BUTTON_FONT_HEIGHT));
            break;
        case 'o':
            glRectf(Xpos(x), Ypos(y), Xpos(x + BUTTON_FONT_STROKE), Ypos(y + BUTTON_FONT_HEIGHT));
            glRectf(Xpos(x + BUTTON_FONT_WIDTH), Ypos(y), Xpos(x + BUTTON_FONT_WIDTH - BUTTON_FONT_STROKE), Ypos(y + BUTTON_FONT_HEIGHT));
            glRectf(Xpos(x), Ypos(y), Xpos(x + BUTTON_FONT_WIDTH), Ypos(y + BUTTON_FONT_STROKE));
            glRectf(Xpos(x), Ypos(y + BUTTON_FONT_HEIGHT - BUTTON_FONT_STROKE), Xpos(x + BUTTON_FONT_WIDTH), Ypos(y + BUTTON_FONT_HEIGHT));
            break;
        case 's':
            glRectf(Xpos(x), Ypos(y), Xpos(x + BUTTON_FONT_WIDTH), Ypos(y + BUTTON_FONT_STROKE));
            glRectf(Xpos(x), Ypos(y + BUTTON_FONT_HEIGHT - BUTTON_FONT_STROKE), Xpos(x + BUTTON_FONT_WIDTH), Ypos(y + BUTTON_FONT_HEIGHT));
            glRectf(Xpos(x), Ypos(y + (BUTTON_FONT_HEIGHT - BUTTON_FONT_STROKE) / 2), Xpos(x + BUTTON_FONT_WIDTH), Ypos(y + (BUTTON_FONT_HEIGHT + BUTTON_FONT_STROKE) / 2));
            glRectf(Xpos(x), Ypos(y + (BUTTON_FONT_HEIGHT - BUTTON_FONT_STROKE) / 2), Xpos(x + BUTTON_FONT_STROKE), Ypos(y + BUTTON_FONT_HEIGHT));
            glRectf(Xpos(x + BUTTON_FONT_WIDTH - BUTTON_FONT_STROKE), Ypos(y), Xpos(x + BUTTON_FONT_WIDTH), Ypos(y + (BUTTON_FONT_HEIGHT + BUTTON_FONT_STROKE) / 2));
            break;
        case 'r':
            glBegin(GL_QUADS);
                glVertex2f(Xpos(x + 1.5 * BUTTON_FONT_STROKE), Ypos(y + (BUTTON_FONT_HEIGHT - BUTTON_FONT_STROKE) / 2));
                glVertex2f(Xpos(x + 2.5 * BUTTON_FONT_STROKE), Ypos(y + (BUTTON_FONT_HEIGHT - BUTTON_FONT_STROKE) / 2));
                glVertex2f(Xpos(x + BUTTON_FONT_WIDTH), Ypos(y));
                glVertex2f(Xpos(x + BUTTON_FONT_WIDTH - BUTTON_FONT_STROKE), Ypos(y));
            glEnd();
        case 'p':
            glRectf(Xpos(x), Ypos(y), Xpos(x + BUTTON_FONT_STROKE), Ypos(y + BUTTON_FONT_HEIGHT));
            glRectf(Xpos(x + BUTTON_FONT_WIDTH), Ypos(y + (BUTTON_FONT_HEIGHT - BUTTON_FONT_STROKE) / 2), Xpos(x + BUTTON_FONT_WIDTH - BUTTON_FONT_STROKE), Ypos(y + BUTTON_FONT_HEIGHT));
            glRectf(Xpos(x), Ypos(y + (BUTTON_FONT_HEIGHT - BUTTON_FONT_STROKE) / 2), Xpos(x + BUTTON_FONT_WIDTH), Ypos(y + (BUTTON_FONT_HEIGHT + BUTTON_FONT_STROKE) / 2));
            glRectf(Xpos(x), Ypos(y + BUTTON_FONT_HEIGHT - BUTTON_FONT_STROKE), Xpos(x + BUTTON_FONT_WIDTH), Ypos(y + BUTTON_FONT_HEIGHT));
            break;
        case 'i':
            glRectf(Xpos(x), Ypos(y), Xpos(x + BUTTON_FONT_WIDTH), Ypos(y + BUTTON_FONT_STROKE));
        case 't':
            glRectf(Xpos(x + (BUTTON_FONT_WIDTH - BUTTON_FONT_STROKE) / 2), Ypos(y), Xpos(x + (BUTTON_FONT_WIDTH + BUTTON_FONT_STROKE) / 2), Ypos(y + BUTTON_FONT_HEIGHT));
            glRectf(Xpos(x), Ypos(y + BUTTON_FONT_HEIGHT - BUTTON_FONT_STROKE), Xpos(x + BUTTON_FONT_WIDTH), Ypos(y + BUTTON_FONT_HEIGHT));
            break;
        case 'w':
            glRectf(Xpos(x + (BUTTON_FONT_WIDTH - BUTTON_FONT_STROKE) / 2), Ypos(y), Xpos(x + (BUTTON_FONT_WIDTH + BUTTON_FONT_STROKE) / 2), Ypos(y + BUTTON_FONT_HEIGHT));
        case 'u':
            glRectf(Xpos(x), Ypos(y), Xpos(x + BUTTON_FONT_STROKE), Ypos(y + BUTTON_FONT_HEIGHT));
            glRectf(Xpos(x), Ypos(y), Xpos(x + BUTTON_FONT_WIDTH), Ypos(y + BUTTON_FONT_STROKE));
            glRectf(Xpos(x + BUTTON_FONT_WIDTH), Ypos(y), Xpos(x + BUTTON_FONT_WIDTH - BUTTON_FONT_STROKE), Ypos(y + BUTTON_FONT_HEIGHT));
            break;
        case 'x':
            glBegin(GL_QUADS);
                glVertex2f(Xpos(x), Ypos(y));
                glVertex2f(Xpos(x + BUTTON_FONT_STROKE), Ypos(y));
                glVertex2f(Xpos(x + BUTTON_FONT_WIDTH), Ypos(y + BUTTON_FONT_HEIGHT));
                glVertex2f(Xpos(x + BUTTON_FONT_WIDTH - BUTTON_FONT_STROKE), Ypos(y + BUTTON_FONT_HEIGHT));
                glVertex2f(Xpos(x + BUTTON_FONT_WIDTH), Ypos(y));
                glVertex2f(Xpos(x + BUTTON_FONT_WIDTH - BUTTON_FONT_STROKE), Ypos(y));
                glVertex2f(Xpos(x), Ypos(y + BUTTON_FONT_HEIGHT));
                glVertex2f(Xpos(x + BUTTON_FONT_STROKE), Ypos(y + BUTTON_FONT_HEIGHT));
            glEnd();
            break;
        case 'y':
            glRectf(Xpos(x + (BUTTON_FONT_WIDTH - BUTTON_FONT_STROKE) / 2), Ypos(y), Xpos(x + (BUTTON_FONT_WIDTH + BUTTON_FONT_STROKE) / 2), Ypos(y + BUTTON_FONT_HEIGHT / 2));
            glBegin(GL_QUADS);
                glVertex2f(Xpos(x + (BUTTON_FONT_WIDTH - BUTTON_FONT_STROKE) / 2), Ypos(y + BUTTON_FONT_HEIGHT / 2));
                glVertex2f(Xpos(x + (BUTTON_FONT_WIDTH + BUTTON_FONT_STROKE) / 2), Ypos(y + BUTTON_FONT_HEIGHT / 2));
                glVertex2f(Xpos(x + BUTTON_FONT_STROKE), Ypos(y + BUTTON_FONT_HEIGHT));
                glVertex2f(Xpos(x), Ypos(y + BUTTON_FONT_HEIGHT));
                glVertex2f(Xpos(x + (BUTTON_FONT_WIDTH - BUTTON_FONT_STROKE) / 2), Ypos(y + BUTTON_FONT_HEIGHT / 2));
                glVertex2f(Xpos(x + (BUTTON_FONT_WIDTH + BUTTON_FONT_STROKE) / 2), Ypos(y + BUTTON_FONT_HEIGHT / 2));
                glVertex2f(Xpos(x + BUTTON_FONT_WIDTH), Ypos(y + BUTTON_FONT_HEIGHT));
                glVertex2f(Xpos(x + BUTTON_FONT_WIDTH - BUTTON_FONT_STROKE), Ypos(y + BUTTON_FONT_HEIGHT));

            glEnd();
            break;
    }
}

/**
 * prints the string at the given coords
 * WARNING! partial implementation, check code to see if your character is supported
 * Note: prints only uppercase, requires string be all lowercase
*/
inline void printButtonString(int x, int y, const char* string) {
    while (*string) {
        printButtonChar(x, y, *string++);
        x += BUTTON_FONT_SPACING + BUTTON_FONT_WIDTH;
    }
}

/**
 * prints the string centered at the given coords
 * WARNING! partial implementation, check code to see if your character is supported
 * Note: prints only uppercase, requires string be all lowercase
*/
inline void printButtonStringCentered(int x, int y, const char* string) {
    size_t l = strlen(string);
    printButtonString(x - (l * BUTTON_FONT_WIDTH + (l - 1) * BUTTON_FONT_SPACING) / 2, y, string);
}

/**
 * prints digit with x and y giving the position of the bottom left corner of the digit
 */
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

/**
 * draws the screen
 * glut callback for screen display
*/
void display() {
    glClearColor(BACKGROUND_COLOR, 1.);
    glClear(GL_COLOR_BUFFER_BIT);

    if (menu) {
        // logo
        glColor3f(LOGO_COLOR);
        printLogoCentered(WINDOW_WIDTHF / 2., WINDOW_HEIGHTF / 2. + BUTTON_OFFSET_Y + BUTTON_SPACING);

        // player number button
        if (playerNumberButtonHover) glColor3f(HOVER_BUTTON_COLOR);
        else glColor3f(BUTTON_COLOR);
        glRectf(Xpos(WINDOW_WIDTHF / 2 - BUTTON_OFFSET_X), Ypos(WINDOW_HEIGHTF / 2 - BUTTON_OFFSET_Y), Xpos(WINDOW_WIDTHF / 2 + BUTTON_OFFSET_X), Ypos(WINDOW_HEIGHTF / 2 + BUTTON_OFFSET_Y));
        const char* str;
        switch (gameType) {
            case ONE_PLAYER:
                str = "one player";
                break;
            case TWO_PLAYER:
                str = "two player";
                break;
            case ZERO_PLAYER:
                str = "computer";
                break;
        }
        glColor3f(BUTTON_TEXT_COLOR);
        printButtonStringCentered(WINDOW_WIDTHF / 2, WINDOW_HEIGHTF / 2 - BUTTON_OFFSET_Y + (BUTTON_HEIGHT - BUTTON_FONT_HEIGHT) / 2, str);

        // play button
        if (playButtonHover) glColor3f(HOVER_BUTTON_COLOR);
        else glColor3f(BUTTON_COLOR);
        glRectf(Xpos(WINDOW_WIDTHF / 2 - BUTTON_OFFSET_X), Ypos(WINDOW_HEIGHTF / 2 - BUTTON_OFFSET_Y - BUTTON_HEIGHT - BUTTON_SPACING), Xpos(WINDOW_WIDTHF / 2 + BUTTON_OFFSET_X), Ypos(WINDOW_HEIGHTF / 2 + BUTTON_OFFSET_Y - BUTTON_HEIGHT - BUTTON_SPACING));
        glColor3f(BUTTON_TEXT_COLOR);
        printButtonStringCentered(WINDOW_WIDTHF / 2, WINDOW_HEIGHTF / 2 - BUTTON_OFFSET_Y - BUTTON_HEIGHT - BUTTON_SPACING + (BUTTON_HEIGHT - BUTTON_FONT_HEIGHT) / 2, "play");
    } else if (pauseMenu) {
        // resume button
        if (resumeButtonHover) glColor3f(HOVER_BUTTON_COLOR);
        else glColor3f(BUTTON_COLOR);
        glRectf(Xpos(WINDOW_WIDTHF / 2 - PAUSE_BUTTON_OFFSET_X), Ypos(WINDOW_HEIGHTF / 2 - BUTTON_OFFSET_Y + BUTTON_SPACING / 2), Xpos(WINDOW_WIDTHF / 2 + PAUSE_BUTTON_OFFSET_X), Ypos(WINDOW_HEIGHTF / 2 - BUTTON_OFFSET_Y + BUTTON_HEIGHT + BUTTON_SPACING / 2));
        glColor3f(BUTTON_TEXT_COLOR);
        printButtonStringCentered(WINDOW_WIDTHF / 2, WINDOW_HEIGHTF / 2 - BUTTON_OFFSET_Y + BUTTON_SPACING / 2 + (BUTTON_HEIGHT - BUTTON_FONT_HEIGHT) / 2, "resume");

        // exit button
        if (exitButtonHover) glColor3f(HOVER_BUTTON_COLOR);
        else glColor3f(BUTTON_COLOR);
        glRectf(Xpos(WINDOW_WIDTHF / 2 - PAUSE_BUTTON_OFFSET_X), Ypos(WINDOW_HEIGHTF / 2 - BUTTON_OFFSET_Y - BUTTON_HEIGHT - BUTTON_SPACING / 2), Xpos(WINDOW_WIDTHF / 2 + PAUSE_BUTTON_OFFSET_X), Ypos(WINDOW_HEIGHTF / 2 - BUTTON_OFFSET_Y - BUTTON_SPACING / 2));
        glColor3f(BUTTON_TEXT_COLOR);
        printButtonStringCentered(WINDOW_WIDTHF / 2, WINDOW_HEIGHTF / 2 - BUTTON_OFFSET_Y - BUTTON_HEIGHT - BUTTON_SPACING / 2 + (BUTTON_HEIGHT - BUTTON_FONT_HEIGHT) / 2, "exit");

        // pause
        glColor3f(TEXT_COLOR);
        printButtonStringCentered(WINDOW_WIDTHF / 2, WINDOW_HEIGHTF / 2 + BUTTON_SPACING + BUTTON_HEIGHT, "pause");
    } else {
        // paddles
        glColor3f(PADDLE_COLOR);
        glRectf(Xpos(LEFT_PADDLE_X - PADDLE_WIDTH), Ypos(leftPaddleY), Xpos(LEFT_PADDLE_X), Ypos(leftPaddleY + PADDLE_HEIGHT));
        glRectf(Xpos(RIGHT_PADDLE_X), Ypos(rightPaddleY), Xpos(RIGHT_PADDLE_X + PADDLE_WIDTH), Ypos(rightPaddleY + PADDLE_HEIGHT));
        // scores (left, right)
        glColor3f(GAME_ENVIRONMENT_COLOR);
        printDigit((WINDOW_WIDTHF / 2) - DIGIT_OFFSET - DIGIT_WIDTH, WINDOW_HEIGHTF - DIGIT_HEIGHT - DIGIT_OFFSET, leftScore);
        printDigit((WINDOW_WIDTHF / 2) + DIGIT_OFFSET, WINDOW_HEIGHTF - DIGIT_HEIGHT - DIGIT_OFFSET, rightScore);
        // dashes
        float xtmp = (WINDOW_WIDTHF / 2) - DASH_OFFSET;
        for (float ytmp = 0; ytmp < WINDOW_HEIGHTF; ytmp += 2 * DASH_HEIGHT) {
            glRectf(Xpos(xtmp), Ypos(ytmp), Xpos(xtmp + DASH_WIDTH), Ypos(ytmp + DASH_HEIGHT));
        }
        // ball
        glColor3f(BALL_COLOR);
        glRectf(Xpos(ballX), Ypos(ballY), Xpos(ballX + BALL_DIM), Ypos(ballY + BALL_DIM));
    }
    
    glFlush();
    glutSwapBuffers();
}

/**
 * glut callback to disable window resizing
*/
void reshape(int width, int height) {
    glutReshapeWindow((int)WINDOW_WIDTHF, (int)WINDOW_HEIGHTF);
}

/**
 * glut callback for keypresses
*/
void keypress(unsigned char key, int mouseX, int mouseY) {
    if (key == 'w') upButton = true;
    else if (key == 's') downButton = true;
    else if (key == 27 /*ESC*/) {
        if (!menu) {
            if (pauseMenu) resumeFromPause();
            else startPauseMenu();
        }
    }
}

/**
 * glut callback for special keypresses
*/
void specialKeypress(int key, int mouseX, int mouseY) {
    if (key == GLUT_KEY_UP) specialUpButton = true;
    else if (key == GLUT_KEY_DOWN) specialDownButton = true;
}

/**
 * glut callback for key releases
*/
void keyrelease(unsigned char key, int mouseX, int mouseY) {
    if (key == 'w') upButton = false;
    else if (key == 's') downButton = false;
}

/**
 * glut callback for special key releases
*/
void specialKeyrelease(int key, int mouseX, int mouseY) {
    if (key == GLUT_KEY_UP) specialUpButton = false;
    else if (key == GLUT_KEY_DOWN) specialDownButton = false;
}

/**
 * glut mouse callback
 * handles shading buttons when hovered
*/
void hoverHandler(int x, int y) {
    if (menu || pauseMenu) {
        // flip x and y
        y = WINDOW_HEIGHTF - y;
        x = WINDOW_WIDTHF - x;
        if(inRect(x, y, (WINDOW_WIDTHF / 2 - BUTTON_OFFSET_X), (WINDOW_HEIGHTF / 2 - BUTTON_OFFSET_Y), (WINDOW_WIDTHF / 2 + BUTTON_OFFSET_X), (WINDOW_HEIGHTF / 2 + BUTTON_OFFSET_Y))) {
            playerNumberButtonHover = true;
            glutPostRedisplay();
        } else if (playerNumberButtonHover) {
            playerNumberButtonHover = false;
            glutPostRedisplay();
        }
        if(inRect(x, y, (WINDOW_WIDTHF / 2 - BUTTON_OFFSET_X), (WINDOW_HEIGHTF / 2 - BUTTON_OFFSET_Y - BUTTON_HEIGHT - BUTTON_SPACING), (WINDOW_WIDTHF / 2 + BUTTON_OFFSET_X), (WINDOW_HEIGHTF / 2 + BUTTON_OFFSET_Y - BUTTON_HEIGHT - BUTTON_SPACING))) {
            playButtonHover = true;
            glutPostRedisplay();
        } else if (playButtonHover) {
            playButtonHover = false;
            glutPostRedisplay();
        }
        if(inRect(x, y, (WINDOW_WIDTHF / 2 - PAUSE_BUTTON_OFFSET_X), (WINDOW_HEIGHTF / 2 - BUTTON_OFFSET_Y + BUTTON_SPACING / 2), (WINDOW_WIDTHF / 2 + PAUSE_BUTTON_OFFSET_X), (WINDOW_HEIGHTF / 2 - BUTTON_OFFSET_Y + BUTTON_HEIGHT + BUTTON_SPACING / 2))) {
            resumeButtonHover = true;
            glutPostRedisplay();
        } else if (resumeButtonHover) {
            resumeButtonHover = false;
            glutPostRedisplay();
        }
        if(inRect(x, y, (WINDOW_WIDTHF / 2 - PAUSE_BUTTON_OFFSET_X), (WINDOW_HEIGHTF / 2 - BUTTON_OFFSET_Y - BUTTON_HEIGHT - BUTTON_SPACING / 2), (WINDOW_WIDTHF / 2 + PAUSE_BUTTON_OFFSET_X), (WINDOW_HEIGHTF / 2 - BUTTON_OFFSET_Y - BUTTON_SPACING / 2))) {
            exitButtonHover = true;
            glutPostRedisplay();
        } else if (exitButtonHover) {
            exitButtonHover = false;
            glutPostRedisplay();
        }
    }
}

/**
 * glut callback for mouse clicks
 * handles button presses
*/
void clickHandler(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        // flip x and y
        y = WINDOW_HEIGHTF - y;
        x = WINDOW_WIDTHF - x;
        if (menu) {
            if (inRect(x, y, (WINDOW_WIDTHF / 2 - BUTTON_OFFSET_X), (WINDOW_HEIGHTF / 2 - BUTTON_OFFSET_Y), (WINDOW_WIDTHF / 2 + BUTTON_OFFSET_X), (WINDOW_HEIGHTF / 2 + BUTTON_OFFSET_Y))) {
                // player number button
                gameType = ++gameType % 3;
                switch (gameType) {
                    case ONE_PLAYER:
                        leftPaddleController = onePlayerController;
                        rightPaddleController = rightComputerController;
                        break;
                    case TWO_PLAYER:
                        leftPaddleController = wasdPlayerController;
                        rightPaddleController = arrowPlayerController;
                        break;
                    case ZERO_PLAYER:
                        leftPaddleController = leftComputerController;
                        rightPaddleController = rightComputerController;
                        break;
                }
                glutPostRedisplay();
            } else if (inRect(x, y, (WINDOW_WIDTHF / 2 - BUTTON_OFFSET_X), (WINDOW_HEIGHTF / 2 - BUTTON_OFFSET_Y - BUTTON_HEIGHT - BUTTON_SPACING), (WINDOW_WIDTHF / 2 + BUTTON_OFFSET_X), (WINDOW_HEIGHTF / 2 + BUTTON_OFFSET_Y - BUTTON_HEIGHT - BUTTON_SPACING))) {
                // play button
                exitMenu();
            }
        } else if (pauseMenu) {
            if (inRect(x, y, (WINDOW_WIDTHF / 2 - PAUSE_BUTTON_OFFSET_X), (WINDOW_HEIGHTF / 2 - BUTTON_OFFSET_Y + BUTTON_SPACING / 2), (WINDOW_WIDTHF / 2 + PAUSE_BUTTON_OFFSET_X), (WINDOW_HEIGHTF / 2 - BUTTON_OFFSET_Y + BUTTON_HEIGHT + BUTTON_SPACING / 2))) {
                // resume button
                resumeFromPause();
            } else if (inRect(x, y, (WINDOW_WIDTHF / 2 - PAUSE_BUTTON_OFFSET_X), (WINDOW_HEIGHTF / 2 - BUTTON_OFFSET_Y - BUTTON_HEIGHT - BUTTON_SPACING / 2), (WINDOW_WIDTHF / 2 + PAUSE_BUTTON_OFFSET_X), (WINDOW_HEIGHTF / 2 - BUTTON_OFFSET_Y - BUTTON_SPACING / 2))) {
                // exit button
                exitFromPause();
            }
        }
    }
}

/**
 * main function, glut init
*/
int main(int argc, char* argv) {
    srand(time(NULL));
    glutInit(&argc, &argv);
    glutInitWindowSize((int)WINDOW_WIDTHF, (int)WINDOW_HEIGHTF);
    glutInitWindowPosition(100, 100);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);

    glutCreateWindow("Pong");
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMouseFunc(clickHandler);
    glutKeyboardFunc(keypress);
    glutSpecialFunc(specialKeypress);
    glutKeyboardUpFunc(keyrelease);
    glutSpecialUpFunc(specialKeyrelease);
    glutMotionFunc(hoverHandler);
    glutPassiveMotionFunc(hoverHandler);
    
    glutMainLoop();
}