/**************************************************************************\
 *
 *  This file is part of the SmallChange extension library for Coin.
 *  Copyright (C) 1998-2003 by Systems in Motion.  All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  ("GPL") version 2 as published by the Free Software Foundation.
 *  See the file LICENSE.GPL at the root directory of this source
 *  distribution for additional information about the GNU GPL.
 *
 *  For using SmallChange with software that can not be combined with the
 *  GNU GPL, and for taking advantage of the additional benefits of our
 *  support services, please contact Systems in Motion about acquiring
 *  a Coin Professional Edition License.
 *
 *  See <URL:http://www.coin3d.org> for  more information.
 *
 *  Systems in Motion, Teknobyen, Abels Gate 5, 7030 Trondheim, NORWAY.
 *  <URL:http://www.sim.no>.
 *
\**************************************************************************/

#include "SmHQSphere.h"
#include <Inventor/SbBSPTree.h>
#include <Inventor/lists/SbList.h>
#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoGetPrimitiveCountAction.h>
#include <Inventor/bundles/SoMaterialBundle.h>
#include <Inventor/elements/SoGLTextureEnabledElement.h>
#include <Inventor/elements/SoGLTexture3EnabledElement.h>
#include <Inventor/elements/SoTextureCoordinateElement.h>
#include <Inventor/SoPickedPoint.h>
#include <Inventor/SbSphere.h>
#include <Inventor/SbVec4f.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/system/gl.h>
#include <stddef.h>
#include <string.h>

class HQSphereGenerator {
public:
  
  HQSphereGenerator(void) {
    this->icosahedron = NULL;
    this->init();
  }
  ~HQSphereGenerator(void) {
    delete this->icosahedron;
  }
    
  class triangle {
  public:
    triangle(void) { }
    triangle(const SbVec3f & p0, const SbVec3f & p1, const SbVec3f & p2) {
      pt[0] = p0;
      pt[1] = p1;
      pt[2] = p2;
    } 
  public:
    SbVec3f pt[3];
  };
  
  class object {
  public:
    object(int npoly, const triangle * poly) {
      this->npoly = npoly;
      this->poly = new triangle[npoly];
      if (poly) {
        memcpy(this->poly, poly, npoly*sizeof(triangle));
      }
    }
    ~object() {
      delete[] this->poly;
    }
  public:
    int npoly;    /* # of triangles in object */
    triangle * poly;     /* Triangles */
  };

  void generate(const int level, SbBSPTree & bsp, SbList <int> & idx);
  
  SbVec3f normalize(const SbVec3f & p);
  SbVec3f midpoint(const SbVec3f & a, const SbVec3f & b);

private:
  void convert(object * obj, SbBSPTree & bsp, SbList <int> & idx);
  void init(void);
  object * icosahedron;
};

class SmHQSphereP {
public:
  SbBSPTree bsp;
  SbList <int32_t> idx;
  HQSphereGenerator generator;
  int currlevel;

  void genGeom(const int level);
};

#define PRIVATE(obj) obj->pimpl

SO_NODE_SOURCE(SmHQSphere);

/*!
  Constructor.
*/
SmHQSphere::SmHQSphere()
{
  SO_NODE_CONSTRUCTOR(SmHQSphere);
  SO_NODE_ADD_FIELD(radius, (1.0f));
  SO_NODE_ADD_FIELD(level, (5));

  PRIVATE(this) = new SmHQSphereP;
  PRIVATE(this)->currlevel = -1;
}

/*!
  Destructor.
*/
SmHQSphere::~SmHQSphere()
{
  delete PRIVATE(this);
}

/*!
  Required Coin method.
*/
void
SmHQSphere::initClass()
{
  static int initialized = 0;
  if (!initialized) {
    initialized = 1;
    SO_NODE_INIT_CLASS(SmHQSphere, SoShape, "Shape");
  }
}

void 
SmHQSphere::GLRender(SoGLRenderAction * action)
{
  if (!this->shouldGLRender(action)) return;

  SoState * state = action->getState();

  SoMaterialBundle mb(action);
  mb.sendFirst();

  SbBool doTextures = FALSE;
  SbBool do3DTextures = FALSE;
  if (SoGLTextureEnabledElement::get(state)) doTextures = TRUE;
  else if (SoGLTexture3EnabledElement::get(state)) do3DTextures = TRUE;

  SbBool sendNormals = !mb.isColorOnly() || 
    (SoTextureCoordinateElement::getType(state) == SoTextureCoordinateElement::FUNCTION);
  
  int l = this->level.getValue();
  if (l != PRIVATE(this)->currlevel) {
    PRIVATE(this)->genGeom(l);
  }

  int n = PRIVATE(this)->idx.getLength();
  const int32_t * idx = PRIVATE(this)->idx.getArrayPtr();
  const SbVec3f * pts = PRIVATE(this)->bsp.getPointsArrayPtr();

  float r = this->radius.getValue();

  if (r != 1.0f) {
    glPushMatrix();
    glScalef(r, r, r);
  }

  // FIXME: use vertex arrays to speed up rendering
  glBegin(GL_TRIANGLES);
  for (int i = 0; i < n; i++) {
    SbVec3f v = pts[idx[i]];
    glNormal3f(v[0], v[1], v[2]);
    glVertex3f(v[0], v[1], v[2]);
  } 
  glEnd();

  if (r != 1.0f) {
    glPopMatrix();
  }
}

void 
SmHQSphere::getPrimitiveCount(SoGetPrimitiveCountAction * action)
{
  if (!this->shouldPrimitiveCount(action)) return;

  action->addNumTriangles(1); // FIXME
}

// internal method used to add a sphere intersection to the ray pick
// action, and set the correct pp normal and texture coordinates
static void
try_add_intersection(SoRayPickAction * action, const SbVec3f & pt)
{
  if (action->isBetweenPlanes(pt)) {
    SoPickedPoint * pp = action->addIntersection(pt);
    if (pp) {
      SbVec3f normal = pt;
      normal.normalize();
      pp->setObjectNormal(normal);
      SbVec4f tc((float) (atan2(pt[0], pt[2]) * (1.0 / (2.0*M_PI)) + 0.5),
                 (float) (atan2(pt[1], sqrt(pt[0]*pt[0] + pt[2]*pt[2])) * (1.0/M_PI) + 0.5),
                 0.0f, 1.0f);
      pp->setObjectTextureCoords(tc);
    }
  }
}

void 
SmHQSphere::rayPick(SoRayPickAction * action)
{
  if (!shouldRayPick(action)) return;

  action->setObjectSpace();
  const SbLine & line = action->getLine();
  SbSphere sphere(SbVec3f(0.0f, 0.0f, 0.0f), this->radius.getValue());
  SbVec3f enter, exit;
  if (sphere.intersect(line, enter, exit)) {
    try_add_intersection(action, enter);
    if (exit != enter) try_add_intersection(action, exit);
  }
}

void 
SmHQSphere::generatePrimitives(SoAction * action)
{
  // FIXME:
}

void 
SmHQSphere::computeBBox(SoAction * action, SbBox3f & box, SbVec3f & center)
{
  float r = this->radius.getValue();

  // Allow negative values.
  if (r < 0.0f) r = -r;

  box.setBounds(SbVec3f(-r, -r, -r), SbVec3f(r, r, r));
  center.setValue(0.0f, 0.0f, 0.0f);
}

#undef PRIVATE

void 
SmHQSphereP::genGeom(const int level)
{
  this->generator.generate(SbClamp(level, 1, 12), this->bsp, this->idx);  
//   int n = this->bsp.numPoints();
//   const SbVec3f * pts = this->bsp.getPointsArrayPtr();
}

void
HQSphereGenerator::init(void)
{  
  /* Twelve vertices of icosahedron on unit sphere */
  const float tau = 0.8506508084f; /* t=(1+sqrt(5))/2, tau=t/sqrt(1+t^2)  */
  const float one =0.5257311121f; /* one=1/sqrt(1+t^2) , unit sphere     */

  const SbVec3f ZA(tau, one, 0.0f);
  const SbVec3f ZB(-tau, one, 0.0f);
  const SbVec3f ZC(-tau, -one, 0.0f);
  const SbVec3f ZD(tau, -one, 0.0f);
  const SbVec3f YA(one, 0.0f ,  tau);
  const SbVec3f YB(one, 0.0f , -tau);
  const SbVec3f YC(-one, 0.0f , -tau);
  const SbVec3f YD(-one, 0.0f ,  tau);
  const SbVec3f XA(0.0f, tau, one);
  const SbVec3f XB(0.0f, -tau, one);
  const SbVec3f XC(0.0f, -tau, -one);
  const SbVec3f XD(0.0f, tau, -one);

  /* Structure for unit icosahedron */
  const triangle triangles[] = {
    triangle(YA, XA, YD),
    triangle(YA, YD, XB),
    triangle(YB, YC, XD),
    triangle(YB, XC, YC),
    triangle(ZA, YA, ZD),
    triangle(ZA, ZD, YB),
    triangle(ZC, YD, ZB),
    triangle(ZC, ZB, YC),
    triangle(XA, ZA, XD),
    triangle(XA, XD, ZB),
    triangle(XB, XC, ZD),
    triangle(XB, ZC, XC),
    triangle(XA, YA, ZA),
    triangle(XD, ZA, YB),
    triangle(YA, XB, ZD),
    triangle(YB, ZD, XC),
    triangle(YD, XA, ZB),
    triangle(YC, ZB, XD),
    triangle(YD, ZC, XB),
    triangle(YC, XC, ZC)
  };
  this->icosahedron = 
    new object(20, triangles);
};

void
HQSphereGenerator::generate(const int maxlevel,
                            SbBSPTree & bsp,
                            SbList <int> & idx)
{
  int level;
  
  bsp.clear();
  idx.truncate(0, TRUE);
  object * old = this->icosahedron;

  /* Subdivide each starting triangle (maxlevel - 1) times */
  for (level = 1; level < maxlevel; level++) {
    /* Allocate a new object */
    /* FIXME: Valgrind reports an 8-byte memory leak here. 20030404 mortene. */
    object * newobj = new object(old->npoly * 4, NULL);
      
    /* Subdivide each triangle in the old approximation and normalize
     *  the new points thus generated to lie on the surface of the unit
     *  sphere.
     * Each input triangle with vertices labelled [0,1,2] as shown
     *  below will be turned into four new triangles:
     *
     *                      Make new points
     *                          a = (0+2)/2
     *                          b = (0+1)/2
     *                          c = (1+2)/2
     *        1
     *       /\             Normalize a, b, c
     *      /  \
     *    b/____\ c         Construct new triangles
     *    /\    /\              [0,b,a]
     *   /  \  /  \             [b,1,c]
     *  /____\/____\            [a,b,c]
     * 0      a     2           [a,c,2]
     */
    for (int i = 0; i < old->npoly; i++) {
      triangle *oldt = &old->poly[i];
      triangle *newt = &newobj->poly[i*4];
      SbVec3f a, b, c;
      
      a = normalize(midpoint(oldt->pt[0], oldt->pt[2]));
      b = normalize(midpoint(oldt->pt[0], oldt->pt[1]));
      c = normalize(midpoint(oldt->pt[1], oldt->pt[2]));
        
      newt->pt[0] = oldt->pt[0];
      newt->pt[1] = b;
      newt->pt[2] = a;
      newt++;
        
      newt->pt[0] = b;
      newt->pt[1] = oldt->pt[1];
      newt->pt[2] = c;
      newt++;
      
      newt->pt[0] = a;
      newt->pt[1] = b;
      newt->pt[2] = c;
      newt++;
      
      newt->pt[0] = a;
      newt->pt[1] = c;
      newt->pt[2] = oldt->pt[2];
    }
    
    if (old != this->icosahedron) delete old;
    /* Continue subdividing new triangles */
    old = newobj;
  }
  this->convert(old, bsp, idx);
  if (old != this->icosahedron) delete old;
}

SbVec3f 
HQSphereGenerator::normalize(const SbVec3f & p)
{
  float mag;
  
  SbVec3f r = p;
  mag = r[0] * r[0] + r[1] * r[1] + r[2] * r[2];
  if (mag != 0.0f) {
    mag = (float) (1.0 / sqrt(mag));
    r[0] *= mag;
    r[1] *= mag;
    r[2] *= mag;
  }
  return r;
}

/* Return the midpoint on the line between two points */
SbVec3f 
HQSphereGenerator::midpoint(const SbVec3f & a, const SbVec3f & b)
{
  SbVec3f r;
  
  r[0] = (a[0] + b[0]) * 0.5f;
  r[1] = (a[1] + b[1]) * 0.5f;
  r[2] = (a[2] + b[2]) * 0.5f;
  
  return r;
}

void 
HQSphereGenerator::convert(object * obj, SbBSPTree & bsp, SbList <int> & idx)
{
  triangle * t = obj->poly;
  int n = obj->npoly;

  for (int i = 0; i < n; i++) {
    for (int j = 0; j < 3; j++) {
      idx.append(bsp.addPoint(t->pt[j]));
    }
    t++;
  }
}
