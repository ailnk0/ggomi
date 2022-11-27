#include "pch.h"

#include "PdfToOfficeProxy.h"

#include "PdfToOfficeLib.h"

#include <msclr/marshal.h>
#include <msclr/marshal_cppstd.h>

namespace PdfToOfficeAppModule
{

PdfToOfficeProxy::PdfToOfficeProxy()
{
    lib = new HpdfToOffice::PdfToOfficeLib();
}

PdfToOfficeProxy::~PdfToOfficeProxy()
{
    if (lib)
    {
        delete lib;
        lib = nullptr;
	}
}

ErrorStatus PdfToOfficeProxy::InitializeSolidFramework()
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

ErrorStatus PdfToOfficeProxy::DoWordConversion(System::String ^ path, System::String ^ password)
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
