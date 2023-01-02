
#include "pch.h"

#include "PdfToPptx.h"

using namespace SolidFramework::Platform;
using namespace SolidFramework::Converters::Plumbing;
namespace HpdfToOffice {
RES_CODE PdfToPptx::Convert(const String& path, const String& password) {
  String filePath = path;

  String outPath;
  if (m_IsSaveToUserDir && !m_UserDir.empty()) {
    outPath = m_UserDir;
  } else {
    outPath = Util::Path::GetDirName(path);
  }

  RES_CODE status = RES_CODE::Success;
  try {
    auto pConverter = std::make_shared<
        SolidFramework::Converters::PdfToPowerPointConverter>();
    m_Converter = pConverter;

    pConverter->AddSourceFile(filePath);
    pConverter->SetOutputDirectory(outPath);
    pConverter->SetOverwriteMode(
        SolidFramework::Plumbing::OverwriteMode::ForceOverwrite);
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
