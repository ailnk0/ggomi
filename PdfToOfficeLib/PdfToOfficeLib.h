#pragma once

#include "Define.h"
#include "IProgressSite.h"
#include "SolidFramework.h"

namespace HpdfToOffice {

class PdfToOfficeLib {
 private:
  static IProgressSite* s_ProgressSite;

 private:
  bool m_IsSaveToUserDir = false;
  String m_UserDir;

 public:
  PdfToOfficeLib() = default;

 public:
  virtual RES_CODE InitializeSolidFramework();
  virtual RES_CODE DoConversion(const String& path,
                                const String& password,
                                FILE_TYPE fileFormat,
                                IMG_TYPE imageFormat,
                                bool overwrite);
  virtual void SetIsSaveToUserDir(bool allow);
  virtual void SetUserDir(const String& path);

 public:
  static void DoProgress(
      SolidFramework::ProgressEventArgsPtr pProgressEventArgs);
  static void SetSite(IProgressSite* progressSite);
};

}  // namespace HpdfToOffice
