/*
 * CS3202/SCS3302 - Computer Graphics and Image Processing
 * Assignment 01 - Question 01
 *
 * Part A: Cyan-colored cube
 * Part B: Square-based pyramid with RGB gradient (smooth-shaded) faces
 * Part C: Composite model - one pyramid on top, and four pyramids around
 *         the cube (matching the reference "flower/star" figure), using
 *         translation, rotation and scaling. Pyramid height == cube height.
 *
 * Compile (Linux):   g++ Q1_cube_pyramid.cpp -o q1 -lGL -lGLU -lglut
 * Compile (MinGW):   g++ Q1_cube_pyramid.cpp -o q1.exe -lfreeglut -lopengl32 -lglu32
 * Run:                ./q1
 */

#include <GL/glut.h>

// ---------------------------------------------------------------------
// Global parameters
// ---------------------------------------------------------------------
const float CUBE_SIZE = 1.0f;      // full side length of the cube
const float CUBE_HALF = CUBE_SIZE / 2.0f;
const float PYRAMID_HEIGHT = CUBE_SIZE;   // PS: pyramid height == cube height
const float PYRAMID_BASE   = CUBE_SIZE;   // base as wide as the cube face

float rotY = 25.0f;   // for a nice 3D viewing angle
float rotX = 20.0f;

// ---------------------------------------------------------------------
// Part A: Draw a cyan cube centered at the origin
// ---------------------------------------------------------------------
void drawCyanCube()
{
    glColor3f(0.0f, 1.0f, 1.0f); // cyan

    glBegin(GL_QUADS);
        // Front face (+Z)
        glVertex3f(-CUBE_HALF, -CUBE_HALF,  CUBE_HALF);
        glVertex3f( CUBE_HALF, -CUBE_HALF,  CUBE_HALF);
        glVertex3f( CUBE_HALF,  CUBE_HALF,  CUBE_HALF);
        glVertex3f(-CUBE_HALF,  CUBE_HALF,  CUBE_HALF);

        // Back face (-Z)
        glVertex3f(-CUBE_HALF, -CUBE_HALF, -CUBE_HALF);
        glVertex3f(-CUBE_HALF,  CUBE_HALF, -CUBE_HALF);
        glVertex3f( CUBE_HALF,  CUBE_HALF, -CUBE_HALF);
        glVertex3f( CUBE_HALF, -CUBE_HALF, -CUBE_HALF);

        // Left face (-X)
        glVertex3f(-CUBE_HALF, -CUBE_HALF, -CUBE_HALF);
        glVertex3f(-CUBE_HALF, -CUBE_HALF,  CUBE_HALF);
        glVertex3f(-CUBE_HALF,  CUBE_HALF,  CUBE_HALF);
        glVertex3f(-CUBE_HALF,  CUBE_HALF, -CUBE_HALF);

        // Right face (+X)
        glVertex3f( CUBE_HALF, -CUBE_HALF, -CUBE_HALF);
        glVertex3f( CUBE_HALF,  CUBE_HALF, -CUBE_HALF);
        glVertex3f( CUBE_HALF,  CUBE_HALF,  CUBE_HALF);
        glVertex3f( CUBE_HALF, -CUBE_HALF,  CUBE_HALF);

        // Top face (+Y)
        glVertex3f(-CUBE_HALF,  CUBE_HALF, -CUBE_HALF);
        glVertex3f(-CUBE_HALF,  CUBE_HALF,  CUBE_HALF);
        glVertex3f( CUBE_HALF,  CUBE_HALF,  CUBE_HALF);
        glVertex3f( CUBE_HALF,  CUBE_HALF, -CUBE_HALF);

        // Bottom face (-Y)
        glVertex3f(-CUBE_HALF, -CUBE_HALF, -CUBE_HALF);
        glVertex3f( CUBE_HALF, -CUBE_HALF, -CUBE_HALF);
        glVertex3f( CUBE_HALF, -CUBE_HALF,  CUBE_HALF);
        glVertex3f(-CUBE_HALF, -CUBE_HALF,  CUBE_HALF);
    glEnd();
}

// ---------------------------------------------------------------------
// Part B: Square-based pyramid, apex pointing +Y, base centered at origin.
// Each of the 4 triangular side faces gets red/green/blue vertex colors
// so OpenGL's default smooth (Gouraud) shading blends them.
// ---------------------------------------------------------------------
void drawGradientPyramid()
{
    float b = PYRAMID_BASE / 2.0f;
    float h = PYRAMID_HEIGHT;

    // Base square corners
    float A[3] = {-b, 0.0f, -b}; // back-left
    float B[3] = { b, 0.0f, -b}; // back-right
    float C[3] = { b, 0.0f,  b}; // front-right
    float D[3] = {-b, 0.0f,  b}; // front-left
    float apex[3] = {0.0f, h, 0.0f};

    glBegin(GL_TRIANGLES);
        // Face 1: A - B - apex
        glColor3f(1, 0, 0); glVertex3fv(A);
        glColor3f(0, 1, 0); glVertex3fv(B);
        glColor3f(0, 0, 1); glVertex3fv(apex);

        // Face 2: B - C - apex
        glColor3f(1, 0, 0); glVertex3fv(B);
        glColor3f(0, 1, 0); glVertex3fv(C);
        glColor3f(0, 0, 1); glVertex3fv(apex);

        // Face 3: C - D - apex
        glColor3f(1, 0, 0); glVertex3fv(C);
        glColor3f(0, 1, 0); glVertex3fv(D);
        glColor3f(0, 0, 1); glVertex3fv(apex);

        // Face 4: D - A - apex
        glColor3f(1, 0, 0); glVertex3fv(D);
        glColor3f(0, 1, 0); glVertex3fv(A);
        glColor3f(0, 0, 1); glVertex3fv(apex);
    glEnd();

    // Base (two triangles), plain gray so it's not distracting
    glColor3f(0.3f, 0.3f, 0.3f);
    glBegin(GL_TRIANGLES);
        glVertex3fv(A); glVertex3fv(B); glVertex3fv(C);
        glVertex3fv(A); glVertex3fv(C); glVertex3fv(D);
    glEnd();
}

// ---------------------------------------------------------------------
// Part C: Composite model - cube in the middle, one pyramid on each of
// the 4 side faces of the cube (rotated to point outward), matching the
// reference "star" figure. Uses translate + rotate; pyramid height ==
// cube height as required.
// ---------------------------------------------------------------------
void drawComposite()
{
    // Center cube
    drawCyanCube();

    float offset = CUBE_HALF; // move pyramid so its base sits flush on cube face

    // Pyramid pointing +X (right)
    glPushMatrix();
        glTranslatef(offset, 0.0f, 0.0f);
        glRotatef(90.0f, 0.0f, 0.0f, 1.0f); // rotate apex to point along +X
        drawGradientPyramid();
    glPopMatrix();

    // Pyramid pointing -X (left)
    glPushMatrix();
        glTranslatef(-offset, 0.0f, 0.0f);
        glRotatef(-90.0f, 0.0f, 0.0f, 1.0f);
        drawGradientPyramid();
    glPopMatrix();

    // Pyramid pointing +Z (front, toward viewer)
    glPushMatrix();
        glTranslatef(0.0f, 0.0f, offset);
        glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
        drawGradientPyramid();
    glPopMatrix();

    // Pyramid pointing -Z (back)
    glPushMatrix();
        glTranslatef(0.0f, 0.0f, -offset);
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
        drawGradientPyramid();
    glPopMatrix();

    // Pyramid pointing +Y (top) - matches reference image's top spike
    glPushMatrix();
        glTranslatef(0.0f, offset, 0.0f);
        drawGradientPyramid();
    glPopMatrix();
}

// ---------------------------------------------------------------------
// GLUT callbacks
// ---------------------------------------------------------------------
void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    gluLookAt(0, 0, 4,   0, 0, 0,   0, 1, 0);
    glRotatef(rotX, 1.0f, 0.0f, 0.0f);
    glRotatef(rotY, 0.0f, 1.0f, 0.0f);

    drawComposite();

    glutSwapBuffers();
}

void reshape(int w, int h)
{
    if (h == 0) h = 1;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (double)w / h, 0.1, 100.0);
    glMatrixMode(GL_MODELVIEW);
}

void keyboard(unsigned char key, int x, int y)
{
    switch (key) {
        case 'a': rotY -= 5.0f; break;
        case 'd': rotY += 5.0f; break;
        case 'w': rotX -= 5.0f; break;
        case 's': rotX += 5.0f; break;
        case 27: exit(0); // ESC
    }
    glutPostRedisplay();
}

void init()
{
    glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_SMOOTH);   // required for the color-interpolated pyramid faces
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // black background, like reference image
}

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(700, 700);
    glutCreateWindow("CS3202 Assignment 01 - Q1: Cube + Gradient Pyramids");

    init();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard); // w/a/s/d to rotate view, ESC to quit

    glutMainLoop();
    return 0;
}