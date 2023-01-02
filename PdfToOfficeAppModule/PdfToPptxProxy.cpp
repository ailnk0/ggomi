#include "pch.h"

#include "PdfToPptxProxy.h"
#include "PdfToPptx.h"

namespace PdfToOfficeAppModule {
PdfToPptxProxy::PdfToPptxProxy() {
  lib = new HpdfToOffice::PdfToPptx();
}

PdfToPptxProxy::~PdfToPptxProxy() {
  if (lib) {
    delete lib;
    lib = nullptr;
  }
}
}  // namespace PdfToOfficeAppModule
