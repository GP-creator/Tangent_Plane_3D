/*
 * Tangent Plane Visualizer for Calc 3
 * =====================================
 * Compile: g++ tangent_plane.cpp -o tangent_plane -lGL -lGLU -lglut -lm
 * macOS:   g++ tangent_plane.cpp -o tangent_plane -framework OpenGL -framework GLUT -lm
 *
 * Controls:
 *   Mouse drag   - Rotate view
 *   Scroll wheel - Zoom
 *   Arrow keys   - Move tangent point
 *   R            - Reset view
 *   Q / ESC      - Quit
 */

#include <GL/glut.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <functional>
#include <iostream>
#include <sstream>

// ─── Function definitions ──────────────────────────────────────────────────

struct Surface {
    std::string name;
    std::string equation;
    std::function<double(double,double)> f;
    double xRange[2], yRange[2];
};

// Partial derivatives via central differences
double dfdx(const Surface& s, double x, double y) {
    const double h = 1e-5;
    return (s.f(x+h, y) - s.f(x-h, y)) / (2*h);
}
double dfdy(const Surface& s, double x, double y) {
    const double h = 1e-5;
    return (s.f(x, y+h) - s.f(x, y-h)) / (2*h);
}

std::vector<Surface> surfaces = {
    {
        "Paraboloid",
        "f(x,y) = x^2 + y^2",
        [](double x, double y){ return x*x + y*y; },
        {-2, 2}, {-2, 2}
    },
    {
        "Saddle (Hyperbolic Paraboloid)",
        "f(x,y) = x^2 - y^2",
        [](double x, double y){ return x*x - y*y; },
        {-2, 2}, {-2, 2}
    },
    {
        "Sine Wave",
        "f(x,y) = sin(x) * cos(y)",
        [](double x, double y){ return sin(x)*cos(y); },
        {-M_PI, M_PI}, {-M_PI, M_PI}
    },
    {
        "Gaussian Bell",
        "f(x,y) = e^(-(x^2+y^2))",
        [](double x, double y){ return exp(-(x*x+y*y)); },
        {-2.5, 2.5}, {-2.5, 2.5}
    },
    {
        "Monkey Saddle",
        "f(x,y) = x^3 - 3xy^2",
        [](double x, double y){ return x*x*x - 3*x*y*y; },
        {-1.5, 1.5}, {-1.5, 1.5}
    },
    {
        "Ripple",
        "f(x,y) = sin(sqrt(x^2+y^2)) / sqrt(x^2+y^2+0.01)",
        [](double x, double y){
            double r = sqrt(x*x+y*y+0.01);
            return sin(r)/r;
        },
        {-5, 5}, {-5, 5}
    },
    {
        "Tilted Plane",
        "f(x,y) = 0.5x + 0.3y + 1",
        [](double x, double y){ return 0.5*x + 0.3*y + 1.0; },
        {-3, 3}, {-3, 3}
    },
    {
        "Cubic Bowl",
        "f(x,y) = x^3 + y^3 - 3xy",
        [](double x, double y){ return x*x*x + y*y*y - 3*x*y; },
        {-2, 2}, {-2, 2}
    }
};

// ─── State ──────────────────────────────────────────────────────────────────

int   activeSurface = 0;
double tx = 0.0, ty = 0.0;   // tangent point x,y
double rotX = 30.0, rotY = -45.0;
double zoom = 1.0;
int   lastMouseX, lastMouseY;
bool  mouseDown = false;
int   gridN = 40;             // surface mesh resolution
bool  showTangentPlane = true;
bool  showNormal = true;

// ─── Helpers ────────────────────────────────────────────────────────────────

void clampTangentPoint() {
    const Surface& s = surfaces[activeSurface];
    double margin = 0.05;
    if (tx < s.xRange[0]+margin) tx = s.xRange[0]+margin;
    if (tx > s.xRange[1]-margin) tx = s.xRange[1]-margin;
    if (ty < s.yRange[0]+margin) ty = s.yRange[0]+margin;
    if (ty > s.yRange[1]-margin) ty = s.yRange[1]-margin;
}

// Map value to color (cool→warm gradient based on z)
void zColor(double z, double zMin, double zMax, float alpha=0.85f) {
    double t = (zMax > zMin) ? (z - zMin)/(zMax - zMin) : 0.5;
    // Blue → Cyan → Green → Yellow → Red
    float r,g,b;
    if (t < 0.25) {
        float s = t/0.25f;
        r=0; g=s; b=1;
    } else if (t < 0.5) {
        float s=(t-0.25f)/0.25f;
        r=0; g=1; b=1-s;
    } else if (t < 0.75) {
        float s=(t-0.5f)/0.25f;
        r=s; g=1; b=0;
    } else {
        float s=(t-0.75f)/0.25f;
        r=1; g=1-s; b=0;
    }
    glColor4f(r,g,b,alpha);
}

// ─── Drawing ────────────────────────────────────────────────────────────────

void drawSurface() {
    const Surface& s = surfaces[activeSurface];
    double dx = (s.xRange[1]-s.xRange[0])/gridN;
    double dy = (s.yRange[1]-s.yRange[0])/gridN;

    // Pre-compute z range for coloring
    double zMin=1e18, zMax=-1e18;
    for(int i=0;i<=gridN;i++){
        for(int j=0;j<=gridN;j++){
            double x=s.xRange[0]+i*dx, y=s.yRange[0]+j*dy;
            double z=s.f(x,y);
            if(z<zMin) zMin=z;
            if(z>zMax) zMax=z;
        }
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    for(int i=0;i<gridN;i++){
        for(int j=0;j<gridN;j++){
            double x0=s.xRange[0]+i*dx,   y0=s.yRange[0]+j*dy;
            double x1=x0+dx,               y1=y0+dy;
            double z00=s.f(x0,y0), z10=s.f(x1,y0);
            double z01=s.f(x0,y1), z11=s.f(x1,y1);

            glBegin(GL_TRIANGLES);
            // Triangle 1
            zColor(z00,zMin,zMax); glVertex3d(x0,y0,z00);
            zColor(z10,zMin,zMax); glVertex3d(x1,y0,z10);
            zColor(z11,zMin,zMax); glVertex3d(x1,y1,z11);
            // Triangle 2
            zColor(z00,zMin,zMax); glVertex3d(x0,y0,z00);
            zColor(z11,zMin,zMax); glVertex3d(x1,y1,z11);
            zColor(z01,zMin,zMax); glVertex3d(x0,y1,z01);
            glEnd();
        }
    }

    // Wireframe overlay
    glColor4f(0,0,0,0.15f);
    glLineWidth(0.5f);
    for(int i=0;i<gridN;i++){
        for(int j=0;j<gridN;j++){
            double x0=s.xRange[0]+i*dx, y0=s.yRange[0]+j*dy;
            double x1=x0+dx, y1=y0+dy;
            glBegin(GL_LINE_LOOP);
            glVertex3d(x0,y0,s.f(x0,y0));
            glVertex3d(x1,y0,s.f(x1,y0));
            glVertex3d(x1,y1,s.f(x1,y1));
            glVertex3d(x0,y1,s.f(x0,y1));
            glEnd();
        }
    }
}

void drawTangentPlane() {
    const Surface& s = surfaces[activeSurface];
    double x0=tx, y0=ty, z0=s.f(x0,y0);
    double fx=dfdx(s,x0,y0), fy=dfdy(s,x0,y0);

    // Tangent plane: z = z0 + fx*(x-x0) + fy*(y-y0)
    double ext = (s.xRange[1]-s.xRange[0])*0.35;
    int n=12;
    double step = 2*ext/n;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Filled plane
    glColor4f(1.0f, 0.9f, 0.3f, 0.45f);
    for(int i=0;i<n;i++){
        for(int j=0;j<n;j++){
            double xi0=x0-ext+i*step,     yj0=y0-ext+j*step;
            double xi1=xi0+step,           yj1=yj0+step;
            auto tz=[&](double x,double y){ return z0+fx*(x-x0)+fy*(y-y0); };
            glBegin(GL_TRIANGLES);
            glVertex3d(xi0,yj0,tz(xi0,yj0));
            glVertex3d(xi1,yj0,tz(xi1,yj0));
            glVertex3d(xi1,yj1,tz(xi1,yj1));
            glVertex3d(xi0,yj0,tz(xi0,yj0));
            glVertex3d(xi1,yj1,tz(xi1,yj1));
            glVertex3d(xi0,yj1,tz(xi0,yj1));
            glEnd();
        }
    }

    // Grid lines on tangent plane
    glColor4f(0.9f,0.7f,0.0f,0.7f);
    glLineWidth(1.0f);
    for(int i=0;i<=n;i++){
        double xi=x0-ext+i*step;
        glBegin(GL_LINE_STRIP);
        for(int j=0;j<=n;j++){
            double yj=y0-ext+j*step;
            glVertex3d(xi,yj, z0+fx*(xi-x0)+fy*(yj-y0));
        }
        glEnd();
    }
    for(int j=0;j<=n;j++){
        double yj=y0-ext+j*step;
        glBegin(GL_LINE_STRIP);
        for(int i=0;i<=n;i++){
            double xi=x0-ext+i*step;
            glVertex3d(xi,yj, z0+fx*(xi-x0)+fy*(yj-y0));
        }
        glEnd();
    }

    // Tangent point dot
    glColor3f(1,0.2f,0.2f);
    glPointSize(10);
    glBegin(GL_POINTS);
    glVertex3d(x0,y0,z0);
    glEnd();

    // Normal vector
    if(showNormal){
        double len = (s.xRange[1]-s.xRange[0])*0.18;
        // Normal = (-fx, -fy, 1) normalized
        double nx=-fx, ny=-fy, nz=1.0;
        double nm=sqrt(nx*nx+ny*ny+nz*nz);
        nx/=nm; ny/=nm; nz/=nm;

        glLineWidth(2.5f);
        glColor3f(0.1f,0.9f,0.3f);
        glBegin(GL_LINES);
        glVertex3d(x0,y0,z0);
        glVertex3d(x0+nx*len, y0+ny*len, z0+nz*len);
        glEnd();

        // Arrowhead
        glPointSize(8);
        glBegin(GL_POINTS);
        glVertex3d(x0+nx*len, y0+ny*len, z0+nz*len);
        glEnd();
    }

    // Drop line to surface contact
    glLineWidth(1.5f);
    glColor4f(1,0.2f,0.2f,0.6f);
    glBegin(GL_LINES);
    glVertex3d(x0,y0,z0);
    glVertex3d(x0,y0,z0-0.001);  // tiny, just a marker
    glEnd();
}

void drawAxes() {
    const Surface& s = surfaces[activeSurface];
    double xL = s.xRange[1]*1.1, yL = s.yRange[1]*1.1;
    double zL = 2.0;
    glLineWidth(1.5f);
    glBegin(GL_LINES);
    glColor3f(0.9f,0.3f,0.3f); glVertex3d(-xL,0,0); glVertex3d(xL,0,0);
    glColor3f(0.3f,0.9f,0.3f); glVertex3d(0,-yL,0); glVertex3d(0,yL,0);
    glColor3f(0.3f,0.3f,0.9f); glVertex3d(0,0,-zL); glVertex3d(0,0,zL);
    glEnd();
}

// Draw text on screen
void drawText2D(float x, float y, const std::string& text, 
                float r=1, float g=1, float b=1) {
    glColor3f(r,g,b);
    glRasterPos2f(x,y);
    for(char c : text)
        glutBitmapCharacter(GLUT_BITMAP_9_BY_15, c);
}

void drawHUD() {
    const Surface& s = surfaces[activeSurface];
    double x0=tx, y0=ty, z0=s.f(x0,y0);
    double fx=dfdx(s,x0,y0), fy=dfdy(s,x0,y0);

    // Switch to 2D overlay
    int w=glutGet(GLUT_WINDOW_WIDTH), h=glutGet(GLUT_WINDOW_HEIGHT);
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    glOrtho(0,w,0,h,-1,1);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    glDisable(GL_DEPTH_TEST);

    char buf[256];

    // Title
    snprintf(buf,sizeof(buf),"Surface: %s",s.name.c_str());
    drawText2D(10,h-20, buf, 1.0f,0.9f,0.3f);
    snprintf(buf,sizeof(buf),"Equation: %s", s.equation.c_str());
    drawText2D(10,h-40, buf, 0.8f,0.8f,0.8f);

    // Tangent info
    snprintf(buf,sizeof(buf),"Point: (%.3f, %.3f, %.3f)",x0,y0,z0);
    drawText2D(10,h-70, buf, 1,0.5f,0.5f);
    snprintf(buf,sizeof(buf),"fx = %.4f   fy = %.4f",fx,fy);
    drawText2D(10,h-90, buf, 0.9f,0.9f,0.3f);
    snprintf(buf,sizeof(buf),"Tangent plane: z = %.3f + %.4f(x-%.3f) + %.4f(y-%.3f)",
             z0,fx,x0,fy,y0);
    drawText2D(10,h-110, buf, 0.6f,1.0f,0.6f);

    // Controls
    drawText2D(10,30, "Arrows: move point  |  Drag: rotate  |  Scroll: zoom  |  R: reset  |  T: toggle plane  |  Q: quit",
               0.5f,0.5f,0.5f);

    glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW); glPopMatrix();
}

// ─── GLUT Callbacks ─────────────────────────────────────────────────────────

void display() {
    glClearColor(0.07f,0.08f,0.12f,1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    // 3D scene
    int w=glutGet(GLUT_WINDOW_WIDTH), h=glutGet(GLUT_WINDOW_HEIGHT);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluPerspective(45.0, (double)w/h, 0.01, 100.0);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();

    const Surface& s = surfaces[activeSurface];
    double cx=(s.xRange[0]+s.xRange[1])/2, cy=(s.yRange[0]+s.yRange[1])/2;
    double dist = (s.xRange[1]-s.xRange[0]) * 1.8 / zoom;
    gluLookAt(cx,cy-dist,dist*0.9, cx,cy,0, 0,0,1);

    glTranslated(cx,cy,0);
    glRotated(rotX,1,0,0);
    glRotated(rotY,0,0,1);
    glTranslated(-cx,-cy,0);

    drawAxes();
    drawSurface();
    if(showTangentPlane) drawTangentPlane();
    drawHUD();

    glutSwapBuffers();
}

void reshape(int w, int h) {
    glViewport(0,0,w,h);
}

void keyboard(unsigned char key, int, int) {
    if(key=='q'||key=='Q'||key==27) exit(0);
    if(key=='r'||key=='R'){ rotX=30; rotY=-45; zoom=1.0; }
    if(key=='t'||key=='T') showTangentPlane=!showTangentPlane;
    if(key=='n'||key=='N') showNormal=!showNormal;
    glutPostRedisplay();
}

void special(int key, int, int) {
    const Surface& s = surfaces[activeSurface];
    double step = (s.xRange[1]-s.xRange[0])/30.0;
    if(key==GLUT_KEY_RIGHT) tx+=step;
    if(key==GLUT_KEY_LEFT)  tx-=step;
    if(key==GLUT_KEY_UP)    ty+=step;
    if(key==GLUT_KEY_DOWN)  ty-=step;
    clampTangentPoint();
    glutPostRedisplay();
}

void mouse(int btn, int state, int x, int y) {
    if(btn==GLUT_LEFT_BUTTON){ mouseDown=(state==GLUT_DOWN); lastMouseX=x; lastMouseY=y; }
    // Scroll
    if(btn==3){ zoom*=1.1; glutPostRedisplay(); }
    if(btn==4){ zoom/=1.1; if(zoom<0.1)zoom=0.1; glutPostRedisplay(); }
}

void motion(int x, int y) {
    if(mouseDown){
        rotY += (x-lastMouseX)*0.5;
        rotX += (y-lastMouseY)*0.5;
        lastMouseX=x; lastMouseY=y;
        glutPostRedisplay();
    }
}

// ─── Main ───────────────────────────────────────────────────────────────────

int main(int argc, char** argv) {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════╗\n";
    std::cout << "║     Calc 3 — Tangent Plane Visualizer        ║\n";
    std::cout << "╚══════════════════════════════════════════════╝\n\n";
    std::cout << "Available surfaces:\n\n";
    for(int i=0;i<(int)surfaces.size();i++){
        std::cout << "  [" << i+1 << "]  " << surfaces[i].name
                  << "  =>  " << surfaces[i].equation << "\n";
    }
    std::cout << "\nSelect a surface (1-" << surfaces.size() << "): ";
    int choice;
    std::cin >> choice;
    if(choice<1||choice>(int)surfaces.size()){
        std::cout << "Invalid choice. Using Paraboloid.\n";
        choice=1;
    }
    activeSurface=choice-1;

    // Set initial tangent point to center
    const Surface& s = surfaces[activeSurface];
    tx=(s.xRange[0]+s.xRange[1])/2 * 0.5;
    ty=(s.yRange[0]+s.yRange[1])/2 * 0.5;

    std::cout << "\nControls:\n";
    std::cout << "  Arrow keys  — move tangent point\n";
    std::cout << "  Mouse drag  — rotate view\n";
    std::cout << "  Scroll      — zoom\n";
    std::cout << "  T           — toggle tangent plane\n";
    std::cout << "  N           — toggle normal vector\n";
    std::cout << "  R           — reset camera\n";
    std::cout << "  Q / Esc     — quit\n\n";

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB|GLUT_DEPTH|GLUT_MULTISAMPLE);
    glutInitWindowSize(1000,700);
    glutCreateWindow(("Tangent Plane — " + surfaces[activeSurface].name).c_str());

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);

    glutMainLoop();
    return 0;
}