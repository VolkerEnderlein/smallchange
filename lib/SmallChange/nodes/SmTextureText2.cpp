/**************************************************************************\
 * Copyright (c) Kongsberg Oil & Gas Technologies AS
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 
 * Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
\**************************************************************************/

/*!
  \class SmTextureText2 SmallChange/nodes/SmTextureText2.h
  \brief The SmTextureText2 node is used for fast text rendering

  This node can render ~200k strings / second. It pregenerates a font
  texture which is used for rendering the text instead of using the
  glBitmap()/glPixmap() method.

  It can be used in two modes. If multiple 'position' values are specified,
  one string is rendered at each position. If a single 'position' value is supplied,
  the strings will be rendered vertically like the SoText2 node.

  This node also supports material binding and will pick materials from the
  state whenever the material binding != OVERALL.

  If the stringIndex field is not empty, it will be used to select
  strings to render. If the material binding != OVERALL, the index
  array will also be used to select the color.
*/

#include "SmTextureText2.h"
#include "SmTextureFont.h"
#include "SmTextureText2Collector.h"
#include <Inventor/misc/SoState.h>
#include <Inventor/SoPrimitiveVertex.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/system/gl.h>
#include <Inventor/actions/SoGetPrimitiveCountAction.h>
#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/bundles/SoMaterialBundle.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/elements/SoTextureQualityElement.h>
#include <Inventor/elements/SoLightModelElement.h>
#include <Inventor/elements/SoGLTextureImageElement.h>
#include <Inventor/elements/SoGLTextureEnabledElement.h>
#include <Inventor/elements/SoViewVolumeElement.h>
#include <Inventor/elements/SoShapeStyleElement.h>
#include <Inventor/elements/SoProjectionMatrixElement.h>
#include <Inventor/elements/SoViewingMatrixElement.h>
#include <Inventor/elements/SoViewportRegionElement.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/elements/SoLazyElement.h>
#include <Inventor/elements/SoMaterialBindingElement.h>
#include <Inventor/elements/SoGLTextureCoordinateElement.h>
#include <Inventor/details/SoTextDetail.h>
#include <Inventor/SoPickedPoint.h>
#include <Inventor/SbXfBox3f.h>
#include <Inventor/SbRotation.h>
#include <Inventor/misc/SoGLImage.h>
#include <Inventor/SbPlane.h>
#include <Inventor/SbLine.h>
#include <Inventor/C/tidbits.h>

#if COIN_DEBUG
#include <Inventor/errors/SoDebugError.h>
#endif // COIN_DEBUG

#include <climits>
#include <cstring>


SO_NODE_SOURCE(SmTextureText2);

/*!
  Constructor.
*/
SmTextureText2::SmTextureText2()
{
  SO_NODE_CONSTRUCTOR(SmTextureText2);

  SO_NODE_ADD_FIELD(string, (""));
  SO_NODE_ADD_FIELD(justification, (LEFT));
  SO_NODE_ADD_FIELD(verticalJustification, (BOTTOM));
  SO_NODE_ADD_FIELD(maxRange, (-1.0f));
  SO_NODE_ADD_FIELD(position, (0.0f, 0.0f, 0.0f));
  SO_NODE_ADD_FIELD(offset, (0.0f, 0.0f, 0.0f));
  SO_NODE_ADD_FIELD(rotation, (0.0f)); // TODO also make it work for SmTextureText2Collector
  SO_NODE_ADD_FIELD(pickOnPixel, (FALSE));
  SO_NODE_ADD_EMPTY_MFIELD(stringIndex);

  SO_NODE_DEFINE_ENUM_VALUE(Justification, CENTER);
  SO_NODE_DEFINE_ENUM_VALUE(Justification, LEFT);
  SO_NODE_DEFINE_ENUM_VALUE(Justification, RIGHT);

  SO_NODE_DEFINE_ENUM_VALUE(VerticalJustification, TOP);
  SO_NODE_DEFINE_ENUM_VALUE(VerticalJustification, BOTTOM);
  SO_NODE_DEFINE_ENUM_VALUE(VerticalJustification, VCENTER);

  SO_NODE_SET_SF_ENUM_TYPE(justification, Justification);
  SO_NODE_SET_SF_ENUM_TYPE(verticalJustification, VerticalJustification);
}

/*!
  Destructor.
*/
SmTextureText2::~SmTextureText2()
{
}

// doc from parent
void
SmTextureText2::initClass(void)
{
  if (getClassTypeId() == SoType::badType()) {
    SO_NODE_INIT_CLASS(SmTextureText2, SoShape, "Shape");
  }
}

int
SmTextureText2::getStrings(SoState *, const SbString * & strings) const
{
  const int num = this->string.getNum();
  strings = num > 0 ? this->string.getValues(0) : 0;
  return num;
}

int
SmTextureText2::getPositions(SoState *, const SbVec3f * & positions) const
{
  const int num = this->position.getNum();
  positions = num > 0 ? this->position.getValues(0) : 0;
  return num;
}

int
SmTextureText2::getRotations(SoState *, const float * & rotations) const
{
  const int num = this->rotation.getNum();
  rotations = num > 0 ? this->rotation.getValues(0) : 0;
  return num;
}

int
SmTextureText2::getStringIndices(SoState *, const int32_t * & indices) const
{
  const int num = this->stringIndex.getNum();
  indices = num > 0 ? this->stringIndex.getValues(0) : 0;
  return num;
}

// doc from parent
void
SmTextureText2::computeBBox(SoAction * action, SbBox3f & box, SbVec3f & center)
{
  SoState * state = action->getState();

  // never cull this node. We do quick culling in the render method
  SoCacheElement::invalidate(state);

  // this boundingbox will _not_ be 100% correct. We just supply an
  // estimate to avoid using lots of processing for calculating the
  // boundingbox.  FIXME: make it configurable if the bbox should be
  // accurate or not
  const SbVec3f * positions;
  const int numpositions = this->getPositions(state, positions);
  const SbVec3f & offset = this->offset.getValue();

  for (int i = 0; i < numpositions; i++) {
    box.extendBy(positions[i] + offset);
  }
  center = box.getCenter();
}

// doc from parent
void
SmTextureText2::GLRender(SoGLRenderAction * action)
{
  SoState * state = action->getState();
  
  const SbString * strings;
  const int numstrings = this->getStrings(state, strings);

  const SbVec3f * positions;
  const int numpositions = this->getPositions(state, positions);

  const float * rotations;
  const int numrotations = this->getRotations(state, rotations);

  const int32_t * indices;
  const int numindices = this->getStringIndices(state, indices);

  const int num = numindices > 0 ? numindices : numstrings;

  if ((numstrings == 0) || (numstrings == 1 && strings[0] == "") && numindices == 0) return;
  
  SbBool perpart =
    SoMaterialBindingElement::get(state) !=
    SoMaterialBindingElement::OVERALL;
  
  if (((numstrings == numpositions) || numindices > 0) &&
      SmTextureText2CollectorElement::isCollecting(state)) {
    SbMatrix modelmatrix = SoModelMatrixElement::get(state);
    const SbVec3f & offset = this->offset.getValue();
    SbVec3f pos;

    SbColor4f col(SoLazyElement::getDiffuse(state, 0),
                  1.0f - SoLazyElement::getTransparency(state, 0));
    
    for (int i = 0; i < num; i++) {
      const int idx = numindices > 0 ? indices[i] : i; 
      if (perpart) {
        col = SbColor4f(SoLazyElement::getDiffuse(state, idx),
                        1.0f - SoLazyElement::getTransparency(state, idx));
      }
      pos = positions[idx] + offset;
      modelmatrix.multVecMatrix(pos, pos);
      SmTextureText2CollectorElement::add(state,
                                          strings[idx],
                                          SmTextureFontElement::get(state),
                                          pos,
                                          this->maxRange.getValue(),
                                          col,
                                          static_cast<Justification>(this->justification.getValue()),
                                          static_cast<VerticalJustification>(this->verticalJustification.getValue()));
    }
    
    // invalidate caches to make sure this node is traversed every frame.
    SoCacheElement::invalidate(state);
    return;
  }
  
  SmTextureFontBundle bundle(action);
  SoCacheElement::invalidate(state);
  
  if (!this->shouldGLRender(action)) {
    return;
  }

  // set up my font texture
  SoLightModelElement::set(state, SoLightModelElement::BASE_COLOR);
  SoMaterialBundle mb(action);
  mb.sendFirst(); // make sure we have the correct material

  SbMatrix modelmatrix = SoModelMatrixElement::get(state);
  SbMatrix inv = modelmatrix.inverse();

  SbMatrix normalize(0.5f, 0.0f, 0.0f, 0.0f,
                     0.0f, 0.5f, 0.0f, 0.0f,
                     0.0f, 0.0f, 1.0f, 0.0f,
                     0.5f, 0.5f, 0.0f, 1.0f);
  SbMatrix projmatrix =
    modelmatrix *
    SoViewingMatrixElement::get(state) *
    SoProjectionMatrixElement::get(state) *
    normalize;

  const SbViewVolume & vv = SoViewVolumeElement::get(state);
  const SbViewportRegion & vp = SoViewportRegionElement::get(state);
  const SbVec2s vpsize = vp.getViewportSizePixels();
  const SbVec3f & offset = this->offset.getValue();

  SbVec3f tmp;

  // Set up new view volume
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glOrtho(0, vpsize[0], 0, vpsize[1], -1.0f, 1.0f);
  glPushAttrib(GL_DEPTH_BUFFER_BIT);
  glDepthFunc(GL_LEQUAL);

  if (numpositions > 1 || numindices > 0) {
    for (int i = 0; i < num; i++) {
      int idx = numindices > 0 ? indices[i] : i;
      float rotation = rotations[numrotations > 1 ? idx : 0];

      if (perpart) {
        mb.send(idx, FALSE);
      }
      tmp = positions[idx] + offset;
      this->renderString(bundle,
                         &strings[SbMin(idx, numstrings - 1)], 1,
                         tmp,
                         vv,
                         vp,
                         projmatrix,
                         modelmatrix,
                         inv,
                         rotation);
    }
  }
  else {
    tmp = numpositions > 0 ? positions[0] : SbVec3f(0.0f, 0.0f, 0.0f);
    tmp += offset;
    float rotation = rotations[0];
    this->renderString(bundle,
                       strings,
                       num,
                       tmp,
                       vv,
                       vp,
                       projmatrix,
                       modelmatrix,
                       inv,
                       rotation);
  }
  glPopAttrib();
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();

//#define DRAW_DEBUG_BBOXES
#ifdef DRAW_DEBUG_BBOXES
  glPushAttrib(GL_ALL_ATTRIB_BITS);
  glDisable(GL_TEXTURE_2D);
  glDepthFunc(GL_ALWAYS);
  for (int i = 0; i < num; i++){
    int idx = numindices > 0 ? indices[i] : i;
    SbVec3f p0, p1, p2, p3;
    this->buildStringQuad(action, idx, p0, p1, p2, p3);
    glBegin(GL_LINE_LOOP);
    glColor3f(1.0f, 1.0f, 1.0f);
    glVertex3fv((const GLfloat*)&p0);
    glVertex3fv((const GLfloat*)&p1);
    glVertex3fv((const GLfloat*)&p2);
    glVertex3fv((const GLfloat*)&p3);
    glEnd();
  }
  glPopAttrib();
#endif
}

void 
SmTextureText2::buildStringQuad(SoAction * action, int idx, SbVec3f & p0, SbVec3f & p1, SbVec3f & p2, SbVec3f & p3)
{
  //FIXME: Support multiple strings at one position (multiline text)

  SoState * state = action->getState();

  const SbString * strings;
  const int numstrings = this->getStrings(state, strings);

  const SbVec3f * positions;
  const int numpositions = this->getPositions(state, positions);

  const float * rotations;
  const int numrotations = this->getRotations(state, rotations);

  const SbVec3f & offset = this->offset.getValue();

  Justification halign = static_cast<Justification>(this->justification.getValue());
  VerticalJustification valign = static_cast<VerticalJustification>(this->verticalJustification.getValue());

  const SbViewVolume & vv = SoViewVolumeElement::get(state);
  const SbViewportRegion & vpr = SoViewportRegionElement::get(state);
  SbMatrix modelmatrix = SoModelMatrixElement::get(state);
  const SmTextureFont::FontImage * font = SmTextureFontElement::get(state);

  SbVec2s vpsize = vpr.getViewportSizePixels();
  float px = vpsize[0];
  float py = vpsize[1];

  SbVec3f world, screen;
  modelmatrix.multVecMatrix(positions[idx] + offset, world);
  vv.projectToScreen(world, screen);

  float up, down, left, right;

  float width = static_cast<float>(font->stringWidth(strings[idx]));
  switch (halign){
    case LEFT:
      right = width;
      left = 0;
      break;
    case CENTER:
      right = width / 2;
      left = -right;
      break;
    case RIGHT:
      right = 0.4f;
      left = -(width - 0.4f);
      break;
  }
  left /= px;
  right /= px;

  float ascent = static_cast<float>(font->getAscent());
  float descent = static_cast<float>(font->getDescent());
  float height = ascent + descent + 1;
  switch(valign){
    case TOP:
      up = 0;
      down = -height;
      break;
    case VCENTER:
      up = height / 2;
      down = -up;
      break;
    case BOTTOM://actually BASELINE
      up = ascent;
      down = -descent;
      break;
  }
  up /= py;
  down /= py;

  SbMatrix rotation = SbMatrix::identity();
  rotation.setRotate(SbRotation(SbVec3f(0, 0, 1), rotations[numrotations > 1 ? idx : 0]));

  SbMatrix translation = SbMatrix::identity();
  translation.setTranslate(SbVec3f(screen[0], screen[1], 0));

  //need to account for viewport aspect ratio as we are working in normalized screen coords:
  float aspectx = px >= py ? 1 : py / px;
  float aspecty = py >= px ? 1 : px / py;
  SbMatrix scale = SbMatrix::identity();
  scale.setScale(SbVec3f(aspectx, aspecty, 1));
  SbMatrix invScale = scale.inverse();

  //screen coords (offsets from text "anchor" point):
  SbVec3f offsets[4];
  offsets[0] = SbVec3f(left, down, 0);
  offsets[1] = SbVec3f(right, down, 0);
  offsets[2] = SbVec3f(right, up, 0);
  offsets[3] = SbVec3f(left, up, 0);

  SbVec2f screenPos[4];

  for (int i = 0; i < 4; i++){
    SbVec3f & offset = offsets[i];
    invScale.multVecMatrix(offset, offset);
    rotation.multVecMatrix(offset, offset);
    scale.multVecMatrix(offset, offset);
    translation.multVecMatrix(offset, offset);
    screenPos[i] = SbVec2f(offset[0], offset[1]);
  }

  float dist = -vv.getPlane(0.0f).getDistance(world);

  // find the four screen points in the plane
  p0 = vv.getPlanePoint(dist, screenPos[0]);
  p1 = vv.getPlanePoint(dist, screenPos[1]);
  p2 = vv.getPlanePoint(dist, screenPos[2]);
  p3 = vv.getPlanePoint(dist, screenPos[3]);

  // transform back to object space
  SbMatrix inv = modelmatrix.inverse();
  inv.multVecMatrix(p0, p0);
  inv.multVecMatrix(p1, p1);
  inv.multVecMatrix(p2, p2);
  inv.multVecMatrix(p3, p3);
}

void
SmTextureText2::rayPick(SoRayPickAction * action)
{
  //FIXME: Support multiple strings at one position (multiline text)?
  //For now, assumes 1 string at each position

  SoState * state = action->getState();

  const SbString * strings;
  const int numstrings = this->getStrings(state, strings);

  const int32_t * indices;
  const int numindices = this->getStringIndices(state, indices);

  const int num = numindices > 0 ? numindices : numstrings;

  this->computeObjectSpaceRay(action);
  const SmTextureFont::FontImage * font = SmTextureFontElement::get(state);
  SoMaterialBindingElement::Binding binding = SoMaterialBindingElement::get(state);

  for (int i = 0; i < num; i++){
    int idx = numindices > 0 ? indices[i] : i;
    SbVec3f p0, p1, p2, p3;
    this->buildStringQuad(action, idx, p0, p1, p2, p3);

    SbVec3f isect;
    SbVec3f bary;
    SbBool front;
    SbBool hit = action->intersect(p0, p1, p2, isect, bary, front);
    if (!hit) hit = action->intersect(p0, p2, p3, isect, bary, front);
    if (hit && action->isBetweenPlanes(isect)) {
      if (!this->pickOnPixel.getValue()){
        SoPickedPoint * pp = action->addIntersection(isect);
        if (pp) {
          SoTextDetail * detail = new SoTextDetail;
          detail->setStringIndex(idx);
          detail->setCharacterIndex(0);
          pp->setDetail(detail, this);
          pp->setMaterialIndex(binding == SoMaterialBindingElement::OVERALL ? 0 : idx);
          pp->setObjectNormal(SbVec3f(0.0f, 0.0f, 1.0f));
        }
        return;//We only calculate 1 picked point per SmTextureText2 instance
      }
      //else:
      //FIXME: Pixel coordinates are off by at least one pixel. 
      //Probably a rounding issue, or related to using center vs lowerleft coordinates of each pixel
      //Find out if the fault lies here, in buildStringQuad or in renderString... wiesener 20091102

      // find normalized 2D hitpoint on quad
      float h = (p3-p0).length();
      float w = (p1-p0).length();

      SbLine horizontal(p2, p3);
      SbVec3f ptonline = horizontal.getClosestPoint(isect);
      float vdist = (ptonline-isect).length();
      vdist /= h;

      SbLine vertical(p0, p3);
      ptonline = vertical.getClosestPoint(isect);
      float hdist = (ptonline-isect).length();
      hdist /= w;

      const SbString & string = strings[idx];
      int width_pixels = font->stringWidth(string);
      int height_pixels = font->height();

      int px = static_cast<int>(width_pixels * hdist);
      int py = static_cast<int>(height_pixels * vdist);

      const char * cstr = string.getString();
      int charindex = 0;

      int lastwidth = 0;
      while (charindex < string.getLength()){
        unsigned char c = cstr[charindex];
        int width = font->getKerning(c, 0);//should return "true width" of the glyph
        int xoffset = font->getXOffset(c);//will be negative for fonts extending to the left
        if (lastwidth + width > px){//we have a character, now let's see if it is a pixel hit
          SbVec2s glyphpos = font->getGlyphPositionPixels(c);//upper left pixel
          int x = px - lastwidth - xoffset + glyphpos[0];
          int y = py + glyphpos[1];
          const SbImage * img = font->getGLImage()->getImage();
          SbVec2s size;
          int nc;
          const unsigned char * pixels = img->getValue(size, nc);

//#define DEBUG_CHARACTER
#ifdef DEBUG_CHARACTER
          for (int fooy = 0; fooy < font->height(); fooy++){
            for (int foox = 0; foox < width - xoffset; foox++){
              int line = glyphpos[1] + fooy;
              char transparency = pixels[(line * size[0] + foox + glyphpos[0]) * nc];
              if (fooy == py && foox == px - lastwidth - xoffset) printf("X");
              else printf(transparency != 0 ? "O" : " ");
            }
            printf("\n");
          }
          printf("--------\n");
#endif //DEBUG_CHARACTER

          const unsigned char * pixel = &pixels[(y * size[0] + x) * nc];
          if (pixel[0] != 0){//not completely transparent
            SoPickedPoint * pp = action->addIntersection(isect);
            if (pp) {
              SoTextDetail * detail = new SoTextDetail;
              detail->setStringIndex(idx);
              detail->setCharacterIndex(0);
              pp->setDetail(detail, this);
              pp->setMaterialIndex(binding == SoMaterialBindingElement::OVERALL ? 0 : idx);
              pp->setObjectNormal(SbVec3f(0.0f, 0.0f, 1.0f));
            }
            return;//We only calculate 1 picked point per SmTextureText2 instance
          }
        }
        unsigned char c2 = cstr[charindex + 1];
        lastwidth += font->getKerning(c, c2);//add the kerning instead of width here
        if (lastwidth + font->getXOffset(c2) > px) break;//passed the point clicked
        charindex++;
      }
    }

  }
}

// doc from parent
void
SmTextureText2::getPrimitiveCount(SoGetPrimitiveCountAction *action)
{
  if (!this->shouldPrimitiveCount(action)) return;

  SoState * state = action->getState();
  const SbVec3f * positions;
  int numpositions = this->getPositions(state, positions);
  action->addNumText(numpositions);
}

// doc from parent
void
SmTextureText2::generatePrimitives(SoAction *action)
{
  // no primitives to generate
}

// convert normalized screen space coordinates into pixel screen
// space. Values that are too far from the viewport are culled.
static SbBool get_screenpoint_pixels(const SbVec3f & screenpoint,
                                     const SbVec2s & vpsize,
                                     SbVec2s & sp)
{
  float sx = screenpoint[0] * vpsize[0];
  float sy = screenpoint[1] * vpsize[1];

  // FIXME: just assume we won't have strings larger than 3000 pixels
  const float limit = 3000.0f;

  if ((sx > -limit) &&
      (sy > -limit) &&
      (sx < limit) &&
      (sy < limit)) {
    sp = SbVec2s(static_cast<short>(sx), static_cast<short>(sy));
    return TRUE;
  }
  return FALSE;
}

void
SmTextureText2::renderString(const SmTextureFontBundle & bundle,
                             const SbString * s,
                             const int numstring,
                             const SbVec3f & pos,
                             const SbViewVolume & vv,
                             const SbViewportRegion & vp,
                             const SbMatrix & projmatrix,
                             const SbMatrix & modelmatrix,
                             const SbMatrix & invmodelmatrix,
                             const float rotation)
{
  // get distance from pos to camera plane
  SbVec3f tmp;
  modelmatrix.multVecMatrix(pos, tmp);
  float dist = -vv.getPlane(0.0f).getDistance(tmp);
  if (dist <= vv.getNearDist()) return;
  if (dist > (vv.getNearDist() + vv.getDepth())) return;

  float maxr = this->maxRange.getValue();
  if (maxr > 0.0f && dist > maxr) return;

  int i;
  SbVec2s vpsize = vp.getViewportSizePixels();

  SbVec3f screenpoint;
  projmatrix.multVecMatrix(pos, screenpoint);

  int xmin = 0;
  int ymax = bundle.getAscent();
  int ymin = ymax - numstring * (bundle.height() + bundle.getLeading());
  ymin += bundle.getLeading();

  short h = ymax - ymin;
  short halfh = h / 2;

  switch (this->verticalJustification.getValue()) {
  case SmTextureText2::BOTTOM:
    break;
  case SmTextureText2::TOP:
    ymin -= bundle.getAscent();
    ymax -= bundle.getAscent();
    break;
  case SmTextureText2::VCENTER:
    ymin -= halfh;
    ymax -= halfh;
    break;
  default:
    assert(0 && "unknown alignment");
    break;
  }
  SbList <int> widthlist;

  for (i = 0; i < numstring; i++) {
    widthlist.append(bundle.stringWidth(s[i]));
  }

  for (i = 0; i < numstring; i++) {

    int len = s[i].getLength();
    if (len == 0) continue;

    SbVec2s sp;
    if (!get_screenpoint_pixels(screenpoint, vpsize, sp)) continue;

    SbVec2s n0 = SbVec2s(sp[0] + xmin,
                         sp[1] + ymax - (i+1)*bundle.height());

    short w = static_cast<short>(widthlist[i]);
    short halfw = w / 2;

    switch (this->justification.getValue()) {
    case SmTextureText2::LEFT:
      break;
    case SmTextureText2::RIGHT:
      n0[0] -= w;
      break;
    case SmTextureText2::CENTER:
      n0[0] -= halfw;
      break;
    default:
      assert(0 && "unknown alignment");
      break;
    }

    if (rotation != 0) {
      float x = static_cast<float>(sp[0]);
      float y = static_cast<float>(sp[1]);
      glPushMatrix();
      glTranslatef(x, y, 0);
      glRotatef(rotation * static_cast<float>(180 / M_PI), 0, 0, 1);
      glTranslatef(-x, -y, 0);
    }
    
    bundle.begin();
    bundle.renderString(s[i], SbVec3f(n0[0], n0[1], screenpoint[2]));
    bundle.end();

    if (rotation != 0) glPopMatrix();
  }
}
