#include "pch.h"

#include "PdfToXlsx.h"

using namespace SolidFramework::Platform;
using namespace SolidFramework::Converters::Plumbing;

namespace HpdfToOffice {
RES_CODE PdfToXlsx::Convert(const String& path, const String& password) {
  String filePath = path;

  String outPath;
  if (m_IsSaveToUserDir && !m_UserDir.empty()) {
    outPath = m_UserDir;
  } else {
    outPath = Util::Path::GetDirName(path);
  }

  RES_CODE status = RES_CODE::Success;
  try {
    auto pConverter =
        std::make_shared<SolidFramework::Converters::PdfToExcelConverter>();
    m_Converter = pConverter;

    pConverter->SetOutputType(
        SolidFramework::Converters::Plumbing::ExcelDocumentType::XlsX);
    pConverter->AddSourceFile(filePath);
    pConverter->SetOutputDirectory(outPath);
    pConverter->SetPassword(password);
    pConverter->OnProgress = &DoProgress;

    if (m_IsOverwrite) {
      pConverter->SetOverwriteMode(
          SolidFramework::Plumbing::OverwriteMode::ForceOverwrite);
    }

    pConverter->Convert();
    status = static_cast<RES_CODE>(pConverter->GetResults()[0]->GetStatus());
  } catch (const std::exception& /*e*/) {
    status = RES_CODE::Unknown;
  }
  return status;
}
}  // namespace HpdfToOffice
