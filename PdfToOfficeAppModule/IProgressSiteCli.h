#pragma once

namespace PdfToOfficeAppModule {

public
interface class IProgressSiteCli {
  virtual void SetPercent(int percent) = 0;
};

}  // namespace PdfToOfficeAppModule
