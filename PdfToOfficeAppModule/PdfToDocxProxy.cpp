#include "pch.h"

#include "PdfToDocxProxy.h"
#include "PdfToDocx.h"

namespace PdfToOfficeAppModule {
PdfToDocxProxy::PdfToDocxProxy() {
  lib = new HpdfToOffice::PdfToDocx();
}

PdfToDocxProxy::~PdfToDocxProxy() {
  if (lib) {
    delete lib;
    lib = nullptr;
  }
}
}  // namespace PdfToOfficeAppModule
