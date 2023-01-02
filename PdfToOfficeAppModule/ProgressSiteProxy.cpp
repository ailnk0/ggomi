#include "pch.h"

#include "IProgressSiteCli.h"
#include "ProgressSiteProxy.h"

namespace PdfToOfficeAppModule {

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
