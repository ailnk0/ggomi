#pragma once

#include "PdfToOffice.h"

namespace HpdfToOffice {

class PdfToImage : public PdfToOffice {
 protected:
  IMG_TYPE m_ImgType = IMG_TYPE::PNG;

 public:
  virtual RES_CODE Convert(const String& sourcePath,
                           const String& outPath,
                           const String& password);
  virtual void SetImgType(IMG_TYPE imgType);
};

}  // namespace HpdfToOffice
