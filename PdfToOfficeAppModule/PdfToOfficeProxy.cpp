#include "pch.h"

#include "PdfToOfficeProxy.h"

#include "IProgressSite.h"
#include "PdfToOfficeLib.h"

#include <msclr/marshal.h>
#include <msclr/marshal_cppstd.h>

namespace PdfToOfficeAppModule {

PdfToOfficeProxy::PdfToOfficeProxy() {
  lib = new HpdfToOffice::PdfToOfficeLib();
}

PdfToOfficeProxy::~PdfToOfficeProxy() {
  if (lib) {
    delete lib;
    lib = nullptr;
  }

  if (m_ProgressSite != nullptr) {
    delete m_ProgressSite;
    m_ProgressSite = nullptr;
  }
}

void PdfToOfficeProxy::SetProgressSiteCli(IProgressSiteCli ^ progressSiteCli) {
  ProgressSiteProxy* progressSiteProxy = new ProgressSiteProxy(progressSiteCli);
  m_ProgressSite = static_cast<HpdfToOffice::IProgressSite*>(progressSiteProxy);
  HpdfToOffice::PdfToOfficeLib::SetSite(m_ProgressSite);
}

RES_CODE PdfToOfficeProxy::InitializeSolidFramework() {
  try {
    return static_cast<RES_CODE>(lib->InitializeSolidFramework());
  } catch (...) {
    return static_cast<RES_CODE>(HpdfToOffice::RES_CODE::Unknown);
  }
}

RES_CODE PdfToOfficeProxy::DoConversion(System::String ^ path,
                                        System::String ^ password,
                                        FILE_TYPE fileType,
                                        IMG_TYPE imageType) {
  try {
    return static_cast<RES_CODE>(lib->DoConversion(
        msclr::interop::marshal_as<HpdfToOffice::String>(path),
        msclr::interop::marshal_as<HpdfToOffice::String>(password),
        static_cast<HpdfToOffice::FILE_TYPE>(fileType),
        static_cast<HpdfToOffice::IMG_TYPE>(imageType)));
  } catch (...) {
    return static_cast<RES_CODE>(HpdfToOffice::RES_CODE::Unknown);
  }
}

ProgressSiteProxy::ProgressSiteProxy(gcroot<IProgressSiteCli ^> site) {
  m_Site = site;
}

ProgressSiteProxy::~ProgressSiteProxy() {
  if (static_cast<IProgressSiteCli ^>(m_Site) != nullptr) {
    delete m_Site;
    m_Site = nullptr;
  }
}

void ProgressSiteProxy::SetPercent(int percent) {
  if (static_cast<IProgressSiteCli ^>(m_Site) != nullptr) {
    m_Site->SetPercent(percent);
  }
}

}  // namespace PdfToOfficeAppModule
