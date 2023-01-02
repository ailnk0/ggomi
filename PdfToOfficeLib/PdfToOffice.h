#pragma once

#include "Define.h"
#include "IProgressSite.h"
#include "SolidFramework.h"

namespace HpdfToOffice {
class PdfToOffice {
 private:
  static IProgressSite* s_ProgressSite;

 protected:
  bool m_IsSaveToUserDir = false;
  String m_UserDir;
  bool m_IsOverwrite = true;
  std::shared_ptr<SolidFramework::Converters::Converter> m_Converter = nullptr;

 public:
  PdfToOffice() = default;

 public:
  virtual RES_CODE InitializeSolidFramework();
  virtual RES_CODE Convert(const String& path, const String& password);
  virtual void Cancel();

  virtual void SetIsSaveToUserDir(bool allow);
  virtual void SetUserDir(const String& path);
  virtual void SetOverwrite(bool allow);

 public:
  static void DoProgress(
      SolidFramework::ProgressEventArgsPtr pProgressEventArgs);
  static void SetSite(IProgressSite* progressSite);
};
}  // namespace HpdfToOffice
