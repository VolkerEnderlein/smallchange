#ifndef COIN_SHAPESCALE_H
#define COIN_SHAPESCALE_H

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

#include <Inventor/nodekits/SoSubKit.h>
#include <Inventor/nodekits/SoBaseKit.h>
#include <Inventor/fields/SoSFFloat.h>
#include <Inventor/fields/SoSFBool.h>

class SbViewport;
class SoState;
class SbColor;
class SbVec2s;

class ShapeScale : public SoBaseKit {
  typedef SoBaseKit inherited;

  SO_KIT_HEADER(ShapeScale);

  SO_KIT_CATALOG_ENTRY_HEADER(topSeparator);
  SO_KIT_CATALOG_ENTRY_HEADER(scale);
  SO_KIT_CATALOG_ENTRY_HEADER(shape);

public:
  ShapeScale(void);

  static void initClass(void);

public:

  SoSFFloat active;
  SoSFFloat projectedSize;
  SoSFBool threadSafe;

  void preRender(SoAction * action);

protected:
  virtual void GLRender(SoGLRenderAction * action);
  virtual ~ShapeScale();
  
private:

};

#endif // ! SHAPESCALE_H
