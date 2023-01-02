#include "pch.h"

#include "PdfToXlsx.h"
#include "PdfToXlsxProxy.h"

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
