/**************************************************************************\
 *
 *  Copyright (C) 1998-2000 by Systems in Motion. All rights reserved.
 *
 *  Systems in Motion, Prof Brochs gate 6, 7030 Trondheim, NORWAY
 *  http://www.sim.no support@sim.no Voice: +47 22114160 Fax: +47 22207097
 *
\**************************************************************************/

#ifndef COIN_LEGENDKIT_H
#define COIN_LEGENDKIT_H
#include <Inventor/nodekits/SoSubKit.h>
#include <Inventor/nodekits/SoBaseKit.h>
#include <Inventor/fields/SoSFEnum.h>
#include <Inventor/fields/SoSFBool.h>
#include <Inventor/fields/SoSFVec2f.h>
#include <Inventor/fields/SoSFFloat.h>
#include <Inventor/fields/SoSFString.h>
#include <Inventor/fields/SoMFString.h>

#ifdef WIN32
#include <SoWinLeaveScope.h>
#endif

class SbViewport;
class SoState;
class SbColor;
class SbVec2s;

class LegendKit : public SoBaseKit {
  typedef SoBaseKit inherited;

  SO_KIT_HEADER(LegendKit);

  SO_KIT_CATALOG_ENTRY_HEADER(topSeparator);
  SO_KIT_CATALOG_ENTRY_HEADER(resetTransform);
  SO_KIT_CATALOG_ENTRY_HEADER(position);
  SO_KIT_CATALOG_ENTRY_HEADER(depthBuffer);
  SO_KIT_CATALOG_ENTRY_HEADER(lightModel);
  SO_KIT_CATALOG_ENTRY_HEADER(camera);
  SO_KIT_CATALOG_ENTRY_HEADER(texture);
  SO_KIT_CATALOG_ENTRY_HEADER(shapeHints);
  SO_KIT_CATALOG_ENTRY_HEADER(pickStyle);
  SO_KIT_CATALOG_ENTRY_HEADER(backgroundMaterial);
  SO_KIT_CATALOG_ENTRY_HEADER(backgroundShape);
  SO_KIT_CATALOG_ENTRY_HEADER(imageSeparator);
  SO_KIT_CATALOG_ENTRY_HEADER(imageTransform);
  SO_KIT_CATALOG_ENTRY_HEADER(imageMaterial);
  SO_KIT_CATALOG_ENTRY_HEADER(imageSwitch);     
  SO_KIT_CATALOG_ENTRY_HEADER(imageGroup);
  SO_KIT_CATALOG_ENTRY_HEADER(imageTranslation);
  SO_KIT_CATALOG_ENTRY_HEADER(image);
  SO_KIT_CATALOG_ENTRY_HEADER(textureGroup);
  SO_KIT_CATALOG_ENTRY_HEADER(textureQuality);
  SO_KIT_CATALOG_ENTRY_HEADER(textureImage);
  SO_KIT_CATALOG_ENTRY_HEADER(textureShape);
  SO_KIT_CATALOG_ENTRY_HEADER(tickMaterial);
  SO_KIT_CATALOG_ENTRY_HEADER(renderCallbackLines);
  SO_KIT_CATALOG_ENTRY_HEADER(textMaterial);
  SO_KIT_CATALOG_ENTRY_HEADER(renderCallbackText);
  SO_KIT_CATALOG_ENTRY_HEADER(extraNodes);

public:
  LegendKit(void);

  static void initClass(void);

public:

  SoSFBool on;
  SoSFFloat imageWidth;
  SoSFFloat space;
  SoSFFloat bigTickSize;
  SoSFFloat smallTickSize;
  SoSFString tickValueFormat;
  SoSFFloat tickValueOffset;
  SoMFString description;
  SoSFBool descriptionOnTop;
  SoSFBool delayedRender;
  SoSFFloat topSpace;
  SoSFBool discreteUseLower;

  void setImageTransparency(const float transparency = 0.0f);
  void useTextureNotImage(const SbBool onoff);
  void setColorCB(uint32_t (*colorCB)(double, void*), void * userdata = NULL);
  void setColorCB(uint32_t (*colorCB)(double));
  
  void clearTicks(void);
  void addSmallTick(double nval);
  void addBigTick(double nval, double tickvalue, const SbString * discretestring = NULL);
  void addBigTick(double nval, const SbString & string, const SbString * discretestring = NULL);

  void setDiscreteMode(const SbBool onoff);
  void addDiscreteColor(double uppernval, uint32_t color);
  void addDiscreteColor(double uppernval);
  
  void clearData(void);

  void enableImage(const SbBool onoff);

public:
  // convenience methods for setting part attributes
  void setTickAndLinesColor(const SbColor & color, const float transparency = 0.0f);
  void setTextColor(const SbColor & color, const float transparency = 0.0f);
  void setPosition(const SbVec2s & pos);
  void setBackgroundColor(const SbColor & color, const float transparency = 0.0f);
  void enableBackground(const SbBool onoff);

protected:
  virtual void GLRender(SoGLRenderAction * action);
  virtual void getBoundingBox(SoGetBoundingBoxAction * action);
  virtual ~LegendKit();
  virtual void notify(SoNotList * list);
  
private:

  void recalcSize(SoState * state);
  void initBackground(const SbBool force = FALSE);
  void initTextureImage(void);
  void initImage(void);
  void reallyInitImage(unsigned char * data, unsigned char * rowdata = NULL);
  void fillImageAlpha(void);
  static void renderCBlines(void * userdata, SoAction * action);
  static void renderCBtext(void * userdata, SoAction * action);
  void render(SoGLRenderAction * action, const SbBool lines);
  void renderLines(SoGLRenderAction * action);
  void renderText(SoGLRenderAction * action);
  void renderString(const char * str, int xpos, int ypos);

  void setSwitchValue(const char * part, const int value);

  class LegendKitP * pimpl;
  friend class LegendKitP;
};

#ifdef WIN32
#include <SoWinEnterScope.h>
#endif

#endif // !COIN_LEGENDKIT_H
