#pragma once

#include "SolidFramework.h"

class PdfToOfficeLib
{
  public:
    PdfToOfficeLib() = default;

  public:
    virtual int InitializeSolidFramework(const std::wstring &frameworkPath);
    virtual bool DoWordConversion(const std::wstring &fullPath, const std::wstring &password);
    virtual int RunSamples(const std::wstring &path);
    virtual int RunSamples();

  public:
    static void DoProgress(SolidFramework::ProgressEventArgsPtr pProgressEventArgs);
};
