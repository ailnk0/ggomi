#pragma once

#include "IPdfToOffice.h"
#include "IProgressSite.h"
#include "SolidFramework.h"

namespace HpdfToOffice {

class PdfToOffice : public IPdfToOffice {
 private:
  static IProgressSite* s_ProgressSite;

 protected:
  std::shared_ptr<SolidFramework::Converters::Converter> m_Converter = nullptr;
  bool m_IsOverwrite = false;
  bool m_IsSaveToUserDir = false;
  String m_UserDir;
  String m_OutPath;

 public:
  PdfToOffice() = default;

 public:
  virtual RES_CODE Init();
  virtual RES_CODE Convert(const String& path, const String& password);
  virtual void Cancel();
  virtual void SetOverwrite(bool overwrite);
  virtual void SetSaveToUserDir(bool saveToUserDir);
  virtual void SetUserDir(const String& path);
  virtual String SetOutPath();

 public:
  static void DoProgress(
      SolidFramework::ProgressEventArgsPtr pProgressEventArgs);
  static void SetSite(IProgressSite* progressSite);
};

}  // namespace HpdfToOffice
