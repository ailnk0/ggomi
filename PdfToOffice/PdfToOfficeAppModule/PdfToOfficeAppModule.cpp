#include "pch.h"

#include "PdfToOfficeAppModule.h"
#include "PdfToOfficeLib.h"

#include <msclr/marshal.h>
#include <msclr/marshal_cppstd.h>

namespace PdfToOfficeAppModule
{
int PdfToOfficeAppModule::RunSample(System::String ^ path)
{
    try
    {
        PdfToOfficeLib lib;
        return lib.RunSamples(msclr::interop::marshal_as<std::wstring>(path));
    }
    catch (const std::exception &)
    {
        // "Unhandled std::exception: " + ex.what()
        return -1;
    }
    catch (...)
    {
        // "Unhandled Exception!"
        return -1;
    }
}
} // namespace PdfToOfficeAppModule
