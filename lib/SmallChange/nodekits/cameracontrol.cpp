#include "cameracontrol.h"

#include <Inventor/SbLinear.h>
#include <Inventor/SbTime.h>
#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <Inventor/nodes/SoOrthographicCamera.h>
#include <SmallChange/nodes/UTMCamera.h>
#include <Inventor/sensors/SoTimerSensor.h>

static void cam_set_pos(SoCamera * camera, 
                        const SbVec3d & position)
{
  camera->isOfType(UTMCamera::getClassTypeId()) ?
    ((UTMCamera *)camera)->utmposition.setValue(position) :
    camera->position.setValue(SbVec3f(position[0], position[1], position[2]));
}

class SeekData {
public:
  SeekData() {
    this->seeking = FALSE;
    this->distance = 50.0f;
    this->seektime= 2.0f;
    this->sensor = new SoTimerSensor(SeekData::seeksensorCB, this);
  }
  
  ~SeekData() {
    delete this->sensor;
  }

  static void seeksensorCB(void * closure, SoSensor * sensor)
  {
    SeekData * thisp = (SeekData *) closure;
    SbTime currenttime = SbTime::getTimeOfDay();
    SoTimerSensor * timersensor = (SoTimerSensor *) sensor;
    
    SbTime dt = currenttime - timersensor->getBaseTime();
    double t = double(dt.getValue()) / thisp->seektime;
    
    if (t > 1.0f) t = 1.0f;
    if (t + timersensor->getInterval().getValue() > 1.0f) t = 1.0f;
    
    SbBool end = (t == 1.0f);
    
    t = (1.0 - cos(M_PI*t)) * 0.5;
    
    thisp->camera->orientation = SbRotation::slerp(thisp->startorient,
                                                   thisp->endorient,
                                                   (float) t);

    cam_set_pos(thisp->camera, thisp->startpoint + (thisp->endpoint - thisp->startpoint) * t);
    
    if (end) {
      thisp->seeking = FALSE;
      thisp->sensor->unschedule();
    }
  }

  SoCamera * camera;
  SbVec3d startpoint;
  SbRotation startorient;
  SbVec3d endpoint;
  SbRotation endorient;
  
  float distance;
  float seektime;
  SoTimerSensor * sensor;
  SbBool seeking;
};

static SeekData seekdata;

void cam_reset_roll(SoCamera * camera, const SbVec3f & viewup)
{
  assert(camera);

  SbVec3f newy = viewup;
  if (newy == SbVec3f(0.0f, 0.0f, 0.0f)) return;

  SbMatrix camerarot;
  camerarot.setRotate(camera->orientation.getValue());
  
  SbVec3f Z;
  Z[0] = camerarot[2][0];
  Z[1] = camerarot[2][1];
  Z[2] = camerarot[2][2];
  
  if (fabs(Z.dot(newy)) > 0.99f) {
    // just give up
    return;
  }
  SbVec3f newx = newy.cross(Z);
  newy = Z.cross(newx);
  
  newx.normalize();
  newy.normalize();

  camerarot[0][0] = newx[0];
  camerarot[0][1] = newx[1];
  camerarot[0][2] = newx[2];
  
  camerarot[1][0] = newy[0];
  camerarot[1][1] = newy[1];
  camerarot[1][2] = newy[2];

  camera->orientation.enableNotify(FALSE);  
  camera->orientation = SbRotation(camerarot);
  camera->orientation.enableNotify(TRUE);
}

void cam_seek_to_point(SoCamera * camera,
                       const SbVec3d & endpoint,
                       const SbRotation & endorient,
                       const float seektime)
{
  if (seektime == 0.0f) {
    cam_set_pos(camera, endpoint);
    camera->orientation.setValue(endorient);
    return;
  }

  seekdata.camera = camera;
  seekdata.endpoint = endpoint;
  seekdata.endorient = endorient;
  seekdata.seektime = seektime;
  
  camera->isOfType(UTMCamera::getClassTypeId()) ?
    seekdata.startpoint = ((UTMCamera *)camera)->utmposition.getValue() :
    seekdata.startpoint.setValue(camera->position.getValue());

  seekdata.startorient = camera->orientation.getValue();

  if (seekdata.sensor->isScheduled()) {
    seekdata.sensor->unschedule();
  }
  
  seekdata.seeking = TRUE;
  seekdata.sensor->setBaseTime(SbTime::getTimeOfDay());
  seekdata.sensor->schedule();
}

void cam_seek_to_node(SoCamera * camera,
                      const float seektime)
{
  
}

SbBool cam_is_seeking(void)
{
  return seekdata.seeking;
}