// bresenham_cb.cpp
// Bresenham Line (all octants) + Thick Lines via filled disk brush
// Designed to be Code::Blocks + FreeGLUT friendly on Windows

#ifdef _WIN32
    #include <windows.h>
#endif
#include <GL/glut.h>
#include <cmath>
#include <cstdlib>
#include <string>

// -------- Config --------
static int  winW = 900, winH = 600;
static bool thickMode = true;
static int  lineWidthW = 7; // odd values look nice: 3,5,7,...

struct Point { int x, y; };
static bool haveP1 = false, haveP2 = false;
static Point P1{ 120, 120 }, P2{ 780, 460 };

// -------- Utilities --------
static inline int clampi(int v, int lo, int hi) {
    return (v < lo ? lo : (v > hi ? hi : v));
}
static inline int toGLY(int yTop) { return winH - 1 - yTop; }

// Submit a single pixel (expects GL_POINTS already begun by caller)
static inline void plotPoint(int x, int y) {
    if ((unsigned)x >= (unsigned)winW || (unsigned)y >= (unsigned)winH) return;
    glVertex2i(x, y);
}

// Draw a horizontal span (x1..x2 inclusive) at row y
static inline void drawHSpan(int x1, int x2, int y) {
    if ((unsigned)y >= (unsigned)winH) return;
    if (x1 > x2) { int t = x1; x1 = x2; x2 = t; }
    x1 = clampi(x1, 0, winW - 1);
    x2 = clampi(x2, 0, winW - 1);
    for (int x = x1; x <= x2; ++x) plotPoint(x, y);
}

// Filled disk with 8-way symmetry (midpoint circle), centered at (xc,yc), radius r
static void drawFilledCircle(int xc, int yc, int r) {
    if (r <= 0) { plotPoint(xc, yc); return; }
    int x = 0, y = r;
    int d = 1 - r;
    while (x <= y) {
        drawHSpan(xc - x, xc + x, yc + y);
        drawHSpan(xc - x, xc + x, yc - y);
        drawHSpan(xc - y, xc + y, yc + x);
        drawHSpan(xc - y, xc + y, yc - x);

        if (d < 0) d += (2 * x + 3);
        else { d += (2 * (x - y) + 5); --y; }
        ++x;
    }
}

static inline void plotThickPixel(int x, int y, int W) {
    int r = W / 2;
    drawFilledCircle(x, y, r);
}

// Integer Bresenham for all octants; calls plot(x,y) per pixel.
template<typename PlotFunc>
static void bresenhamLine(int x0, int y0, int x1, int y1, const PlotFunc& plot) {
    int dx = std::abs(x1 - x0);
    int dy = std::abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;

    if (dy <= dx) {
        int err = 2 * dy - dx;
        for (int i = 0; i <= dx; ++i) {
            plot(x0, y0);
            if (x0 == x1) break;
            if (err >= 0) { y0 += sy; err -= 2 * dx; }
            x0 += sx;
            err += 2 * dy;
        }
    } else {
        int err = 2 * dx - dy;
        for (int i = 0; i <= dy; ++i) {
            plot(x0, y0);
            if (y0 == y1) break;
            if (err >= 0) { x0 += sx; err -= 2 * dy; }
            y0 += sy;
            err += 2 * dx;
        }
    }
}

static void drawLine(int x0, int y0, int x1, int y1, int W) {
    if (W <= 1) {
        bresenhamLine(x0, y0, x1, y1, [](int x, int y){ plotPoint(x, y); });
    } else {
        bresenhamLine(x0, y0, x1, y1, [W](int x, int y){ plotThickPixel(x, y, W); });
    }
}

static void drawInfo() {
    glColor3f(1, 1, 0);
    std::string s = "Left-click to set P1,P2 | T: Thick ON/OFF | +/- : Width | C: Clear | R: Random | W="
                    + std::to_string(lineWidthW) + (thickMode ? " (Thick)" : " (Thin)");
    glRasterPos2i(10, winH - 20);
    for (char c : s) glutBitmapCharacter(GLUT_BITMAP_9_BY_15, c);
}

static void drawEndpoints() {
    if (!(haveP1 && haveP2)) return;
    glPointSize(6.f);
    glBegin(GL_POINTS);
    glVertex2i(P1.x, P1.y);
    glVertex2i(P2.x, P2.y);
    glEnd();
}

// -------- GLUT Callbacks --------
static void displayCB() {
    glClear(GL_COLOR_BUFFER_BIT);

    // Draw axes (optional)
    glColor3f(0.15f, 0.15f, 0.16f);
    glBegin(GL_POINTS);
    for (int x = 0; x < winW; ++x) plotPoint(x, winH / 2);
    for (int y = 0; y < winH; ++y) plotPoint(winW / 2, y);
    glEnd();

    // Draw line
    glColor3f(1, 1, 1);
    glBegin(GL_POINTS);
    if (haveP1 && haveP2) {
        drawLine(P1.x, P1.y, P2.x, P2.y, thickMode ? lineWidthW : 1);
    }
    glEnd();

    // Endpoints + HUD
    glColor3f(0.2f, 0.8f, 1.0f);
    drawEndpoints();
    drawInfo();

    glutSwapBuffers();
}

static void reshapeCB(int w, int h) {
    winW = (w < 1 ? 1 : w);
    winH = (h < 1 ? 1 : h);
    glViewport(0, 0, winW, winH);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, winW, 0, winH);  // pixel coords
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

static void mouseCB(int button, int state, int x, int yTop) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        int y = toGLY(yTop);
        if (!haveP1) {
            P1 = { clampi(x, 0, winW - 1), clampi(y, 0, winH - 1) };
            haveP1 = true; haveP2 = false;
        } else if (!haveP2) {
            P2 = { clampi(x, 0, winW - 1), clampi(y, 0, winH - 1) };
            haveP2 = true;
        } else {
            haveP1 = haveP2 = false;
        }
        glutPostRedisplay();
    }
}

static void keyboardCB(unsigned char key, int, int) {
    switch (key) {
        case 27: std::exit(0); break; // Esc
        case 't': case 'T':
            thickMode = !thickMode; glutPostRedisplay(); break;
        case '+':
            lineWidthW = (lineWidthW < 99 ? lineWidthW + 1 : 99); glutPostRedisplay(); break;
        case '-':
            lineWidthW = (lineWidthW > 1 ? lineWidthW - 1 : 1); glutPostRedisplay(); break;
        case 'c': case 'C':
            haveP1 = haveP2 = false; glutPostRedisplay(); break;
        case 'r': case 'R':
            haveP1 = haveP2 = true;
            P1 = { std::rand() % winW, std::rand() % winH };
            P2 = { std::rand() % winW, std::rand() % winH };
            glutPostRedisplay(); break;
    }
}

int main(int argc, char** argv) {
    std::srand(20251024);

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(winW, winH);
    glutCreateWindow("Bresenham + Thick Lines (GLUT, Code::Blocks Friendly)");

    glClearColor(0.05f, 0.06f, 0.08f, 1.0f);
    glDisable(GL_POINT_SMOOTH);   // we want exact integer raster look
    glPointSize(1.0f);

    glutDisplayFunc(displayCB);
    glutReshapeFunc(reshapeCB);
    glutMouseFunc(mouseCB);
    glutKeyboardFunc(keyboardCB);

    // show an initial line
    haveP1 = haveP2 = true;

    glutMainLoop();
    return 0;
}
