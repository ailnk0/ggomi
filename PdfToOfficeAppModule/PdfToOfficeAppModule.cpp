#include "pch.h"

#include "PdfToOfficeAppModule.h"
#include "PdfToOfficeLib.h"

#include <msclr/marshal.h>
#include <msclr/marshal_cppstd.h>

namespace PdfToOfficeAppModule
{

PdfToOfficeAppModule::PdfToOfficeAppModule()
{
    lib = new HpdfToOffice::PdfToOfficeLib();
}

PdfToOfficeAppModule::~PdfToOfficeAppModule()
{
    if (lib)
    {
        delete lib;
        lib = nullptr;
	}
}

ErrorStatus PdfToOfficeAppModule::InitializeSolidFramework()
{
    try
    {
        return static_cast<ErrorStatus>(lib->InitializeSolidFramework());
    }
    catch (...)
    {
        return static_cast<ErrorStatus>(HpdfToOffice::ErrorStatus::Unknown);
    }
}

ErrorStatus PdfToOfficeAppModule::DoWordConversion(System::String ^ path, System::String ^ password)
{
    try
    {
        return static_cast<ErrorStatus>(
            lib->DoWordConversion(msclr::interop::marshal_as<HpdfToOffice::String>(path),
                                                      msclr::interop::marshal_as<HpdfToOffice::String>(password)));
    }
    catch (...)
    {
        return static_cast<ErrorStatus>(HpdfToOffice::ErrorStatus::Unknown);
    }
}

} // namespace PdfToOfficeAppModule
