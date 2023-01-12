#include "pch.h"

#include "PdfToDocx.h"

using namespace SolidFramework::Platform;
using namespace SolidFramework::Converters::Plumbing;

namespace HpdfToOffice {

RES_CODE PdfToDocx::Convert(const String& sourcePath,
                            const String& outPath,
                            const String& password) {
  RES_CODE status = RES_CODE::Success;
  try {
    auto pConverter =
        std::make_shared<SolidFramework::Converters::PdfToWordConverter>();
    m_Converter = pConverter;

    pConverter->OnProgress = &DoProgress;
    pConverter->SetPassword(password);
    pConverter->AddSourceFile(sourcePath);
    pConverter->SetOutputDirectory(Util::Path::GetDirName(outPath));

    pConverter->SetOutputType(
        SolidFramework::Converters::Plumbing::WordDocumentType::DocX);
    pConverter->SetReconstructionMode(
        SolidFramework::Converters::Plumbing::ReconstructionMode::Flowing);
    pConverter->SetSupportRightToLeftWritingDirection(true);

    pConverter->ConvertTo(outPath, m_IsOverwrite);

    status = static_cast<RES_CODE>(pConverter->GetResults()[0]->GetStatus());

  } catch (const std::exception& /*e*/) {
    status = RES_CODE::Unknown;
  }
  return status;
}

}  // namespace HpdfToOffice
