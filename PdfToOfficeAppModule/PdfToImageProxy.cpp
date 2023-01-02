#include "pch.h"

#include "PdfToImage.h"
#include "PdfToImageProxy.h"

namespace PdfToOfficeAppModule {
PdfToImageProxy::PdfToImageProxy() {
  lib = new HpdfToOffice::PdfToImage();
}

PdfToImageProxy::~PdfToImageProxy() {
  if (lib) {
    delete lib;
    lib = nullptr;
  }
}

void PdfToImageProxy::SetImgType(IMG_TYPE imgType) {
  if (!lib) {
    return;
  }
  HpdfToOffice::PdfToImage* temp = dynamic_cast<HpdfToOffice::PdfToImage*>(lib);
  temp->SetImgType(static_cast<HpdfToOffice::IMG_TYPE>(imgType));
}
}  // namespace PdfToOfficeAppModule
