#include <Inventor/SbBasic.h>

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

/*!
  \class SmOceanKit SmOceanKit.h
  \brief The SmOceanKit class... 
  \ingroup nodekits

  Based on the water rendering algorithm and example code in GPU Gems.
  
  FIXME: doc
*/


#include "SmOceanKit.h"
#if defined(__COIN__) && (COIN_MAJOR_VERSION >= 3)

#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/actions/SoHandleEventAction.h>
#include <Inventor/sensors/SoFieldSensor.h>
#include <Inventor/fields/SoSFNode.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSubNode.h>
#include <Inventor/nodes/SoShape.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoCallback.h>
#include <Inventor/nodes/SoInfo.h>
#include <Inventor/SoPickedPoint.h>
#include <Inventor/SoFullPath.h>
#include <Inventor/SbPlane.h>
#include <Inventor/SoPrimitiveVertex.h>
#include <Inventor/details/SoFaceDetail.h>
#include <Inventor/details/SoPointDetail.h>
#include <Inventor/bundles/SoMaterialBundle.h>
#include <Inventor/elements/SoViewVolumeElement.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/elements/SoCullElement.h>
#include <Inventor/elements/SoGLCacheContextElement.h>

#include <Inventor/sensors/SoTimerSensor.h>
#include <SmallChange/nodes/UTMPosition.h>
#include <SmallChange/misc/SbHash.h>
#include <Inventor/system/gl.h>
#include <Inventor/C/glue/gl.h>
#include <Inventor/C/base/memalloc.h>
#include <Inventor/misc/SoContextHandler.h>

#include <Inventor/nodes/SoShaderProgram.h>
#include <Inventor/nodes/SoVertexShader.h>
#include <Inventor/nodes/SoFragmentShader.h>
#include <Inventor/nodes/SoShaderParameter.h>

#include <math.h>
#include <stdlib.h>

static int vbo_vertex_count_min_limit = -1;
static int vbo_vertex_count_max_limit = -1;

class SmVBO {
 public:
  SmVBO(const GLenum target = GL_ARRAY_BUFFER,
        const GLenum usage = GL_STATIC_DRAW);
  ~SmVBO();

  static void init(void);
  
  void setBufferData(const GLvoid * data, intptr_t size, uint32_t dataid = 0);  
  void * allocBufferData(intptr_t size, uint32_t dataid = 0);  
  uint32_t getBufferDataId(void) const;
  void getBufferData(const GLvoid *& data, intptr_t & size);
  void bindBuffer(uint32_t contextid);
  
  static void setVertexCountLimits(const int minlimit, const int maxlimit);
  static int getVertexCountMinLimit(void);
  static int getVertexCountMaxLimit(void);

  static void testGLPerformance(const uint32_t contextid);
  static SbBool shouldCreateVBO(const uint32_t contextid, const int numdata);

 private:
  static void context_created(const cc_glglue * glue, void * closure);
  static SbBool isVBOFast(const uint32_t contextid);
  static void context_destruction_cb(uint32_t context, void * userdata);
  static void vbo_schedule(const uint32_t & key,
                           const GLuint & value,
                           void * closure);
  static void vbo_delete(void * closure, uint32_t contextid);
  
  GLenum target;
  GLenum usage;
  const GLvoid * data;
  intptr_t datasize;
  uint32_t dataid;
  SbBool didalloc;

  SbHash <GLuint, uint32_t> vbohash;
};

class SmOceanKitP {
public:
  SmOceanKitP(void) 
  { }
};

static const float kGravConst = 30.f;

class ocean_quadnode;

// helper class for actually rendering the ocean
class OceanShape : public SoShape {
  typedef SoShape inherited;

  SO_NODE_HEADER(OceanShape);

public:
  static void initClass(void);
  OceanShape(void);

  SoSFNode shader;
  SoSFVec2f size;
  SoSFFloat chop;
  SoSFFloat angleDeviation;
  SoSFVec2f windDirection;
  SoSFFloat minWaveLength;
  SoSFFloat maxWaveLength;
  SoSFFloat amplitudeRatio;
  
  SoSFFloat specAttenuation;
  SoSFFloat specEnd;
  SoSFFloat specTrans;
  
  SoSFFloat envHeight;
  SoSFFloat envRadius;
  SoSFFloat waterLevel;
  SoSFFloat transitionSpeed;
  SoSFFloat sharpness;

  void tick(void);

  virtual void GLRender(SoGLRenderAction * action);
  virtual void getPrimitiveCount(SoGetPrimitiveCountAction * action);
  virtual void rayPick(SoRayPickAction * action);
  virtual void notify(SoNotList * list);

protected:
  virtual ~OceanShape();
  virtual void generatePrimitives(SoAction * action);
  virtual void computeBBox(SoAction * action, SbBox3f & box, SbVec3f & center);
  
private:
  void createCosLUT();
  void createBiasNoiseBuffer();
  void updateShader(void);
  void initShader(void);
  void renderGrid(ocean_quadnode * node, const int bitmask);
  void wavefunc(const SbVec3f & in, SbVec3f &v, SbVec3f &n);
  void updateQuadtree(SoState * state);
  ocean_quadnode * root;
  SbList <ocean_quadnode*> nodelist;
  SoTimerSensor * timersensor;

  void initTexState(void);
  void updateTexWave(const int i, const float dt);
  void initTexWave(const int i);

  void updateParameters();
  void updateGeoWave(const int i, const float dt);
  void initGeoWave(const int i);
  void updateWaves(const double dt);
  void copyGeoState();
  void initWaves();
  void initLevels();
  static void timerCB(void * closure, SoSensor * s);
  SbTime currtime;

  SoVertexShader * vertexshader;
  SoFragmentShader * fragmentshader;
  SoShaderParameter4f * param_geowaveamp;
  SoShaderParameter4f * param_geowavephase;
  SoShaderParameter4f * param_geowavefreq;
  SoShaderParameter4f * param_geowaveQ;

  SoShaderParameter2f * param_geowave1dir;
  SoShaderParameter2f * param_geowave2dir;
  SoShaderParameter2f * param_geowave3dir;
  SoShaderParameter2f * param_geowave4dir;

  //TODO: add some precalculated variables
  //SoShaderParameter4f * param_geowave_freqamp;
  //SoShaderParameter4f * param_geowave_freqampx;
  //SoShaderParameter4f * param_geowave_freqampy;

public:
  enum {
    NUM_GEO_WAVES = 4,
    NUM_TEX_WAVES = 16,
    BUMPSIZE = 256,
    GRIDSIZE = 17
  };

private:
  SbVec3f * grid;
  SmVBO * gridvbo;
  SmVBO * idxvbo[16];
  unsigned short * grididx[16];
  int idxlen[16];
  int minlevel;
  int maxlevel;

  typedef struct {
    float chop;
    float angleDeviation;
    SbVec2f windDir;
    float minLength;
    float maxLength;
    float ampRatio;
    
    float specAtten;
    float specEnd;
    float specTrans;
    
    float envHeight;
    float envRadius;
    float waterLevel;
    
    float transDel;
    int transIdx;
    float Q;
  } geostate;

  typedef struct {
    float noise;
    float chop;
    float angleDeviation;
    SbVec2f windDir;
    float maxLength;
    float minLength;
    float ampRatio;
    float rippleScale;
    float speedDeviation;

    int	transIdx;
    float transDel;
  } texstate;
  
  geostate geostate_cache;
  texstate texstate_cache;

  SbBool invalidstate;

  typedef struct {
    float phase;
    float amp;
    float fullamp;
    float len;
    float freq;
    SbVec2f dir;
    float fade;
  } geowave;

  geowave geowaves[4];

  typedef struct {
    float phase;
    float amp;
    float fullamp;
    float len;
    float speed;
    float freq;
    SbVec2f dir;
    SbVec2f rotScale;
    float fade;
  } texwave;
  
  texwave texwaves[16];
  unsigned char * coslut;
  unsigned char * biasnoisebuf;

};

class ocean_quadnode {
public:
  enum Neighbor {
    BOTTOM = 0,
    RIGHT  = 1,
    TOP    = 2,
    LEFT =   3
  };

  enum Child {
    BOTTOM_LEFT  = 0,
    BOTTOM_RIGHT = 1,
    TOP_RIGHT    = 2,
    TOP_LEFT     = 3
  };

  ocean_quadnode(ocean_quadnode * parent,
                 const SbVec3f & c0,
                 const SbVec3f & c1,
                 const SbVec3f & c2,
                 const SbVec3f & c3);

  ~ocean_quadnode(void);
  const SbVec3f & getCorner(const int i) const;
  
  void split(const int findneighbors = 0);
  void distanceSplit(const SbVec3f & pos, 
                     const int level,
                     const int minlevel,
                     const int maxlevel);
  int isSplit(void) const;
  int hasChildren(void) const;

  ocean_quadnode * getChild(const int i);
  ocean_quadnode * getNeighbor(const int i);
  void setNeighbor(const int i, ocean_quadnode * node);
  ocean_quadnode * getParent(void);
  int getChildIndex(const ocean_quadnode * n) const;
  void clearNeighbors(void);

  ocean_quadnode * getNeighborChild(const int neighbor, const int child);
  ocean_quadnode * searchNeighbor(const int i);

  const SbVec3f * getCorners() {
    return this->corner;
  }
  int getNodeBitmask(void);

private:
  void rotate(void);
  friend class quadsphere;

  ocean_quadnode * parent;
  SbVec3f corner[4];
  ocean_quadnode * child[4];
  ocean_quadnode * neighbor[4];

  unsigned char neighbor_rotate_bits;
  unsigned char flags;

  GLuint gridvbo;

private:
  friend class OceanShape;
  void findNeighbors(void);
  void updateChildRotate(void);
  void setNeighborRotate(const int i, const int rot);
  int getNeighborRotate(const int i) const;
  void unsplit(void);
  void deleteHiddenChildren(const cc_glglue * glue);

public:
  float debugcolor[3];
  static cc_memalloc * getAllocator();
  void * operator new(size_t size, cc_memalloc * memhandler);
  void operator delete(void * ptr);
};

static cc_memalloc * node_memalloc = NULL;
static int numnodes = 0;

#define PRIVATE(obj) (obj)->pimpl

SO_KIT_SOURCE(SmOceanKit);

/*!
  Constructor. 
*/
SmOceanKit::SmOceanKit(void)
{
  PRIVATE(this) = new SmOceanKitP;

  SO_KIT_CONSTRUCTOR(SmOceanKit);
  
  SO_KIT_ADD_FIELD(size, (10000.0f, 10000.0f));
  SO_KIT_ADD_FIELD(chop, (2.5f));
  SO_KIT_ADD_FIELD(angleDeviation, (15.0f));
  SO_KIT_ADD_FIELD(windDirection, (0.0f, 1.0f));
  
  SO_KIT_ADD_FIELD(minWaveLength, (15.f));
  SO_KIT_ADD_FIELD(maxWaveLength, (25.f));
  SO_KIT_ADD_FIELD(amplitudeRatio, (0.1f));
  
  SO_KIT_ADD_FIELD(envHeight, (-50.f));
  SO_KIT_ADD_FIELD(envRadius, (100.f));
  SO_KIT_ADD_FIELD(waterLevel, (-2.0f));
    
  SO_KIT_ADD_FIELD(specAttenuation, (1.0f));
  SO_KIT_ADD_FIELD(specEnd, (200.0f));
  SO_KIT_ADD_FIELD(specTrans, (100.0f));
  SO_KIT_ADD_FIELD(transitionSpeed, (1.0f / 6.0f));
  SO_KIT_ADD_FIELD(sharpness, (0.5f));

  SO_KIT_ADD_CATALOG_ENTRY(topSeparator, SoSeparator, FALSE, this, "", FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(utmposition, UTMPosition, FALSE, topSeparator, material, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(material, SoMaterial, FALSE, topSeparator, shapeHints, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(shapeHints, SoShapeHints, FALSE, topSeparator, shader, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(shader, SoShaderProgram, TRUE, topSeparator, oceanShape, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(oceanShape, OceanShape, FALSE, topSeparator, "", FALSE);

  SO_KIT_INIT_INSTANCE();

  OceanShape * shape = (OceanShape*) this->getAnyPart("oceanShape", TRUE);
  shape->size.connectFrom(&this->size);
  shape->chop.connectFrom(&this->chop);
  shape->angleDeviation.connectFrom(&this->angleDeviation);
  shape->windDirection.connectFrom(&this->windDirection);
  shape->minWaveLength.connectFrom(&this->minWaveLength);
  shape->maxWaveLength.connectFrom(&this->maxWaveLength);
  shape->amplitudeRatio.connectFrom(&this->amplitudeRatio);
  
  shape->specAttenuation.connectFrom(&this->specAttenuation);
  shape->specEnd.connectFrom(&this->specEnd);
  shape->specTrans.connectFrom(&this->specTrans);
  
  shape->envHeight.connectFrom(&this->envHeight);
  shape->envRadius.connectFrom(&this->envRadius);
  shape->waterLevel.connectFrom(&this->waterLevel);
  shape->transitionSpeed.connectFrom(&this->transitionSpeed);
  shape->sharpness.connectFrom(&this->sharpness);

  SoShaderProgram * shader = (SoShaderProgram*) this->getAnyPart("shader", TRUE);
  shape->shader = shader;
}

/*!
  Destructor.
*/
SmOceanKit::~SmOceanKit(void)
{
  delete PRIVATE(this);
}

// Documented in superclass
void
SmOceanKit::initClass(void)
{
  static int first = 1;
  if (first) {
    first = 0;
    SO_KIT_INIT_CLASS(SmOceanKit, SoBaseKit, "BaseKit");
    OceanShape::initClass();
  }
}

void 
SmOceanKit::setDefaultOnNonWritingFields(void)
{
  this->oceanShape.setDefault(TRUE);
}

//********************************************************************************************

SO_NODE_SOURCE(OceanShape);

static unsigned short * add_triangles(unsigned short * ptr,
                                      const unsigned short * fanptr,
                                      int fanlen)
{
  int i = 0;
  int v0 = fanptr[i++];
  int v1 = fanptr[i++];
  while (i < fanlen) {
    int v2 = fanptr[i++];
    *ptr++ = v0;
    *ptr++ = v1;
    *ptr++ = v2;
    v1 = v2;
  }
  return ptr;
}

static unsigned short * 
add_fan(unsigned short * ptr, int x, int y, int gridsize, int bitmask) 
{
  unsigned short fanidx[10];
  unsigned short * fanptr = fanidx;
  int i = 0;

#define IDX(_x_, _y_) ((_y_)*gridsize + (_x_))

  // create triangle fan first
  fanptr[i++] = IDX(x,y);
  fanptr[i++] = IDX(x-1, y-1);
  if ((y > 1) || (bitmask&0x1)) {
    fanptr[i++] = IDX(x,y-1);
  }
  fanptr[i++] = IDX(x+1, y-1);
  if ((x < gridsize-2) || (bitmask&0x2)) {
    fanptr[i++] = IDX(x+1, y);
  }
  fanptr[i++] = IDX(x+1, y+1);
  if ((y < gridsize-2) || (bitmask&0x4)) {
    fanptr[i++] = IDX(x, y+1);
  }
  fanptr[i++] = IDX(x-1, y+1);
  if ((x > 1) || (bitmask & 0x8)) {
    fanptr[i++] = IDX(x-1,y);
  }
  fanptr[i++] = IDX(x-1, y-1);
  
#undef IDX

  // convert triangle fan to triangles for fast rendering
  return add_triangles(ptr, fanptr, i);
}


OceanShape::OceanShape()
{
  SO_NODE_CONSTRUCTOR(OceanShape);
  SO_NODE_ADD_FIELD(size, (10000.0f, 10000.0f));
  SO_NODE_ADD_FIELD(chop, (2.5f));
  SO_NODE_ADD_FIELD(angleDeviation, (15.0f));
  SO_NODE_ADD_FIELD(windDirection, (0.0f, 1.0f));
  
  SO_NODE_ADD_FIELD(minWaveLength, (15.f));
  SO_NODE_ADD_FIELD(maxWaveLength, (25.f));
  SO_NODE_ADD_FIELD(amplitudeRatio, (0.1f));
  
  SO_NODE_ADD_FIELD(envHeight, (-50.f));
  SO_NODE_ADD_FIELD(envRadius, (100.f));
  SO_NODE_ADD_FIELD(waterLevel, (-2.0f));
    
  SO_NODE_ADD_FIELD(specAttenuation, (1.0f));
  SO_NODE_ADD_FIELD(specEnd, (200.0f));
  SO_NODE_ADD_FIELD(specTrans, (100.0f));
  SO_NODE_ADD_FIELD(transitionSpeed, (1.0f / 6.0f));
  SO_NODE_ADD_FIELD(sharpness, (0.5f));
  SO_NODE_ADD_FIELD(shader, (NULL));

  this->root = NULL;
  this->coslut = NULL;
  this->biasnoisebuf = NULL;
  this->createBiasNoiseBuffer();

  if (node_memalloc == NULL) {
    node_memalloc = cc_memalloc_construct(sizeof(ocean_quadnode));
    numnodes++;
  }
  this->geostate_cache.transIdx = 0;
  this->texstate_cache.transIdx = 0;
  this->invalidstate = TRUE;
  this->minlevel = 4;
  this->maxlevel = 12;
  this->currtime = SbTime::zero();
  this->timersensor = new SoTimerSensor(timerCB, this);
  this->timersensor->setInterval(SbTime(1.0f/60.0f));
  this->timersensor->schedule();

  this->grid = new SbVec3f[GRIDSIZE*GRIDSIZE];
  int i = 0;
  int x, y;
  for (y = 0; y < GRIDSIZE; y++) {
    float fy = float(y) / float(GRIDSIZE-1);
    for (x = 0; x < GRIDSIZE; x++) {
      this->grid[i++] = SbVec3f(float(x)/float(GRIDSIZE-1),
                                fy,
                                0.0f);
    }
  }
  this->gridvbo = new SmVBO(GL_ARRAY_BUFFER, GL_STATIC_DRAW);
  this->gridvbo->setBufferData(this->grid, GRIDSIZE*GRIDSIZE*3*sizeof(float));
  int size = (GRIDSIZE/2)*(GRIDSIZE/2)*3*8;
  
  for (i = 0; i < 16; i++) {
    this->grididx[i] = new unsigned short[size];
    unsigned short * ptr = this->grididx[i]; 
    unsigned short * orgptr = ptr;
    for (y = 1; y < GRIDSIZE; y += 2) {
      for (x = 1; x < GRIDSIZE; x += 2) {
        ptr = add_fan(ptr, x, y, GRIDSIZE, i);
      }
    }
    this->idxlen[i] = ptr-orgptr;
    this->idxvbo[i] = new SmVBO(GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW);
    this->idxvbo[i]->setBufferData(this->grididx[i], this->idxlen[i]*sizeof(short));
    
    fprintf(stderr,"idxlen %d: %d\n", i , this->idxlen[i]);
  }
  this->vertexshader = NULL;
  this->fragmentshader = NULL;
}

OceanShape::~OceanShape()
{
  delete[] this->coslut;
  delete[] this->biasnoisebuf;
  delete this->timersensor;

  if (this->vertexshader) {
    this->vertexshader->unref();
  }
  if (this->fragmentshader) {
    this->fragmentshader->unref();
  }
  delete this->root;
  delete[] this->grid;
  delete this->gridvbo;

  for (int i = 0; i < 16; i++) {
    delete[] this->grididx[i];
    delete this->idxvbo[i];
  }

  numnodes--;
  if (numnodes == 0) {
    cc_memalloc_destruct(node_memalloc);
    node_memalloc = NULL;
  }
}

void
OceanShape::initClass()
{
  static int first = 1;
  if (first) {
    first = 0;
    SO_NODE_INIT_CLASS(OceanShape, SoShape, "Shape");
  }
}

void 
OceanShape::timerCB(void * closure, SoSensor * s)
{
  ((OceanShape*)closure)->touch(); // force a redraw
}

void
OceanShape::tick()
{
  SbTime t = SbTime::getTimeOfDay();
  if (this->currtime == SbTime::zero() || this->invalidstate) {
    this->createCosLUT();
    this->createBiasNoiseBuffer();
    this->copyGeoState();
    this->initTexState();
    this->initWaves();
    this->initLevels();
    this->invalidstate = FALSE;
    this->initShader();
  }
  else {
    this->updateWaves((t - this->currtime).getValue());
    this->updateShader();
  }
  this->currtime = t;
}

static void add_node_rec(SoState * state,
                         const float h,
                         SbList <ocean_quadnode*> & list, ocean_quadnode * node)
{
  int i;

  state->push();
  if (!SoCullElement::completelyInside(state)) {
    const SbVec3f * corners = node->getCorners();
    SbBox3f box(corners[0][0], corners[0][1], -h,
                corners[2][0], corners[2][1], h);
    if (SoCullElement::cullBox(state, box)) {
      state->pop();
      return;
    }
  }
  if (!node->isSplit()) list.append(node);
  else {
    for (i = 0; i < 4; i++) {
      ocean_quadnode * child = node->getChild(i);
      if (child->isSplit()) add_node_rec(state, h, list, child);
      else list.append(child);
    }
  }
  state->pop();
}

void
OceanShape::updateQuadtree(SoState * state)
{
  SbVec2f s = this->size.getValue();
  const SbMatrix & mat = SoModelMatrixElement::get(state);
  const SbViewVolume & vv = SoViewVolumeElement::get(state);

  uint32_t contextid = SoGLCacheContextElement::get(state);
  const cc_glglue * glue = cc_glglue_instance(contextid);

  SbVec3f pos = vv.getProjectionPoint();
  // move camera position to object space
  mat.inverse().multVecMatrix(pos, pos);

  if (this->root) {
    this->root->clearNeighbors();
    this->root->unsplit();
    this->root->distanceSplit(pos, 1, this->minlevel, this->maxlevel);
    this->root->deleteHiddenChildren(glue);
  }
  else {
    this->root = new (node_memalloc) ocean_quadnode(NULL,
                                                    SbVec3f(0.0, 0.0, 0.0),
                                                    SbVec3f(s[0], 0.0, 0.0),
                                                    SbVec3f(s[0], s[1], 0.0),
                                                    SbVec3f(0.0, s[1], 0.0));
    this->root->distanceSplit(pos, 1, minlevel, maxlevel); 
  }

}


void
OceanShape::GLRender(SoGLRenderAction * action)
{
  if (!this->shouldGLRender(action)) return;

  this->tick();
  SoState * state = action->getState();

  SoMaterialBundle mb(action);
  mb.sendFirst();

  this->updateQuadtree(state);
  this->nodelist.truncate(0);
  float h = this->amplitudeRatio.getValue() * this->maxWaveLength.getValue();

  add_node_rec(state, h, this->nodelist, this->root);

  SbVec3f n, v;
  
  uint32_t contextid = action->getCacheContext();
  const cc_glglue * glue = cc_glglue_instance(contextid);

#if 1 // render waves using VBO and vertex/fragment shaders
  this->updateParameters();
  this->gridvbo->bindBuffer((uint32_t) contextid);
  cc_glglue_glEnableClientState(glue, GL_VERTEX_ARRAY);
  cc_glglue_glVertexPointer(glue, 3, GL_FLOAT, 0, NULL);

  for (int i = 0; i < nodelist.getLength(); i++) {
    ocean_quadnode * node = this->nodelist[i];
    int mask = node->getNodeBitmask();
    const SbVec3f * corners = node->getCorners();
    SbVec3f scale = corners[2] - corners[0];
#if 0
    glPushMatrix();
    glTranslatef(corners[0][0], corners[0][1], 0.0f); 
    glScalef(scale[0], scale[1], 1.0f);
    this->idxvbo[mask]->bindBuffer(contextid);
    cc_glglue_glDrawElements(glue,
                             GL_TRIANGLES,
                             this->idxlen[mask],
                             GL_UNSIGNED_SHORT, NULL);
    glPopMatrix();
#else
    if (!node->gridvbo) {
      SbMatrix m;
      m.setScale(scale);
      SbMatrix m2;
      m2.setTranslate(corners[0]);
      m.multRight(m2);

      int i = 0;
      for (int y = 0; y < GRIDSIZE; y++) {
        float fy = float(y) / float(GRIDSIZE-1);
        for (int x = 0; x < GRIDSIZE; x++) {
          float fx = float(x) / float(GRIDSIZE-1);
          SbVec3f tmp(fx, fy, 0.0f);
          m.multVecMatrix(tmp, tmp);
          this->grid[i++] = tmp;
        }
      }
      cc_glglue_glGenBuffers(glue, 1, &node->gridvbo);
      cc_glglue_glBindBuffer(glue, GL_ARRAY_BUFFER, node->gridvbo);
      cc_glglue_glBufferData(glue, GL_ARRAY_BUFFER, i*sizeof(SbVec3f),
                             this->grid,
                             GL_STATIC_DRAW); 
    }
    else {
      cc_glglue_glBindBuffer(glue, GL_ARRAY_BUFFER, node->gridvbo);
    }
    cc_glglue_glVertexPointer(glue, 3, GL_FLOAT, 0, NULL);
    this->idxvbo[mask]->bindBuffer(contextid);
    cc_glglue_glDrawElements(glue,
                             GL_TRIANGLES,
                             this->idxlen[mask],
                             GL_UNSIGNED_SHORT, NULL);
#endif
  }
  cc_glglue_glDisableClientState(glue, GL_VERTEX_ARRAY);
  cc_glglue_glBindBuffer(glue, GL_ARRAY_BUFFER, 0);
  cc_glglue_glBindBuffer(glue, GL_ELEMENT_ARRAY_BUFFER, 0);

#else // just render everything using the CPU
  for (int i = 0; i < nodelist.getLength(); i++) {
    ocean_quadnode * node = this->nodelist[i];
    int mask = node->getNodeBitmask();    
    this->renderGrid(node, mask);
  }
#endif
}

void
OceanShape::renderGrid(ocean_quadnode * node, const int bitmask) 
{
  const SbVec3f * corners = node->getCorners();
  SbVec3f scale = corners[2] - corners[0];
  scale[2] = 1.0f;

  SbMatrix m;
  m.setScale(scale);
  SbMatrix m2;
  m2.setTranslate(corners[0]);
  m.multRight(m2);

  unsigned short * iptr = this->grididx[bitmask];
  int len = this->idxlen[bitmask];

  glBegin(GL_TRIANGLES);
  SbVec3f v0,v,n;
  while (len--) {
    int idx = *iptr++;
    // fprintf(stderr,"idx: %d\n", idx);
    v0 = this->grid[idx];
    m.multVecMatrix(v0,v0);
    this->wavefunc(v0, v, n);
    glNormal3fv(n.getValue());
    glVertex3fv(v.getValue());
  }
  glEnd();
}

/*!
*/
void
OceanShape::generatePrimitives(SoAction * action)
{
  SbVec2f s = this->size.getValue();
  SoPrimitiveVertex vertex;
  SoFaceDetail faceDetail;
  SoPointDetail pointDetail;

  vertex.setDetail(&pointDetail);
  vertex.setNormal(SbVec3f(0.0f, 0.0f, 1.0f));

  this->beginShape(action, QUADS, &faceDetail);
  vertex.setPoint(SbVec3f(0.0f, 0.0f, 0.0f));
  this->shapeVertex(&vertex);

  vertex.setPoint(SbVec3f(s[0], 0.0f, 0.0f));
  this->shapeVertex(&vertex);

  vertex.setPoint(SbVec3f(s[0], s[1], 0.0f));
  this->shapeVertex(&vertex);

  vertex.setPoint(SbVec3f(0.0f, s[1], 0.0f));
  this->shapeVertex(&vertex);
  this->endShape();
}

void
OceanShape::computeBBox(SoAction * action, SbBox3f & box, SbVec3f & center)
{
  SbVec2f s = this->size.getValue();
  float h = this->amplitudeRatio.getValue() * this->maxWaveLength.getValue();
  box.setBounds(0.0f, 0.0f, -h, s[0], s[1], h);

  center.setValue(s[0]*0.5f, s[1]*0.5f, 0.0f);
}

void
OceanShape::getPrimitiveCount(SoGetPrimitiveCountAction * action)
{
}

void 
OceanShape::rayPick(SoRayPickAction * action)
{
  if (!this->shouldRayPick(action)) return;
  action->setObjectSpace();

  SbVec2f s = this->size.getValue();
  SbPlane p(SbVec3f(0.0f, 0.0, 1.0f), 0.0f);
  SbVec3f isect;
  if (p.intersect(action->getLine(), isect)) {
    if ((isect[0] >= 0.0f) && (isect[1] >= 0.0f) &&
        (isect[0] <= s[0]) && (isect[1] <= s[1]) &&
        action->isBetweenPlanes(isect)) {
      // any details needed?
      (void) action->addIntersection(isect);
    }
  }
}

void 
OceanShape::notify(SoNotList * list)
{
  SoField * f = list->getLastField();
  if (f == &this->size) {
    delete this->root;
    this->root = NULL;
  }
  else if (f != &this->shader) {
    //this->invalidstate = TRUE;
  }
  inherited::notify(list);
}


//*****************************************************************************************

#define FLAG_ISSPLIT 0x1

cc_memalloc * 
ocean_quadnode::getAllocator()
{
  assert(node_memalloc);
  return node_memalloc;
}

ocean_quadnode::ocean_quadnode(ocean_quadnode * parent,
                               const SbVec3f & c0,
                               const SbVec3f & c1,
                               const SbVec3f & c2,
                               const SbVec3f & c3)
{
  this->parent = parent;

  this->corner[0] = c0;
  this->corner[1] = c1;
  this->corner[2] = c2;
  this->corner[3] = c3;
  
  this->child[0] = NULL;
  this->child[1] = NULL;
  this->child[2] = NULL;
  this->child[3] = NULL;

  this->neighbor[0] = NULL;
  this->neighbor[1] = NULL;
  this->neighbor[2] = NULL;
  this->neighbor[3] = NULL;

  this->neighbor_rotate_bits = 0;
  this->flags = 0;

  if (parent && 0) {
    this->debugcolor[0] = parent->debugcolor[0];
    this->debugcolor[1] = parent->debugcolor[1];
    this->debugcolor[2] = parent->debugcolor[2];
  }
  else {
    for (int i = 0; i < 3; i++) {
      float val = (float) rand();
      val /= float(RAND_MAX);
      val += 1.0;
      val *= 0.5f;
      this->debugcolor[i] = val;
    }
  }
  this->gridvbo = 0;
}

ocean_quadnode::~ocean_quadnode(void)
{
  for (int i = 0; i < 4; i++) {
    delete this->child[i];
  }
}

int
ocean_quadnode::getNodeBitmask(void)
{
  int splittest[4] = {0,0,0,0};
  int mask = 0;

  if (this->parent) {
    switch (this->parent->getChildIndex(this)) {
    case BOTTOM_LEFT:
      splittest[LEFT] = 1;
      splittest[BOTTOM] = 1;
      break;
    case BOTTOM_RIGHT:
      splittest[BOTTOM] = 1;
      splittest[RIGHT] = 1;
      break;
    case TOP_RIGHT:
      splittest[TOP] = 1;
      splittest[RIGHT] = 1;
      break;
    case TOP_LEFT:
      splittest[TOP] = 1;
      splittest[LEFT] = 1;
      break;
    default:
      assert(0 && "oops");
      break;
    }
    
    for (int j = 0; j < 4; j++) {
      if (!splittest[j]) {
        mask |= 1<<j;
      }
      else {
        ocean_quadnode * n = this->parent->searchNeighbor(j);
        if (!n || n->isSplit()) {
          mask |= 1<<j;
        }
      }
    }
  }
  else mask = 0xf;
  return mask;
}

#define SPLIT_K 8.0f

void 
ocean_quadnode::distanceSplit(const SbVec3f & pos,
                              const int level,
                              const int minlevel,
                              const int maxlevel)
{
  if (level == maxlevel) return;
  
  SbVec3f c = (this->corner[0] + this->corner[2]) * 0.5;
  SbVec3f s = c - this->corner[0];
  double len = s.sqrLength();
  double dist = (pos-c).sqrLength();

  // fprintf(stderr,"test: %g %g\n", len, dist);
  if ((dist < len*SPLIT_K) || (level < minlevel)) {
    if (!this->isSplit()) this->split(TRUE);
    else this->findNeighbors();
    for (int i = 0; i < 4; i++) {
      this->child[i]->distanceSplit(pos, level+1, minlevel, maxlevel);
    }
  }
  else {
    this->unsplit();
  }
}

void
ocean_quadnode::unsplit(void)
{
  if (this->isSplit()) { 
    this->flags &= ~FLAG_ISSPLIT;
    for (int i = 0; i < 4; i++) {
      this->child[i]->unsplit();
    }
  }
}

void 
ocean_quadnode::deleteHiddenChildren(const cc_glglue * glue)
{
  if (!this->isSplit() && this->hasChildren()) {
    for (int i = 0; i < 4; i++) {
      this->child[i]->deleteHiddenChildren(glue);
      if (this->child[i]->gridvbo) {
        cc_glglue_glDeleteBuffers(glue, 1, &this->child[i]->gridvbo);
      }
      delete this->child[i];
      this->child[i] = NULL;
    }
  }
}

void 
ocean_quadnode::clearNeighbors(void)
{
  int i;
  for (i = 0; i < 4; i++) {
    this->neighbor[i] = NULL;
  }
  if (this->hasChildren()) {
    for (i = 0; i < 4; i++) {
      this->child[i]->clearNeighbors();
    }
  }
}

void 
ocean_quadnode::split(const int findneighbors)
{
  if (this->isSplit()) {
    this->child[0]->split(1);
    this->child[1]->split(1);
    this->child[2]->split(1);
    this->child[3]->split(1);
  }
  else {
    if (!this->hasChildren()) {
      SbVec3f c = (this->corner[0] + this->corner[2]) * 0.5;
      SbVec3f e0 = (this->corner[0] + this->corner[1]) * 0.5;
      SbVec3f e1 = (this->corner[1] + this->corner[2]) * 0.5;
      SbVec3f e2 = (this->corner[2] + this->corner[3]) * 0.5;
      SbVec3f e3 = (this->corner[3] + this->corner[0]) * 0.5;
      
      this->child[BOTTOM_LEFT] = new (node_memalloc) ocean_quadnode(this, this->corner[0], e0, c, e3);
      this->child[BOTTOM_RIGHT] = new (node_memalloc)ocean_quadnode(this, e0, this->corner[1], e1, c);
      this->child[TOP_RIGHT] = new (node_memalloc)   ocean_quadnode(this, c, e1, this->corner[2], e2);
      this->child[TOP_LEFT] = new (node_memalloc)    ocean_quadnode(this, e3, c, e2, this->corner[3]);
      
      this->updateChildRotate();
    }
    this->flags |= FLAG_ISSPLIT;
    if (this->parent) this->parent->findNeighbors();
    
    if (findneighbors) {
      this->findNeighbors();
    }
  }
}


const SbVec3f & 
ocean_quadnode::getCorner(const int i) const
{
  return this->corner[i];
}

int 
ocean_quadnode::isSplit(void) const
{
  return (this->flags & FLAG_ISSPLIT) != 0;
}

int 
ocean_quadnode::hasChildren(void) const
{
  return this->child[0] != NULL;
}

ocean_quadnode * 
ocean_quadnode::getChild(const int i)
{
  return this->child[i];
}

void 
ocean_quadnode::rotate(void)
{
  SbVec3f tmp = this->corner[0];
  
  this->corner[0] = this->corner[1];
  this->corner[1] = this->corner[2];
  this->corner[2] = this->corner[3];
  this->corner[3] = tmp;
}

ocean_quadnode * 
ocean_quadnode::getNeighbor(const int i)
{
  return this->neighbor[i];
}

void 
ocean_quadnode::setNeighbor(const int i, ocean_quadnode * node)
{
  this->neighbor[i] = node;
}

ocean_quadnode * 
ocean_quadnode::getParent(void)
{
  return this->parent;
}


void
ocean_quadnode::findNeighbors()
{
  if (!this->parent) return;
  if (this->neighbor[0] &&
      this->neighbor[1] &&
      this->neighbor[2] &&
      this->neighbor[3]) return;
  
  ocean_quadnode *n[4];
  int i;
  int dosplit[4];
  int didsplit[4];

  for (i = 0; i < 4; i++) {
    n[i] = this->parent->getNeighbor(i);
    dosplit[i] = 0;
    didsplit[i] = 0;
  }
  switch (this->parent->getChildIndex(this)) {
  case BOTTOM_LEFT:
    dosplit[LEFT] = 1;
    dosplit[BOTTOM] = 1;
    break;
  case BOTTOM_RIGHT:
    dosplit[BOTTOM] = 1;
    dosplit[RIGHT] = 1;
    break;
  case TOP_RIGHT:
    dosplit[TOP] = 1;
    dosplit[RIGHT] = 1;
    break;
  case TOP_LEFT:
    dosplit[TOP] = 1;
    dosplit[LEFT] = 1;
    break;
  default:
    assert(0 && "oops");
    break;
  }
  for (i = 0; i < 4; i++) {
    if (dosplit[i]) {
      if (n[i] && !n[i]->isSplit()) {
        n[i]->split(0);
        didsplit[i] = 1;
      }
    }
  }
  for (i = 0; i < 4; i++) {
    this->neighbor[i] = this->searchNeighbor(i);
  }
  for (i = 0; i < 4; i++) {
    if (didsplit[i] && n[i]) {
      n[i]->findNeighbors();
    }
  }
}

ocean_quadnode * 
ocean_quadnode::searchNeighbor(const int i)
{
  if (this->neighbor[i]) return this->neighbor[i];
  if (this->parent) {
    switch (this->parent->getChildIndex(this)) {
    case BOTTOM_LEFT:
      switch (i) {
      case LEFT:   return this->parent->getNeighborChild(LEFT, BOTTOM_RIGHT);
      case RIGHT:  return this->parent->getChild(BOTTOM_RIGHT);
      case TOP:    return this->parent->getChild(TOP_LEFT);
      case BOTTOM: return this->parent->getNeighborChild(BOTTOM, TOP_LEFT);
      default: assert(0 && "oops");
      }
      break;
    case BOTTOM_RIGHT:
      switch (i) {
      case RIGHT:  return this->parent->getNeighborChild(RIGHT, BOTTOM_LEFT);
      case LEFT:   return this->parent->getChild(BOTTOM_LEFT);
      case TOP:    return this->parent->getChild(TOP_RIGHT);
      case BOTTOM: return this->parent->getNeighborChild(BOTTOM, TOP_RIGHT);
      default: assert(0 && "oops");
      }
      break;
    case TOP_LEFT:
      switch (i) {
      case LEFT:   return this->parent->getNeighborChild(LEFT, TOP_RIGHT);
      case RIGHT:  return this->parent->getChild(TOP_RIGHT);
      case BOTTOM: return this->parent->getChild(BOTTOM_LEFT);
      case TOP:    return this->parent->getNeighborChild(TOP, BOTTOM_LEFT);
      default: assert(0 && "oops");
      }
      break;
    case TOP_RIGHT:
      switch (i) {
      case RIGHT:  return this->parent->getNeighborChild(RIGHT, TOP_LEFT);
      case LEFT:   return this->parent->getChild(TOP_LEFT);
      case BOTTOM: return this->parent->getChild(BOTTOM_RIGHT);
      case TOP:    return this->parent->getNeighborChild(TOP, BOTTOM_RIGHT);
      default: assert(0 && "oops");
      }
      break;
    default:
      assert(0 && "oops");
      break;
    }
  }
  return NULL;
}

int 
ocean_quadnode::getChildIndex(const ocean_quadnode * n) const
{
  for (int i = 0; i < 4; i++) {
    if (this->child[i] == n) return i;
  }
  return -1;
}

void
ocean_quadnode::updateChildRotate(void)
{
  for (int i = 0; i < 4; i++) {
    ocean_quadnode * c = this->child[i];
    switch (i) {
    case BOTTOM_LEFT:
      c->setNeighborRotate(BOTTOM, this->getNeighborRotate(BOTTOM));
      c->setNeighborRotate(LEFT, this->getNeighborRotate(LEFT));
      break;
    case BOTTOM_RIGHT:
      c->setNeighborRotate(BOTTOM, this->getNeighborRotate(BOTTOM));
      c->setNeighborRotate(RIGHT, this->getNeighborRotate(RIGHT));
      break;
    case TOP_LEFT:
      c->setNeighborRotate(TOP, this->getNeighborRotate(TOP));
      c->setNeighborRotate(LEFT, this->getNeighborRotate(LEFT));
      break;
    case TOP_RIGHT:
      c->setNeighborRotate(TOP, this->getNeighborRotate(TOP));
      c->setNeighborRotate(RIGHT, this->getNeighborRotate(RIGHT));
      break;
    default:
      assert(0 && "oops");
      break;
    }
  }
}

void 
ocean_quadnode::setNeighborRotate(const int i, const int rot)
{
  int shift = i*2;
  unsigned int mask = 0x3<<shift;
  this->neighbor_rotate_bits &= ~mask;
  this->neighbor_rotate_bits |= rot<<shift;
}

int 
ocean_quadnode::getNeighborRotate(const int i) const
{
  return (this->neighbor_rotate_bits>>(i*2))&0x3;
}

ocean_quadnode * 
ocean_quadnode::getNeighborChild(const int neighbor, const int child)
{
  ocean_quadnode * n = this->neighbor[neighbor];
  if (n) {
    int idx = child - this->getNeighborRotate(neighbor);
    return n->getChild((idx+4)%4);
  }
  return NULL;
}

void * 
ocean_quadnode::operator new(size_t size, cc_memalloc *memhandler)
{
  return cc_memalloc_allocate(memhandler);
}

void 
ocean_quadnode::operator delete(void * ptr)
{
  cc_memalloc_deallocate(node_memalloc, ptr);
}

// All code below is just for testing the wave functions


static float RandMinusOneToOne()
{
  return float( double(rand()) / double(RAND_MAX) * 2.0 - 1.0 );
}

static float RandZeroToOne()
{
  return float( double(rand()) / double(RAND_MAX) );
}


void 
OceanShape::copyGeoState()
{
  this->geostate_cache.chop = this->chop.getValue();
  this->geostate_cache.angleDeviation = this->angleDeviation.getValue();
  this->geostate_cache.windDir = this->windDirection.getValue();
  (void) this->geostate_cache.windDir.normalize();

  this->geostate_cache.minLength = this->minWaveLength.getValue();
  this->geostate_cache.maxLength = this->maxWaveLength.getValue();
  this->geostate_cache.ampRatio = this->amplitudeRatio.getValue();
  
  this->geostate_cache.envHeight = this->envHeight.getValue();
  this->geostate_cache.envRadius = this->envRadius.getValue();
  this->geostate_cache.waterLevel = this->waterLevel.getValue();
  
  this->geostate_cache.transDel = -this->transitionSpeed.getValue();
  this->geostate_cache.transIdx = 0;
  this->geostate_cache.specAtten = this->specAttenuation.getValue();
  this->geostate_cache.specEnd = this->specEnd.getValue();
  this->geostate_cache.specTrans = this->specTrans.getValue();
  this->geostate_cache.Q = this->sharpness.getValue();
}

void 
OceanShape::initGeoWave(const int i)
{
  this->geowaves[i].phase = RandZeroToOne() * float(M_PI) * 2.0f;
  this->geowaves[i].len = this->geostate_cache.minLength + RandZeroToOne() * (this->geostate_cache.maxLength - this->geostate_cache.minLength);
  this->geowaves[i].amp = this->geowaves[i].fullamp = this->geowaves[i].len * this->geostate_cache.ampRatio / float(NUM_GEO_WAVES);
  this->geowaves[i].freq = 2.0f * float(M_PI) / this->geowaves[i].len;
  this->geowaves[i].fade = 1.0f;
  
  float rotBase = this->geostate_cache.angleDeviation * float(M_PI) / 180.f;
  
  float rads = rotBase * RandMinusOneToOne();
  float rx = float(cos(rads));
  float ry = float(sin(rads));
  
  float x = this->geostate_cache.windDir[0];
  float y = this->geostate_cache.windDir[1];
  this->geowaves[i].dir[0] = x * rx + y * ry;
  this->geowaves[i].dir[1] = x * -ry + y * rx;
}

void 
OceanShape::updateGeoWave(const int i, const float dt)
{
  if (i == this->geostate_cache.transIdx) {
    this->geowaves[i].fade += this->geostate_cache.transDel * dt;
    if (this->geowaves[i].fade < 0.0f) {
      // This wave is faded out. Re-init and fade it back up.
      this->initGeoWave(i);
      this->geowaves[i].fade = 0.0f;
      this->geostate_cache.transDel = -this->geostate_cache.transDel;
    }
    else if (this->geowaves[i].fade > 1.0f) {
      // This wave is faded back up. Start fading another down.
      this->geowaves[i].fade = 1.0f;
      this->geostate_cache.transDel = -this->geostate_cache.transDel;
      this->geostate_cache.transIdx++;
      if (this->geostate_cache.transIdx >= NUM_GEO_WAVES) {
        this->geostate_cache.transIdx = 0;
      }
    }
  }
  const float speed = float(1.0 / sqrt(this->geowaves[i].len / (2.f * float(M_PI) * kGravConst)));
  
  this->geowaves[i].phase += speed * dt;
  this->geowaves[i].phase = float(fmod(this->geowaves[i].phase, 2.0*M_PI));
  this->geowaves[i].amp = (this->geowaves[i].len * this->geostate_cache.ampRatio / float(NUM_GEO_WAVES)) * this->geowaves[i].fade;
}

void 
OceanShape::updateTexWave(const int i, const float dt)
{
  if (i == this->texstate_cache.transIdx) {
    this->texwaves[i].fade += this->texstate_cache.transDel * dt;
    if (this->texwaves[i].fade < 0.0f) {
      // This wave is faded out. Re-init and fade it back up.
      this->initTexWave(i);
      this->texwaves[i].fade = 0.0f;
      this->texstate_cache.transDel = -this->texstate_cache.transDel;
    }
    else if (this->texwaves[i].fade > 1.0f) {
      // This wave is faded back up. Start fading another down.
      this->texwaves[i].fade = 1.0f;
      this->texstate_cache.transDel = -this->texstate_cache.transDel;
      this->texstate_cache.transIdx++;
      if (this->texstate_cache.transIdx >= NUM_TEX_WAVES)
        this->texstate_cache.transIdx = 0;
    }
  }
  this->texwaves[i].phase -= dt * this->texwaves[i].speed;
  this->texwaves[i].phase -= int(this->texwaves[i].phase);
}

void 
OceanShape::initTexWave(const int i) 
{
  float rads = RandMinusOneToOne() * this->texstate_cache.angleDeviation * float(M_PI) / 180.f;
  float dx = float(sin(rads));
  float dy = float(cos(rads));

  float tx = dx;
  dx = this->texstate_cache.windDir[1] * dx - this->texstate_cache.windDir[0] * dy;
  dy = this->texstate_cache.windDir[0] * tx + this->texstate_cache.windDir[1] * dy;

  float maxLen = this->texstate_cache.maxLength * BUMPSIZE / this->texstate_cache.rippleScale;
  float minLen = this->texstate_cache.minLength * BUMPSIZE / this->texstate_cache.rippleScale;
  float len = float(i) / float(NUM_TEX_WAVES-1) * (maxLen - minLen) + minLen;
  
  float reps = float(BUMPSIZE) / len;
  
  dx *= reps;
  dy *= reps;
  dx = float(int(dx >= 0.0f ? dx + 0.5f : dx - 0.5f));
  dy = float(int(dy >= 0.0f ? dy + 0.5f : dy - 0.5f));
  
  this->texwaves[i].rotScale[0] = dx;
  this->texwaves[i].rotScale[1] = dy;
  
  float effK = float(1.0 / sqrt(dx*dx + dy*dy));
  this->texwaves[i].len = float(BUMPSIZE) * effK;
  this->texwaves[i].freq = float(M_PI) * 2.0f / this->texwaves[i].len;
  this->texwaves[i].amp = this->texwaves[i].len * this->texstate_cache.ampRatio;
  this->texwaves[i].phase = RandZeroToOne();
  
  this->texwaves[i].dir[0] = dx * effK;
  this->texwaves[i].dir[1] = dy * effK;
  
  this->texwaves[i].fade = 1.0f;
  
  float speed = float( 1.0 / sqrt(this->texwaves[i].len / (2.0f * float(M_PI) * kGravConst)) ) / 3.0f;
  speed *= 1.0f + RandMinusOneToOne() * this->texstate_cache.speedDeviation;
  this->texwaves[i].speed = speed;
}

void
OceanShape::initTexState()
{
  this->texstate_cache.noise = 0.2f;
  this->texstate_cache.chop = 1.0f;
  this->texstate_cache.angleDeviation = 15.f;
  this->texstate_cache.windDir = this->windDirection.getValue();
  this->texstate_cache.windDir[1] = 1.f;
  this->texstate_cache.maxLength = 10.f;
  this->texstate_cache.minLength = 1.0f;
  this->texstate_cache.ampRatio = 0.1f;
  this->texstate_cache.rippleScale = 25.f;
  this->texstate_cache.speedDeviation = 0.1f;
  
  this->texstate_cache.transIdx = 0;
  this->texstate_cache.transDel = -1.0f / 5.f;
}

void 
OceanShape::updateWaves(const double dt)
{
  int i;
  for (i = 0; i < NUM_GEO_WAVES; i++) {
    this->updateGeoWave(i, (float) dt);
  }
  for (i = 0; i < NUM_TEX_WAVES; i++) {
    this->updateTexWave(i, (float) dt);
  }
}

void 
OceanShape::initLevels()
{
  float minlen = this->geostate_cache.minLength / 4.0f;
  float minsize = SbMin(this->size.getValue()[0], this->size.getValue()[1]);

  float vdist = minsize / float(GRIDSIZE-1);
  int level = 1;
  while (vdist > minlen) {
    level++;
    vdist *= 0.5f;
  }

  this->maxlevel = level;
  this->minlevel = 1;

  fprintf(stderr,"min/max levels: %d %d\n", this->minlevel, this->maxlevel);
}

void 
OceanShape::initWaves()
{
  int i;
  for (i = 0; i < NUM_GEO_WAVES; i++) {
    this->initGeoWave(i);
  }
  for (i = 0; i < NUM_TEX_WAVES; i++) {
    this->initTexWave(i);
  }
}

void
OceanShape::updateShader(void)
{
}

void 
OceanShape::initShader(void)
{
  SoShaderProgram * s = (SoShaderProgram*) this->shader.getValue();
  if (!s) return;

  if (!this->vertexshader) {

    this->fragmentshader = new SoFragmentShader();
    this->fragmentshader->ref();
    this->fragmentshader->sourceProgram = "smocean_fragment.cg";

    this->vertexshader = new SoVertexShader();
    this->vertexshader->ref();
    this->vertexshader->sourceProgram = "smocean_vertex.cg";

    this->param_geowaveamp = new SoShaderParameter4f();
    this->param_geowaveamp->ref();
    this->param_geowaveamp->name = "geowaveAmp";

    this->param_geowavefreq = new SoShaderParameter4f();
    this->param_geowavefreq->ref();
    this->param_geowavefreq->name = "geowaveFreq";

    this->param_geowavephase = new SoShaderParameter4f();
    this->param_geowavephase->ref();
    this->param_geowavephase->name = "geowavePhase";

    this->param_geowaveQ = new SoShaderParameter4f();
    this->param_geowaveQ->ref();
    this->param_geowaveQ->name = "geowaveQ";
    
    this->param_geowave1dir = new SoShaderParameter2f();
    this->param_geowave1dir->ref();
    this->param_geowave1dir->name = "geowave1dir";
    this->param_geowave2dir = new SoShaderParameter2f();
    this->param_geowave2dir->ref();
    this->param_geowave2dir->name = "geowave2dir";
    this->param_geowave3dir = new SoShaderParameter2f();
    this->param_geowave3dir->ref();
    this->param_geowave3dir->name = "geowave3dir";
    this->param_geowave4dir = new SoShaderParameter2f();
    this->param_geowave4dir->ref();
    this->param_geowave4dir->name = "geowave4dir";


    this->vertexshader->parameter.set1Value(0, this->param_geowaveamp);
    this->vertexshader->parameter.set1Value(1, this->param_geowavephase);
    this->vertexshader->parameter.set1Value(2, this->param_geowavefreq);
    this->vertexshader->parameter.set1Value(3, this->param_geowaveQ);
    this->vertexshader->parameter.set1Value(4, this->param_geowave1dir);
    this->vertexshader->parameter.set1Value(5, this->param_geowave2dir);
    this->vertexshader->parameter.set1Value(6, this->param_geowave3dir);
    this->vertexshader->parameter.set1Value(7, this->param_geowave4dir);

    s->shaderObject.set1Value(0, this->fragmentshader);
    s->shaderObject.set1Value(1, this->vertexshader);
  }
}

void OceanShape::updateParameters()
{
  SbVec4f Q;
  int i;
  for (i = 0; i < NUM_GEO_WAVES; i++) {
    if (this->geowaves[i].amp > 0.01f) {
      Q[i] = this->geostate_cache.Q / (this->geowaves[i].freq * this->geowaves[i].amp * float(NUM_GEO_WAVES));
    }
    else {
      Q[i] = 0.0f;
    }
    Q[i] = this->geostate_cache.Q / (this->geowaves[i].freq * this->geowaves[i].fullamp * float(NUM_GEO_WAVES));
  }
  this->param_geowaveQ->value = Q;

  SbVec4f tmp;
  tmp.setValue(this->geowaves[0].amp, this->geowaves[1].amp, this->geowaves[2].amp, this->geowaves[3].amp);
  this->param_geowaveamp->value = tmp;

  tmp.setValue(this->geowaves[0].freq, this->geowaves[1].freq, this->geowaves[2].freq, this->geowaves[3].freq);
  this->param_geowavefreq->value = tmp;

  tmp.setValue(this->geowaves[0].phase, this->geowaves[1].phase, this->geowaves[2].phase, this->geowaves[3].phase);
  this->param_geowavephase->value = tmp;

  this->param_geowave1dir->value = this->geowaves[0].dir;
  this->param_geowave2dir->value = this->geowaves[1].dir;
  this->param_geowave3dir->value = this->geowaves[2].dir;
  this->param_geowave4dir->value = this->geowaves[3].dir;

}

void OceanShape::wavefunc(const SbVec3f & in, SbVec3f & v, SbVec3f & n) 
{
  SbVec3f tv(in[0], in[1], 0.0f);
  SbVec3f tn(0.0f, 0.0f, 1.0f);

  float Q[NUM_GEO_WAVES];
  int i;
  for (i = 0; i < NUM_GEO_WAVES; i++) {
    if (this->geowaves[i].amp > 0.01f) {
      Q[i] = this->geostate_cache.Q / (this->geowaves[i].freq * this->geowaves[i].amp * float(NUM_GEO_WAVES));
    }
    else {
      Q[i] = 0.0f;
    }
    Q[i] = this->geostate_cache.Q / (this->geowaves[i].freq * this->geowaves[i].fullamp * float(NUM_GEO_WAVES));
  }

  for (i = 0; i < NUM_GEO_WAVES; i++) {

    float dot = this->geowaves[i].dir[0] * in[0] + this->geowaves[i].dir[1] * in[1];
    float cossinval = (float) (dot *
                               this->geowaves[i].freq + 
                               this->geowaves[i].phase);
    
    float cosval = (float) cos(cossinval);
    float sinval = (float) sin(cossinval);

    tv[0] += Q[i]*this->geowaves[i].amp*this->geowaves[i].dir[0] * cosval;
    tv[1] += Q[i]*this->geowaves[i].amp*this->geowaves[i].dir[1] * cosval;
    tv[2] += this->geowaves[i].amp * sinval;
  }
  for (i = 0; i < NUM_GEO_WAVES; i++) {
    float dot = this->geowaves[i].dir[0] * tv[0] + this->geowaves[i].dir[1] * tv[1];
    float cossinval = (float) (dot *
                               this->geowaves[i].freq + 
                               this->geowaves[i].phase);
    float cosval = (float) cos(cossinval);
    float sinval = (float) sin(cossinval);

    tn[0] -= (this->geowaves[i].freq * 
              this->geowaves[i].amp *
              cosval);
    tn[1] -= (this->geowaves[i].freq * 
              this->geowaves[i].amp *
              cosval);
    tn[2] -= Q[i] * this->geowaves[i].freq * this->geowaves[i].amp * sinval;
  }
  v = tv;
  n = tn;
}

void
OceanShape::createBiasNoiseBuffer()
{
  unsigned char * buf = new unsigned char[BUMPSIZE*BUMPSIZE*4];
  this->biasnoisebuf = buf;
  
  for(int i = 0; i < BUMPSIZE*BUMPSIZE; i++) {
    float x = RandZeroToOne();
    float y = RandZeroToOne();
    
    *buf++ = (unsigned char)(x * 255.999f);
    *buf++ = (unsigned char)(y * 255.999f);
    *buf++ = 255;
    *buf++ = 255;
  }
}

void
OceanShape::createCosLUT()
{
  delete[] this->coslut;
  unsigned char * buf = new unsigned char[BUMPSIZE*4];
  this->coslut = buf;

  for(int i = 0; i < BUMPSIZE; i++) {
    float dist = float(i) / float(BUMPSIZE-1) * 2.0f * float(M_PI);
    float c = float(cos(dist));
    float s = float(sin(dist));
    s *= 0.5f;
    s += 0.5f;
    s = float(pow(s, this->texstate_cache.chop));
    c *= s;
    unsigned char cosDist = (unsigned char)((c * 0.5 + 0.5) * 255.999f);
    *buf++ = cosDist;
    *buf++ = cosDist;
    *buf++ = 255;
    *buf++ = 255;
  }
}

#undef FLAG_ISSPLIT



/*!
  Constructor
*/
SmVBO::SmVBO(const GLenum target, const GLenum usage)
  : target(target),
    usage(usage),
    data(NULL),
    datasize(0),
    dataid(0),
    didalloc(FALSE),
    vbohash(5)
{
  SoContextHandler::addContextDestructionCallback(context_destruction_cb, this);
}


/*!
  Destructor
*/
SmVBO::~SmVBO()
{
  SoContextHandler::removeContextDestructionCallback(context_destruction_cb, this);
  // schedule delete for all allocated GL resources
  this->vbohash.apply(vbo_schedule, NULL);
  if (this->didalloc) {
    char * ptr = (char*) this->data;
    delete[] ptr;
  }
}

void 
SmVBO::init(void)
{
  // coin_glglue_add_instance_created_callback(context_created, NULL);
}

/*!
  Used to allocate buffer data. The user is responsible for filling in
  the correct type of data in the buffer before the buffer is used.

  \sa setBufferData()
*/
void *
SmVBO::allocBufferData(intptr_t size, uint32_t dataid)
{
  // schedule delete for all allocated GL resources
  this->vbohash.apply(vbo_schedule, NULL);
  // clear hash table
  this->vbohash.clear();

  if (this->didalloc && this->datasize == size) {
    return (void*)this->data;
  }
  if (this->didalloc) {
    char * ptr = (char*) this->data;
    delete[] ptr;
  }

  char * ptr = new char[size];
  this->didalloc = TRUE;
  this->data = (const GLvoid*) ptr;
  this->datasize = size;
  this->dataid = dataid;
  return (void*) this->data;
}

/*!
  Sets the buffer data. \a dataid is a unique id used to identify 
  the buffer data. In Coin it's possible to use the node id
  (SoNode::getNodeId()) to test if a buffer is valid for a node.
*/
void 
SmVBO::setBufferData(const GLvoid * data, intptr_t size, uint32_t dataid)
{
  // schedule delete for all allocated GL resources
  this->vbohash.apply(vbo_schedule, NULL);
  // clear hash table
  this->vbohash.clear();

  // clean up old buffer (if any)
  if (this->didalloc) {
    char * ptr = (char*) this->data;
    delete[] ptr;
  }
  
  this->data = data;
  this->datasize = size;
  this->dataid = dataid;
  this->didalloc = FALSE;
}

/*!
  Returns the buffer data id. 
  
  \sa setBufferData()
*/
uint32_t 
SmVBO::getBufferDataId(void) const
{
  return this->dataid;
}

/*!
  Returns the data pointer and size.
*/
void 
SmVBO::getBufferData(const GLvoid *& data, intptr_t & size)
{
  data = this->data;
  size = this->datasize;
}


/*!
  Binds the buffer for the context \a contextid.
*/
void 
SmVBO::bindBuffer(uint32_t contextid)
{
  if ((this->data == NULL) ||
      (this->datasize == 0)) {
    assert(0 && "no data in buffer");
    return;
  }

  const cc_glglue * glue = cc_glglue_instance((int) contextid);

  GLuint buffer;
  if (!this->vbohash.get(contextid, buffer)) {
    // need to create a new buffer for this context
    cc_glglue_glGenBuffers(glue, 1, &buffer);
    cc_glglue_glBindBuffer(glue, this->target, buffer);
    cc_glglue_glBufferData(glue, this->target,
                           this->datasize,
                           this->data,
                           this->usage);
    this->vbohash.put(contextid, buffer);
  }
  else {
    // buffer already exists, bind it
    cc_glglue_glBindBuffer(glue, this->target, buffer);
  }
}

//
// Callback from SbHash
//
void 
SmVBO::vbo_schedule(const uint32_t & key,
                    const GLuint & value,
                    void * closure)
{
  void * ptr = (void*) ((uintptr_t) value);
  SoGLCacheContextElement::scheduleDeleteCallback(key, vbo_delete, ptr);
}

//
// Callback from SoGLCacheContextElement
//
void 
SmVBO::vbo_delete(void * closure, uint32_t contextid)
{
  const cc_glglue * glue = cc_glglue_instance((int) contextid);
  GLuint id = (GLuint) ((uintptr_t) closure);
  cc_glglue_glDeleteBuffers(glue, 1, &id);
}

//
// Callback from SoContextHandler
//
void 
SmVBO::context_destruction_cb(uint32_t context, void * userdata)
{
  GLuint buffer;
  SmVBO * thisp = (SmVBO*) userdata;

  if (thisp->vbohash.get(context, buffer)) {
    const cc_glglue * glue = cc_glglue_instance((int) context);
    cc_glglue_glDeleteBuffers(glue, 1, &buffer);    
    thisp->vbohash.remove(context);
  }
}


/*!
  Sets the global limits on the number of vertex data in a node before
  vertex buffer objects are considered to be used for rendering.
*/
void 
SmVBO::setVertexCountLimits(const int minlimit, const int maxlimit)
{
  vbo_vertex_count_min_limit = minlimit;
  vbo_vertex_count_max_limit = maxlimit;
}

/*!
  Returns the vertex VBO minimum limit.

  \sa setVertexCountLimits()
 */
int 
SmVBO::getVertexCountMinLimit(void)
{
  if (vbo_vertex_count_min_limit < 0) {
    const char * env = coin_getenv("COIN_VBO_MIN_LIMIT");
    if (env) {
      vbo_vertex_count_min_limit = atoi(env);
    }
    else {
      vbo_vertex_count_min_limit = 40;
    }
  } 
  return vbo_vertex_count_min_limit;
}

/*!
  Returns the vertex VBO maximum limit.

  \sa setVertexCountLimits()
 */
int 
SmVBO::getVertexCountMaxLimit(void)
{
  if (vbo_vertex_count_max_limit < 0) {
    const char * env = coin_getenv("COIN_VBO_MAX_LIMIT");
    if (env) {
      vbo_vertex_count_max_limit = atoi(env);
    }
    else {
      vbo_vertex_count_max_limit = 10000000;
    }
  }
  return vbo_vertex_count_max_limit;
}

SbBool 
SmVBO::shouldCreateVBO(const uint32_t contextid, const int numdata)
{
  int minv = SmVBO::getVertexCountMinLimit();
  int maxv = SmVBO::getVertexCountMaxLimit();
  return (numdata >= minv) && (numdata <= maxv) && SmVBO::isVBOFast(contextid);
}


SbBool 
SmVBO::isVBOFast(const uint32_t contextid)
{
  return TRUE;
}

//
// callback from glglue (when a new glglue instance is created)
//
void 
SmVBO::context_created(const cc_glglue * glue, void * closure)
{
  // SmVBO::testGLPerformance(coin_glglue_get_contextid(glue));
}

#endif // temporary compile fix













