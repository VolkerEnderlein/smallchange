#ifndef COIN_FRUSTUM_CAMERA_H
#define COIN_FRUSTUM_CAMERA_H

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

#include <Inventor/nodes/SoSubNode.h>
#include <Inventor/nodes/SoCamera.h>
#include <Inventor/fields/SoSFFloat.h>

class FrustumCamera : public SoCamera {
  typedef SoCamera inherited;

  SO_NODE_HEADER(FrustumCamera);

public:
  static void initClass(void);
  FrustumCamera(void);

  SoSFFloat left;
  SoSFFloat right;
  SoSFFloat top;
  SoSFFloat bottom;

  virtual void scaleHeight(float scalefactor);
  virtual SbViewVolume getViewVolume(float useaspectratio = 0.0f) const;

protected:
  virtual ~FrustumCamera();
  virtual void viewBoundingBox(const SbBox3f & box, float aspect, float slack);  
};

#endif // COIN_FRUSTUM_CAMERA_H
