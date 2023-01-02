#include "pch.h"

#include "PdfToXlsxProxy.h"
#include "PdfToXlsx.h"

namespace PdfToOfficeAppModule {
PdfToXlsxProxy::PdfToXlsxProxy() {
  lib = new HpdfToOffice::PdfToXlsx();
}

PdfToXlsxProxy::~PdfToXlsxProxy() {
  if (lib) {
    delete lib;
    lib = nullptr;
  }
}
}  // namespace PdfToOfficeAppModule
