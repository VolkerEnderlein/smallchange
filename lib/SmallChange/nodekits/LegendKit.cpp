/**************************************************************************\
 *
 *  This file is part of the SmallChange extension library for Coin.
 *  Copyright (C) 1998-2002 by Systems in Motion.  All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  version 2 as published by the Free Software Foundation.  See the
 *  file LICENSE.GPL at the root directory of this source distribution
 *  for more details.
 *
 *  If you desire to use SmallChange with software that is incompatible
 *  licensewise with the GPL, and / or you would like to take
 *  advantage of the additional benefits with regard to our support
 *  services, please contact Systems in Motion about acquiring a Coin
 *  Professional Edition License.  See <URL:http://www.coin3d.org> for
 *  more information.
 *
 *  Systems in Motion, Prof Brochs gate 6, 7030 Trondheim, NORWAY
 *  <URL:http://www.sim.no>, <mailto:support@sim.no>
 *
\**************************************************************************/

/*!
  \class LegendKit LegendKit.h
  \brief The LegendKit class is used to draw a simple legend....
  \ingroup nodekits

  The class is organized as a nodekit for conveniece. Most parts are public,
  but we recommend that normal users only change the backgroundMaterial, tickMaterial
  and extraNodes parts.
  
  The class should be initialized using initLegend(). This will initialize
  internal data and parts to display the legend correctly. The coordinate
  system used will be set up to range from (0,0) to the chosen size.

  This nodekit contains the follwing parts:

  \li topSeparator The separator that holds all subscene for this kit. Private.

  \li resetTransform Used to reset any transform that might be on the state

  \li viewport Used to set the viewport for the legend. Contains a ViewportRegion 
  node. This part is NULL by default. Set this part if you want to restrict the
  area the legend is drawn into.

  \li depthBuffer Used to disable depth buffer tests. Contains a DepthBuffer node.

  \li lightModel Used to disable lighting. Contains an SoLightModel node.

  \li camera Contains an SoOrthographicCamera, which defines the view volume
  for the shapes in the kit.

  \li position Can be used to move the legend from the lower left corner.
  The coordinate system of this node is one unit per pixel. This part
  is NULL by default, but a SoTranslation node can be inserted here.

  \li texture Its only purpose is to disable texturing. Contains an SoTexture2 node.

  \li shapeHints Contains an SoShapeHints node, and sets default shape hints. This
  is needed to render the shapes in this kit correctly.

  \li backgroundMaterial Might be used to change the background color. Contains an 
  SoMaterial node.

  \li backgroundShape Contains the shape (SoIndexedFaceSet) that renders the 
  background frame. This is rendered as four quads surrounding the image.
  Set this part to NULL if you don't want a background frame.

  \li imageSeparator A separator that holds nodes needed to render the image. Private.

  \li imageTransform Applies a transformation to the image to account for image offset.
  Will be set in LegendKit::initLegend().

  \li imageMaterial Is used to reset the material to default before the image is
  rendered. Contains a default SoMaterial node by default. Will only take
  effect if you render the image a a textured quad.

  \li imageSwitch Is used to switch between image rendered as a raw image 
  (whichChild = 0), or rendered as a textured quad (whichChild = 1). This
  is useful since some 3D hardware might render raw images very slowly.
  Default value is 0, and you can set this value using 
  Legendkit::useTextureNotImage().

  \li image Hold the raw image data for the legend as an SoImage node.

  \li textureGroup A group used to hold the texture image and shape.

  \li textureImage A one row texture used when rendering the image as a
  textured quad.
  
  \li textureShape An SoFaceSet that renders one quad.

  \li tickMaterial Can be used to set the material of the ticks and lines.
  In the default node, diffuse color is set to (0 0 0).

  \li renderCallbackLines Is used to render ticks and other lines using OpenGL.

  \li textMaterial Can be used to set the material of the text.
  This part is NULL by default, and the text will then be rendered in the same
  color as the ticks and lines.

  \li renderCallbackText Is used to render text using OpenGL.

  \li extraNodes Is NULL by default, but can be used to add geometry
  that can be rendered after all other geometry in this kit.
  Please note that the coordinate system in the legend is one
  unit per pixel. This means that the world position (32.0f, 32.0f)
  is at pixel position (32,32) in the legend viewport.
*/

/*!
  \var SoSFBool LegendKit::on
  Specifies whether the legend is enabled (visible) or not.
  Default value is TRUE.
*/


/*!
  \var SoSFFloat LegendKit::imageWidth
  The width of the legend image. Default value is 32.
*/

/*!
  \var SoSFFloat LegendKit::space
  The space (in pixels) between items in the legend. Default value
  is 6.0, which usually looks pretty good.
*/

/*!
  \var SoSFFloat LegendKit::smallTickSize
  The size of the small ticks, in pixels.
*/

/*!
  \var SoSFFloat LegendKit::bigTickSize
  The size of the big ticks, in pixels.
*/

/*!
  \var SoSFString LegendKit::tickValueFormat
  The format string used to convert the tick value into a string.
  Default value is %g.
*/

/*!
  \var SoSFFloat LegendKit::tickValueOffset
  The offset from the end of the big tick to the printed value.
  Default value is 2.
*/

/*!
  \var SoSFString LegendKit::description
  The description/title for the legend. Default value is an empty string.
  If you specify several strings, they will be printed one string per line.
*/


/*!
  \var SoSFBool LegendKit::descriptionOnTop
  Specifies whether description should be on top or at the bottom.
  Default value is TRUE.
*/

/*!
  \var SoSFBool LegendKit::delayedRender
  Specifies whether legend should be rendered using SoGLRenderAction::addDelayedPath().
  Default value is TRUE.
*/

/*!
  \var SoSFFloat LegendKit::topSpace
  The distance, in pixels, from the top of the viewport to the top of the legend.
  Default value is 0.0.
*/

/*!
  \var SoSFFloat LegendKit::discreteUseLower
  When calculating the discrete color between bigticks, use the lower
  value of that section (the value right above the previous bigtick),
  instead of the upper value (the value right below the current
  bigtick).
*/


#include "LegendKit.h"

#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/nodekits/SoNodeKitListPart.h>
#include <Inventor/nodes/SoOrthographicCamera.h>
#include <Inventor/nodes/SoImage.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoTexture2.h>
#include <Inventor/nodes/SoLightModel.h>
#include <Inventor/nodes/SoTranslation.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoPickStyle.h>
#include <Inventor/nodes/SoResetTransform.h>
#include <Inventor/nodekits/SoAppearanceKit.h>
#include <Inventor/nodes/SoImage.h>
#include <Inventor/nodes/SoComplexity.h>
#include <Inventor/nodes/SoCallback.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodes/SoTranslation.h>
#include <Inventor/nodes/SoFaceSet.h>
#include <Inventor/nodes/SoIndexedFaceSet.h>
#include <Inventor/bundles/SoMaterialBundle.h>
#include <Inventor/elements/SoViewportRegionElement.h>

#ifdef __COIN__
#include <Inventor/lists/SbList.h>
#else
#undef COIN_DLL_API
#define COIN_DLL_API
#include "SbList.h"
#endif

#include <SmallChange/nodes/DepthBuffer.h>
#include <string.h>
#include <float.h>

#include "bitmapfont.cpp" // the default font
#define FONT_HEIGHT 12
#define FONT_SPACE   4
#define FONT_WIDTH   8

#ifdef _WIN32
#include <windows.h> // *sigh* this is needed for gl.h.
#endif // _WIN32

#include <GL/gl.h>

#ifndef DOXYGEN_SKIP_THIS

// used to store private (hidden) data members
class LegendKitP {
public:
  LegendKitP(LegendKit * kit) : kit(kit) { }

  LegendKit * kit;
  SbVec2f size;
  SbVec2f imageoffset;
  SbVec2f imagesize;
  uint32_t imagealpha;
  SbBool needimageinit;
  SbBool needalphainit;
  SbBool recalcsize;
  SbBool imageenabled;
  SbBool backgroundenabled;
  
  SbBool discrete;
  uint32_t (*colorCB)(double, void*);
  uint32_t (*colorCB2)(double);

  void * colorCBdata;
  uint32_t getLineColor(const double nval);
  int getLineNumber(double nval);
  
  typedef struct {
    double nval;
    double tickval;
    SbString string;
    SbBool discretestringset;
    SbString discretestring;
  } legend_tick;

  SbList <legend_tick> bigtick;
  SbList <double> smalltick;
  SbBool usetexture;
  SbVec2s prevvpsize;

  void addBigTickSorted(const legend_tick & tick); 

  typedef struct {
    double uppernval;
    uint32_t color;
    SbBool colorset;
  } legend_discrete;
  SbList <legend_discrete> discretelist;

};

#endif // DOXYGEN_SKIP_THIS

// convenience define to access private data
#undef THIS // windoze defines this somewhere *sigh*
#define THIS this->pimpl

SO_KIT_SOURCE(LegendKit);

/*!
  Constructor.
*/
LegendKit::LegendKit(void) 
  : pimpl(NULL)
{
  SO_KIT_CONSTRUCTOR(LegendKit);

  SO_KIT_ADD_FIELD(on, (TRUE));
  SO_KIT_ADD_FIELD(imageWidth, (32.0f));  
  SO_KIT_ADD_FIELD(space, (6.0f));
  SO_KIT_ADD_FIELD(smallTickSize, (3.0f));
  SO_KIT_ADD_FIELD(bigTickSize, (6.0f));
  SO_KIT_ADD_FIELD(tickValueFormat, ("%g"));
  SO_KIT_ADD_FIELD(tickValueOffset, (2.0f));
  SO_KIT_ADD_FIELD(description, (""));
  SO_KIT_ADD_FIELD(descriptionOnTop, (TRUE));
  SO_KIT_ADD_FIELD(delayedRender, (TRUE));
  SO_KIT_ADD_FIELD(topSpace, (0.0f));
  SO_KIT_ADD_FIELD(discreteUseLower, (FALSE));
  SO_KIT_ADD_FIELD(threadSafe, (FALSE));
  
  // Note: we must use "" instead of , , to humour MS VisualC++ 6.

#if defined(__COIN__) // the logical (Coin) way to do it, order is not important (yay, mortene rulez!)

  SO_KIT_ADD_CATALOG_ENTRY(topSeparator, SoSeparator, FALSE, this, "", FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(resetTransform, SoResetTransform, FALSE, topSeparator, depthBuffer, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(depthBuffer, DepthBuffer, TRUE, topSeparator, lightModel, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(lightModel, SoLightModel, FALSE, topSeparator, camera, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(camera, SoOrthographicCamera, FALSE, topSeparator, position, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(position, SoTranslation, TRUE, topSeparator, texture, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(texture, SoTexture2, FALSE, topSeparator, shapeHints, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(shapeHints, SoShapeHints, FALSE, topSeparator, pickStyle, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(pickStyle, SoPickStyle, TRUE, topSeparator, backgroundMaterial, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(backgroundMaterial, SoMaterial, FALSE, topSeparator, backgroundShape, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(backgroundShape, SoIndexedFaceSet, FALSE, topSeparator, imageSeparator, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(imageSeparator, SoSeparator, FALSE, topSeparator, tickMaterial, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(imageTransform, SoTransform, FALSE, imageSeparator, imageMaterial, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(imageMaterial, SoMaterial, FALSE, imageSeparator, imageSwitch, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(imageSwitch, SoSwitch, FALSE, imageSeparator, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(imageGroup, SoGroup, FALSE, imageSwitch, textureGroup, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(imageTranslation, SoTranslation, FALSE, imageGroup, image, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(image, SoImage, FALSE, imageGroup, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(textureGroup, SoGroup, FALSE, imageSwitch, "", FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(textureQuality, SoComplexity, FALSE, textureGroup, textureImage, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(textureImage, SoTexture2, FALSE, textureGroup, textureShape, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(textureShape, SoFaceSet, FALSE, textureGroup, "", TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(tickMaterial, SoMaterial, FALSE, topSeparator, renderCallbackLines, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(renderCallbackLines, SoCallback, TRUE, topSeparator, textMaterial, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(textMaterial, SoMaterial, TRUE, topSeparator, renderCallbackText, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(renderCallbackText, SoCallback, TRUE, topSeparator, extraNodes, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(extraNodes, SoSeparator, TRUE, topSeparator, "", TRUE);

#else // the Inventor way of doing it (order is very important)

#define EMPTY \x0 // Inventor doesn't handle "" either...

  SO_KIT_ADD_CATALOG_ENTRY(topSeparator, SoSeparator, FALSE, this, EMPTY, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(extraNodes, SoSeparator, TRUE, topSeparator, EMPTY, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(renderCallbackText, SoCallback, TRUE, topSeparator, extraNodes, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(textMaterial, SoMaterial, TRUE, topSeparator, renderCallbackText, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(renderCallbackLines, SoCallback, TRUE, topSeparator, textMaterial, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(tickMaterial, SoMaterial, FALSE, topSeparator, renderCallbackLines, TRUE);

  SO_KIT_ADD_CATALOG_ENTRY(imageSeparator, SoSeparator, FALSE, topSeparator, tickMaterial, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(backgroundShape, SoIndexedFaceSet, FALSE, topSeparator, imageSeparator, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(backgroundMaterial, SoMaterial, FALSE, topSeparator, backgroundShape, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(pickStyle, SoPickStyle, TRUE, topSeparator, backgroundMaterial, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(shapeHints, SoShapeHints, FALSE, topSeparator, pickStyle, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(texture, SoTexture2, FALSE, topSeparator, shapeHints, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(position, SoTranslation, TRUE, topSeparator, texture, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(camera, SoOrthographicCamera, FALSE, topSeparator, position, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(lightModel, SoLightModel, FALSE, topSeparator, camera, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(depthBuffer, DepthBuffer, TRUE, topSeparator, lightModel, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(resetTransform, SoResetTransform, FALSE, topSeparator, position, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(imageSwitch, SoSwitch, FALSE, imageSeparator, EMPTY, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(imageMaterial, SoMaterial, FALSE, imageSeparator, imageSwitch, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(imageTransform, SoTransform, FALSE, imageSeparator, imageMaterial, TRUE);

  SO_KIT_ADD_CATALOG_ENTRY(textureGroup, SoGroup, FALSE, imageSwitch, EMPTY, FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(textureShape, SoFaceSet, FALSE, textureGroup, EMPTY, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(textureImage, SoTexture2, FALSE, textureGroup, textureShape, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(textureQuality, SoComplexity, FALSE, textureGroup, textureImage, FALSE);

  SO_KIT_ADD_CATALOG_ENTRY(imageGroup, SoGroup, FALSE, imageSwitch, textureGroup, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(image, SoImage, FALSE, imageGroup, EMPTY, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(imageTranslation, SoTranslation, FALSE, imageGroup, image, TRUE);

#undef EMPTY

#endif // OIV kit specification


  SO_KIT_INIT_INSTANCE();

  THIS = new LegendKitP(this); // private and hidden data members are stored here
  THIS->size.setValue(0.0f, 0.0f);
  THIS->needimageinit = FALSE;
  THIS->imagealpha = 255;
  THIS->needalphainit = FALSE;
  THIS->recalcsize = TRUE;
  THIS->colorCB = NULL;
  THIS->colorCB2 = NULL;
  THIS->discrete = FALSE;
  THIS->usetexture = FALSE;
  THIS->prevvpsize.setValue(-1,-1);
  THIS->imageenabled = TRUE;
  THIS->backgroundenabled = TRUE;

  // disable picking on geometry below
  SoPickStyle * ps = (SoPickStyle*) this->getAnyPart("pickStyle", TRUE);
  ps->style = SoPickStyle::UNPICKABLE;

  // initialize switch node
  this->useTextureNotImage(THIS->usetexture);

  // initialize tick material to black
  SoMaterial * mat = (SoMaterial*) this->getAnyPart("tickMaterial", TRUE);
  mat->diffuseColor.setValue(SbColor(0.0f, 0.0f, 0.0f));
  mat->transparency.setValue(0.0f);

  // initialize texture quality
  SoComplexity * cmplx = (SoComplexity*) this->getAnyPart("textureQuality", TRUE);
  cmplx->textureQuality = 0.01f; // avoid linear filtering and mipmaps

  // enable render callbacks to draw text and lines
  SoCallback * callb = (SoCallback*) this->getAnyPart("renderCallbackLines", TRUE);
  callb->setCallback(renderCBlines, this);
  callb = (SoCallback*) this->getAnyPart("renderCallbackText", TRUE);
  callb->setCallback(renderCBtext, this);

  // disable lighting
  SoLightModel * lm = (SoLightModel*)this->getAnyPart("lightModel", TRUE);
  lm->model = SoLightModel::BASE_COLOR;

  // turn off render and bbox caching for image separator 
  SoSeparator * imsep = (SoSeparator*) this->getAnyPart("imageSeparator", TRUE);
  imsep->renderCulling = SoSeparator::OFF;
  imsep->boundingBoxCaching = SoSeparator::OFF;

  // turn off render and bbox caching for top separator 
  SoSeparator * topsep = (SoSeparator*) this->getAnyPart("topSeparator", TRUE);
  topsep->renderCulling = SoSeparator::OFF;
  topsep->boundingBoxCaching = SoSeparator::OFF;

  // disable depth buffer
  DepthBuffer * db = (DepthBuffer*) this->getAnyPart("depthBuffer", TRUE);
  db->enable = FALSE;

  // avoid rounding errors
  SoTranslation * t = (SoTranslation*) this->getAnyPart("imageTranslation", TRUE);
  t->translation = SbVec3f(0.5f, 0.5f, 0.0f);
  
  // set texture material to 1,1,1
  mat = (SoMaterial*) this->getPart("imageMaterial", TRUE);
  mat->diffuseColor = SbColor(1.0f, 1.0f, 1.0f);
}

/*!
  Destructor.
*/
LegendKit::~LegendKit()
{
  delete THIS;
}

/*!
  Initializes this class. Call before using it.
*/
void
LegendKit::initClass(void)
{
  static int first = 1;
  if (first) {
    first = 0;
    DepthBuffer::initClass();
    SO_KIT_INIT_CLASS(LegendKit, SoBaseKit, "BaseKit");
  }
}

/*!  
  Sets the transparency for the image in the legend (the color
  bars).  Because the image might be drawn as an image, and not a
  texture, each pixel in the image has to be reset to the new alpha
  value the next time the legend is rendered.  
*/
void 
LegendKit::setImageTransparency(const float transparency)
{
  uint32_t alpha = (uint32_t) ((1.0f - transparency) * 255.0f);
  if (alpha != THIS->imagealpha) {
    THIS->imagealpha = alpha;
    THIS->needalphainit = TRUE;
    this->touch(); // trigger a redraw in the near future
  }
}

/*!
  Method needed for thread safe rendering. If multiple threads are used to
  render a scene graph containing this nodekit, you must set the threadSafe
  field to TRUE, and use an SoCallbackAction to call this method before 
  rendering the scene graph.
*/
void
LegendKit::preRender(SoAction * action)
{
  SoState * state = action->getState();
  if (THIS->size == SbVec2f(0.0f, 0.0f)) return; // not initialized
  if (THIS->needimageinit) this->initImage();
  if (THIS->needalphainit) this->fillImageAlpha();  
}

/*!
  Overloaded to (re)initialize image and other data before rendering.
*/
void 
LegendKit::GLRender(SoGLRenderAction * action)
{
  if (!this->on.getValue()) return;

  if (this->delayedRender.getValue() && !action->isRenderingDelayedPaths()) {
    action->addDelayedPath(action->getCurPath()->copy());
    return;
  }
  SoState * state = action->getState();
  if (!this->threadSafe.getValue()) this->preRender(action);
  inherited::GLRender(action);
}

void 
LegendKit::getBoundingBox(SoGetBoundingBoxAction * action)
{
  // this node hasn't got a bounding box
}


// some functions for finding the next power of two
static int 
cnt_bits(int val, int * highbit)
{
  int cnt = 0;
  *highbit = 0;
  while (val) {
    if (val & 1) cnt++;
    val>>=1;
    (*highbit)++;
  }
  return cnt;
}

static int 
next_power_of_two(int val)
{
  int highbit;
  if (cnt_bits(val, &highbit) > 1) {
    return 1<<highbit;
  }
  return val;  
}

/*!
  Will draw image data, based on callbacks or discrete data.
*/
void 
LegendKit::initImage(void)
{
  if (THIS->imagesize[0] <= 0 || THIS->imagesize[1] <= 0) return;
#ifdef LEGEND_DEBUG
  fprintf(stderr,"(re)initializing image\n");
#endif

  THIS->needimageinit = FALSE;
  THIS->needalphainit = FALSE;

  SoImage * image = (SoImage*) this->getAnyPart("image", TRUE);
  SbVec2f size, texsize;
  int nc;

  SbVec2s tmpsize;
  unsigned char * data = (unsigned char*) image->image.getValue(tmpsize, nc);
  size[0] = float(tmpsize[0]);
  size[1] = float(tmpsize[1]);
  
  SbBool didallocimage = FALSE;
  if (size != THIS->imagesize) {
    didallocimage = TRUE;
    data = new unsigned char[int(THIS->imagesize[0])*int(THIS->imagesize[1])*4];
    size = THIS->imagesize;
  } else {
    data = image->image.startEditing(tmpsize, nc);
  }
 
  SoTexture2 * tex = (SoTexture2*) this->getAnyPart("textureImage", TRUE);
  tex->enableNotify(FALSE);
  tex->model = SoTexture2::MODULATE;
  
  unsigned char * rowdata = (unsigned char *) tex->image.getValue(tmpsize, nc);
  texsize[0] = 2.0f;
  texsize[1] = float(tmpsize[1]);
  int texh = next_power_of_two((int)THIS->imagesize[1]);

  SbBool didalloctexture = FALSE;
  if (texsize != SbVec2f(2.0f, float(texh))) {
    didalloctexture = TRUE;
    rowdata = new unsigned char[texh*4*2];
    texsize = SbVec2f(2.0f, float(texh));
  }
  else {
    rowdata = (unsigned char *) tex->image.getValue(tmpsize, nc);
  }

  this->reallyInitImage(data, rowdata);

  if (didallocimage) {
    tmpsize[0] = short(size[0]);
    tmpsize[1] = short(size[1]);
    image->image.setValue(tmpsize, 4, data);
    delete[] data;
  }
  else image->image.finishEditing();
  if (didalloctexture) {
    tmpsize[0] = 2;
    tmpsize[1] = short(texh);
    tex->image.setValue(tmpsize, 4, rowdata);
    delete[] rowdata;
  }
  else tex->image.finishEditing();

  // we disabled notification to avoid an extra redraw because we changed
  // the images. When we get here we are in a GLRender() traversal anyway.
  image->enableNotify(TRUE);
  tex->enableNotify(TRUE);
}

/*!
  Fill image data into \a data.
*/
void 
LegendKit::reallyInitImage(unsigned char * data, unsigned char * rowdata)
{
  int w = (int)THIS->imagesize[0];
  int h = (int)THIS->imagesize[1];
  double delta = 1.0 / double(h);
  double val = 0.0;

  for (int y = 0; y < h; y++) {
    uint32_t col = THIS->getLineColor(val);
    unsigned char r  = col>>24;
    unsigned char g = (col>>16)&0xff;
    unsigned char b = (col>>8)&0xff;
    unsigned char a = THIS->imagealpha;
    
    if (rowdata) {
      *rowdata++ = r;
      *rowdata++ = g;
      *rowdata++ = b;
      *rowdata++ = a;
      *rowdata++ = r;
      *rowdata++ = g;
      *rowdata++ = b;
      *rowdata++ = a;
    }
    for (int x = 0; x < w; x++) {
      *data++ = r;
      *data++ = g;
      *data++ = b;
      *data++ = a;
    }
    val += delta;
  }
}

/*!
  Will fill in new alpha (transparency) value in image.
*/
void 
LegendKit::fillImageAlpha(void)
{
  if (THIS->imagesize[0] <= 0 || THIS->imagesize[1] <= 0) return;
  // FIXME: implement
}

/*!
  Sets the callback used to decide the color per line in the image.
*/
void 
LegendKit::setColorCB(uint32_t (*colorCB)(double, void*), void * userdata)
{
  THIS->colorCB2 = NULL;
  THIS->colorCB = colorCB;
  THIS->colorCBdata = userdata;
  this->touch(); // trigger redraw
}

void 
LegendKit::setColorCB(uint32_t (*colorCB)(double))
{
  THIS->colorCB = NULL;
  THIS->colorCB2 = colorCB;
  this->touch(); // trigger redraw
}

/*!
  Enable/disable legend image. Is enabled by default.
*/
void 
LegendKit::enableImage(const SbBool onoff)
{
  THIS->imageenabled = onoff;
  if (onoff) {
    // this will set switch to the correct value.
    this->useTextureNotImage(THIS->usetexture);
    this->enableBackground(THIS->backgroundenabled);
  }
  else {
    SoSwitch * sw = (SoSwitch*) this->getAnyPart("imageSwitch", TRUE);
    sw->whichChild = SO_SWITCH_NONE;
    
    // disable background also
    this->setPart("backgroundShape", NULL);  
  }
}

/*!
  Private static callback used to render lines.
  Simply calls LegendKit::render when the action is an
  SoGLRenderAction.
*/
void 
LegendKit::renderCBlines(void * userdata, SoAction * action)
{
  if (action->isOfType(SoGLRenderAction::getClassTypeId())) {
    ((LegendKit*)userdata)->render((SoGLRenderAction*)action, TRUE);
  }
}

/*!
  Private static callback used to render text.
  Simply calls LegendKit::render when the action is an
  SoGLRenderAction.
*/
void 
LegendKit::renderCBtext(void * userdata, SoAction * action)
{
  if (action->isOfType(SoGLRenderAction::getClassTypeId())) {
    ((LegendKit*)userdata)->render((SoGLRenderAction*)action, FALSE);
  }
}

/*!
  Renders ticks and text.
*/
void 
LegendKit::render(SoGLRenderAction * action, const SbBool lines)
{    
  SoMaterial * mat = (SoMaterial*) this->getAnyPart("tickMaterial", TRUE);
  if (!lines) {
    SoMaterial * tmp = (SoMaterial*) this->getAnyPart("textMaterial", FALSE);
    if (tmp) mat = tmp;
  }
  SbBool trsp = mat->transparency.getNum() > 1 || mat->transparency[0] > 0.0f;
  
  if (action->handleTransparency(trsp))
    return;
  
  SoMaterialBundle mb(action);
  mb.sendFirst(); // make sure we have the correct material/color

  if (lines) {
    this->renderLines(action);
  }
  else {
    this->renderText(action);
  }
}

/*!
  Renders ticks.
*/
void 
LegendKit::renderLines(SoGLRenderAction * action)
{
  float starty = THIS->imageoffset[1];
  double sizey = (double) THIS->imagesize[1];
  float startx = THIS->imageoffset[0] + THIS->imagesize[0];
  float ticksize = this->smallTickSize.getValue();

  if (THIS->imagesize[1] > 1.0f && THIS->imageenabled) {
    glBegin(GL_LINES);
    int i, n = THIS->smalltick.getLength();
    for (i = 0; i < n; i++) {
      float ypos = float(THIS->smalltick[i] * sizey) + starty; 
      glVertex3f(startx, ypos, 0.0f);
      glVertex3f(startx + ticksize, ypos, 0.0f);
    }

    n = THIS->bigtick.getLength();
    ticksize = this->bigTickSize.getValue();
    for (i = 0; i < n; i++) {
      LegendKitP::legend_tick tick = THIS->bigtick[i];
      float ypos = float(tick.nval * sizey) + starty; 
      glVertex3f(startx, ypos, 0.0f);
      glVertex3f(startx + ticksize, ypos, 0.0f);
    }
    glEnd();
    
    // render lines around image
    glBegin(GL_LINE_LOOP);
    glVertex3f(THIS->imageoffset[0], THIS->imageoffset[1], 0.0f);
    glVertex3f(THIS->imageoffset[0]+THIS->imagesize[0], THIS->imageoffset[1], 0.0f);
    glVertex3f(THIS->imageoffset[0]+THIS->imagesize[0], THIS->imageoffset[1]+THIS->imagesize[1], 0.0f);
    glVertex3f(THIS->imageoffset[0], THIS->imageoffset[1]+THIS->imagesize[1], 0.0f);
    glEnd();
  }

  // render lines around description
  int numtext = this->description.getNum();
  float border = this->space.getValue();
  float ypos = this->descriptionOnTop.getValue() ? 
    THIS->size[1] - border :
    numtext*(FONT_HEIGHT+FONT_SPACE) + border * 2.0f;
  float ysize = float(numtext) * float(FONT_HEIGHT+FONT_SPACE) + FONT_SPACE;
  glBegin(GL_LINE_LOOP);
  glVertex3f(border, ypos, 0.0f);
  glVertex3f(THIS->size[0] - border, ypos, 0.0f);
  glVertex3f(THIS->size[0] - border, ypos - ysize, 0.0f);
  glVertex3f(border, ypos - ysize, 0.0f);
  glEnd();
}

/*!
  Renders text.
*/
void 
LegendKit::renderText(SoGLRenderAction * action)
{
  int i;
  glPixelStorei(GL_UNPACK_ALIGNMENT,1);
  glPixelStorei(GL_PACK_ALIGNMENT,1);

  // first, render tick values
  int offsety = (int) (THIS->imageoffset[1]);
  int offsetx = (int) (THIS->imageoffset[0] + THIS->imagesize[0] + 
                       this->bigTickSize.getValue() + this->tickValueOffset.getValue()); 
  double sizey = (double) THIS->imagesize[1];

  double prevnval = 0.0;
  int xpos, ypos;
  const char * str = NULL;

  if (THIS->imagesize[1] > 1.0f && THIS->imageenabled) {
    for (i = 0; i < THIS->bigtick.getLength(); i++) {
      LegendKitP::legend_tick tick = THIS->bigtick[i];
      str = tick.string.getString();
      if (!THIS->discrete || !tick.discretestringset) {
        xpos = offsetx;
        ypos = offsety + int(sizey*tick.nval) - FONT_HEIGHT/2;
      }
      else {
        xpos = offsetx;
        ypos = offsety + int(sizey*(tick.nval + prevnval)*0.5) - FONT_HEIGHT/2;
        str = tick.discretestring.getString();
      }
      prevnval = tick.nval;
      this->renderString(str, xpos, ypos);
    }
  }
  
  // then, the description
  int numtext = this->description.getNum();
  ypos = this->descriptionOnTop.getValue() ? 
    int(THIS->size[1]) - (FONT_HEIGHT+FONT_SPACE) - int(this->space.getValue()) :
    (numtext-1)*(FONT_HEIGHT+FONT_SPACE) + int(2.0f*this->space.getValue());
  
  for (i = 0; i < numtext; i++) {
    if (this->description[i].getLength()) {
      const char * s = this->description[i].getString();
      this->renderString(s, 
                         int(this->space.getValue() * 2.0f),
                         ypos);
      ypos -= FONT_HEIGHT + FONT_SPACE; 
    }
  }
  // restore pack/unpack alignment to default values
  glPixelStorei(GL_UNPACK_ALIGNMENT,4);    
  glPixelStorei(GL_PACK_ALIGNMENT,4);    
}

/*!
  Renders a single string at the specified raster position.
*/
void 
LegendKit::renderString(const char * str, int xpos, int ypos)
{
  int len = strlen(str);
  const unsigned char * ustr = (const unsigned char*) str;
  for (int i = 0; i < len; i++) {
    if (ustr[i] >= 32) { // just in case?
      glRasterPos2i(xpos, ypos);
      glBitmap(FONT_WIDTH, FONT_HEIGHT, 0, 0, 0, 0, 
               (const GLubyte *) bitmapfont_data + 
               FONT_HEIGHT * bitmapfont_isolatin1_mapping[ustr[i]]);
    }
    xpos += FONT_WIDTH;
  }
}

/*!
  Clear all tick information.
*/
void 
LegendKit::clearTicks(void)
{
  THIS->smalltick.truncate(0);
  THIS->bigtick.truncate(0);
  this->touch(); // trigger redraw
}

/*!
  Adds a small tick at the normalized (0-1) position \a nval.
*/
void 
LegendKit::addSmallTick(double nval)
{
  // we don't care if these are sorted, just add
  THIS->smalltick.append(nval); 
  this->touch(); // trigger redraw
}

/*!
  Adds a big tick at the normalized (0-1) position \a nval.  Prints
  the value \a tickvalue to the right of the tick.  If \a discretetext
  is != NULL, this text will be used when in discrete mode, and will
  be printed between this tick and the previous tick.
*/
void 
LegendKit::addBigTick(double nval, double tickvalue, const SbString * discretetext)
{
  LegendKitP::legend_tick tick;

  char buf[1024];
  sprintf(buf, this->tickValueFormat.getValue().getString(), tickvalue); 
  tick.string = SbString(buf);

  tick.nval = nval;
  tick.tickval = tickvalue;
  tick.discretestringset = FALSE;
  if (discretetext) {
    tick.discretestringset = TRUE;
    tick.discretestring = *discretetext;
  }
  THIS->addBigTickSorted(tick);
  this->touch(); // trigger redraw
}

/*!
  \overload
*/
void 
LegendKit::addBigTick(double nval, const SbString & string, const SbString * discretetext)
{
  LegendKitP::legend_tick tick;
  tick.nval = nval;
  tick.tickval = 0.0;
  tick.string = string;
  tick.discretestringset = FALSE;
  if (discretetext) {
    tick.discretestringset = TRUE;
    tick.discretestring = *discretetext;
  }
  THIS->addBigTickSorted(tick);
  this->touch(); // trigger redraw
}

/*!
  By default the image is drawn as a raw GL image. Call this with \a onoff
  TRUE if you want to use a textured quad instead. This is much faster on
  most PC cards. Maybe a test for __linux__ and _WIN32 could be a good
  idea to enable this. On SGI and HP (AFAIK) platforms raw GL image drawing is
  pretty fast.
*/
void 
LegendKit::useTextureNotImage(const SbBool onoff)
{
  THIS->usetexture = onoff;
  if (!THIS->imageenabled) return;

  SoSwitch * sw = (SoSwitch*) this->getAnyPart("imageSwitch", TRUE);
  sw->whichChild = onoff ? 1 : 0;
  THIS->recalcsize = TRUE;
}

/*!
  Switch to discrete mode. If a callback is registered and one or
  more big ticks have been added, a call to this method will cause
  the discrete data to generated based on those values.
  For each big tick, the color value is found, and the discrete
  color for the area from the tick to the next big tick is set to 
  the color of the line right under the next big tick.
*/
void 
LegendKit::setDiscreteMode(const SbBool onoff)
{
  if (THIS->discrete == FALSE && onoff && (THIS->colorCB || THIS->colorCB2) && THIS->bigtick.getLength()) {
    THIS->discretelist.truncate(0);
    int n = THIS->bigtick.getLength();
    double delta = 1.0 / (THIS->recalcsize ? 512.0 : double(THIS->imagesize[1]));
    for (int i = 0; i < n; i++) {
      LegendKitP::legend_tick tick = THIS->bigtick[i];
      // no use adding discrete if we have a bigtick at nval == 0
      if (tick.nval > delta) {
#if 0 // we calculate the discrete color using the colorCB now
        uint32_t color = THIS->getLineColor(tick.nval - delta);
        this->addDiscreteColor(tick.nval, color);
#else
        this->addDiscreteColor(tick.nval);
#endif
      }
    }
  }
  THIS->discrete = onoff;
  THIS->needimageinit= TRUE;
  this->touch(); // force redraw
}

/*!  
  Adds a discrete color. The area from the previous discrete color
  added (or from 0 if this is the first one) to \a uppernval will be
  painted with \a color.
*/
void
LegendKit::addDiscreteColor(double uppernval, uint32_t color)
{
  LegendKitP::legend_discrete item;
  item.uppernval = uppernval;
  item.color = color;
  item.colorset = TRUE;

  int i = 0;
  int n = THIS->discretelist.getLength();
  while (i < n && THIS->discretelist[i].uppernval <= uppernval) i++;
  if (i < n) THIS->discretelist.insert(item, i);
  else THIS->discretelist.append(item);
  this->touch(); // trigger redraw
}

/*!
  Adds a discrete color. The area from the previous discrete color
  added (or from 0 if this is the first one) to \a uppernval will be
  painted with the color found from the color callback for
  the value \a uppernval or from the previous discrete uppernval
  if \a discreteUseLower is TRUE.
*/
void 
LegendKit::addDiscreteColor(double uppernval)
{
  LegendKitP::legend_discrete item;
  item.uppernval = uppernval;
  item.colorset = FALSE;

  int i = 0;
  int n = THIS->discretelist.getLength();
  while (i < n && THIS->discretelist[i].uppernval <= uppernval) i++;
  if (i < n) THIS->discretelist.insert(item, i);
  else THIS->discretelist.append(item);
  this->touch(); // trigger redraw
}


/*!
  Clears all data in this kit.
*/
void 
LegendKit::clearData(void)
{
  THIS->discretelist.truncate(0);
  this->clearTicks();
  THIS->recalcsize = TRUE;
  this->touch(); // trigger redraw
}

void 
LegendKit::initBackground(const SbBool force)
{
  // initialize background shape
  SoIndexedFaceSet * faceset = (SoIndexedFaceSet*) this->getAnyPart("backgroundShape", force);
  if (!faceset) return;
  SoVertexProperty * prop = (SoVertexProperty*) faceset->vertexProperty.getValue();
  if (!prop) {
    prop = new SoVertexProperty;
    faceset->vertexProperty = prop;
  }
  if (THIS->recalcsize) return; // delay

  prop->vertex.setNum(8);
  prop->vertex.set1Value(0, 0.0f, 0.0f, 0.0f);
  prop->vertex.set1Value(1, THIS->size[0], 0.0f, 0.0f);
  prop->vertex.set1Value(2, THIS->size[0], THIS->size[1], 0.0f);
  prop->vertex.set1Value(3, 0.0f, THIS->size[1], 0.0f);
  prop->vertex.set1Value(4, THIS->imageoffset[0], THIS->imageoffset[1], 0.0f);
  prop->vertex.set1Value(5, THIS->imageoffset[0]+THIS->imagesize[0], THIS->imageoffset[1], 0.0f);
  prop->vertex.set1Value(6, THIS->imageoffset[0]+THIS->imagesize[0], THIS->imageoffset[1]+THIS->imagesize[1], 0.0f);
  prop->vertex.set1Value(7, THIS->imageoffset[0], THIS->imageoffset[1]+THIS->imagesize[1], 0.0f);

  int32_t indices[] = {0,1,5,4,-1,1,2,6,5,-1,6,2,3,7,-1,0,4,7,3,-1};
  faceset->coordIndex.setNum(20);
  faceset->coordIndex.setValues(0, 20, indices);
}

void 
LegendKit::initTextureImage(void)
{
  // initialize faceset to use one-row texture for the second rendering method
  SoFaceSet * faceset = (SoFaceSet*) this->getAnyPart("textureShape", TRUE);

  SoVertexProperty * prop = (SoVertexProperty*) faceset->vertexProperty.getValue();
  if (!prop) {
    prop = new SoVertexProperty;
    faceset->vertexProperty = prop;
  }
  prop->vertex.setNum(4);
  prop->vertex.set1Value(0, 0.0f, 0.0f, 0.0f);
  prop->vertex.set1Value(1, THIS->imagesize[0], 0.0f, 0.0f);
  prop->vertex.set1Value(2, THIS->imagesize[0], THIS->imagesize[1], 0.0f);
  prop->vertex.set1Value(3, 0.0f, THIS->imagesize[1], 0.0f);

  // since GL textures always must be a power of two, adjust
  // texture coordinates to account for this.
  int texh = next_power_of_two(int(THIS->imagesize[1]));
  float topy = THIS->imagesize[1] / float(texh);
#ifdef LEGEND_DEBUG
  fprintf(stderr,"topy: %g\n", topy); 
#endif

  prop->texCoord.setNum(4);
  prop->texCoord.set1Value(0, SbVec2f(0.0f, 0.0f));
  prop->texCoord.set1Value(1, SbVec2f(1.0f, 0.0f));
  prop->texCoord.set1Value(2, SbVec2f(1.0f, topy));
  prop->texCoord.set1Value(3, SbVec2f(0.0f, topy));
  faceset->numVertices.setValue(4);
}

/*!
  Overloaded to recalculate stuff when necessary.
*/
void 
LegendKit::notify(SoNotList * list)
{
  if (THIS) {
    SoField *f = list->getLastField();
    if (f == &this->description || 
        f == &this->descriptionOnTop || 
        f == &this->imageWidth ||
        f == &this->bigTickSize ||
        f == &this->tickValueFormat ||
        f == &this->space ||
        f == &this->tickValueOffset) {
      THIS->recalcsize = TRUE;
    }
  }
  inherited::notify(list);
}

void 
LegendKit::recalcSize(SoState * state)
{
  int i;
  const SbViewportRegion & vp = SoViewportRegionElement::get(state);
  SbVec2s vpsize = vp.getViewportSizePixels();
  if (vpsize == THIS->prevvpsize && !THIS->recalcsize) return;
  
  THIS->needimageinit = TRUE; // ok, need to init image again
  float border = this->space.getValue();
  SbVec2f descsize(0.0f, 4.0f * border + this->description.getNum()*(FONT_HEIGHT+FONT_SPACE)+FONT_SPACE);
  for (i = 0; i < this->description.getNum(); i++) {
    float len = float(this->description[i].getLength()) * FONT_WIDTH;
    if (len > descsize[0]) descsize[0] = len;
  }
  descsize[0] += border*4.0f;
  
  float ticktextw = 0.0f;
  for (i = 0; i < THIS->bigtick.getLength(); i++) {
    LegendKitP::legend_tick tick = THIS->bigtick[i];
    float len = float(tick.string.getLength()) * FONT_WIDTH;
    if (len > ticktextw) ticktextw = len;
  }
  ticktextw += 2.0f * border + this->imageWidth.getValue() +
    this->bigTickSize.getValue() + this->tickValueOffset.getValue();
  
  THIS->size[0] = ticktextw > descsize[0] ? ticktextw : descsize[0];

  SbVec2f position(0.0f, 0.0f);
  SoTranslation * tnode = (SoTranslation*) this->getPart("position", FALSE); 
  if (tnode) {
    position[0] = tnode->translation.getValue()[0];
    position[1] = tnode->translation.getValue()[1];
  }
  
  THIS->size[1] = vpsize[1] - position[1] - this->topSpace.getValue();
  if (THIS->size[1] < descsize[1]+2.0f) THIS->size[1] = descsize[1]+2.0f;

#ifdef LEGEND_DEBUG
  fprintf(stderr,"(re)calcing size: %g %g\n", THIS->size[0], THIS->size[1]);
#endif
  THIS->imagesize[0] = this->imageWidth.getValue();
  THIS->imagesize[1] = THIS->size[1] - this->space.getValue() - descsize[1];

  if (THIS->imagesize[1] <= 0) {
    THIS->imagesize[1] = 1.0f; 
    this->setSwitchValue("imageSwitch", SO_SWITCH_NONE);
  }
  else if (!THIS->imageenabled) {
    this->setSwitchValue("imageSwitch", SO_SWITCH_NONE);
  }
  else if (THIS->usetexture) {
    this->setSwitchValue("imageSwitch", 1);
  }
  else {
    this->setSwitchValue("imageSwitch", 0);
  }

  THIS->imageoffset = this->descriptionOnTop.getValue() ?
    SbVec2f(this->space.getValue(),
            this->space.getValue()) :
    SbVec2f(this->space.getValue(), descsize[1]);

  // transform image to offset position
  SoTransform * trans = (SoTransform*) this->getAnyPart("imageTransform", TRUE);
  trans->translation = SbVec3f(float(THIS->imageoffset[0]), 
                               float(THIS->imageoffset[1]), 0.0f);
  
  
  float size[2];
  size[0] = float(vpsize[0]);
  size[1] = float(vpsize[1]);

  // set up orthographic camera to span from (0,0) to (vpsize[0], vpsize[1])
  SoOrthographicCamera * camera = (SoOrthographicCamera*) this->getAnyPart("camera", TRUE);
  camera->viewportMapping = SoCamera::LEAVE_ALONE;
  camera->position = SbVec3f(size[0]*0.5f, size[1]*0.5f,
                             2.0f);
  camera->orientation = SbRotation(SbVec3f(0.0f, 0.0f, 1.0f), 0.0f);
  camera->aspectRatio = float(vpsize[0]) / float(vpsize[1]);
  camera->nearDistance = 1.0f;
  camera->farDistance = 10.0f;
  camera->focalDistance = 2.0f;
  camera->height = float(vpsize[1]);

  THIS->recalcsize = FALSE;

  this->initBackground();  
  this->initTextureImage();
  
  THIS->prevvpsize = vpsize;
}

/*!
  Sets the color used when rendering ticks and other lines.
*/
void 
LegendKit::setTickAndLinesColor(const SbColor & color, const float transparency)
{
  SoMaterial * mat = (SoMaterial*) this->getPart("tickMaterial", TRUE);
  mat->diffuseColor = color;
  mat->transparency = transparency;
}

/*!
  Sets the color used when rendering text.
*/
void 
LegendKit::setTextColor(const SbColor & color, const float transparency)
{
  SoMaterial * mat = (SoMaterial*) this->getPart("textMaterial", TRUE);
  mat->diffuseColor = color;
  mat->transparency = transparency;
}

/*!
  Sets the position, in pixels from the lower left corner of the GL widget.
*/
void 
LegendKit::setPosition(const SbVec2s & pos)
{
  SoTranslation * t = (SoTranslation*) this->getPart("position", TRUE);
  t->translation = SbVec3f(pos[0], pos[1], 0.0f);
}

/*!
  Sets the background color.
*/
void 
LegendKit::setBackgroundColor(const SbColor & color, const float transparency)
{
  SoMaterial * mat = (SoMaterial*) this->getPart("backgroundMaterial", TRUE);
  mat->diffuseColor = color;
  mat->transparency = transparency;
}

/*!
  Enables or disables the Legend background.
*/
void
LegendKit::enableBackground(const SbBool onoff)
{
  THIS->backgroundenabled = onoff;
  if (!onoff) this->setPart("backgroundShape", NULL);
  else this->initBackground(TRUE);
}

// convenience
void 
LegendKit::setSwitchValue(const char * part, const int value)
{
  SoSwitch * sw = (SoSwitch*) this->getAnyPart(part, FALSE);
  if (sw && sw->whichChild.getValue() != value) {
    sw->whichChild = value;
  }
}

#undef THIS

// methods in LegendKitP


// returns the line color at nval.
uint32_t 
LegendKitP::getLineColor(const double nval)
{
  if (!this->discrete && this->colorCB) return (this->colorCB(nval, this->colorCBdata)&0xffffff00)|this->imagealpha;
  else if (!this->discrete && this->colorCB2) return (this->colorCB2(nval)&0xffffff00)|this->imagealpha;
  else if (this->discrete && this->discretelist.getLength()) { // discrete values
    int i = 0, n = this->discretelist.getLength();
    while (i < n-1 && this->discretelist[i].uppernval < nval) i++;
    if (this->discretelist[i].colorset) {
      return (this->discretelist[i].color&0xffffff00) | this->imagealpha;
    }
    else if (this->colorCB || this->colorCB2) {
      double val = this->discretelist[i].uppernval - DBL_EPSILON;
      if (this->kit->discreteUseLower.getValue()) {
        if (i > 0) val = this->discretelist[i-1].uppernval + DBL_EPSILON;
        else val = 0.0;
      }
      if (val < 0.0) val = 0.0;
      else if (val > 1.0) val = 1.0;
      if (this->colorCB) return (this->colorCB(val, this->colorCBdata)&0xffffff00)|this->imagealpha;
      else return (this->colorCB2(val)&0xffffff00)|this->imagealpha;
    }
  }
  return 0xffffff00|this->imagealpha; // should never happen...
}

// calculates line number in image based on the normalized value
int
LegendKitP::getLineNumber(double nval)
{
  return (int) (nval * double(this->imagesize[1]));
}

// adds a big tick sorted based on the normalized value
void 
LegendKitP::addBigTickSorted(const legend_tick & tick)
{
  int i = 0, n = this->bigtick.getLength();
  while (i < n && this->bigtick[i].nval <= tick.nval) i++;
  if (i < n) this->bigtick.insert(tick, i);
  else this->bigtick.append(tick);
}
