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
  bool m_IsOverwrite = true;

 public:
  PdfToOffice() = default;

 public:
  virtual RES_CODE Init();
  virtual RES_CODE Convert(const String& sourcePath,
                           const String& outPath,
                           const String& password) = 0;
  virtual void Cancel();
  virtual bool GetOverwrite();
  virtual void SetOverwrite(bool overwrite);

 public:
  static void DoProgress(
      SolidFramework::ProgressEventArgsPtr pProgressEventArgs);
  static void SetSite(IProgressSite* progressSite);
};

}  // namespace HpdfToOffice
