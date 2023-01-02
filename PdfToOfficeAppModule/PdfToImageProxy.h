#pragma once

#include "PdfToOfficeProxy.h"

namespace PdfToOfficeAppModule {

public
ref class PdfToImageProxy : public PdfToOfficeProxy {
 public:
  PdfToImageProxy();
  ~PdfToImageProxy();

 public:
  virtual void SetImgType(IMG_TYPE imgType);
};

}  // namespace PdfToOfficeAppModule
