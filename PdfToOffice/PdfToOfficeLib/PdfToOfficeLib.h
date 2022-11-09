#pragma once

#include "SolidFramework.h"

class PdfToOfficeLib
{
  public:
    PdfToOfficeLib() = default;

  public:
    virtual bool InitializeSolidFramework(const std::wstring &frameworkPath);
    virtual bool DoWordConversion(const std::wstring &fullPath, const std::wstring &password);
    virtual int RunSamples(const std::wstring &path);

  public:
    static void DoProgress(SolidFramework::ProgressEventArgsPtr pProgressEventArgs);
};
