/**************************************************************************\
 *
 *  Copyright (C) 1998-2000 by Systems in Motion.  All rights reserved.
 *
 *  Systems in Motion AS, Prof. Brochs gate 6, N-7030 Trondheim, NORWAY
 *  http://www.sim.no/ sales@sim.no Voice: +47 22114160 Fax: +4722207097
 *
\**************************************************************************/

#include "ViewportRegion.h"
#include <Inventor/elements/SoViewportRegionElement.h>
#include <Inventor/actions/SoGLRenderAction.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif // HAVE_WINDOWS_H

#include <GL/gl.h>

/*!
  \class ViewportRegion ViewportRegion.h Inventor/nodes/ViewportRegion.h
  \brief The ViewportRegion class is used to specify a sub-viewport.
  \ingroup nodes
*/

/*!
  \var SoSFVec2f ViewportRegion::origin
  Origin of new viewport. (0,0) is lower left corner of current viewport. (1,1) is upper right.
  Default value is (0,0). The field last written into of origin and pixelOrigin will be 
  used to set the origin when the node is traversed.

  \sa pixelOrigin
*/

/*!
  \var SoSFVec2f ViewportRegion::size
  Size of new viewport. (1,1) will create a viewport with the same size as the current.
  Default value is (1,1). The field last written into of size and pixelSize will be 
  used to set the size when the node is traversed.

  \sa pixelSize
*/

/*!
  \var SoSFVec2f ViewportRegion::pixelOrigin
  Can be used to set origin directly, in pixels from lower left corner of viewport window.
  Default value is (0,0).

  \sa origin
*/

/*!
  \var SoSFVec2f ViewportRegion::pixelSize
  Can be used to set size directly, in pixels.
  Default value is (128,128).
  
  \sa size
*/

/*!
  \var SoSFBool ViewportRegion::clearDepthBuffer
  Set to TRUE to clear depth buffer for the new viewport when node is traversed.
*/

/*!
  \var SoSFBool ViewportRegion::clearColorBuffer
  Set to TRUE to clear color buffer for the new viewport when the node is traversed.
*/

/*!
  \var SoSFColor ViewportRegion::clearColor
  The color used when clearing color buffer. Default is (0,0,0).
*/

/*!
  \var SoSFBool ViewportRegion::flipY
  Invert the y coordinate. Normally y=0 is at the bottom, setting this to TRUE will make y=0 at the top.
*/

SO_NODE_SOURCE(ViewportRegion);

/*!
  Constructor.
*/
ViewportRegion::ViewportRegion()
{
  SO_NODE_CONSTRUCTOR(ViewportRegion);

  SO_NODE_ADD_FIELD(origin, (0.0f, 0.0f));
  SO_NODE_ADD_FIELD(size, (1.0f, 1.0f));
  SO_NODE_ADD_FIELD(pixelSize, (1.0f, 1.0f));
  SO_NODE_ADD_FIELD(pixelOrigin, (0.0f, 0.0f));
  SO_NODE_ADD_FIELD(clearColorBuffer, (FALSE));
  SO_NODE_ADD_FIELD(clearDepthBuffer, (FALSE));
  SO_NODE_ADD_FIELD(clearColor, (0.0f, 0.0f, 0.0f));
  SO_NODE_ADD_FIELD(flipY, (FALSE));
  SO_NODE_ADD_FIELD(clampSize, (FALSE));

  this->usepixelsize = FALSE;
  this->usepixelorigin = FALSE;
}

/*!
  Destructor.
*/
ViewportRegion::~ViewportRegion()
{
}

/*!
  Required Coin method.
*/
void
ViewportRegion::initClass()
{
  SO_NODE_INIT_CLASS(ViewportRegion, SoNode, "Node");
}

/*!
  Generic traversal method for this node.
*/
void
ViewportRegion::doAction(SoAction * action)
{
  SoState * state = action->getState();
  SbViewportRegion vp = SoViewportRegionElement::get(state);
  SbVec2s winsize = vp.getWindowSize();

  SbVec2f siz = this->pixelSize.getValue();

  if (this->usepixelsize) {
    siz[0] /= float(winsize[0]);
    siz[1] /= float(winsize[1]);
  }
  else {
    siz = this->size.getValue();
  }

  SbVec2f org = this->pixelOrigin.getValue();
  if (this->usepixelorigin) {
    org[0] /= float(winsize[0]);
    org[1] /= float(winsize[1]);
  }
  else {
    org = this->origin.getValue();
  }
    
  if (siz[0] < 0.0f) siz[0] = 0.0f;
  else if (siz[0] > 1.0f) siz[0] = 1.0f;
  if (siz[1] < 0.0f) siz[1] = 0.0f;
  else if (siz[1] > 1.0f) siz[1] = 1.0f;

  if (this->clampSize.getValue()) {
    if (org[0] + siz[0] > 1.0f) siz[0] = 1.0f - org[0];
    if (org[1] + siz[1] > 1.0f) siz[1] = 1.0f - org[1];
  }
  else {
    if (org[0] + siz[0] > 1.0f) org[0] = 1.0f - siz[0];
    if (org[1] + siz[1] > 1.0f) org[1] = 1.0f - siz[1];
  }
  if (org[0] < 0.0f) org[0] = 0.0f;
  if (org[1] < 0.0f) org[1] = 0.0f;

  // write clamped values back into fields
  SbBool nsize, norigin, npsize, nporigin;
  nsize = this->size.enableNotify(FALSE);
  norigin = this->origin.enableNotify(FALSE);
  npsize = this->pixelSize.enableNotify(FALSE);
  nporigin = this->pixelOrigin.enableNotify(FALSE);

  //  this->size = siz;
  this->origin = org;
  
  // this->pixelSize = SbVec2f(siz[0]*float(winsize[0]), siz[1]*float(winsize[1]));
  this->pixelOrigin = SbVec2f(org[0]*float(winsize[0]), org[1]*float(winsize[1]));

  this->size.enableNotify(nsize);
  this->origin.enableNotify(norigin);
  this->pixelSize.enableNotify(npsize);
  this->pixelOrigin.enableNotify(nporigin);

  if (this->flipY.getValue()) {
    org[1] = 1.0f - org[1];
    org[1] -= siz[1];
  }

  vp.setViewport(org, siz);
  SoViewportRegionElement::set(action->getState(), vp);
  if (action->isOfType(SoGLRenderAction::getClassTypeId())) {
    GLenum mask = 0;
    if (this->clearDepthBuffer.getValue()) mask |= GL_DEPTH_BUFFER_BIT;
    if (this->clearColorBuffer.getValue()) {
      mask |= GL_COLOR_BUFFER_BIT;
      glClearColor(this->clearColor.getValue()[0],
                   this->clearColor.getValue()[1],
                   this->clearColor.getValue()[2],
                   0.0f);
    }
    if (mask) {
      glScissor(vp.getViewportOriginPixels()[0],
                vp.getViewportOriginPixels()[1],
                vp.getViewportSizePixels()[0],
                vp.getViewportSizePixels()[1]);
      glEnable(GL_SCISSOR_TEST);
      glClear(mask);
      glDisable(GL_SCISSOR_TEST);
    }
  }
}

/*!
  Coin method. Updates SoGLViewportElement.
*/
void
ViewportRegion::GLRender(SoGLRenderAction * action)
{
  ViewportRegion::doAction((SoAction*)action);
}

/*!
  Coin method. Updates SoViewportElement.
*/
void
ViewportRegion::getMatrix(SoGetMatrixAction * action)
{
  ViewportRegion::doAction((SoAction*)action);
}

/*!
  Coin method. Updates SoViewportElement.
*/
void
ViewportRegion::handleEvent(SoHandleEventAction * action)
{
  ViewportRegion::doAction((SoAction*)action);
}

/*!
  Coin method. Updates SoViewportElement.
*/
void
ViewportRegion::pick(SoPickAction * action)
{
  ViewportRegion::doAction((SoAction*)action);
}

void
ViewportRegion::notify(SoNotList * list)
{
  SoField *f = list->getLastField();
  if (f == &this->pixelSize) {
    this->usepixelsize = TRUE;
  }
  else if (f == &this->size) {
    this->usepixelsize = FALSE;
  }
  else if (f == &this->pixelOrigin) {
    this->usepixelorigin = TRUE;
  }
  else if (f == &this->origin) {
    this->usepixelorigin = FALSE;
  }
  SoNode::notify(list);
}
