/**************************************************************************\
 *
 *  Copyright (C) 1998-2000 by Systems in Motion. All rights reserved.
 *
 *  Systems in Motion, Prof Brochs gate 6, 7030 Trondheim, NORWAY
 *  http://www.sim.no support@sim.no Voice: +47 22114160 Fax: +47 22207097
 *
\**************************************************************************/

/*!
  \class Rot2Heading Rot2Heading.h
  \brief Rot2Heading is a class for transforming a rotation into a vector.

  It is a useful engine, since it can, for instance, be used to
  create a headlight in a scene graph by connecting the camera
  orientation to the input, and a directional light direction to the
  output.
*/

/*!
  \var SoSFBool Rot2Heading::inverse
  Set to \e TRUE if the rotation should be inverted.
*/

/*!
  \var SoMFRotation Rot2Heading::rotation
  The rotation input.
*/

/*!  
  \var SoEngineOutput Rot2Heading::heading
  (SoSFVec3f) The rotated vector.
*/


#include "Rot2Heading.h"

SO_ENGINE_SOURCE(Rot2Heading);

// doc from parent
void
Rot2Heading::initClass(void)
{
  SO_ENGINE_INIT_CLASS(Rot2Heading, SoEngine, "Engine");
}

/*!
  Constructor.
*/
Rot2Heading::Rot2Heading(void)
{
  SO_ENGINE_CONSTRUCTOR(Rot2Heading);
  
  SO_ENGINE_ADD_INPUT(rotation, (SbRotation::identity()));
  SO_ENGINE_ADD_INPUT(inverse, (FALSE));

  SO_ENGINE_ADD_OUTPUT(heading, SoMFVec3f);  
}

/*!
  Destructor.
*/
Rot2Heading::~Rot2Heading()
{
}

// doc from parent
void
Rot2Heading::evaluate(void)
{
  int num = this->rotation.getNum();
  SO_ENGINE_OUTPUT(heading, SoMFVec3f, setNum(num));
  
  SbVec3f tmp;
  for (int i = 0; i < num; i++) {
    tmp.setValue(0.0f, 0.0f, -1.0f);
    if (this->inverse.getValue()) {
      this->rotation[i].inverse().multVec(tmp, tmp);
    }
    else {
      this->rotation[i].multVec(tmp, tmp);
    }
    SO_ENGINE_OUTPUT(heading, SoMFVec3f, set1Value(i, tmp));
  }
}
