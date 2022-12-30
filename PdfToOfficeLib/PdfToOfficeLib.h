#pragma once

#include "Define.h"
#include "IProgressSite.h"
#include "SolidFramework.h"

namespace HpdfToOffice {

class PdfToOfficeLib {
 private:
  static IProgressSite* m_ProgressSite;

 public:
  PdfToOfficeLib() = default;

 public:
  virtual RES_CODE InitializeSolidFramework();
  virtual RES_CODE DoConversion(const String& path,
                                const String& password,
                                FILE_TYPE fileFormat,
                                IMG_TYPE imageFormat,
                                bool overwrite);

 public:
  static void DoProgress(
      SolidFramework::ProgressEventArgsPtr pProgressEventArgs);
  static void SetSite(IProgressSite* progressSite);
};

}  // namespace HpdfToOffice
