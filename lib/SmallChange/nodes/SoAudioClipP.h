#ifndef SMALLCHANGE_SOAUDIOCLIPP_H
#define SMALLCHANGE_SOAUDIOCLIPP_H

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

#ifndef DOXYGEN_SKIP_THIS

#include <Inventor/sensors/SoFieldSensor.h>

class SoAudioClipP
{
public:
  SoAudioClipP(SoAudioClip * interfaceptr) : ifacep(interfaceptr) {};
  SoAudioClip *ifacep;

  double duration; // duration at pitch = 1.0
  ALuint bufferId;
  int readstatus;
  SoFieldSensor * urlsensor;
  static void urlSensorCBWrapper(void *, SoSensor *);
  void urlSensorCB(SoSensor *);
  SbBool playedOnce;
};

#endif // !DOXYGEN_SKIP_THIS
#endif // !SMALLCHANGE_SOAUDIOCLIPP_H
