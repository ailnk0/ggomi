#include "pch.h"

#include "PdfToPptx.h"

namespace HpdfToOffice {

RES_CODE PdfToPptx::Convert(const String& sourcePath,
                            const String& outPath,
                            const String& password) {
  RES_CODE status = RES_CODE::Success;
  try {
    auto pConverter = std::make_shared<
        SolidFramework::Converters::PdfToPowerPointConverter>();
    m_Converter = pConverter;

    pConverter->OnProgress = &DoProgress;
    pConverter->SetPassword(password);
    pConverter->AddSourceFile(sourcePath);
    pConverter->SetOutputDirectory(Util::Path::GetDirName(outPath));

    pConverter->ConvertTo(outPath, m_IsOverwrite);

    status = static_cast<RES_CODE>(pConverter->GetResults()[0]->GetStatus());

  } catch (const std::exception& /*e*/) {
    status = RES_CODE::Unknown;
  }
  return status;
}

}  // namespace HpdfToOffice
