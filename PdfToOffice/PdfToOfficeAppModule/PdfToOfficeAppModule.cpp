#include "pch.h"
#include "PdfToOfficeAppModule.h"
#include "PdfToOfficeLib.h"

namespace PdfToOfficeAppModule {
	int PdfToOfficeAppModule::RunSample()
	{
		try {
			PdfToOfficeLib lib;
			return lib.RunSamples();
		}
		catch (const std::exception&) {
			// "Unhandled std::exception: " + ex.what()
			return -1;
		}
		catch (...) {
			// "Unhandled Exception!"
			return -1;
		}
	}
}
