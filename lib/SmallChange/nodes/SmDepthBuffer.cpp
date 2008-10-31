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
  \class SmDepthBuffer SmDepthBuffer.h
  \brief The SmDepthBuffer class is a node used to control the GL depth buffer.
  \ingroup nodes

  It is basically a direct mapping of glDepthFunc(). In addition it is possible
  to enable/disable depth buffer and clear the depth buffer when the node is
  traversed.
*/

#include "SmDepthBuffer.h"
#include <Inventor/actions/SoGLRenderAction.h>

#if HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#ifdef __COIN__
#include <Inventor/system/gl.h>
#else // SGI/TGS Inventor
#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif // HAVE_WINDOWS_H
#include <GL/gl.h>
#endif // SGI/TGS Inventor

/*!
  \enum SmDepthBuffer::Func
  Enumeration for the various depth functions.
*/

/*!
  \var SmDepthBuffer::Func SmDepthBuffer::NEVER
  Never passes.
*/

/*!
  \var SmDepthBuffer::Func SmDepthBuffer::ALWAYS
  Always passes.
*/

/*!
  \var SmDepthBuffer::Func SmDepthBuffer::LESS
  Passes if the incoming depth value is less than the stored depth value.
*/

/*!
  \var SmDepthBuffer::Func SmDepthBuffer::LEQUAL
  Passes if the incoming depth value is less than or equal to the stored depth value.
*/

/*!
  \var SmDepthBuffer::Func SmDepthBuffer::EQUAL
  Passes if the incoming depth value is equal to the stored depth value.
*/

/*!
  \var SmDepthBuffer::Func SmDepthBuffer::GEQUAL
  Passes if the incoming depth value is greater than or equal to the stored depth value.
*/

/*!
  \var SmDepthBuffer::Func SmDepthBuffer::GREATER
  Passes if the incoming depth value is greater than the stored depth value.
*/

/*!
  \var SmDepthBuffer::Func SmDepthBuffer::NOTEQUAL
  Passes if the incoming depth value is not equal to the stored depth value.
*/

/*!
  \var SoSFEnum SmDepthBuffer::func

  Which depth function to use. Defaults to LESS.
*/

/*!
  \var SoSFBool SmDepthBuffer::enable

  Enable depth buffer writes. Defaults to TRUE.
*/

/*!
  \var SoSFFloat SmDepthBuffer::clearNow

  If TRUE, clear buffer when node is traversed. Default is FALSE.
*/


SO_NODE_SOURCE(SmDepthBuffer);

/*!
  Constructor.
*/
SmDepthBuffer::SmDepthBuffer(void)
{
  SO_NODE_CONSTRUCTOR(SmDepthBuffer);

  SO_NODE_ADD_FIELD(func, (SmDepthBuffer::LESS));
  SO_NODE_ADD_FIELD(enable, (TRUE));
  SO_NODE_ADD_FIELD(clearNow, (FALSE));

  SO_NODE_DEFINE_ENUM_VALUE(Func, NEVER);
  SO_NODE_DEFINE_ENUM_VALUE(Func, ALWAYS);
  SO_NODE_DEFINE_ENUM_VALUE(Func, LESS);
  SO_NODE_DEFINE_ENUM_VALUE(Func, LEQUAL);
  SO_NODE_DEFINE_ENUM_VALUE(Func, EQUAL);
  SO_NODE_DEFINE_ENUM_VALUE(Func, GEQUAL);
  SO_NODE_DEFINE_ENUM_VALUE(Func, GREATER);
  SO_NODE_DEFINE_ENUM_VALUE(Func, NOTEQUAL);
  SO_NODE_SET_SF_ENUM_TYPE(func, Func);
}

/*!
  Destructor.
*/
SmDepthBuffer::~SmDepthBuffer()
{
}

/*!
  Required Coin method.
*/
void
SmDepthBuffer::initClass(void)
{
  static int first = 1;
  if (first) {
    first = 0;
    GLDepthBufferElement::initClass();
    SO_NODE_INIT_CLASS(SmDepthBuffer, SoNode, "Node");
    SO_ENABLE(SoGLRenderAction, GLDepthBufferElement);
  }
}

/*!
  Coin method.
*/
void
SmDepthBuffer::GLRender(SoGLRenderAction * action)
{
  GLDepthBufferElement::set(action->getState(),
                            (GLDepthBufferElement::Func) this->func.getValue(),
                            this->enable.getValue());
  if (this->clearNow.getValue()) glClear(GL_DEPTH_BUFFER_BIT);
}