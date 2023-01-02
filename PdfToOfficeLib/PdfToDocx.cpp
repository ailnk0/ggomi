#include "pch.h"

#include "PdfToDocx.h"

using namespace SolidFramework::Platform;
using namespace SolidFramework::Converters::Plumbing;

namespace HpdfToOffice {

RES_CODE PdfToDocx::Convert(const String& path, const String& password) {
  String outPath = path;
  if (m_IsSaveToUserDir && !m_UserDir.empty()) {
    outPath = m_UserDir + L"\\" + Util::Path::GetFileName(outPath);
  }
  outPath = Util::Path::ChangeExt(outPath, L".docx");
  outPath = Util::Path::GetAvailFileName(outPath);

  RES_CODE status = RES_CODE::Success;
  try {
    auto pConverter =
        std::make_shared<SolidFramework::Converters::PdfToWordConverter>();
    m_Converter = pConverter;

    pConverter->OnProgress = &DoProgress;
    pConverter->SetPassword(password);
    pConverter->SetReconstructionMode(
        SolidFramework::Converters::Plumbing::ReconstructionMode::Flowing);
    pConverter->SetOutputType(
        SolidFramework::Converters::Plumbing::WordDocumentType::DocX);

    status = static_cast<RES_CODE>(pConverter->Convert(path, outPath, true));

  } catch (const std::exception& /*e*/) {
    status = RES_CODE::Unknown;
  }
  return status;
}

}  // namespace HpdfToOffice
