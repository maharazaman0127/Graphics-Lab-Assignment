// Liang-Barsky Line Clipping (FreeGLUT, Code::Blocks / Windows)
// Add segments with mouse, adjust clipping window with keyboard.
// Original lines: gray. Clipped parts: bright cyan. Clipping rect: yellow.

#include <GL/glut.h>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>

struct Pt { int x, y; };
struct Seg { Pt a, b; };

static int winW = 900, winH = 600;

// Clipping window (ensure xmin<=xmax, ymin<=ymax)
static int xminC = 200, yminC = 150, xmaxC = 700, ymaxC = 450;

// Data
static std::vector<Seg> segments;

// Mouse helpers
static bool haveFirst = false;
static Pt firstPt;

// --------------- Utils ---------------
inline int clampi(int v, int lo, int hi){ return std::max(lo, std::min(hi, v)); }

// Convert GLUT mouse y (top-down) to world y (bottom-up)
inline int toGLY(int y) { return winH - 1 - y; }

// --------------- Liang–Barsky ---------------
// Returns true if a visible portion exists; outputs clipped endpoints (cx0,cy0) - (cx1,cy1)
bool liangBarskyClip(int xmin, int ymin, int xmax, int ymax,
                     float x0, float y0, float x1, float y1,
                     float &cx0, float &cy0, float &cx1, float &cy1)
{
    float dx = x1 - x0, dy = y1 - y0;
    float p[4] = {-dx, dx, -dy, dy};
    float q[4] = {x0 - xmin, xmax - x0, y0 - ymin, ymax - y0};

    float u1 = 0.0f, u2 = 1.0f;

    for (int i = 0; i < 4; ++i) {
        if (std::fabs(p[i]) < 1e-9f) {
            if (q[i] < 0) return false; // parallel & outside
        } else {
            float r = q[i] / p[i];
            if (p[i] < 0) { // entering
                if (r > u1) u1 = r;
            } else {        // leaving
                if (r < u2) u2 = r;
            }
            if (u1 > u2) return false;
        }
    }

    cx0 = x0 + u1 * dx; cy0 = y0 + u1 * dy;
    cx1 = x0 + u2 * dx; cy1 = y0 + u2 * dy;
    return true;
}

// --------------- Drawing helpers ---------------
void drawClippingRect()
{
    glColor3ub(255, 210, 60); // yellow
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2i(xminC, yminC);
    glVertex2i(xmaxC, yminC);
    glVertex2i(xmaxC, ymaxC);
    glVertex2i(xminC, ymaxC);
    glEnd();
    glLineWidth(1.0f);
}

void drawSegments()
{
    // Original segments (gray)
    glColor3ub(140, 140, 150);
    glBegin(GL_LINES);
    for (const auto& s : segments) {
        glVertex2i(s.a.x, s.a.y);
        glVertex2i(s.b.x, s.b.y);
    }
    glEnd();

    // Clipped visible parts (cyan)
    glColor3ub(90, 240, 255);
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    for (const auto& s : segments) {
        float cx0, cy0, cx1, cy1;
        if (liangBarskyClip(xminC, yminC, xmaxC, ymaxC,
                            (float)s.a.x, (float)s.a.y,
                            (float)s.b.x, (float)s.b.y,
                            cx0, cy0, cx1, cy1)) {
            glVertex2f(cx0, cy0);
            glVertex2f(cx1, cy1);
        }
    }
    glEnd();
    glLineWidth(1.0f);
}

void drawHUD()
{
    // Simple text instructions (optional)
    glColor3ub(220, 220, 220);
    glRasterPos2i(10, winH - 20);
    const char* s1 = "Left click: first point | Right click: second point (add segment)";
    for (const char* p = s1; *p; ++p) glutBitmapCharacter(GLUT_BITMAP_8_BY_13, *p);

    glRasterPos2i(10, winH - 38);
    const char* s2 = "W/S/A/D: move clip window | Arrow keys: resize | R: randomize | C: clear | Q/Esc: quit";
    for (const char* p = s2; *p; ++p) glutBitmapCharacter(GLUT_BITMAP_8_BY_13, *p);
}

// --------------- GLUT callbacks ---------------
void display()
{
    glClear(GL_COLOR_BUFFER_BIT);

    drawClippingRect();
    drawSegments();
    drawHUD();

    glutSwapBuffers();
}

void reshape(int w, int h)
{
    winW = std::max(1, w);
    winH = std::max(1, h);

    glViewport(0, 0, winW, winH);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, winW, 0, winH);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Keep the clipping rect inside the window bounds
    xminC = clampi(xminC, 0, winW - 1);
    xmaxC = clampi(xmaxC, 0, winW - 1);
    yminC = clampi(yminC, 0, winH - 1);
    ymaxC = clampi(ymaxC, 0, winH - 1);

    if (xminC > xmaxC) std::swap(xminC, xmaxC);
    if (yminC > ymaxC) std::swap(yminC, ymaxC);
}

void keyboard(unsigned char key, int, int)
{
    const int stepMove = 10;
    switch (key) {
        case 27: case 'q': case 'Q':
            std::exit(0);
            break;

        // Move clipping window (WASD)
        case 'w': case 'W':
            yminC += stepMove; ymaxC += stepMove; break;
        case 's': case 'S':
            yminC -= stepMove; ymaxC -= stepMove; break;
        case 'a': case 'A':
            xminC -= stepMove; xmaxC -= stepMove; break;
        case 'd': case 'D':
            xminC += stepMove; xmaxC += stepMove; break;

        // Random segments
        case 'r': case 'R':
        {
            segments.clear();
            for (int i = 0; i < 20; ++i) {
                Seg s;
                s.a.x = rand() % winW; s.a.y = rand() % winH;
                s.b.x = rand() % winW; s.b.y = rand() % winH;
                segments.push_back(s);
            }
            break;
        }

        // Clear segments
        case 'c': case 'C':
            segments.clear();
            haveFirst = false;
            break;
    }

    // keep window inside bounds
    xminC = clampi(xminC, 0, winW - 1);
    xmaxC = clampi(xmaxC, 0, winW - 1);
    yminC = clampi(yminC, 0, winH - 1);
    ymaxC = clampi(ymaxC, 0, winH - 1);
    if (xminC > xmaxC) std::swap(xminC, xmaxC);
    if (yminC > ymaxC) std::swap(yminC, ymaxC);

    glutPostRedisplay();
}

void special(int key, int, int)
{
    // Resize clipping window with arrow keys
    const int stepResize = 8;
    switch (key) {
        case GLUT_KEY_LEFT:  xminC -= stepResize; break;
        case GLUT_KEY_RIGHT: xmaxC += stepResize; break;
        case GLUT_KEY_DOWN:  yminC -= stepResize; break;
        case GLUT_KEY_UP:    ymaxC += stepResize; break;
    }

    xminC = clampi(xminC, 0, winW - 1);
    xmaxC = clampi(xmaxC, 0, winW - 1);
    yminC = clampi(yminC, 0, winH - 1);
    ymaxC = clampi(ymaxC, 0, winH - 1);

    if (xminC > xmaxC) std::swap(xminC, xmaxC);
    if (yminC > ymaxC) std::swap(yminC, ymaxC);

    glutPostRedisplay();
}

void mouse(int button, int state, int x, int y)
{
    if (state != GLUT_DOWN) return;

    int gx = clampi(x, 0, winW - 1);
    int gy = clampi(toGLY(y), 0, winH - 1);

    if (button == GLUT_LEFT_BUTTON) {
        firstPt = {gx, gy};
        haveFirst = true;
    } else if (button == GLUT_RIGHT_BUTTON) {
        if (haveFirst) {
            Seg s; s.a = firstPt; s.b = {gx, gy};
            segments.push_back(s);
            haveFirst = false;
        }
    }

    glutPostRedisplay();
}

// --------------- Init ---------------
void initGL()
{
    glClearColor(0.07f, 0.08f, 0.11f, 1.0f); // dark bg
    glDisable(GL_LINE_SMOOTH);
    glDisable(GL_POINT_SMOOTH);
    std::srand((unsigned)std::time(nullptr));

    // Seed with some example segments
    for (int i = 0; i < 10; ++i) {
        Seg s;
        s.a.x = 30 + i * 80; s.a.y = 20 + (i % 2 ? 480 : 80);
        s.b.x = 850 - i * 60; s.b.y = 550 - (i % 2 ? 450 : 120);
        segments.push_back(s);
    }
}

// --------------- main ---------------
int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(winW, winH);
    glutCreateWindow("Liang-Barsky Line Clipping (GLUT)");

    initGL();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special);
    glutMouseFunc(mouse);

    glutMainLoop();
    return 0;
}
