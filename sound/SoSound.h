#ifndef COIN_SOSOUND_H
#define COIN_SOSOUND_H

#include <Inventor/nodes/SoSubNode.h>
#include <Inventor/fields/SoSFVec3f.h>
#include <Inventor/fields/SoSFFloat.h>
#include <Inventor/fields/SoSFNode.h>
#include <Inventor/fields/SoSFBool.h>

#include <SmallChange/actions/SoAudioRenderAction.h>

class SoSound : 
  public SoNode
{
  typedef SoNode inherited;
  SO_NODE_HEADER(SoSound);

  friend class SoAudioRenderAction;

public:

  static void initClass();
  SoSound();

  SoSFVec3f  direction;
  SoSFFloat  intensity;
  SoSFVec3f  location;
  SoSFFloat  maxBack;
  SoSFFloat  maxFront;
  SoSFFloat  minBack;
  SoSFFloat  minFront;
  SoSFFloat  priority;
  SoSFNode   source;
  SoSFBool   spatialize;

  static void setDefaultBufferingProperties(int bufferLength, int numBuffers, SbTime sleepTime);
  void setBufferingProperties(int bufferLength, int numBuffers, SbTime sleepTime);
  void getBufferingProperties(int &bufferLength, int &numBuffers, SbTime &sleepTime);

protected:

  virtual void audioRender(SoAudioRenderAction *action);

protected:

  class SoSoundP *sosound_impl;
  friend class SoSoundP;

private:

  virtual ~SoSound();

};

#endif // COIN_SOSOUND_H
