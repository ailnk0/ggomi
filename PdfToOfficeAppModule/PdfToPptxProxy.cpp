#include "pch.h"

#include "PdfToPptx.h"
#include "PdfToPptxProxy.h"

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
