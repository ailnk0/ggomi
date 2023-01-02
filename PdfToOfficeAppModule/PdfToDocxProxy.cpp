#include "pch.h"

#include "PdfToDocx.h"
#include "PdfToDocxProxy.h"

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
