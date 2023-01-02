#include "pch.h"

#include "PdfToOfficeProxy.h"

#include "IProgressSiteCli.h"
#include "IProgressSite.h"
#include "PdfToOffice.h"
#include "ProgressSiteProxy.h"

#include <msclr/marshal.h>
#include <msclr/marshal_cppstd.h>

namespace PdfToOfficeAppModule {
PdfToOfficeProxy::PdfToOfficeProxy() {}

PdfToOfficeProxy::~PdfToOfficeProxy() {
  if (m_ProgressSite != nullptr) {
    delete m_ProgressSite;
    m_ProgressSite = nullptr;
  }
}

void PdfToOfficeProxy::SetProgressSiteCli(IProgressSiteCli ^ progressSiteCli) {
  ProgressSiteProxy* progressSiteProxy = new ProgressSiteProxy(progressSiteCli);
  m_ProgressSite = static_cast<HpdfToOffice::IProgressSite*>(progressSiteProxy);
  HpdfToOffice::PdfToOffice::SetSite(m_ProgressSite);
}

RES_CODE PdfToOfficeProxy::InitializeSolidFramework() {
  try {
    return static_cast<RES_CODE>(lib->InitializeSolidFramework());
  } catch (...) {
    return static_cast<RES_CODE>(HpdfToOffice::RES_CODE::Unknown);
  }
}

RES_CODE PdfToOfficeProxy::Convert(System::String ^ path,
                                   System::String ^ password) {
  if (!lib) {
    return RES_CODE::Unknown;
  }

  return static_cast<RES_CODE>(
      lib->Convert(msclr::interop::marshal_as<HpdfToOffice::String>(path),
                   msclr::interop::marshal_as<HpdfToOffice::String>(password)));
}

void PdfToOfficeProxy::Cancel() {
  if (!lib) {
    return;
  }
  lib->Cancel();
}

void PdfToOfficeProxy::SetIsSaveToUserDir(bool allow) {
  if (!lib) {
    return;
  }
  lib->SetIsSaveToUserDir(allow);
}

void PdfToOfficeProxy::SetUserDir(System::String ^ path) {
  if (!lib) {
    return;
  }
  lib->SetUserDir(msclr::interop::marshal_as<HpdfToOffice::String>(path));
}

void PdfToOfficeProxy::SetOverwrite(bool allow) {
  if (!lib) {
    return;
  }
  lib->SetOverwrite(allow);
}
}  // namespace PdfToOfficeAppModule
