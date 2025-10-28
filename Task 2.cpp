// Concentric circles with Bresenham (Midpoint) + thickness + HSV color gradient
// Works in Code::Blocks on Windows with FreeGLUT.

#include <GL/glut.h>
#include <cmath>
#include <algorithm>

// ---------- Window / scene params ----------
static int winW = 800, winH = 600;
static int cx, cy; // center

// Circle set params (modifiable via keyboard)
static int numCircles   = 18;  // how many circles
static int baseRadius   = 18;  // first circle radius
static int radiusStep   = 12;  // radius increment per circle
static int baseThick    = 2;   // thickness of innermost ring
static int thickStep    = 1;   // thickness increment per circle

// ---------- Helpers ----------
inline int clampi(int v, int lo, int hi){ return std::max(lo, std::min(hi, v)); }

// Simple HSV→RGB (H in [0,1), S,V in [0,1])
static void hsv2rgb(float h, float s, float v, float& r, float& g, float& b){
    if(s <= 1e-6f){ r = g = b = v; return; }
    h = fmodf(h, 1.0f); if(h < 0) h += 1.0f;
    float hf = h * 6.0f;
    int   i  = (int)floorf(hf);
    float f  = hf - i;
    float p = v * (1.0f - s);
    float q = v * (1.0f - s * f);
    float t = v * (1.0f - s * (1.0f - f));
    switch(i % 6){
        case 0: r=v; g=t; b=p; break;
        case 1: r=q; g=v; b=p; break;
        case 2: r=p; g=v; b=t; break;
        case 3: r=p; g=q; b=v; break;
        case 4: r=t; g=p; b=v; break;
        case 5: r=v; g=p; b=q; break;
    }
}

// Draw filled square brush centered at (x,y), radius r (in pixels)
static void putThickPixel(int x, int y, int r){
    int x0 = clampi(x - r, 0, winW - 1);
    int x1 = clampi(x + r, 0, winW - 1);
    int y0 = clampi(y - r, 0, winH - 1);
    int y1 = clampi(y + r, 0, winH - 1);

    glBegin(GL_QUADS);
    glVertex2i(x0, y0);
    glVertex2i(x1 + 1, y0);
    glVertex2i(x1 + 1, y1 + 1);
    glVertex2i(x0, y1 + 1);
    glEnd();
}

// Plot the 8-way symmetric points for a circle point (x,y) around center (xc,yc)
static void plot8(int xc, int yc, int x, int y, int brushR){
    putThickPixel(xc + x, yc + y, brushR);
    putThickPixel(xc - x, yc + y, brushR);
    putThickPixel(xc + x, yc - y, brushR);
    putThickPixel(xc - x, yc - y, brushR);
    putThickPixel(xc + y, yc + x, brushR);
    putThickPixel(xc - y, yc + x, brushR);
    putThickPixel(xc + y, yc - x, brushR);
    putThickPixel(xc - y, yc - x, brushR);
}

// Midpoint (Bresenham) circle with thickness W (in pixels)
static void drawCircleMidpoint(int xc, int yc, int radius, int W){
    if(radius <= 0 || W <= 0) return;

    // Brush radius (square), 4-way symmetric stamp
    int brushR = std::max(0, (W - 1) / 2);

    // Standard midpoint algorithm
    int x = 0;
    int y = radius;
    int d = 1 - radius; // decision

    plot8(xc, yc, x, y, brushR);
    while (x < y){
        x++;
        if (d < 0){
            d += 2*x + 1;
        } else {
            y--;
            d += 2*(x - y) + 1;
        }
        plot8(xc, yc, x, y, brushR);
    }
}

// ---------- Rendering ----------
static void display(){
    glClear(GL_COLOR_BUFFER_BIT);

    // Draw concentric circles
    for(int i = 0; i < numCircles; ++i){
        int r  = baseRadius + i * radiusStep;
        int W  = std::max(1, baseThick + i * thickStep);

        // Gradient: hue from 0.00 → 0.85 across circles
        float t  = (numCircles <= 1) ? 0.f : (float)i / (float)(numCircles - 1);
        float rr, gg, bb;
        hsv2rgb(0.85f * t, 0.95f, 1.0f, rr, gg, bb);
        glColor3f(rr, gg, bb);

        drawCircleMidpoint(cx, cy, r, W);
    }

    glutSwapBuffers();
}

static void reshape(int w, int h){
    winW = std::max(1, w);
    winH = std::max(1, h);
    cx = winW / 2;
    cy = winH / 2;

    glViewport(0, 0, winW, winH);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, winW, 0, winH);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

static void resetParams(){
    numCircles = 18;
    baseRadius = 18;
    radiusStep = 12;
    baseThick  = 2;
    thickStep  = 1;
}

static void keyboard(unsigned char key, int, int){
    switch(key){
        case 27: case 'q': case 'Q': std::exit(0); break;
        case '+': numCircles = std::min(200, numCircles + 1); glutPostRedisplay(); break;
        case '-': numCircles = std::max(1,   numCircles - 1); glutPostRedisplay(); break;

        case ']': thickStep  = std::min(10,  thickStep + 1);  glutPostRedisplay(); break;
        case '[': thickStep  = std::max(0,   thickStep - 1);  glutPostRedisplay(); break;

        case '.': radiusStep = std::min(50,  radiusStep + 1); glutPostRedisplay(); break;
        case ',': radiusStep = std::max(1,   radiusStep - 1); glutPostRedisplay(); break;

        case 'r': case 'R': resetParams(); glutPostRedisplay(); break;
    }
}

static void initGL(){
    glClearColor(0.06f, 0.07f, 0.10f, 1.0f); // dark bg
    glDisable(GL_LINE_SMOOTH);
    glDisable(GL_POINT_SMOOTH);
    glDisable(GL_BLEND);
}

int main(int argc, char** argv){
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(winW, winH);
    glutCreateWindow("Concentric Circles - Midpoint + Thickness + Gradient");

    initGL();
    reshape(winW, winH); // set initial matrices & center

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);

    glutMainLoop();
    return 0;
}
