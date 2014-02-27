/*
  CSCI 480
  Assignment 2
 */

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>

#include <math.h>
#include "pic.h"

int g_vMousePos[2] = {0, 0};
int g_iLeftMouseButton = 0;    /* 1 if pressed, 0 if not */
int g_iMiddleMouseButton = 0;
int g_iRightMouseButton = 0;

typedef enum { ROTATE, TRANSLATE, SCALE } CONTROLSTATE;

CONTROLSTATE g_ControlState = ROTATE;

/* state of the world */
float g_vLandRotate[3] = {0.0, 0.0, 0.0};
float g_vLandTranslate[3] = {0.0, 0.0, 0.0};
float g_vLandScale[3] = {1.0, 1.0, 1.0};

/* represents one control point along the spline */
struct point {
   double x;
   double y;
   double z;
};

/* spline struct which contains how many control points, and an array of control points */
struct spline {
   int numControlPoints;
   struct point *points;
};

/* the spline array */
struct spline *g_Splines;

/* total number of splines */
int g_iNumOfSplines;

int totalPointNum = 0;
float **vertices;
float **tangent;

int pos = 1;
GLfloat currPos[3];
GLfloat currUpDir[3]={0,0,1};
GLfloat currFwDir[3];

static GLuint _skybox[6];
Pic * g_pSkybox[6]; //Skybox texture

int stopFlag = 0;
int isPause = 0;

int loadSplines(char *argv) {
  char *cName = (char *)malloc(128 * sizeof(char));
  FILE *fileList;
  FILE *fileSpline;
  int iType, i = 0, j, iLength;


  /* load the track file */
  fileList = fopen(argv, "r");
  if (fileList == NULL) {
    printf ("can't open file\n");
    exit(1);
  }

  /* stores the number of splines in a global variable */
  fscanf(fileList, "%d", &g_iNumOfSplines);

  g_Splines = (struct spline *)malloc(g_iNumOfSplines * sizeof(struct spline));

  /* reads through the spline files */
  for (j = 0; j < g_iNumOfSplines; j++) {
    i = 0;
    fscanf(fileList, "%s", cName);
    fileSpline = fopen(cName, "r");

    if (fileSpline == NULL) {
      printf ("can't open file\n");
      exit(1);
    }

    /* gets length for spline file */
    fscanf(fileSpline, "%d %d", &iLength, &iType);

    /* allocate memory for all the points */
    g_Splines[j].points = (struct point *)malloc(iLength * sizeof(struct point));
    g_Splines[j].numControlPoints = iLength;

    /* saves the data to the struct */
    while (fscanf(fileSpline, "%lf %lf %lf",
	   &g_Splines[j].points[i].x,
	   &g_Splines[j].points[i].y,
	   &g_Splines[j].points[i].z) != EOF) {
      i++;
    }
  }

  free(cName);

  return 0;
}

void crossMul(float* v1, float* v2, float* r)
{
    r[0] = v1[1]*v2[2]-v1[2]*v2[1];
    r[1] = v1[2]*v2[0]-v1[0]*v2[2];
    r[2] = v1[0]*v2[1]-v1[1]*v2[0];
}
void normalize(float* v)
{
    float sum = 0;
    for(int i=0;i<3;i++)
    {
        sum += v[i]*v[i];
    }
    sum = sqrt(sum);
    for(int i=0;i<3;i++)
    {
        v[i] /= sum;
    }
}

float* getPointFromSpline(float u, point p0, point p1, point p2, point p3, float* tangent)
{
    // s=0.5
    float *result = (float*)malloc(3*sizeof(float));

    result[0] = 0.5*((2*p1.x)+(-p0.x+p2.x)*u+(2*p0.x-5*p1.x+4*p2.x-p3.x)*u*u+(-p0.x+3*p1.x-3*p2.x+p3.x)*u*u*u);
    result[1] = 0.5*((2*p1.y)+(-p0.y+p2.y)*u+(2*p0.y-5*p1.y+4*p2.y-p3.y)*u*u+(-p0.y+3*p1.y-3*p2.y+p3.y)*u*u*u);
    result[2] = 0.5*((2*p1.z)+(-p0.z+p2.z)*u+(2*p0.z-5*p1.z+4*p2.z-p3.z)*u*u+(-p0.z+3*p1.z-3*p2.z+p3.z)*u*u*u);

    tangent[0] = 0.5*((-p0.x+p2.x)+(2*p0.x-5*p1.x+4*p2.x-p3.x)*2*u+(-p0.x+3*p1.x-3*p2.x+p3.x)*3*u*u);
    tangent[1] = 0.5*((-p0.y+p2.y)+(2*p0.y-5*p1.y+4*p2.y-p3.y)*2*u+(-p0.y+3*p1.y-3*p2.y+p3.y)*3*u*u);
    tangent[2] = 0.5*((-p0.z+p2.z)+(2*p0.z-5*p1.z+4*p2.z-p3.z)*2*u+(-p0.z+3*p1.z-3*p2.z+p3.z)*3*u*u);

    normalize(tangent);
    return result;
}

void Init()
{
    glClearColor(0,0,0,0);
    glEnable(GL_DEPTH_TEST);

    for(int i=0;i<6;i++)
    {
        glGenTextures(1,&_skybox[i]);
        glBindTexture(GL_TEXTURE_2D,_skybox[i]);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,g_pSkybox[i]->nx,g_pSkybox[i]->ny,0,GL_RGB,GL_UNSIGNED_BYTE,g_pSkybox[i]->pix);
    }

    totalPointNum = 0;
    for(int i=0; i<g_iNumOfSplines; i++)
    {
        for(int j=3; j<g_Splines->numControlPoints; j++)
        {
            for(float u=0; u<=1; u+=0.01)
            {
                totalPointNum++;
            }
        }
    }
    vertices = (float**)malloc(sizeof(float*)*totalPointNum);
    tangent = (float**)malloc(sizeof(float*)*totalPointNum);
    int count = 0;
    for(int i=0; i<g_iNumOfSplines; i++)
    {
        for(int j=3; j<g_Splines->numControlPoints; j++)
        {
            for(float u=0; u<=1; u+=0.01)
            {
                tangent[count] = (float*)malloc(sizeof(float)*3);
                vertices[count] = getPointFromSpline(u,g_Splines->points[j-3],g_Splines->points[j-2],g_Splines->points[j-1],g_Splines->points[j],tangent[count]);
                count++;
            }
        }
    }

}

void drawSpline()
{
    int count = 0;
    glLineWidth(5);
    glColor3f(0,0,0);

    float left[3], right[3];
    glBegin(GL_LINE_STRIP);
    for(int i=0; i<totalPointNum; i++)
    {
        float *p = vertices[i];
        float *t = tangent[i];
        count ++;
        if(count==pos)
        {
            currPos[0] = p[0];
            currPos[1] = p[1];
            currPos[2] = p[2];

            currFwDir[0] = currPos[0]+t[0];
            currFwDir[1] = currPos[1]+t[1];
            currFwDir[2] = currPos[2]+t[2];
        }
        float n[3];
        crossMul(t,currUpDir,n);
        normalize(n);
        float alpha = 1;
        right[0] = p[0] + alpha*(n[0]-currUpDir[0]);
        right[1] = p[1] + alpha*(n[1]-currUpDir[1]);
        right[2] = p[2] + alpha*(n[2]-currUpDir[2]);

        glVertex3fv(right);
    }
    glEnd();

    glBegin(GL_LINES);
    for(int i=0; i<totalPointNum; i+=20)
    {
        float *p = vertices[i];
        float *t = tangent[i];

        float n[3];
        crossMul(t,currUpDir,n);
        normalize(n);
        float alpha = 1;
        left[0] = p[0] + alpha*(-n[0]-currUpDir[0]);
        left[1] = p[1] + alpha*(-n[1]-currUpDir[1]);
        left[2] = p[2] + alpha*(-n[2]-currUpDir[2]);

        right[0] = p[0] + alpha*(n[0]-currUpDir[0]);
        right[1] = p[1] + alpha*(n[1]-currUpDir[1]);
        right[2] = p[2] + alpha*(n[2]-currUpDir[2]);

        glVertex3fv(left);
        glVertex3fv(right);
    }
    glEnd();

    glBegin(GL_LINE_STRIP);
    for(int i=0; i<totalPointNum; i++)
    {
        float *p = vertices[i];
        float *t = tangent[i];

        float n[3];
        crossMul(t,currUpDir,n);
        normalize(n);
        float alpha = 1;
        left[0] = p[0] + alpha*(-n[0]-currUpDir[0]);
        left[1] = p[1] + alpha*(-n[1]-currUpDir[1]);
        left[2] = p[2] + alpha*(-n[2]-currUpDir[2]);

        glVertex3fv(left);
    }
    glEnd();
    if(pos>=totalPointNum)
    {
        stopFlag = 1;
        printf("Stopped!\n");
    }
}

void drawSkybox()
{
    glPushMatrix();

    int len = 100;
    glEnable(GL_TEXTURE_2D);
    glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);

    glBindTexture(GL_TEXTURE_2D,_skybox[0]);
    glBegin(GL_QUADS);
    glTexCoord2f(1,0);
    glVertex3f(-len,-len,-len);
    glTexCoord2f(0,0);
    glVertex3f(-len,len,-len);
    glTexCoord2f(0,1);
    glVertex3f(-len,len,len);
    glTexCoord2f(1,1);
    glVertex3f(-len,-len,len);
    glEnd();

    glBindTexture(GL_TEXTURE_2D,_skybox[1]);
    glBegin(GL_QUADS);
    glTexCoord2f(1,0);
    glVertex3f(-len,len,-len);
    glTexCoord2f(0,0);
    glVertex3f(len,len,-len);
    glTexCoord2f(0,1);
    glVertex3f(len,len,len);
    glTexCoord2f(1,1);
    glVertex3f(-len,len,len);
    glEnd();

    glBindTexture(GL_TEXTURE_2D,_skybox[2]);
    glBegin(GL_QUADS);
    glTexCoord2f(1,0);
    glVertex3f(len,len,-len);
    glTexCoord2f(0,0);
    glVertex3f(len,-len,-len);
    glTexCoord2f(0,1);
    glVertex3f(len,-len,len);
    glTexCoord2f(1,1);
    glVertex3f(len,len,len);
    glEnd();

    glBindTexture(GL_TEXTURE_2D,_skybox[3]);
    glBegin(GL_QUADS);
    glTexCoord2f(1,0);
    glVertex3f(len,-len,-len);
    glTexCoord2f(0,0);
    glVertex3f(-len,-len,-len);
    glTexCoord2f(0,1);
    glVertex3f(-len,-len,len);
    glTexCoord2f(1,1);
    glVertex3f(len,-len,len);
    glEnd();

    glBindTexture(GL_TEXTURE_2D,_skybox[4]);
    glBegin(GL_QUADS);
    glTexCoord2f(1,0);
    glVertex3f(-len,-len,-len);
    glTexCoord2f(0,0);
    glVertex3f(len,-len,-len);
    glTexCoord2f(1,1);
    glVertex3f(len,len,-len);
    glTexCoord2f(1,1);
    glVertex3f(-len,len,-len);
    glEnd();

    glBindTexture(GL_TEXTURE_2D,_skybox[5]);
    glBegin(GL_QUADS);
    glTexCoord2f(0,0);
    glVertex3f(-len,len,len);
    glTexCoord2f(1,0);
    glVertex3f(len,len,len);
    glTexCoord2f(1,1);
    glVertex3f(len,-len,len);
    glTexCoord2f(0,1);
    glVertex3f(-len,-len,len);
    glEnd();

    glDisable(GL_TEXTURE_2D);

    glPopMatrix();
}

void drawADot()
{
    glPushMatrix();

    glPointSize(20);
    glColor3f(1,0,0);
    glBegin(GL_POINTS);
        glVertex3fv(currPos);
    glEnd();
    glColor3f(1,1,1);

    glPopMatrix();
}

void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    gluLookAt(currPos[0],currPos[1],currPos[2],currFwDir[0],currFwDir[1],currFwDir[2],currUpDir[0],currUpDir[1],currUpDir[2]);

    //gluLookAt(10,10,30,10,10,0,0,1,0);
    glTranslatef(g_vLandTranslate[0],g_vLandTranslate[1],g_vLandTranslate[2]);
    glRotatef(g_vLandRotate[0],1,0,0);
    glRotatef(g_vLandRotate[1],0,1,0);
    glRotatef(g_vLandRotate[2],0,0,1);

    drawSkybox();

    drawSpline();

    //drawADot();

    glutSwapBuffers();
}

static void resize(int width, int height)
{
    const float ar = (float) width / (float) height;

    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60,ar,1,1000);

    glMatrixMode(GL_MODELVIEW);
}

void mousedrag(int x, int y)
{
  int vMouseDelta[2] = {x-g_vMousePos[0], y-g_vMousePos[1]};

  switch (g_ControlState)
  {
    case TRANSLATE:
      if (g_iLeftMouseButton)
      {
        g_vLandTranslate[0] += vMouseDelta[0]*0.01;
        g_vLandTranslate[1] -= vMouseDelta[1]*0.01;
      }
      if (g_iMiddleMouseButton)
      {
        g_vLandTranslate[2] += vMouseDelta[1]*0.01;
      }
      break;
    case ROTATE:
      if (g_iLeftMouseButton)
      {
        g_vLandRotate[0] += vMouseDelta[1];
        g_vLandRotate[1] += vMouseDelta[0];
      }
      if (g_iMiddleMouseButton)
      {
        g_vLandRotate[2] += vMouseDelta[1];
      }
      break;
    case SCALE:
      if (g_iLeftMouseButton)
      {
        g_vLandScale[0] *= 1.0+vMouseDelta[0]*0.01;
        g_vLandScale[1] *= 1.0-vMouseDelta[1]*0.01;
      }
      if (g_iMiddleMouseButton)
      {
        g_vLandScale[2] *= 1.0-vMouseDelta[1]*0.01;
      }
      break;
  }
  g_vMousePos[0] = x;
  g_vMousePos[1] = y;
}

void mouseidle(int x, int y)
{
  g_vMousePos[0] = x;
  g_vMousePos[1] = y;
}

void mousebutton(int button, int state, int x, int y)
{

  switch (button)
  {
    case GLUT_LEFT_BUTTON:
      g_iLeftMouseButton = (state==GLUT_DOWN);
      break;
    case GLUT_MIDDLE_BUTTON:
      g_iMiddleMouseButton = (state==GLUT_DOWN);
      break;
    case GLUT_RIGHT_BUTTON:
      g_iRightMouseButton = (state==GLUT_DOWN);
      if(state==GLUT_UP)
        isPause = (isPause+1)%2;
      break;
  }

  switch(glutGetModifiers())
  {
    case GLUT_ACTIVE_CTRL:
      g_ControlState = TRANSLATE;
      break;
    case GLUT_ACTIVE_SHIFT:
      g_ControlState = SCALE;
      break;
    default:
      g_ControlState = ROTATE;
      break;
  }

  g_vMousePos[0] = x;
  g_vMousePos[1] = y;
}

void update(int value)
{
    if(stopFlag==0 && isPause==0)
    {
        pos++;
        if(pos>=totalPointNum)
            pos = 0;
        glutPostRedisplay();
    }
    int speed = 5 + 2*vertices[pos][2];
    glutTimerFunc(speed,update,0);
}

int main (int argc, char ** argv)

{
    if (argc<2)
    {
        printf ("usage: %s <trackfile>\n", argv[0]);
        exit(0);
    }

    g_pSkybox[0] = jpeg_read("0.jpg", NULL);
    g_pSkybox[1] = jpeg_read("1.jpg", NULL);
    g_pSkybox[2] = jpeg_read("2.jpg", NULL);
    g_pSkybox[3] = jpeg_read("3.jpg", NULL);
    g_pSkybox[4] = jpeg_read("4.jpg", NULL);
    g_pSkybox[5] = jpeg_read("5.jpg", NULL);
    for(int i=0;i<6;i++)
    {
        if (!g_pSkybox[i])
        {
            printf ("error reading %d.\n",i);
            exit(1);
        }
    }

    loadSplines(argv[1]);

    glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowPosition(100,100);
    glutInitWindowSize(500,500);
    glutCreateWindow("Hello World!");

    Init();
    glutDisplayFunc(display);
    glutReshapeFunc(resize);
    glutTimerFunc(10,update,0);

    /* callback for mouse drags */
    glutMotionFunc(mousedrag);
    /* callback for idle mouse movement */
    glutPassiveMotionFunc(mouseidle);
    /* callback for mouse button changes */
    glutMouseFunc(mousebutton);

    glutMainLoop();

    return 0;
}
