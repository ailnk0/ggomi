#pragma once

#include "PdfToOffice.h"

namespace HpdfToOffice {
class PdfToDocx : public PdfToOffice {
 public:
  virtual RES_CODE Convert(const String& path, const String& password);
};
}  // namespace HpdfToOffice
