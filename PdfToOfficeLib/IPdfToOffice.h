#pragma once
#include "Define.h"
#include "IProgressSite.h"
#include "SolidFramework.h"

namespace HpdfToOffice {
class IPdfToOffice {
 public:
  virtual RES_CODE InitializeSolidFramework() = 0;
  virtual RES_CODE Convert(const String& path, const String& password) = 0;
  virtual void Cancel() = 0;

  virtual void SetIsSaveToUserDir(bool allow) = 0;
  virtual void SetUserDir(const String& path) = 0;
  virtual void SetOverwrite(bool allow) = 0;
};
}  // namespace HpdfToOffice
