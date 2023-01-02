#pragma once

#include "IProgressSite.h"

namespace HpdfToOffice {

class IPdfToOffice {
 public:
  virtual RES_CODE InitializeSolidFramework() = 0;
  virtual RES_CODE Convert(const String& path, const String& password) = 0;
  virtual void Cancel() = 0;
  virtual void SetOverwrite(bool overwrite) = 0;
  virtual void SetSaveToUserDir(bool saveToUserDir) = 0;
  virtual void SetUserDir(const String& path) = 0;
};

}  // namespace HpdfToOffice
