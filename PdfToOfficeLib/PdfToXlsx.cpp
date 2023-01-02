#include "pch.h"

#include "PdfToXlsx.h"

using namespace SolidFramework::Platform;
using namespace SolidFramework::Converters::Plumbing;

namespace HpdfToOffice {

RES_CODE PdfToXlsx::Convert(const String& path, const String& password) {
  String outPath = path;
  if (m_IsSaveToUserDir && !m_UserDir.empty()) {
    outPath = m_UserDir + L"\\" + Util::Path::GetFileName(outPath);
  }
  outPath = Util::Path::ChangeExt(outPath, L".xlsx");
  outPath = Util::Path::GetAvailFileName(outPath);

  RES_CODE status = RES_CODE::Success;
  try {
    auto pConverter =
        std::make_shared<SolidFramework::Converters::PdfToExcelConverter>();
    m_Converter = pConverter;

    pConverter->OnProgress = &DoProgress;
    pConverter->SetPassword(password);
    pConverter->SetOutputType(
        SolidFramework::Converters::Plumbing::ExcelDocumentType::XlsX);

    status = static_cast<RES_CODE>(pConverter->Convert(path, outPath, true));

  } catch (const std::exception& /*e*/) {
    status = RES_CODE::Unknown;
  }
  return status;
}

}  // namespace HpdfToOffice
