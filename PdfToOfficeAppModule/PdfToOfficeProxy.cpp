﻿#include "pch.h"

#include "PdfToOfficeProxy.h"

#include "IProgressSite.h"
#include "IProgressSiteCli.h"
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

RES_CODE PdfToOfficeProxy::Init() {
  if (!lib) {
    return RES_CODE::Unknown;
  }
  return static_cast<RES_CODE>(lib->Init());
}

RES_CODE PdfToOfficeProxy::Convert(System::String ^ sourcePath,
                                   System::String ^ outPath,
                                   System::String ^ password) {
  if (!lib) {
    return RES_CODE::Unknown;
  }
  return static_cast<RES_CODE>(
      lib->Convert(msclr::interop::marshal_as<HpdfToOffice::String>(sourcePath),
                   msclr::interop::marshal_as<HpdfToOffice::String>(outPath),
                   msclr::interop::marshal_as<HpdfToOffice::String>(password)));
}

void PdfToOfficeProxy::Cancel() {
  if (!lib) {
    return;
  }
  lib->Cancel();
}

void PdfToOfficeProxy::SetOverwrite(bool overwrite) {
  if (!lib) {
    return;
  }
  lib->SetOverwrite(overwrite);
}

void PdfToOfficeProxy::SetProgressSiteCli(IProgressSiteCli ^ progressSiteCli) {
  ProgressSiteProxy* progressSiteProxy = new ProgressSiteProxy(progressSiteCli);
  m_ProgressSite = static_cast<HpdfToOffice::IProgressSite*>(progressSiteProxy);
  HpdfToOffice::PdfToOffice::SetSite(m_ProgressSite);
}

}  // namespace PdfToOfficeAppModule
