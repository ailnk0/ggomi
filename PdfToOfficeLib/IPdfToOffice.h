#pragma once

#include "IProgressSite.h"

namespace HpdfToOffice {

class IPdfToOffice {
 public:
  virtual RES_CODE Init() = 0;
  virtual RES_CODE Convert(const String& sourcePath,
                           const String& outPath,
                           const String& password) = 0;
  virtual void Cancel() = 0;
  virtual bool GetOverwrite() = 0;
  virtual void SetOverwrite(bool overwrite) = 0;
};

}  // namespace HpdfToOffice
