/*
 * Version: 10.0.14502
 */

/*
* If Visual Studio complains about #include "stdafx.h" being missing from this file
*   Right-click on SolidFramework.cpp in Solution Explorer and click Properties
*   Expand the C/C++ property section
*   You can force SolidFramework.cpp to include the precompiled header by
*     Opening the 'Advanced' section and setting 'Forced Include File' to stdafx.h
*   Or you can turn off precompiled headers for SolidFramework.cpp by
*     Opening the 'Precompiled Headers' section and setting 'Precompiled Header' to 'Not Using Precompiled Headers'
*   Click OK and then rebuild the project
*/

#include "SolidFramework.h"

#if defined(__SOLIDPOSIX__)
//#include <solid_assert.h>
//#include </usr/include/dlfcn.h>
#include <dlfcn.h>
#endif

static void InitializeCallbacks();

void RaiseSolidException(const std::string& id, const char* param = nullptr);

static bool initialized = false;
static bool supportPlatformIndependentPaths = false;
static bool initializedWithPlatformIndependentPaths = false;

namespace SolidFramework {

    FrameworkException::FrameworkException(): std::runtime_error("")
    {
        rawMessage = "";
    }

    FrameworkException::FrameworkException(const char* msg): std::runtime_error(msg)
    {
        this->rawMessage = msg;
    }

    FrameworkException::FrameworkException(const char* msg, const char* rawMessage): std::runtime_error(msg)
    {
        this->rawMessage = rawMessage;
    }
    
    std::string FrameworkException::GetRawMessage()
    {
        return rawMessage;
    }

    std::wstring FileNotFoundException::GetPath()
    {
        return path;
    }
}

namespace SolidFramework { namespace Platform { namespace Plumbing {

    // Registers callbacks used to marshal strings and exceptions (this only needs to be called when switching between programming languages)
    void RegisterCallbacks();

    // Unregisters callbacks used to marshal strings and exceptions (this only needs to be called when switching between programming languages)
    void UnregisterCallbacks();

    Logging::Logging()
    {
        isLogging = false;
        // Create a mutex with no initial owner
      #if defined __SOLIDWINDOWS__
        bool failed = ((hMutex = CreateMutex(NULL, FALSE, NULL)) == NULL);
      #elif defined __SOLIDPOSIX__
        bool failed = (pthread_mutex_init(&hMutex, NULL) != 0);
      #else
        NOT_IMPLEMENTED
      #endif
        if (failed)
        {
            throw SolidFramework::FrameworkException("Failed to create mutex in Logging::Logging()");
        }
    }

    Logging::~Logging()
    {
      #if defined __SOLIDWINDOWS__
        CloseHandle(hMutex);
        hMutex = NULL;
      #elif defined __SOLIDPOSIX__
        pthread_mutex_destroy(&hMutex);
      #else
        NOT_IMPLEMENTED
      #endif
    }

    Logging& Logging::GetInstance()
    {
        static Logging instance;
        return instance;
    }

    void Logging::SetLogPath(const std::wstring& path)
    {
        static Logging& instance(GetInstance());
        instance.isLogging = !path.empty();
        Platform::SetLogFilename(path);
        instance.loggingPath = path;
    }

    void Logging::SetRotationSize(const unsigned long rotationSize)
    {
        static Logging& instance(GetInstance());
        instance.rotationSize = rotationSize;
    }

    void RotateLog(std::wstring loggingPath)
    {
        std::wstring extension = Path::GetExtension(loggingPath);
        std::wstring filename = Path::GetFileNameWithoutExtension(loggingPath);
        std::wstring directory = Path::GetDirectoryName(loggingPath);
        std::wstring log1 = Path::Combine(directory, filename + L"_1" + extension);
        std::wstring log2 = Path::Combine(directory, filename + L"_2" + extension);
        if (File::Exists(log2))
        {
            File::Delete(log2);
        }
        if (File::Exists(log1))
        {
            File::Move(log1, log2);
        }
        if (File::Exists(loggingPath))
        {
            File::Move(loggingPath, log1);
        }
    }

    void Logging::WriteLine(const std::wstring& content)
    {
        static Logging& instance(GetInstance());
        if (!instance.isLogging || instance.loggingPath.empty())
        {
            return;
        }
      #if defined __SOLIDWINDOWS__
        if (WaitForSingleObject(instance.hMutex, INFINITE) != WAIT_OBJECT_0)
        {
            throw SolidFramework::FrameworkException("Failed to get mutex in Logging::writeLine");
        }
        WIN32_FILE_ATTRIBUTE_DATA fInfo;
        GET_FILEEX_INFO_LEVELS fInfoLevelId = GetFileExInfoStandard;
        if (instance.rotationSize > 0 && ::GetFileAttributesEx(instance.loggingPath.c_str(), fInfoLevelId, &fInfo))
        {
            if (fInfo.nFileSizeLow > instance.rotationSize)
            {
                RotateLog(instance.loggingPath);
            }
        }
        Platform::WriteToLog(content);
        ReleaseMutex(instance.hMutex);
      #elif defined __SOLIDPOSIX__
        pthread_mutex_lock(&instance.hMutex);
        std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converterX;
        std::string strPath = converterX.to_bytes(instance.loggingPath.c_str());
        struct stat myStat;
        if (instance.rotationSize > 0 && ::stat(strPath.c_str(), &myStat) == 0)
        {
            if (myStat.st_size > instance.rotationSize)
            {
                RotateLog(instance.loggingPath);
            }
        }
        Platform::WriteToLog(content);
        pthread_mutex_unlock(&instance.hMutex);
      #else
        NOT_IMPLEMENTED
      #endif
    }

}}} // SolidFramework::Platform::Plumbing

static std::string temporaryString;
void* SOLIDCALLBACK SetString(const char* value)
{
    temporaryString = value;
    return (void*)(temporaryString.c_str());
}

const char* GetExceptionDescription(const char* id)
{
    class CExceptionID2Description: protected std::map<std::string, std::string>
    {
    public:
        CExceptionID2Description()
        {
            insert(value_type("ArgumentNull", "At least one argument that was passed to the method was null or empty.")); 
            insert(value_type("InvalidParameter", "At least one parameter that was passed to the method was invalid."));
            insert(value_type("InvalidPageRangeFormat", "Input could not be parsed as a PageRange."));
            insert(value_type("InvalidPdfDateFormat", "Input could not be parsed as a PdfDate."));
            insert(value_type("InvalidLicense", "The license is not valid.")); 
            insert(value_type("InvalidCloudLicense", "The license is not valid on this platform. Please use License::CloudImport(..) to import licenses on this platform."));
            insert(value_type("CloudImportDenied", "CloudImport isn't enabled for your account, please contact support@soliddocuments.com to discuss options."));
            insert(value_type("WebValidateDenied", "WebValidate isn't enabled for your account, please contact support@soliddocuments.com to discuss options."));
            insert(value_type("BadLicenseXML", "The license XML was invalid or could not be parsed."));
            insert(value_type("LicenseWebCallFailure", "It was not possible to connect to SolidDocuments server to validate the license."));
            insert(value_type("LicenseWebCallError", "The SolidDocuments license server returned an unexpected error response."));
            insert(value_type("LicenseExpired", "The license you are trying to use has expired.")); 
            insert(value_type("LicenseViolation", "Operation not possible without correct license (use License::Import(..))"));
            insert(value_type("ReadonlyObject", "This object is readonly."));
            insert(value_type("DocumentMustBeOpen", "Document is not Open."));
            insert(value_type("DocumentAlreadyOpen", "Document is already Open."));
            insert(value_type("DontUseNativeDocumentAfterClose", "NativeDocument is not valid after destruction."));
            insert(value_type("DontUsePagesModelAfterClose", "The PagesModel has been closed and can no longer be used."));
            insert(value_type("DontUsePageSourceAfterClose", "The page source has been closed and can no longer be used."));
            insert(value_type("DontUsePageHolderAfterClose", "The page holder has been closed and can no longer be used."));
            insert(value_type("DontUseCoreModelAfterClose", "The CoreModel has been closed and can no longer be used.")); 
            insert(value_type("CoreModelCreationCancelled", "CoreModel creation was cancelled by the user."));
            insert(value_type("PathMustBeSet", "Path must be set before calling Open."));
            insert(value_type("PathMustBeSetForSave", "Path must be set before calling Save."));
            insert(value_type("PathTooLong", "Path exceeded 260 character path limit."));
            insert(value_type("FileAlreadyExists", "Failed to overwrite existing file. Call SetOverwriteMode(OverwriteMode::ForceOverwrite)."));
            insert(value_type("FileAlreadyExists2", "Failed to overwrite existing file. Call with overwrite = true."));
            insert(value_type("FileHasCopyProtection", "This PDF file is password protected. Please specify the correct user password or owner password."));
            insert(value_type("SourceFileNotFound", "The source file does not exist."));
            insert(value_type("SourceDirectoryNotFound", "The source directory does not exist."));
            insert(value_type("DirectoryNotFound", "The directory was not found."));
            insert(value_type("SourceFilesNotDefined", "Source files or directory need to be set."));
            insert(value_type("SourceFilesMustBeOne_", "Only one source file can be set."));
            insert(value_type("SourceFilesMustBeOne", "ConvertTo requires only one source file to be set."));
            insert(value_type("ResultsFilesMustBeOne", "ConvertTo can only be used for conversion that result in single result file."));
            insert(value_type("UserButNoOwner", "When specifying a user password, an owner password is also required."));
            insert(value_type("UserButMustBeOwner", "The owner password is required to perform this action.")); 
            insert(value_type("NoUserAndNoOwner", "To use Encryption either a user password or an owner password is required."));
            insert(value_type("BadDataOpeningFile", "PDF file is corrupt and cannot be opened."));
            insert(value_type("DocumentNotModified", "Document was not modified since Open. Save is meaningless."));
            insert(value_type("UnknownFrameworkError", "Unknown error in Framework.dll."));
            insert(value_type("IncorrectOcrLanguage", "Invalid Language. Use Ocr::Languages to see list of valid languages."));
            insert(value_type("MSWordNotInstalled", "Couldn't create WordDocument because MS Word is not installed."));
            insert(value_type("PageRangeMultipleFiles", "PageRange can only be set for single file conversions."));
            insert(value_type("InvalidImageType", "Attempted to use unsupported image type with Html converter."));
            insert(value_type("DPIValueInvalid", "DPI cannot be set for ConversionType of ExtractImages."));
            insert(value_type("BadHtmlSplittingUsing", "When using HtmlNavigation::SplitIntoMultipleFiles only one of the Converters::HtmlSplittingUsing may be selected."));
            insert(value_type("ValidationModePdfInvalid", "ValidationMode::Pdf is not a valid mode for PdfToPdfAConverter."));
            insert(value_type("ValidationModeAutomaticInvalid", "ValidationMode::Automatic is only a valid mode for PdfToPdfAConverter when ValidateOnly is true."));
            insert(value_type("ValidationModeOtherThanDetectInvalid", "For VerifyOnly the only valid ValidationMode is Detect. Nothing else makes sense."));
            insert(value_type("ConflictingRecontstructionModeAndWorddocumentType", "ReconstructionMode and WordDocumentType properties are incompatible. ReconstructionMode::PlainText should only be used with WordDocumentType::Txt."));
            insert(value_type("PageRotationAngleInvalid", "The specified rotation angle is not valid."));
            insert(value_type("PortraitLandscapeNotDone", "PageRanges::Portrait and PageRanges::Landscape are not yet implemented."));
            insert(value_type("SelectedNotDone", "PageRanges::Selected is not yet implemented."));
            insert(value_type("CurrentIsExclusive", "PageRanges::Current should not be used with other PageRanges flags."));
            insert(value_type("UnsupportedFileType", "Attempted to use an unsupported file type with print provider."));
            insert(value_type("DateObjectInvalid", "The date object is invalid.")); 
            insert(value_type("PagesArrayEmpty", "The Pages Array is empty."));
            insert(value_type("PdfItemNull", "The PdfItem is null."));
            insert(value_type("PdfItemTypeInvalid", "The PdfItem has an invalid type and could not be be processed."));
            insert(value_type("PdfItemAbsent", "The specified PdfItem could not be created."));
            insert(value_type("PdfItemAlreadyExists", "An item already exists in the PdfDictionary with the specified key."));
            insert(value_type("BadThreadingModel", "Framework requires single threaded apartment thread state WordDocumentType::Doc or TextRecoveryEngine::MODI. Use STA on Main or call Framework using new thread with STA."));
            insert(value_type("InvalidFontsDataBase", "The specified file is not a valid font database file."));
            insert(value_type("FontsDataBaseNotFound", "The font database file does not exist."));
        }

        const char* GetDescription(const char* id) const
        {
            const char* description(id);
            if (id)
            {
                std::map <std::string, std::string>::const_iterator it(find(id));
                if (it != end())
                {
                    description = it->second.c_str();
                }
            }
            return description;
        }
    };
    static CExceptionID2Description exceptionID2Description;
    return exceptionID2Description.GetDescription(id);
}

void RaiseSolidException(const std::string& id, const char* param /* = nullptr */)
{
    std::string description = GetExceptionDescription(id.c_str());
    if (param != nullptr)
    {
        description.append(", ");
        description.append(param);
    }
    if (id == "InvalidLicense" || id == "InvalidCloudLicense" || id == "BadLicenseXML" || id == "LicenseExpired" || id == "LicenseViolation" || id == "CloudImportDenied" || id == "WebValidateDenied" || id == "LicenseWebCallError" || id == "LicenseWebCallFailure")
    {
        throw SolidFramework::InvalidLicenseException(description.c_str(), id.c_str());
    }
    if (id == "CoreModelCreationCancelled")
    {
        throw SolidFramework::OperationCancelledException(description.c_str(), id.c_str());
    }
    if (id == "BadDataOpeningFile")
    {
        throw SolidFramework::BadDataException(description.c_str(), id.c_str());
    }
    if (id == "UnknownFrameworkError")
    {
        throw SolidFramework::UnknownFrameworkError(description.c_str(), id.c_str());
    }
    if (id == "FileSystemError" || id == "SourceDirectoryNotFound")
    {
        throw SolidFramework::FrameworkIOException(description.c_str(), id.c_str());
    }
    if (id == "PathTooLong")
    {
        throw SolidFramework::PathTooLongException(description.c_str(), id.c_str());
    }
    if (id == "DirectoryNotFound")
    {
        throw SolidFramework::DirectoryNotFoundException(description.c_str(), id.c_str());
    }
    if (id == "FileAlreadyExists" || id == "FileAlreadyExists2")
    {
        throw SolidFramework::FileAlreadyExistsException(description.c_str(), id.c_str());
    }
    if (id == "InvalidPageRangeFormat" || id == "InvalidPdfDateFormat")
    {
        throw SolidFramework::FormatException(description.c_str(), id.c_str());
    }
    if (id == "ArgumentNull" || id == "PathMustBeSet" || id == "DPIValueInvalid" || id == "InvalidParameter" || 
        id == "InvalidImageType" || id == "CurrentIsExclusive" || id == "IncorrectOcrLanguage" || 
        id == "PathMustBeSetForSave" || id == "SourceFilesMustBeOne" || id == "SourceFilesMustBeOne_" || 
        id == "SourceFilesNotDefined" || id == "BadHtmlSplittingUsing" || id == "PageRangeMultipleFiles" ||
        id == "PageRotationAngleInvalid" || id == "ValidationModePdfInvalid" || id == "ValidationModeAutomaticInvalid" ||
        id == "ValidationModeOtherThanDetectInvalid" || id == "ConflictingRecontstructionModeAndWorddocumentType")
    {
        throw SolidFramework::ArgumentException(description.c_str(), id.c_str());
    }
    if (id == "ReadonlyObject" || id == "DocumentMustBeOpen" || id == "DocumentAlreadyOpen" || 
        id == "DontUseCoreModelAfterClose" || id == "DontUsePageHolderAfterClose" || id == "DontUsePageSourceAfterClose" || 
        id == "DontUsePagesModelAfterClose" || id == "DontUseNativeDocumentAfterClose" || id == "InvalidFontsDataBase")
    {
        throw SolidFramework::InvalidOperationException(description.c_str(), id.c_str());
    }
    if (id == "UserButNoOwner" || id == "NoUserAndNoOwner" || id == "UserButMustBeOwner" || id == "FileHasCopyProtection")
    {
        throw SolidFramework::PasswordRequiredException(description.c_str(), id.c_str());
    }
    throw SolidFramework::FrameworkException(description.c_str(), id.c_str());
}

void SOLIDCALLBACK RaiseException(const char* id)
{
    RaiseSolidException(id);
}

void SOLIDCALLBACK RaiseUnknownFrameworkError(const char* id)
{
    throw SolidFramework::UnknownFrameworkError(GetExceptionDescription(id), id);
}

void SOLIDCALLBACK RaiseFileNotFoundException(const wchar_t* path)
{
    std::string asciiPath;
    for (std::size_t i = 0; path[i] != 0; i++)
    {
        asciiPath += (path[i] < 128) ? (char)path[i] : '?';
    }
    throw SolidFramework::FileNotFoundException(("File " + asciiPath + " was not found.").c_str(), "FileNotFound", path);
}

void SOLIDCALLBACK RaiseArgumentException(const char* id, const char* paramName)
{
    RaiseSolidException(id, paramName);
}

class DLLWrapper
{
private:
    HANDLE handle;
public:
    DLLWrapper(): handle(NULL) {}

    ~DLLWrapper() { FreeLibrary(); }

    HANDLE GetModule() { return handle; }

    static std::wstring libraryPath;

    void FreeLibrary()
    {
        if (handle == NULL)
        {
            return;
        }
      #if defined __SOLIDWINDOWS__
        ::FreeLibrary((HMODULE)handle);
      #elif defined __SOLIDPOSIX__
        dlclose(handle);
      #else
        NOT_IMPLEMENTED
      #endif
        handle = NULL;
    }
    
    bool LoadLibrary(const std::wstring libFileName)
    {
        FreeLibrary();
        std::wstring path;
      #if defined __SOLIDWINDOWS__
        wchar_t directorySeparator = L'\\';
        wchar_t alternateSeparator = L'/';
      #else
        wchar_t directorySeparator = L'/';
        wchar_t alternateSeparator = L'\\';
      #endif
        if (!libraryPath.empty())
        {
            path.append(libraryPath);
            if (path[path.length() - 1] != directorySeparator)
            {
                path.append(1, directorySeparator);
            }
        }
        path.append(libFileName);
        if (supportPlatformIndependentPaths)
        {
            for (size_t i = 0; i < path.length(); i++)
            {
                if (path[i] == alternateSeparator) { path[i] = directorySeparator; }
            }
        }

      #if defined __SOLIDWINDOWS__
        if (path[1] == L':')
        {
            handle = (HANDLE)(::LoadLibraryExW(path.c_str(), NULL, LOAD_WITH_ALTERED_SEARCH_PATH));
        }
        else
        {
            handle = (HANDLE)(::LoadLibraryW(path.c_str()));
            if (handle == NULL)
            {
                wchar_t fullPath[MAX_PATH];
                GetFullPathName(path.c_str(), MAX_PATH, fullPath, NULL);
                handle = (HANDLE)(::LoadLibraryExW(fullPath, NULL, LOAD_WITH_ALTERED_SEARCH_PATH));
            }
        }
        return handle != NULL;
      #elif defined __SOLIDPOSIX__
        // Using RTLD_LAZY to lazy load only when we need an address.
        // Convert wstring to string
        std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
        std::string stringPath(myconv.to_bytes(path.c_str()));
        handle = (HANDLE)dlopen(stringPath.c_str(), RTLD_LAZY);
      #if defined __SOLIDMAC__
        if (handle == NULL && !libraryPath.empty())
        {
            // Try loading without the libraryPath
            stringPath = myconv.to_bytes(libFileName.c_str());
            handle = (HANDLE)dlopen(stringPath.c_str(), RTLD_LAZY);
        }
      #endif
        return handle != NULL;
      #else
        NOT_IMPLEMENTED
      #endif
    }
};
std::wstring DLLWrapper::libraryPath;

HANDLE GetSolidFrameworkNativeModule()
{
    static DLLWrapper dllWrapper;
    static HANDLE handle = NULL;
    if (handle == NULL)
    {
      #if defined __SOLIDWINDOWS__
        dllWrapper.LoadLibrary(L"SolidFrameworkNative.dll");
      #elif defined __SOLIDMAC__
        dllWrapper.LoadLibrary(L"SolidFrameworkNative.framework/SolidFrameworkNative");
        handle = dllWrapper.GetModule();
        if (handle == NULL)
        {
            dllWrapper.LoadLibrary(L"Frameworks/SolidFrameworkNative.framework/SolidFrameworkNative");
            handle = dllWrapper.GetModule();
        }
        if (handle == NULL)
        {
            dllWrapper.LoadLibrary(L"../SolidFrameworkNative");
            handle = dllWrapper.GetModule();
        }
        if (handle == NULL)
        {
            dllWrapper.LoadLibrary(L"SolidFrameworkNative.dylib");
        }
      #elif defined __SOLIDPOSIX__
        dllWrapper.LoadLibrary(L"libSolidFrameworkNative.so");
      #else
        NOT_IMPLEMENTED 
      #endif
        handle = dllWrapper.GetModule();
        if (handle != NULL)
        {
            initialized = true;
            InitializeCallbacks();
            if (initializedWithPlatformIndependentPaths)
            {
                SolidFramework::SupportPlatformIndependentPaths(supportPlatformIndependentPaths);
            }
        }
    }
    return handle;
}

namespace SolidFramework
{
    void SupportPlatformIndependentPaths(bool value)
    {
        SolidFramework::Platform::Platform::SupportPlatformIndependentPaths(value);
    }

    bool Initialize(std::wstring path)
    {
        DLLWrapper::libraryPath = path;
        return GetSolidFrameworkNativeModule() != NULL;
    }

    std::wstring GetTargetPlatformName()
    {
      #if defined __SOLIDWINDOWS__
      #if defined _WIN64
        return L"Win64";
      #else
        return L"Win32";
      #endif
      #elif defined __SOLIDMAC__
        return L"OSX";
      #elif defined __SOLIDLINUX__
        return L"Linux";
      #else
        NOT_IMPLEMENTED 
      #endif
    }
}

template<typename ProcType> static ProcType GetFunction(const std::string procName)
{
    HANDLE module = GetSolidFrameworkNativeModule();
    if (nullptr == module)
    {
        // error should not be ignored
        throw std::runtime_error("Could not load SolidFrameworkNative.dll, use SolidFramework::Platform::Platform::Initialize(path) to specify the path to the SolidFramework dlls");
    }
    ProcType proc;
  #if defined __SOLIDWINDOWS__
    proc = (ProcType)(::GetProcAddress((HMODULE)module, procName.c_str()));
  #elif defined __SOLIDPOSIX__
    proc = (ProcType)(dlsym(module, procName.c_str()));
  #else
    NOT_IMPLEMENTED
  #endif
    if (nullptr == proc)
    {
        // error should not be ignored
        std::string exceptionMessage = "GetProcAddress failed for " + procName;
        throw std::runtime_error(exceptionMessage.c_str());
    }
    return proc;
}

// copies and deletes a temporary string returned by SolidFrameworkNative
std::wstring UnwrapWString(const wchar_t * tempString)
{
    typedef void (SOLIDCALLBACK *InvokeType)(const wchar_t*);
    static InvokeType deleteProc = GetFunction<InvokeType>("SWIG_DeleteWBuffer");
    std::wstring unwrappedString(tempString);
    deleteProc(tempString);
    return unwrappedString;
}

namespace SolidFramework {

    FrameworkObject::FrameworkObject():
        cPtr(nullptr),
        ownsMemory(false)
    {
    }

    void FrameworkObject::Wrap(HANDLE handle, bool ownsObject)
    {
        if (handle == nullptr) throw std::invalid_argument("HANDLE cannot be null"); // never good
        cPtr = handle;
        ownsMemory = ownsObject;
    }

    void FrameworkObject::Dispose()
    {
    }

    void FrameworkObject::Dispose(FrameworkObject::DestructorCallback destructor) {
        if (cPtr != nullptr) {
            if (ownsMemory) {
                ownsMemory = false;
                destructor(cPtr);
            }
            cPtr = nullptr;
        }
    }

} // SolidFramework

// Common typedefs
typedef BYTE (SOLIDCALLBACK *InvokeType_BH)(HANDLE);
typedef double (SOLIDCALLBACK *InvokeType_DH)(HANDLE);
typedef float (SOLIDCALLBACK *InvokeType_FH)(HANDLE);
typedef HANDLE (SOLIDCALLBACK *InvokeType_HH)(HANDLE);
typedef HANDLE (SOLIDCALLBACK *InvokeType_HHH)(HANDLE, HANDLE);
typedef HANDLE (SOLIDCALLBACK *InvokeType_HHHHSS)(HANDLE, HANDLE, HANDLE, const wchar_t*, const wchar_t*);
typedef HANDLE (SOLIDCALLBACK *InvokeType_HHI)(HANDLE, int);
typedef HANDLE (SOLIDCALLBACK *InvokeType_HHII)(HANDLE, int, int);
typedef HANDLE (SOLIDCALLBACK *InvokeType_HHIII)(HANDLE, int, int, int);
typedef HANDLE (SOLIDCALLBACK *InvokeType_HHS)(HANDLE, const wchar_t*);
typedef HANDLE (SOLIDCALLBACK *InvokeType_HHSS)(HANDLE, const wchar_t*, const wchar_t*);
typedef HANDLE (SOLIDCALLBACK *InvokeType_HI)(int);
typedef HANDLE (SOLIDCALLBACK *InvokeType_HIH)(int, HANDLE);
typedef HANDLE (SOLIDCALLBACK *InvokeType_HII)(int, int);
typedef HANDLE (SOLIDCALLBACK *InvokeType_HSH)(const wchar_t*, HANDLE);
typedef HANDLE (SOLIDCALLBACK *InvokeType_HSI)(const wchar_t*, int);
typedef HANDLE (SOLIDCALLBACK *InvokeType_HV)();
typedef int (SOLIDCALLBACK *InvokeType_IH)(HANDLE);
typedef int (SOLIDCALLBACK *InvokeType_IHF)(HANDLE, float);
typedef int (SOLIDCALLBACK *InvokeType_IHH)(HANDLE, HANDLE);
typedef int (SOLIDCALLBACK *InvokeType_IHI)(HANDLE, int);
typedef int (SOLIDCALLBACK *InvokeType_IHS)(HANDLE, const wchar_t*);
typedef int (SOLIDCALLBACK *InvokeType_IHSU)(HANDLE, const wchar_t*, unsigned int);
typedef int (SOLIDCALLBACK *InvokeType_II)(int);
typedef int (SOLIDCALLBACK *InvokeType_ISS)(const wchar_t*, const wchar_t*);
typedef int (SOLIDCALLBACK *InvokeType_ISSU)(const wchar_t*, const wchar_t*, unsigned int);
typedef int (SOLIDCALLBACK *InvokeType_ISU)(const wchar_t*, unsigned int);
typedef int (SOLIDCALLBACK *InvokeType_IV)();
typedef HANDLE (SOLIDCALLBACK *InvokeType_PC)(const char*);
typedef HANDLE (SOLIDCALLBACK *InvokeType_PS)(const wchar_t*);
typedef const wchar_t* (SOLIDCALLBACK *InvokeType_SH)(HANDLE);
typedef const wchar_t* (SOLIDCALLBACK *InvokeType_SHHU)(HANDLE, HANDLE, unsigned int);
typedef const wchar_t* (SOLIDCALLBACK *InvokeType_SHI)(HANDLE, int);
typedef const wchar_t* (SOLIDCALLBACK *InvokeType_SHS)(HANDLE, const wchar_t*);
typedef const wchar_t* (SOLIDCALLBACK *InvokeType_SS)(const wchar_t*);
typedef const wchar_t* (SOLIDCALLBACK *InvokeType_SSS)(const wchar_t*, const wchar_t*);
typedef const wchar_t* (SOLIDCALLBACK *InvokeType_SV)();
typedef unsigned int (SOLIDCALLBACK *InvokeType_UH)(HANDLE);
typedef unsigned int (SOLIDCALLBACK *InvokeType_UHF)(HANDLE, float);
typedef unsigned int (SOLIDCALLBACK *InvokeType_UHH)(HANDLE, HANDLE);
typedef unsigned int (SOLIDCALLBACK *InvokeType_UHHI)(HANDLE, HANDLE, int);
typedef unsigned int (SOLIDCALLBACK *InvokeType_UHHU)(HANDLE, HANDLE, unsigned int);
typedef unsigned int (SOLIDCALLBACK *InvokeType_UHI)(HANDLE, int);
typedef unsigned int (SOLIDCALLBACK *InvokeType_UHS)(HANDLE, const wchar_t*);
typedef void (SOLIDCALLBACK *InvokeType_VC)(const char*);
typedef void (SOLIDCALLBACK *InvokeType_VCC)(const char*, const char*);
typedef void (SOLIDCALLBACK *InvokeType_VH)(HANDLE);
typedef void (SOLIDCALLBACK *InvokeType_VHB)(HANDLE, BYTE);
typedef void (SOLIDCALLBACK *InvokeType_VHD)(HANDLE, double);
typedef void (SOLIDCALLBACK *InvokeType_VHF)(HANDLE, float);
typedef void (SOLIDCALLBACK *InvokeType_VHH)(HANDLE, HANDLE);
typedef void (SOLIDCALLBACK *InvokeType_VHI)(HANDLE, int);
typedef void (SOLIDCALLBACK *InvokeType_VHIH)(HANDLE, int, HANDLE);
typedef void (SOLIDCALLBACK *InvokeType_VHII)(HANDLE, int, int);
typedef void (SOLIDCALLBACK *InvokeType_VHS)(HANDLE, const wchar_t*);
typedef void (SOLIDCALLBACK *InvokeType_VHSH)(HANDLE, const wchar_t*, HANDLE);
typedef void (SOLIDCALLBACK *InvokeType_VHSI)(HANDLE, const wchar_t*, int);
typedef void (SOLIDCALLBACK *InvokeType_VHSU)(HANDLE, const wchar_t*, unsigned int);
typedef void (SOLIDCALLBACK *InvokeType_VHU)(HANDLE, unsigned int);
typedef void (SOLIDCALLBACK *InvokeType_VI)(int);
typedef void (SOLIDCALLBACK *InvokeType_VS)(const wchar_t*);
typedef void (SOLIDCALLBACK *InvokeType_VV)();
// Callback typedefs
typedef InvokeType_VC CallbackType_ApplicationException;
typedef InvokeType_VC CallbackType_ArithmeticException;
typedef InvokeType_VC CallbackType_DivideByZeroException;
typedef InvokeType_VC CallbackType_IndexOutOfRangeException;
typedef InvokeType_VC CallbackType_InvalidCastException;
typedef InvokeType_VC CallbackType_InvalidOperationException;
typedef InvokeType_VC CallbackType_IOException;
typedef InvokeType_VC CallbackType_NullReferenceException;
typedef InvokeType_VC CallbackType_OutOfMemoryException;
typedef InvokeType_VC CallbackType_OverflowException;
typedef InvokeType_VC CallbackType_SystemException;
typedef InvokeType_VC CallbackType_UnknownError;
typedef InvokeType_VC CallbackType_FrameworkError;
typedef InvokeType_VS CallbackType_FileNotFoundException;
typedef InvokeType_VC CallbackType_NotImplementedException;
typedef InvokeType_VCC CallbackType_ArgumentException;
typedef InvokeType_VCC CallbackType_ArgumentNullException;
typedef InvokeType_VCC CallbackType_ArgumentOutOfRangeException;
typedef InvokeType_PC CallbackType_RegisterString;
typedef InvokeType_VV CallbackType_Convert;
typedef InvokeType_VS CallbackType_AddSourceFile;
typedef InvokeType_VH CallbackType_AddSourceFile2;
typedef InvokeType_ISU CallbackType_ConvertTo;
typedef InvokeType_VV CallbackType_Close;
typedef InvokeType_SV CallbackType_GetSourceFileExtension;
typedef InvokeType_SV CallbackType_GetOutputFileExtension;
typedef InvokeType_HH CallbackType_DoConversion;
typedef InvokeType_VH CallbackType_OnProgress;
typedef InvokeType_VH CallbackType_OnWarning;
typedef InvokeType_IV CallbackType_CalcMemorySize;
typedef InvokeType_IH CallbackType_ApplyWatermark;
typedef InvokeType_VH CallbackType_OnModelChanged;
typedef InvokeType_VH CallbackType_OnPagesChanged;
typedef InvokeType_VH CallbackType_OnSaveProgress;
typedef InvokeType_VH CallbackType_OnPasswordEntered;
typedef InvokeType_VH CallbackType_OnOfferPasswordPrompt;
typedef InvokeType_VH CallbackType_OnThumbnailChanged;
typedef InvokeType_VH CallbackType_OnRepairFailed;
typedef InvokeType_VH CallbackType_Initialize;
typedef void (SOLIDCALLBACK *CallbackType_InsertRange)(int, HANDLE);
typedef void (SOLIDCALLBACK *CallbackType_InsertRange2)(int, HANDLE);
typedef void (SOLIDCALLBACK *CallbackType_InsertRange3)(int, HANDLE, unsigned int);
typedef InvokeType_VS CallbackType_OpenPDF;
typedef void (SOLIDCALLBACK *CallbackType_OpenPDF2)(const wchar_t*, const wchar_t*);
typedef void (SOLIDCALLBACK *CallbackType_OpenPDF3)(const wchar_t*, int, int, const wchar_t*);
typedef InvokeType_IV CallbackType_GetThumbnailDpi;
typedef InvokeType_VI CallbackType_SetThumbnailDpi;
typedef InvokeType_HH CallbackType_CreateImageDocument;
typedef InvokeType_SS CallbackType_GetResourceString;
// Director typedefs
typedef void (SOLIDCALLBACK *ConnectType_Exceptions)(
    CallbackType_ApplicationException,
    CallbackType_ArithmeticException,
    CallbackType_DivideByZeroException,
    CallbackType_IndexOutOfRangeException,
    CallbackType_InvalidCastException,
    CallbackType_InvalidOperationException,
    CallbackType_IOException,
    CallbackType_NullReferenceException,
    CallbackType_OutOfMemoryException,
    CallbackType_OverflowException,
    CallbackType_SystemException);
typedef void (SOLIDCALLBACK *ConnectType_SolidExceptions)(
    CallbackType_UnknownError,
    CallbackType_InvalidOperationException,
    CallbackType_FrameworkError,
    CallbackType_FileNotFoundException,
    CallbackType_NotImplementedException);
typedef void (SOLIDCALLBACK *ConnectType_ArgumentExceptions)(
    CallbackType_ArgumentException,
    CallbackType_ArgumentNullException,
    CallbackType_ArgumentOutOfRangeException);
typedef void (SOLIDCALLBACK *ConnectType_RegisterStringCallback)(
    CallbackType_RegisterString);
typedef void (SOLIDCALLBACK *ConnectType_TempFileIsAboutToBeCreated)(
    SolidFramework::Platform::Platform::TempFileIsAboutToBeCreatedCallback);
typedef void (SOLIDCALLBACK *DirectorConnectType_Converter)(HANDLE,
    CallbackType_Convert,
    CallbackType_AddSourceFile,
    CallbackType_AddSourceFile2,
    CallbackType_ConvertTo,
    CallbackType_Close,
    CallbackType_GetSourceFileExtension,
    CallbackType_GetOutputFileExtension,
    CallbackType_DoConversion,
    CallbackType_OnProgress,
    CallbackType_OnWarning,
    CallbackType_CalcMemorySize);
typedef void (SOLIDCALLBACK *DirectorConnectType_PdfToPdfConverter)(HANDLE,
    CallbackType_Convert,
    CallbackType_AddSourceFile,
    CallbackType_AddSourceFile2,
    CallbackType_ConvertTo,
    CallbackType_Close,
    CallbackType_GetSourceFileExtension,
    CallbackType_GetOutputFileExtension,
    CallbackType_DoConversion,
    CallbackType_OnProgress,
    CallbackType_OnWarning,
    CallbackType_CalcMemorySize,
    CallbackType_ApplyWatermark);
typedef void (SOLIDCALLBACK *DirectorConnectType_PdfOptions)(HANDLE,
    CallbackType_CalcMemorySize,
    CallbackType_OnProgress,
    CallbackType_OnWarning);
typedef void (SOLIDCALLBACK *DirectorConnectType_PagesModel)(HANDLE,
    CallbackType_OnModelChanged,
    CallbackType_OnPagesChanged,
    CallbackType_OnSaveProgress,
    CallbackType_OnPasswordEntered,
    CallbackType_OnOfferPasswordPrompt,
    CallbackType_OnThumbnailChanged,
    CallbackType_OnRepairFailed,
    CallbackType_Initialize,
    CallbackType_InsertRange,
    CallbackType_InsertRange2,
    CallbackType_InsertRange3,
    CallbackType_OpenPDF,
    CallbackType_OpenPDF2,
    CallbackType_OpenPDF3,
    CallbackType_GetThumbnailDpi,
    CallbackType_SetThumbnailDpi,
    CallbackType_CreateImageDocument,
    CallbackType_Close,
    CallbackType_GetResourceString,
    CallbackType_CalcMemorySize);

static bool automaticallyRegisteredCallbacks = false;
static bool manuallyRegisteringCallbacks = false;

namespace SolidFramework { namespace Platform { namespace Plumbing {

void RegisterCallbacks() {
    if (automaticallyRegisteredCallbacks) {
        automaticallyRegisteredCallbacks = false;
        return;
    }
    manuallyRegisteringCallbacks = true;
    ConnectType_RegisterStringCallback registerStringCallback = GetFunction<ConnectType_RegisterStringCallback>("PushStringHandler");
    ConnectType_Exceptions registerExceptionsCallback = GetFunction<ConnectType_Exceptions>("PushExceptionHandlers");
    ConnectType_SolidExceptions registerSolidExceptionsCallback = GetFunction<ConnectType_SolidExceptions>("PushSolidExceptionHandlers");
    ConnectType_ArgumentExceptions registerArgumentExceptionsCallback = GetFunction<ConnectType_ArgumentExceptions>("PushArgumentExceptionHandlers");
    manuallyRegisteringCallbacks = false;
    registerStringCallback(&SetString);
    registerExceptionsCallback(RaiseException, RaiseException, RaiseException, RaiseException, RaiseException, RaiseException, RaiseException, RaiseException, RaiseException, RaiseException, RaiseException);
    registerSolidExceptionsCallback(RaiseUnknownFrameworkError, RaiseException, RaiseException, RaiseFileNotFoundException, RaiseException);
    registerArgumentExceptionsCallback(RaiseArgumentException, RaiseArgumentException, RaiseArgumentException);
}

void UnregisterCallbacks() {
    automaticallyRegisteredCallbacks = false;
    static InvokeType_VV unregisterStringCallback = GetFunction<InvokeType_VV>("PopStringHandler");
    static InvokeType_VV unregisterExceptionsCallback = GetFunction<InvokeType_VV>("PopExceptionHandlers");
    static InvokeType_VV unregisterSolidExceptionsCallback = GetFunction<InvokeType_VV>("PopSolidExceptionHandlers");
    static InvokeType_VV unregisterArgumentExceptionsCallback = GetFunction<InvokeType_VV>("PopArgumentExceptionHandlers");
    unregisterStringCallback();
    unregisterExceptionsCallback();
    unregisterSolidExceptionsCallback();
    unregisterArgumentExceptionsCallback();
}

}}} // SolidFramework::Platform::Plumbing

static void InitializeCallbacks() {
    if (manuallyRegisteringCallbacks) {
        return;
    }
    if (automaticallyRegisteredCallbacks) {
        SolidFramework::Platform::Plumbing::UnregisterCallbacks();
    }
    SolidFramework::Platform::Plumbing::RegisterCallbacks();
    automaticallyRegisteredCallbacks = true;
}


class SolidFrameworkNative {
public:
    
    static void SolidFramework_Converters_Converter_SetCustomData(HANDLE arg0, HANDLE arg1) {
        static InvokeType_VHH proc = GetFunction<InvokeType_VHH>("SolidFramework_Converters_Converter_SetCustomData");
        proc(arg0, arg1);
    }
    
    static int SolidFramework_Model_Plumbing_SolidObject_GetObjectType(HANDLE arg0) {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Plumbing_SolidObject_GetObjectType");
        return proc(arg0);
    }
    
    static void SolidFramework_Converters_Converter_HandleProgress(HANDLE args) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_ProgressEventArgs_GetCustomData");
        HANDLE customData = proc(args);
        if (customData) {
            SolidFramework::ProgressEventArgsPtr wrappedArgs = SolidFramework::ProgressEventArgsPtr(new SolidFramework::ProgressEventArgs(args, true));
            ((SolidFramework::Converters::Converter*)customData)->FireProgress(wrappedArgs);
        }
    }
    
    static void SolidFramework_Converters_Converter_HandleWarning(HANDLE args) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_WarningEventArgs_GetCustomData");
        HANDLE customData = proc(args);
        if (customData) {
            SolidFramework::WarningEventArgsPtr wrappedArgs = SolidFramework::WarningEventArgsPtr(new SolidFramework::WarningEventArgs(args, true));
            ((SolidFramework::Converters::Converter*)customData)->FireWarning(wrappedArgs);
        }
    }
    
    
    static void SolidFramework_Model_Pdf_Pages_PagesModel_HandleModelChanged(HANDLE args) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Pdf_Pages_ModelChangedEventArgs_GetCustomData");
        HANDLE customData = proc(args);
        if (customData) {
            SolidFramework::Model::Pdf::Pages::ModelChangedEventArgsPtr wrappedArgs = SolidFramework::Model::Pdf::Pages::ModelChangedEventArgsPtr(new SolidFramework::Model::Pdf::Pages::ModelChangedEventArgs(args, true));
            ((SolidFramework::Model::Pdf::Pages::PagesModel*)customData)->FireModelChanged(wrappedArgs);
        }
    }
    
    static void SolidFramework_Model_Pdf_Pages_PagesModel_HandlePagesChanged(HANDLE args) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Pdf_Pages_PagesChangedEventArgs_GetCustomData");
        HANDLE customData = proc(args);
        if (customData) {
            SolidFramework::Model::Pdf::Pages::PagesChangedEventArgsPtr wrappedArgs = SolidFramework::Model::Pdf::Pages::PagesChangedEventArgsPtr(new SolidFramework::Model::Pdf::Pages::PagesChangedEventArgs(args, true));
            ((SolidFramework::Model::Pdf::Pages::PagesModel*)customData)->FirePagesChanged(wrappedArgs);
        }
    }
    
    static void SolidFramework_Model_Pdf_Pages_PagesModel_HandleSaveProgress(HANDLE args) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Pdf_Pages_SaveProgressEventArgs_GetCustomData");
        HANDLE customData = proc(args);
        if (customData) {
            SolidFramework::Model::Pdf::Pages::SaveProgressEventArgsPtr wrappedArgs = SolidFramework::Model::Pdf::Pages::SaveProgressEventArgsPtr(new SolidFramework::Model::Pdf::Pages::SaveProgressEventArgs(args, true));
            ((SolidFramework::Model::Pdf::Pages::PagesModel*)customData)->FireSaveProgress(wrappedArgs);
        }
    }
    
    static void SolidFramework_Model_Pdf_Pages_PagesModel_HandlePasswordEntered(HANDLE args) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Pdf_Pages_PasswordEnteredEventArgs_GetCustomData");
        HANDLE customData = proc(args);
        if (customData) {
            SolidFramework::Model::Pdf::Pages::PasswordEnteredEventArgsPtr wrappedArgs = SolidFramework::Model::Pdf::Pages::PasswordEnteredEventArgsPtr(new SolidFramework::Model::Pdf::Pages::PasswordEnteredEventArgs(args, true));
            ((SolidFramework::Model::Pdf::Pages::PagesModel*)customData)->FirePasswordEntered(wrappedArgs);
        }
    }
    
    static void SolidFramework_Model_Pdf_Pages_PagesModel_HandleOfferPasswordPrompt(HANDLE args) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Pdf_Pages_OfferPasswordEventArgs_GetCustomData");
        HANDLE customData = proc(args);
        if (customData) {
            SolidFramework::Model::Pdf::Pages::OfferPasswordEventArgsPtr wrappedArgs = SolidFramework::Model::Pdf::Pages::OfferPasswordEventArgsPtr(new SolidFramework::Model::Pdf::Pages::OfferPasswordEventArgs(args, true));
            ((SolidFramework::Model::Pdf::Pages::PagesModel*)customData)->FireOfferPasswordPrompt(wrappedArgs);
        }
    }
    
    static void SolidFramework_Model_Pdf_Pages_PagesModel_HandleThumbnailChanged(HANDLE args) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Pdf_Pages_ThumbnailChangedEventArgs_GetCustomData");
        HANDLE customData = proc(args);
        if (customData) {
            SolidFramework::Model::Pdf::Pages::ThumbnailChangedEventArgsPtr wrappedArgs = SolidFramework::Model::Pdf::Pages::ThumbnailChangedEventArgsPtr(new SolidFramework::Model::Pdf::Pages::ThumbnailChangedEventArgs(args, true));
            ((SolidFramework::Model::Pdf::Pages::PagesModel*)customData)->FireThumbnailChanged(wrappedArgs);
        }
    }
    
    static void SolidFramework_Model_Pdf_Pages_PagesModel_HandleRepairFailed(HANDLE args) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Pdf_Pages_RepairFailedEventArgs_GetCustomData");
        HANDLE customData = proc(args);
        if (customData) {
            SolidFramework::Model::Pdf::Pages::RepairFailedEventArgsPtr wrappedArgs = SolidFramework::Model::Pdf::Pages::RepairFailedEventArgsPtr(new SolidFramework::Model::Pdf::Pages::RepairFailedEventArgs(args, true));
            ((SolidFramework::Model::Pdf::Pages::PagesModel*)customData)->FireRepairFailed(wrappedArgs);
        }
    }
    
    
    static void SolidFramework_Model_PdfOptions_HandleProgress(HANDLE args) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_ProgressEventArgs_GetCustomData");
        HANDLE customData = proc(args);
        if (customData) {
            SolidFramework::ProgressEventArgsPtr wrappedArgs = SolidFramework::ProgressEventArgsPtr(new SolidFramework::ProgressEventArgs(args, true));
            ((SolidFramework::Model::PdfOptions*)customData)->FireProgress(wrappedArgs);
        }
    }
    
    static void SolidFramework_Model_PdfOptions_HandleWarning(HANDLE args) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_WarningEventArgs_GetCustomData");
        HANDLE customData = proc(args);
        if (customData) {
            SolidFramework::WarningEventArgsPtr wrappedArgs = SolidFramework::WarningEventArgsPtr(new SolidFramework::WarningEventArgs(args, true));
            ((SolidFramework::Model::PdfOptions*)customData)->FireWarning(wrappedArgs);
        }
    }
    
    
    static SolidFramework::Plumbing::ICatalogPtr MakeICatalog(HANDLE cPtr, bool owner) {
        owner = true;
        if (nullptr == cPtr) return nullptr;
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Plumbing_ICatalog_GetObjectType");
        switch (((SolidFramework::Plumbing::CatalogType)proc(cPtr))) {
            case SolidFramework::Plumbing::CatalogType::Catalog:
                return std::make_shared<SolidFramework::Pdf::Catalog>(cPtr, owner);
            default:
                break;
        }
        return std::make_shared<SolidFramework::Plumbing::ICatalog>(cPtr, owner);
    }
    
    static SolidFramework::Imaging::IOcrRegionPtr MakeIOcrRegion(HANDLE cPtr, bool owner) {
        owner = true;
        if (nullptr == cPtr) return nullptr;
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Imaging_IOcrRegion_GetObjectType");
        switch (((SolidFramework::Imaging::OcrRegionType)proc(cPtr))) {
            case SolidFramework::Imaging::OcrRegionType::OcrTextRegion:
                return std::make_shared<SolidFramework::Imaging::OcrTextRegion>(cPtr, owner);
            case SolidFramework::Imaging::OcrRegionType::OcrImageRegion:
                return std::make_shared<SolidFramework::Imaging::OcrImageRegion>(cPtr, owner);
            case SolidFramework::Imaging::OcrRegionType::OcrGraphicRegion:
                return std::make_shared<SolidFramework::Imaging::OcrGraphicRegion>(cPtr, owner);
            default:
                break;
        }
        return std::make_shared<SolidFramework::Imaging::IOcrRegion>(cPtr, owner);
    }
    
    static SolidFramework::Converters::Plumbing::IConversionResultPtr MakeIConversionResult(HANDLE cPtr, bool owner) {
        owner = true;
        if (nullptr == cPtr) return nullptr;
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Converters_Plumbing_IConversionResult_GetObjectType");
        switch (((SolidFramework::Converters::Plumbing::ConversionResultType)proc(cPtr))) {
            case SolidFramework::Converters::Plumbing::ConversionResultType::PdfAConversionResult:
                return std::make_shared<SolidFramework::Converters::Plumbing::PdfAConversionResult>(cPtr, owner);
            case SolidFramework::Converters::Plumbing::ConversionResultType::ConversionResult:
                return std::make_shared<SolidFramework::Converters::Plumbing::ConversionResult>(cPtr, owner);
            default:
                break;
        }
        return std::make_shared<SolidFramework::Converters::Plumbing::IConversionResult>(cPtr, owner);
    }
    
    static SolidFramework::Pdf::Plumbing::PdfItemPtr MakePdfItem(HANDLE cPtr, bool owner) {
        owner = true;
        if (nullptr == cPtr) return nullptr;
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Pdf_Plumbing_PdfItem_GetObjectType");
        switch (((SolidFramework::Pdf::Plumbing::PdfObjectType)proc(cPtr))) {
            case SolidFramework::Pdf::Plumbing::PdfObjectType::Uri:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfUriDictionary>(cPtr, owner);
            case SolidFramework::Pdf::Plumbing::PdfObjectType::GoTo:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfGoTo>(cPtr, owner);
            case SolidFramework::Pdf::Plumbing::PdfObjectType::Action:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfAction>(cPtr, owner);
            case SolidFramework::Pdf::Plumbing::PdfObjectType::Pages:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfPages>(cPtr, owner);
            case SolidFramework::Pdf::Plumbing::PdfObjectType::Page:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfPage>(cPtr, owner);
            case SolidFramework::Pdf::Plumbing::PdfObjectType::Outlines:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfOutlines>(cPtr, owner);
            case SolidFramework::Pdf::Plumbing::PdfObjectType::OutlineItem:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfOutlineItem>(cPtr, owner);
            case SolidFramework::Pdf::Plumbing::PdfObjectType::Font:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfFont>(cPtr, owner);
            case SolidFramework::Pdf::Plumbing::PdfObjectType::Stream:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfStreamObject>(cPtr, owner);
            case SolidFramework::Pdf::Plumbing::PdfObjectType::TextAnnot:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfTextAnnot>(cPtr, owner);
            case SolidFramework::Pdf::Plumbing::PdfObjectType::FreeTextAnnot:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfFreeTextAnnot>(cPtr, owner);
            case SolidFramework::Pdf::Plumbing::PdfObjectType::LineAnnot:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfLineAnnot>(cPtr, owner);
            case SolidFramework::Pdf::Plumbing::PdfObjectType::SquareAnnot:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfSquareAnnot>(cPtr, owner);
            case SolidFramework::Pdf::Plumbing::PdfObjectType::CircleAnnot:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfCircleAnnot>(cPtr, owner);
            case SolidFramework::Pdf::Plumbing::PdfObjectType::PolygonAnnot:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfPolygonAnnot>(cPtr, owner);
            case SolidFramework::Pdf::Plumbing::PdfObjectType::PolyLineAnnot:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfPolyLineAnnot>(cPtr, owner);
            case SolidFramework::Pdf::Plumbing::PdfObjectType::HighlightAnnot:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfHighlightAnnot>(cPtr, owner);
            case SolidFramework::Pdf::Plumbing::PdfObjectType::UnderlineAnnot:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfUnderlineAnnot>(cPtr, owner);
            case SolidFramework::Pdf::Plumbing::PdfObjectType::SquigglyAnnot:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfSquigglyAnnot>(cPtr, owner);
            case SolidFramework::Pdf::Plumbing::PdfObjectType::StrikeOutAnnot:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfStrikeOutAnnot>(cPtr, owner);
            case SolidFramework::Pdf::Plumbing::PdfObjectType::StampAnnot:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfStampAnnot>(cPtr, owner);
            case SolidFramework::Pdf::Plumbing::PdfObjectType::CaretAnnot:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfCaretAnnot>(cPtr, owner);
            case SolidFramework::Pdf::Plumbing::PdfObjectType::InkAnnot:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfInkAnnot>(cPtr, owner);
            case SolidFramework::Pdf::Plumbing::PdfObjectType::FileAttachmentAnnot:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfFileAttachmentAnnot>(cPtr, owner);
            case SolidFramework::Pdf::Plumbing::PdfObjectType::SoundAnnot:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfSoundAnnot>(cPtr, owner);
            case SolidFramework::Pdf::Plumbing::PdfObjectType::MarkupAnnot:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfMarkupAnnot>(cPtr, owner);
            case SolidFramework::Pdf::Plumbing::PdfObjectType::Link:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfLink>(cPtr, owner);
            case SolidFramework::Pdf::Plumbing::PdfObjectType::PopupAnnot:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfPopupAnnot>(cPtr, owner);
            case SolidFramework::Pdf::Plumbing::PdfObjectType::Annot:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfAnnot>(cPtr, owner);
            case SolidFramework::Pdf::Plumbing::PdfObjectType::Dictionary:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfDictionary>(cPtr, owner);
            case SolidFramework::Pdf::Plumbing::PdfObjectType::Kids:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfKids>(cPtr, owner);
            case SolidFramework::Pdf::Plumbing::PdfObjectType::Rectangle:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfRectangle>(cPtr, owner);
            case SolidFramework::Pdf::Plumbing::PdfObjectType::Array:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfArray>(cPtr, owner);
            case SolidFramework::Pdf::Plumbing::PdfObjectType::String:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfString>(cPtr, owner);
            case SolidFramework::Pdf::Plumbing::PdfObjectType::Reference:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfReference>(cPtr, owner);
            case SolidFramework::Pdf::Plumbing::PdfObjectType::Object:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfObject>(cPtr, owner);
            case SolidFramework::Pdf::Plumbing::PdfObjectType::Number:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfNumber>(cPtr, owner);
            case SolidFramework::Pdf::Plumbing::PdfObjectType::Null:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfNull>(cPtr, owner);
            case SolidFramework::Pdf::Plumbing::PdfObjectType::Name:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfName>(cPtr, owner);
            case SolidFramework::Pdf::Plumbing::PdfObjectType::Boolean:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfBoolean>(cPtr, owner);
            default:
                break;
        }
        return std::make_shared<SolidFramework::Pdf::Plumbing::PdfItem>(cPtr, owner);
    }
    
    static SolidFramework::Pdf::Plumbing::PdfArrayPtr MakePdfArray(HANDLE cPtr, bool owner) {
        owner = true;
        if (nullptr == cPtr) return nullptr;
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Pdf_Plumbing_PdfItem_GetObjectType");
        switch (((SolidFramework::Pdf::Plumbing::PdfObjectType)proc(cPtr))) {
            case SolidFramework::Pdf::Plumbing::PdfObjectType::Kids:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfKids>(cPtr, owner);
            case SolidFramework::Pdf::Plumbing::PdfObjectType::Rectangle:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfRectangle>(cPtr, owner);
            default:
                break;
        }
        return std::make_shared<SolidFramework::Pdf::Plumbing::PdfArray>(cPtr, owner);
    }
    
    static SolidFramework::Pdf::Plumbing::PdfDictionaryPtr MakePdfDictionary(HANDLE cPtr, bool owner) {
        owner = true;
        if (nullptr == cPtr) return nullptr;
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Pdf_Plumbing_PdfItem_GetObjectType");
        switch (((SolidFramework::Pdf::Plumbing::PdfObjectType)proc(cPtr))) {
            case SolidFramework::Pdf::Plumbing::PdfObjectType::Uri:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfUriDictionary>(cPtr, owner);
            case SolidFramework::Pdf::Plumbing::PdfObjectType::GoTo:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfGoTo>(cPtr, owner);
            case SolidFramework::Pdf::Plumbing::PdfObjectType::Action:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfAction>(cPtr, owner);
            case SolidFramework::Pdf::Plumbing::PdfObjectType::Pages:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfPages>(cPtr, owner);
            case SolidFramework::Pdf::Plumbing::PdfObjectType::Page:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfPage>(cPtr, owner);
            case SolidFramework::Pdf::Plumbing::PdfObjectType::Outlines:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfOutlines>(cPtr, owner);
            case SolidFramework::Pdf::Plumbing::PdfObjectType::OutlineItem:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfOutlineItem>(cPtr, owner);
            case SolidFramework::Pdf::Plumbing::PdfObjectType::Font:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfFont>(cPtr, owner);
            case SolidFramework::Pdf::Plumbing::PdfObjectType::Stream:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfStreamObject>(cPtr, owner);
            case SolidFramework::Pdf::Plumbing::PdfObjectType::TextAnnot:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfTextAnnot>(cPtr, owner);
            case SolidFramework::Pdf::Plumbing::PdfObjectType::FreeTextAnnot:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfFreeTextAnnot>(cPtr, owner);
            case SolidFramework::Pdf::Plumbing::PdfObjectType::LineAnnot:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfLineAnnot>(cPtr, owner);
            case SolidFramework::Pdf::Plumbing::PdfObjectType::SquareAnnot:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfSquareAnnot>(cPtr, owner);
            case SolidFramework::Pdf::Plumbing::PdfObjectType::CircleAnnot:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfCircleAnnot>(cPtr, owner);
            case SolidFramework::Pdf::Plumbing::PdfObjectType::PolygonAnnot:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfPolygonAnnot>(cPtr, owner);
            case SolidFramework::Pdf::Plumbing::PdfObjectType::PolyLineAnnot:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfPolyLineAnnot>(cPtr, owner);
            case SolidFramework::Pdf::Plumbing::PdfObjectType::HighlightAnnot:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfHighlightAnnot>(cPtr, owner);
            case SolidFramework::Pdf::Plumbing::PdfObjectType::UnderlineAnnot:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfUnderlineAnnot>(cPtr, owner);
            case SolidFramework::Pdf::Plumbing::PdfObjectType::SquigglyAnnot:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfSquigglyAnnot>(cPtr, owner);
            case SolidFramework::Pdf::Plumbing::PdfObjectType::StrikeOutAnnot:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfStrikeOutAnnot>(cPtr, owner);
            case SolidFramework::Pdf::Plumbing::PdfObjectType::StampAnnot:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfStampAnnot>(cPtr, owner);
            case SolidFramework::Pdf::Plumbing::PdfObjectType::CaretAnnot:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfCaretAnnot>(cPtr, owner);
            case SolidFramework::Pdf::Plumbing::PdfObjectType::InkAnnot:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfInkAnnot>(cPtr, owner);
            case SolidFramework::Pdf::Plumbing::PdfObjectType::FileAttachmentAnnot:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfFileAttachmentAnnot>(cPtr, owner);
            case SolidFramework::Pdf::Plumbing::PdfObjectType::SoundAnnot:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfSoundAnnot>(cPtr, owner);
            case SolidFramework::Pdf::Plumbing::PdfObjectType::MarkupAnnot:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfMarkupAnnot>(cPtr, owner);
            case SolidFramework::Pdf::Plumbing::PdfObjectType::Link:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfLink>(cPtr, owner);
            case SolidFramework::Pdf::Plumbing::PdfObjectType::PopupAnnot:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfPopupAnnot>(cPtr, owner);
            case SolidFramework::Pdf::Plumbing::PdfObjectType::Annot:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfAnnot>(cPtr, owner);
            default:
                break;
        }
        return std::make_shared<SolidFramework::Pdf::Plumbing::PdfDictionary>(cPtr, owner);
    }
    
    static SolidFramework::Pdf::Plumbing::PdfActionPtr MakePdfAction(HANDLE cPtr, bool owner) {
        owner = true;
        if (nullptr == cPtr) return nullptr;
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Pdf_Plumbing_PdfItem_GetObjectType");
        switch (((SolidFramework::Pdf::Plumbing::PdfObjectType)proc(cPtr))) {
            case SolidFramework::Pdf::Plumbing::PdfObjectType::Uri:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfUriDictionary>(cPtr, owner);
            case SolidFramework::Pdf::Plumbing::PdfObjectType::GoTo:
                return std::make_shared<SolidFramework::Pdf::Plumbing::PdfGoTo>(cPtr, owner);
            default:
                break;
        }
        return std::make_shared<SolidFramework::Pdf::Plumbing::PdfAction>(cPtr, owner);
    }
    
    static SolidFramework::Pdf::Transformers::ITransformationResultPtr MakeITransformationResult(HANDLE cPtr, bool owner) {
        owner = true;
        if (nullptr == cPtr) return nullptr;
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Pdf_Transformers_ITransformationResult_GetObjectType");
        switch (((SolidFramework::Pdf::Transformers::TransformationResultType)proc(cPtr))) {
            case SolidFramework::Pdf::Transformers::TransformationResultType::OCRTransformationResult:
                return std::make_shared<SolidFramework::Pdf::Transformers::OCRTransformationResult>(cPtr, owner);
            case SolidFramework::Pdf::Transformers::TransformationResultType::TransformationResult:
                return std::make_shared<SolidFramework::Pdf::Transformers::TransformationResult>(cPtr, owner);
            default:
                break;
        }
        return std::make_shared<SolidFramework::Pdf::Transformers::ITransformationResult>(cPtr, owner);
    }
    
    static SolidFramework::Model::Plumbing::FillStylePtr MakeFillStyle(HANDLE cPtr, bool owner) {
        owner = true;
        if (nullptr == cPtr) return nullptr;
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Plumbing_FillStyle_FillType_get");
        switch (((SolidFramework::Model::Plumbing::FillStyleType)proc(cPtr))) {
            case SolidFramework::Model::Plumbing::FillStyleType::Solid:
                return std::make_shared<SolidFramework::Model::Plumbing::SolidFill>(cPtr, owner);
            case SolidFramework::Model::Plumbing::FillStyleType::Gradient:
                return std::make_shared<SolidFramework::Model::Plumbing::GradientFill>(cPtr, owner);
            case SolidFramework::Model::Plumbing::FillStyleType::Image:
                return std::make_shared<SolidFramework::Model::Plumbing::ImageFill>(cPtr, owner);
            default:
                break;
        }
        return std::make_shared<SolidFramework::Model::Plumbing::FillStyle>(cPtr, owner);
    }
    
    static SolidFramework::Model::Plumbing::GraphicSegmentPtr MakeGraphicSegment(HANDLE cPtr, bool owner) {
        owner = true;
        if (nullptr == cPtr) return nullptr;
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Plumbing_GraphicSegment_GetObjectType");
        switch (((SolidFramework::Model::Plumbing::GraphicSegmentType)proc(cPtr))) {
            case SolidFramework::Model::Plumbing::GraphicSegmentType::BezierGraphicSegment:
                return std::make_shared<SolidFramework::Model::Plumbing::BezierGraphicSegment>(cPtr, owner);
            default:
                break;
        }
        return std::make_shared<SolidFramework::Model::Plumbing::GraphicSegment>(cPtr, owner);
    }
    
    static SolidFramework::Model::Plumbing::SolidObjectPtr MakeSolidObject(HANDLE cPtr, bool owner) {
        owner = true;
        if (nullptr == cPtr) return nullptr;
        switch (((SolidFramework::Model::Plumbing::SolidObjectType)SolidFrameworkNative::SolidFramework_Model_Plumbing_SolidObject_GetObjectType(cPtr))) {
            case SolidFramework::Model::Plumbing::SolidObjectType::SOT_HeaderFooterBlock:
                return std::make_shared<SolidFramework::Model::Plumbing::HeaderFooterBlockCollection>(cPtr, owner);
            case SolidFramework::Model::Plumbing::SolidObjectType::SOT_Header:
            case SolidFramework::Model::Plumbing::SolidObjectType::SOT_Footer:
                return std::make_shared<SolidFramework::Model::Plumbing::HeaderFooter>(cPtr, owner);
            case SolidFramework::Model::Plumbing::SolidObjectType::SOT_Cell:
                return std::make_shared<SolidFramework::Model::Plumbing::Cell>(cPtr, owner);
            case SolidFramework::Model::Plumbing::SolidObjectType::SOT_Section:
                return std::make_shared<SolidFramework::Model::Plumbing::Section>(cPtr, owner);
            case SolidFramework::Model::Plumbing::SolidObjectType::SOT_ExternalHyperlink:
                return std::make_shared<SolidFramework::Model::Plumbing::ExternalHyperlink>(cPtr, owner);
            case SolidFramework::Model::Plumbing::SolidObjectType::SOT_InternalHyperlink:
                return std::make_shared<SolidFramework::Model::Plumbing::InternalHyperlink>(cPtr, owner);
            case SolidFramework::Model::Plumbing::SolidObjectType::SOT_Font:
                return std::make_shared<SolidFramework::Model::Plumbing::Font>(cPtr, owner);
            case SolidFramework::Model::Plumbing::SolidObjectType::SOT_Hyperlink:
                return std::make_shared<SolidFramework::Model::Plumbing::Hyperlink>(cPtr, owner);
            case SolidFramework::Model::Plumbing::SolidObjectType::SOT_ParagraphStyle:
                return std::make_shared<SolidFramework::Model::Plumbing::ParagraphStyleTemplate>(cPtr, owner);
            case SolidFramework::Model::Plumbing::SolidObjectType::SOT_TextStyle:
                return std::make_shared<SolidFramework::Model::Plumbing::TextStyleTemplate>(cPtr, owner);
            case SolidFramework::Model::Plumbing::SolidObjectType::SOT_ListStyle:
                return std::make_shared<SolidFramework::Model::Plumbing::ListLevels>(cPtr, owner);
            case SolidFramework::Model::Plumbing::SolidObjectType::SOT_Style:
                return std::make_shared<SolidFramework::Model::Plumbing::StyleTemplate>(cPtr, owner);
            case SolidFramework::Model::Plumbing::SolidObjectType::SOT_Image:
                return std::make_shared<SolidFramework::Model::Plumbing::ImageShape>(cPtr, owner);
            case SolidFramework::Model::Plumbing::SolidObjectType::SOT_Graphic:
                return std::make_shared<SolidFramework::Model::Plumbing::Graphic>(cPtr, owner);
            case SolidFramework::Model::Plumbing::SolidObjectType::SOT_TextBox:
                return std::make_shared<SolidFramework::Model::Plumbing::TextBox>(cPtr, owner);
            case SolidFramework::Model::Plumbing::SolidObjectType::SOT_Table:
                return std::make_shared<SolidFramework::Model::Plumbing::Table>(cPtr, owner);
            case SolidFramework::Model::Plumbing::SolidObjectType::SOT_Group:
                return std::make_shared<SolidFramework::Model::Plumbing::Group>(cPtr, owner);
            case SolidFramework::Model::Plumbing::SolidObjectType::SOT_Shape:
                return std::make_shared<SolidFramework::Model::Plumbing::Shape>(cPtr, owner);
            case SolidFramework::Model::Plumbing::SolidObjectType::SOT_Topic:
                return std::make_shared<SolidFramework::Model::Topic>(cPtr, owner);
            case SolidFramework::Model::Plumbing::SolidObjectType::SOT_BookmarksCollection:
                return std::make_shared<SolidFramework::Model::BookmarksCollection>(cPtr, owner);
            case SolidFramework::Model::Plumbing::SolidObjectType::SOT_ListsCollection:
                return std::make_shared<SolidFramework::Model::ListsCollection>(cPtr, owner);
            case SolidFramework::Model::Plumbing::SolidObjectType::SOT_FontsCollection:
                return std::make_shared<SolidFramework::Model::FontsCollection>(cPtr, owner);
            case SolidFramework::Model::Plumbing::SolidObjectType::SOT_ParagraphStylesCollection:
                return std::make_shared<SolidFramework::Model::StyleTemplatesCollection>(cPtr, owner);
            case SolidFramework::Model::Plumbing::SolidObjectType::SOT_HyperlinksCollection:
                return std::make_shared<SolidFramework::Model::HyperlinksCollection>(cPtr, owner);
            case SolidFramework::Model::Plumbing::SolidObjectType::SOT_SolidCollection:
                return std::make_shared<SolidFramework::Model::Plumbing::SolidCollection>(cPtr, owner);
            case SolidFramework::Model::Plumbing::SolidObjectType::SOT_Paragraph:
                return std::make_shared<SolidFramework::Model::Plumbing::Paragraph>(cPtr, owner);
            case SolidFramework::Model::Plumbing::SolidObjectType::SOT_Bookmark:
                return std::make_shared<SolidFramework::Model::Plumbing::Bookmark>(cPtr, owner);
            case SolidFramework::Model::Plumbing::SolidObjectType::SOT_List:
                return std::make_shared<SolidFramework::Model::Plumbing::List>(cPtr, owner);
            case SolidFramework::Model::Plumbing::SolidObjectType::SOT_TableOfContents:
                return std::make_shared<SolidFramework::Model::Plumbing::TOC>(cPtr, owner);
            default:
                break;
        }
        return std::make_shared<SolidFramework::Model::Plumbing::SolidObject>(cPtr, owner);
    }
    
    static SolidFramework::Model::Plumbing::StyleTemplatePtr MakeStyleTemplate(HANDLE cPtr, bool owner) {
        owner = true;
        if (nullptr == cPtr) return nullptr;
        switch (((SolidFramework::Model::Plumbing::SolidObjectType)SolidFrameworkNative::SolidFramework_Model_Plumbing_SolidObject_GetObjectType(cPtr))) {
            case SolidFramework::Model::Plumbing::SolidObjectType::SOT_ParagraphStyle:
                return std::make_shared<SolidFramework::Model::Plumbing::ParagraphStyleTemplate>(cPtr, owner);
            case SolidFramework::Model::Plumbing::SolidObjectType::SOT_TextStyle:
                return std::make_shared<SolidFramework::Model::Plumbing::TextStyleTemplate>(cPtr, owner);
            case SolidFramework::Model::Plumbing::SolidObjectType::SOT_ListStyle:
                return std::make_shared<SolidFramework::Model::Plumbing::ListLevels>(cPtr, owner);
            default:
                break;
        }
        return std::make_shared<SolidFramework::Model::Plumbing::StyleTemplate>(cPtr, owner);
    }
    
    static SolidFramework::Model::Plumbing::ShapePtr MakeShape(HANDLE cPtr, bool owner) {
        owner = true;
        if (nullptr == cPtr) return nullptr;
        switch (((SolidFramework::Model::Plumbing::SolidObjectType)SolidFrameworkNative::SolidFramework_Model_Plumbing_SolidObject_GetObjectType(cPtr))) {
            case SolidFramework::Model::Plumbing::SolidObjectType::SOT_Image:
                return std::make_shared<SolidFramework::Model::Plumbing::ImageShape>(cPtr, owner);
            case SolidFramework::Model::Plumbing::SolidObjectType::SOT_Graphic:
                return std::make_shared<SolidFramework::Model::Plumbing::Graphic>(cPtr, owner);
            case SolidFramework::Model::Plumbing::SolidObjectType::SOT_TextBox:
                return std::make_shared<SolidFramework::Model::Plumbing::TextBox>(cPtr, owner);
            case SolidFramework::Model::Plumbing::SolidObjectType::SOT_Table:
                return std::make_shared<SolidFramework::Model::Plumbing::Table>(cPtr, owner);
            case SolidFramework::Model::Plumbing::SolidObjectType::SOT_Group:
                return std::make_shared<SolidFramework::Model::Plumbing::Group>(cPtr, owner);
            default:
                break;
        }
        return std::make_shared<SolidFramework::Model::Plumbing::Shape>(cPtr, owner);
    }
    
    static SolidFramework::Model::Plumbing::HyperlinkPtr MakeHyperlink(HANDLE cPtr, bool owner) {
        owner = true;
        if (nullptr == cPtr) return nullptr;
        switch (((SolidFramework::Model::Plumbing::SolidObjectType)SolidFrameworkNative::SolidFramework_Model_Plumbing_SolidObject_GetObjectType(cPtr))) {
            case SolidFramework::Model::Plumbing::SolidObjectType::SOT_ExternalHyperlink:
                return std::make_shared<SolidFramework::Model::Plumbing::ExternalHyperlink>(cPtr, owner);
            case SolidFramework::Model::Plumbing::SolidObjectType::SOT_InternalHyperlink:
                return std::make_shared<SolidFramework::Model::Plumbing::InternalHyperlink>(cPtr, owner);
            default:
                break;
        }
        return std::make_shared<SolidFramework::Model::Plumbing::Hyperlink>(cPtr, owner);
    }
    
    static SolidFramework::Model::Layout::LayoutObjectPtr MakeLayoutObject(HANDLE cPtr, bool owner) {
        owner = true;
        if (nullptr == cPtr) return nullptr;
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Layout_LayoutObject_GetObjectType");
        switch (((SolidFramework::Model::Layout::LayoutObjectType)proc(cPtr))) {
            case SolidFramework::Model::Layout::LayoutObjectType::Page:
                return std::make_shared<SolidFramework::Model::Layout::LayoutPage>(cPtr, owner);
            case SolidFramework::Model::Layout::LayoutObjectType::Image:
                return std::make_shared<SolidFramework::Model::Layout::LayoutImage>(cPtr, owner);
            case SolidFramework::Model::Layout::LayoutObjectType::Graphic:
                return std::make_shared<SolidFramework::Model::Layout::LayoutGraphic>(cPtr, owner);
            case SolidFramework::Model::Layout::LayoutObjectType::Paragraph:
                return std::make_shared<SolidFramework::Model::Layout::LayoutParagraph>(cPtr, owner);
            case SolidFramework::Model::Layout::LayoutObjectType::ParagraphLine:
                return std::make_shared<SolidFramework::Model::Layout::LayoutParagraphLine>(cPtr, owner);
            case SolidFramework::Model::Layout::LayoutObjectType::ParagraphWord:
                return std::make_shared<SolidFramework::Model::Layout::LayoutParagraphWord>(cPtr, owner);
            case SolidFramework::Model::Layout::LayoutObjectType::ParagraphListItem:
                return std::make_shared<SolidFramework::Model::Layout::LayoutParagraphListItem>(cPtr, owner);
            case SolidFramework::Model::Layout::LayoutObjectType::TextBox:
                return std::make_shared<SolidFramework::Model::Layout::LayoutTextBox>(cPtr, owner);
            case SolidFramework::Model::Layout::LayoutObjectType::Group:
                return std::make_shared<SolidFramework::Model::Layout::LayoutGroup>(cPtr, owner);
            case SolidFramework::Model::Layout::LayoutObjectType::Table:
                return std::make_shared<SolidFramework::Model::Layout::LayoutTable>(cPtr, owner);
            default:
                break;
        }
        return std::make_shared<SolidFramework::Model::Layout::LayoutObject>(cPtr, owner);
    }
    
    static SolidFramework::Model::Pdf::Pages::IPageHolderPtr MakeIPageHolder(HANDLE cPtr, bool owner) {
        owner = true;
        if (nullptr == cPtr) return nullptr;
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Pdf_Pages_IPageHolder_GetObjectType");
        switch (((SolidFramework::Model::Pdf::Pages::PageHolderType)proc(cPtr))) {
            case SolidFramework::Model::Pdf::Pages::PageHolderType::PdfPageHolder:
                return std::make_shared<SolidFramework::Model::Pdf::Pages::PdfPageHolder>(cPtr, owner);
            case SolidFramework::Model::Pdf::Pages::PageHolderType::ImagePageHolder:
                return std::make_shared<SolidFramework::Model::Pdf::Pages::ImagePageHolder>(cPtr, owner);
            default:
                break;
        }
        return std::make_shared<SolidFramework::Model::Pdf::Pages::IPageHolder>(cPtr, owner);
    }
    
    static SolidFramework::Model::Pdf::Pages::IPageSourcePtr MakeIPageSource(HANDLE cPtr, bool owner) {
        owner = true;
        if (nullptr == cPtr) return nullptr;
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Pdf_Pages_IPageSource_GetObjectType");
        switch (((SolidFramework::Model::Pdf::Pages::PageSourceType)proc(cPtr))) {
            case SolidFramework::Model::Pdf::Pages::PageSourceType::PdfPageSource:
                return std::make_shared<SolidFramework::Model::Pdf::Pages::PdfPageSource>(cPtr, owner);
            case SolidFramework::Model::Pdf::Pages::PageSourceType::ImagePageSource:
                return std::make_shared<SolidFramework::Model::Pdf::Pages::ImagePageSource>(cPtr, owner);
            default:
                break;
        }
        return std::make_shared<SolidFramework::Model::Pdf::Pages::IPageSource>(cPtr, owner);
    }
    
    static SolidFramework::Model::Pdf::Pages::OperationPtr MakeOperation(HANDLE cPtr, bool owner) {
        owner = true;
        if (nullptr == cPtr) return nullptr;
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Pdf_Pages_Operation_GetObjectType");
        switch (((SolidFramework::Model::Pdf::Pages::ModelOperation)proc(cPtr))) {
            case SolidFramework::Model::Pdf::Pages::ModelOperation::Move:
                return std::make_shared<SolidFramework::Model::Pdf::Pages::Operations::Move>(cPtr, owner);
            case SolidFramework::Model::Pdf::Pages::ModelOperation::Copy:
                return std::make_shared<SolidFramework::Model::Pdf::Pages::Operations::Copy>(cPtr, owner);
            case SolidFramework::Model::Pdf::Pages::ModelOperation::Undo:
                return std::make_shared<SolidFramework::Model::Pdf::Pages::Operations::Undo>(cPtr, owner);
            case SolidFramework::Model::Pdf::Pages::ModelOperation::Redo:
                return std::make_shared<SolidFramework::Model::Pdf::Pages::Operations::Redo>(cPtr, owner);
            case SolidFramework::Model::Pdf::Pages::ModelOperation::Insert:
                return std::make_shared<SolidFramework::Model::Pdf::Pages::Operations::Insert>(cPtr, owner);
            case SolidFramework::Model::Pdf::Pages::ModelOperation::Delete:
                return std::make_shared<SolidFramework::Model::Pdf::Pages::Operations::Delete>(cPtr, owner);
            case SolidFramework::Model::Pdf::Pages::ModelOperation::Rotate:
                return std::make_shared<SolidFramework::Model::Pdf::Pages::Operations::Rotate>(cPtr, owner);
            case SolidFramework::Model::Pdf::Pages::ModelOperation::AddComment:
                return std::make_shared<SolidFramework::Model::Pdf::Pages::Operations::AddComment>(cPtr, owner);
            case SolidFramework::Model::Pdf::Pages::ModelOperation::DeleteComment:
                return std::make_shared<SolidFramework::Model::Pdf::Pages::Operations::DeleteComment>(cPtr, owner);
            case SolidFramework::Model::Pdf::Pages::ModelOperation::ModifyComment:
                return std::make_shared<SolidFramework::Model::Pdf::Pages::Operations::ModifyComment>(cPtr, owner);
            default:
                break;
        }
        return std::make_shared<SolidFramework::Model::Pdf::Pages::Operation>(cPtr, owner);
    }
    
    static SolidFramework::Model::Pdf::Pages::ParameterPtr MakeParameter(HANDLE cPtr, bool owner) {
        owner = true;
        if (nullptr == cPtr) return nullptr;
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Pdf_Pages_Parameter_GetObjectType");
        switch (((SolidFramework::Model::Pdf::Pages::ParameterType)proc(cPtr))) {
            case SolidFramework::Model::Pdf::Pages::ParameterType::Int:
                return std::make_shared<SolidFramework::Model::Pdf::Pages::IntParameter>(cPtr, owner);
            case SolidFramework::Model::Pdf::Pages::ParameterType::Insert:
                return std::make_shared<SolidFramework::Model::Pdf::Pages::Operations::InsertParameter>(cPtr, owner);
            case SolidFramework::Model::Pdf::Pages::ParameterType::AddComment:
                return std::make_shared<SolidFramework::Model::Pdf::Pages::Operations::AddCommentParameter>(cPtr, owner);
            case SolidFramework::Model::Pdf::Pages::ParameterType::DeleteComment:
                return std::make_shared<SolidFramework::Model::Pdf::Pages::Operations::DeleteCommentParameter>(cPtr, owner);
            case SolidFramework::Model::Pdf::Pages::ParameterType::ModifyComment:
                return std::make_shared<SolidFramework::Model::Pdf::Pages::Operations::ModifyCommentParameter>(cPtr, owner);
            default:
                break;
        }
        return std::make_shared<SolidFramework::Model::Pdf::Pages::Parameter>(cPtr, owner);
    }
    
    static SolidFramework::Model::Pdf::Pages::Plumbing::AnnotationPtr MakeAnnotation(HANDLE cPtr, bool owner) {
        owner = true;
        if (nullptr == cPtr) return nullptr;
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Pdf_Pages_Plumbing_Annotation_GetObjectType");
        switch (((SolidFramework::Model::Pdf::Pages::Plumbing::AnnotationType)proc(cPtr))) {
            case SolidFramework::Model::Pdf::Pages::Plumbing::AnnotationType::GoToLink:
                return std::make_shared<SolidFramework::Model::Pdf::Pages::Plumbing::GoToLink>(cPtr, owner);
            case SolidFramework::Model::Pdf::Pages::Plumbing::AnnotationType::URILink:
                return std::make_shared<SolidFramework::Model::Pdf::Pages::Plumbing::URILink>(cPtr, owner);
            case SolidFramework::Model::Pdf::Pages::Plumbing::AnnotationType::HighlightComment:
                return std::make_shared<SolidFramework::Model::Pdf::Pages::Plumbing::HighlightComment>(cPtr, owner);
            case SolidFramework::Model::Pdf::Pages::Plumbing::AnnotationType::StrikeOutComment:
                return std::make_shared<SolidFramework::Model::Pdf::Pages::Plumbing::StrikeOutComment>(cPtr, owner);
            case SolidFramework::Model::Pdf::Pages::Plumbing::AnnotationType::UnderlineComment:
                return std::make_shared<SolidFramework::Model::Pdf::Pages::Plumbing::UnderlineComment>(cPtr, owner);
            case SolidFramework::Model::Pdf::Pages::Plumbing::AnnotationType::TextComment:
                return std::make_shared<SolidFramework::Model::Pdf::Pages::Plumbing::TextComment>(cPtr, owner);
            case SolidFramework::Model::Pdf::Pages::Plumbing::AnnotationType::Link:
                return std::make_shared<SolidFramework::Model::Pdf::Pages::Plumbing::Link>(cPtr, owner);
            case SolidFramework::Model::Pdf::Pages::Plumbing::AnnotationType::Comment:
                return std::make_shared<SolidFramework::Model::Pdf::Pages::Plumbing::Comment>(cPtr, owner);
            default:
                break;
        }
        return std::make_shared<SolidFramework::Model::Pdf::Pages::Plumbing::Annotation>(cPtr, owner);
    }
    
    static SolidFramework::Model::Pdf::Pages::Plumbing::LinkPtr MakeLink(HANDLE cPtr, bool owner) {
        owner = true;
        if (nullptr == cPtr) return nullptr;
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Pdf_Pages_Plumbing_Annotation_GetObjectType");
        switch (((SolidFramework::Model::Pdf::Pages::Plumbing::AnnotationType)proc(cPtr))) {
            case SolidFramework::Model::Pdf::Pages::Plumbing::AnnotationType::GoToLink:
                return std::make_shared<SolidFramework::Model::Pdf::Pages::Plumbing::GoToLink>(cPtr, owner);
            case SolidFramework::Model::Pdf::Pages::Plumbing::AnnotationType::URILink:
                return std::make_shared<SolidFramework::Model::Pdf::Pages::Plumbing::URILink>(cPtr, owner);
            default:
                break;
        }
        return std::make_shared<SolidFramework::Model::Pdf::Pages::Plumbing::Link>(cPtr, owner);
    }
    
    static SolidFramework::Model::Pdf::Pages::Plumbing::CommentPtr MakeComment(HANDLE cPtr, bool owner) {
        owner = true;
        if (nullptr == cPtr) return nullptr;
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Pdf_Pages_Plumbing_Annotation_GetObjectType");
        switch (((SolidFramework::Model::Pdf::Pages::Plumbing::AnnotationType)proc(cPtr))) {
            case SolidFramework::Model::Pdf::Pages::Plumbing::AnnotationType::HighlightComment:
                return std::make_shared<SolidFramework::Model::Pdf::Pages::Plumbing::HighlightComment>(cPtr, owner);
            case SolidFramework::Model::Pdf::Pages::Plumbing::AnnotationType::StrikeOutComment:
                return std::make_shared<SolidFramework::Model::Pdf::Pages::Plumbing::StrikeOutComment>(cPtr, owner);
            case SolidFramework::Model::Pdf::Pages::Plumbing::AnnotationType::UnderlineComment:
                return std::make_shared<SolidFramework::Model::Pdf::Pages::Plumbing::UnderlineComment>(cPtr, owner);
            case SolidFramework::Model::Pdf::Pages::Plumbing::AnnotationType::TextComment:
                return std::make_shared<SolidFramework::Model::Pdf::Pages::Plumbing::TextComment>(cPtr, owner);
            default:
                break;
        }
        return std::make_shared<SolidFramework::Model::Pdf::Pages::Plumbing::Comment>(cPtr, owner);
    }
    
    class TemporaryWrapper
    {
    private:
        HANDLE cPtr;
        bool memoryOwn;
        InvokeType_VH destructor;
    public:
        TemporaryWrapper(HANDLE handle, bool ownsObject, InvokeType_VH deleteProc) {
            cPtr = handle;
            memoryOwn = ownsObject;
            destructor = deleteProc;
        }
        ~TemporaryWrapper() {
            if (memoryOwn) {
                destructor(cPtr);
            }
        }
        HANDLE GetCPtr() {
            return cPtr;
        }
    };
    
    typedef std::shared_ptr<TemporaryWrapper> TemporaryWrapperPtr;
    
    template<typename Type> static std::vector<std::shared_ptr<Type>> CreateVectorFromPtrCollection(HANDLE cPtr, bool memoryOwn, InvokeType_UH getSize, InvokeType_HHI getItem, InvokeType_VH destructor) {
        if (cPtr == nullptr) {
            return std::vector<std::shared_ptr<Type>>();
        }
        unsigned int size = getSize(cPtr);
        std::vector<std::shared_ptr<Type>> vector(size);
        for (unsigned int i = 0; i < size; i++) {
            HANDLE newCPtr = getItem(cPtr, i);
            vector[i] = newCPtr == nullptr? nullptr : std::make_shared<Type>(newCPtr, true);
        }
        if (memoryOwn) {
            destructor(cPtr);
        }
        return vector;
    }
    
    static std::vector<BYTE> BytesArrayToVector(HANDLE cPtr, bool memoryOwn) {
        static InvokeType_UH sizeProc = GetFunction<InvokeType_UH>("SolidFramework_BytesArray_size");
        typedef BYTE (SOLIDCALLBACK *InvokeType)(HANDLE, int);
        static InvokeType getItemProc = GetFunction<InvokeType>("SolidFramework_BytesArray_getitem");
        static InvokeType_VH deleteProc = GetFunction<InvokeType_VH>("delete_SolidFramework_BytesArray");
        if (cPtr == nullptr) return std::vector<BYTE>();
        unsigned int size = sizeProc(cPtr);
        std::vector<BYTE> vector(size);
        for (unsigned int i = 0; i < size; i++) {
            vector[i] = getItemProc(cPtr, i);
        }
        if (memoryOwn) {
            deleteProc(cPtr);
        }
        return vector;
    }
    
    static TemporaryWrapperPtr VectorToBytesArray(const std::vector<BYTE>& vector) {
        static InvokeType_HV createProc = GetFunction<InvokeType_HV>("new_SolidFramework_BytesArray__SWIG_0");
        static InvokeType_VHB addProc = GetFunction<InvokeType_VHB>("SolidFramework_BytesArray_Add");
        static InvokeType_VH deleteProc = GetFunction<InvokeType_VH>("delete_SolidFramework_BytesArray");
        HANDLE cPtr = createProc();
        if (cPtr == nullptr) throw SolidFramework::FrameworkException("Failed to create collection");
        for (size_t i = 0; i < vector.size(); i++)  {
            addProc(cPtr, vector[i]);
        }
        return TemporaryWrapperPtr(new TemporaryWrapper(cPtr, true, deleteProc));
    }
    
    static std::vector<SolidFramework::Converters::Plumbing::IConversionResultPtr> IConversionResultsCollectionToVector(HANDLE cPtr, bool memoryOwn) {
        static InvokeType_UH sizeProc = GetFunction<InvokeType_UH>("SolidFramework_Converters_Plumbing_IConversionResultsCollection_size");
        static InvokeType_HHI getItemProc = GetFunction<InvokeType_HHI>("SolidFramework_Converters_Plumbing_IConversionResultsCollection_getitem");
        static InvokeType_VH deleteProc = GetFunction<InvokeType_VH>("delete_SolidFramework_Converters_Plumbing_IConversionResultsCollection");
        if (cPtr == nullptr) return std::vector<SolidFramework::Converters::Plumbing::IConversionResultPtr>();
        unsigned int size = sizeProc(cPtr);
        std::vector<SolidFramework::Converters::Plumbing::IConversionResultPtr> vector(size);
        for (unsigned int i = 0; i < size; i++) {
            HANDLE newCPtr = getItemProc(cPtr, i);
            vector[i] = SolidFrameworkNative::MakeIConversionResult(newCPtr, true);
        }
        if (memoryOwn) {
            deleteProc(cPtr);
        }
        return vector;
    }
    
    static std::vector<SolidFramework::Converters::Plumbing::SelectedAreaPtr> SelectedAreasCollectionToVector(HANDLE cPtr, bool memoryOwn) {
        static InvokeType_UH sizeProc = GetFunction<InvokeType_UH>("SolidFramework_Converters_Plumbing_SelectedAreasCollection_size");
        static InvokeType_HHI getItemProc = GetFunction<InvokeType_HHI>("SolidFramework_Converters_Plumbing_SelectedAreasCollection_getitem");
        static InvokeType_VH deleteProc = GetFunction<InvokeType_VH>("delete_SolidFramework_Converters_Plumbing_SelectedAreasCollection");
        return CreateVectorFromPtrCollection<SolidFramework::Converters::Plumbing::SelectedArea>(cPtr, memoryOwn, sizeProc, getItemProc, deleteProc);
    }
    
    static std::vector<float> FloatsArrayToVector(HANDLE cPtr, bool memoryOwn) {
        static InvokeType_UH sizeProc = GetFunction<InvokeType_UH>("SolidFramework_FloatsArray_size");
        typedef float (SOLIDCALLBACK *InvokeType)(HANDLE, int);
        static InvokeType getItemProc = GetFunction<InvokeType>("SolidFramework_FloatsArray_getitem");
        static InvokeType_VH deleteProc = GetFunction<InvokeType_VH>("delete_SolidFramework_FloatsArray");
        if (cPtr == nullptr) return std::vector<float>();
        unsigned int size = sizeProc(cPtr);
        std::vector<float> vector(size);
        for (unsigned int i = 0; i < size; i++) {
            vector[i] = getItemProc(cPtr, i);
        }
        if (memoryOwn) {
            deleteProc(cPtr);
        }
        return vector;
    }
    
    static TemporaryWrapperPtr VectorToFloatsArray(const std::vector<float>& vector) {
        static InvokeType_HV createProc = GetFunction<InvokeType_HV>("new_SolidFramework_FloatsArray__SWIG_0");
        static InvokeType_VHF addProc = GetFunction<InvokeType_VHF>("SolidFramework_FloatsArray_Add");
        static InvokeType_VH deleteProc = GetFunction<InvokeType_VH>("delete_SolidFramework_FloatsArray");
        HANDLE cPtr = createProc();
        if (cPtr == nullptr) throw SolidFramework::FrameworkException("Failed to create collection");
        for (size_t i = 0; i < vector.size(); i++)  {
            addProc(cPtr, vector[i]);
        }
        return TemporaryWrapperPtr(new TemporaryWrapper(cPtr, true, deleteProc));
    }
    
    static std::vector<SolidFramework::Imaging::ImagePtr> ImagesCollectionToVector(HANDLE cPtr, bool memoryOwn) {
        static InvokeType_UH sizeProc = GetFunction<InvokeType_UH>("SolidFramework_Imaging_ImagesCollection_size");
        static InvokeType_HHI getItemProc = GetFunction<InvokeType_HHI>("SolidFramework_Imaging_ImagesCollection_getitem");
        static InvokeType_VH deleteProc = GetFunction<InvokeType_VH>("delete_SolidFramework_Imaging_ImagesCollection");
        return CreateVectorFromPtrCollection<SolidFramework::Imaging::Image>(cPtr, memoryOwn, sizeProc, getItemProc, deleteProc);
    }
    
    static std::vector<SolidFramework::Imaging::IOcrRegionPtr> IOcrRegionsCollectionToVector(HANDLE cPtr, bool memoryOwn) {
        static InvokeType_UH sizeProc = GetFunction<InvokeType_UH>("SolidFramework_Imaging_IOcrRegionsCollection_size");
        static InvokeType_HHI getItemProc = GetFunction<InvokeType_HHI>("SolidFramework_Imaging_IOcrRegionsCollection_getitem");
        static InvokeType_VH deleteProc = GetFunction<InvokeType_VH>("delete_SolidFramework_Imaging_IOcrRegionsCollection");
        if (cPtr == nullptr) return std::vector<SolidFramework::Imaging::IOcrRegionPtr>();
        unsigned int size = sizeProc(cPtr);
        std::vector<SolidFramework::Imaging::IOcrRegionPtr> vector(size);
        for (unsigned int i = 0; i < size; i++) {
            HANDLE newCPtr = getItemProc(cPtr, i);
            vector[i] = SolidFrameworkNative::MakeIOcrRegion(newCPtr, true);
        }
        if (memoryOwn) {
            deleteProc(cPtr);
        }
        return vector;
    }
    
    static std::vector<SolidFramework::Imaging::OcrLinePtr> OcrLinesCollectionToVector(HANDLE cPtr, bool memoryOwn) {
        static InvokeType_UH sizeProc = GetFunction<InvokeType_UH>("SolidFramework_Imaging_OcrLinesCollection_size");
        static InvokeType_HHI getItemProc = GetFunction<InvokeType_HHI>("SolidFramework_Imaging_OcrLinesCollection_getitem");
        static InvokeType_VH deleteProc = GetFunction<InvokeType_VH>("delete_SolidFramework_Imaging_OcrLinesCollection");
        return CreateVectorFromPtrCollection<SolidFramework::Imaging::OcrLine>(cPtr, memoryOwn, sizeProc, getItemProc, deleteProc);
    }
    
    static std::vector<SolidFramework::Imaging::OcrWordPtr> OcrWordsCollectionToVector(HANDLE cPtr, bool memoryOwn) {
        static InvokeType_UH sizeProc = GetFunction<InvokeType_UH>("SolidFramework_Imaging_OcrWordsCollection_size");
        static InvokeType_HHI getItemProc = GetFunction<InvokeType_HHI>("SolidFramework_Imaging_OcrWordsCollection_getitem");
        static InvokeType_VH deleteProc = GetFunction<InvokeType_VH>("delete_SolidFramework_Imaging_OcrWordsCollection");
        return CreateVectorFromPtrCollection<SolidFramework::Imaging::OcrWord>(cPtr, memoryOwn, sizeProc, getItemProc, deleteProc);
    }
    
    static TemporaryWrapperPtr VectorToOcrWordsCollection(const std::vector<SolidFramework::Imaging::OcrWordPtr>& vector) {
        static InvokeType_HV createProc = GetFunction<InvokeType_HV>("new_SolidFramework_Imaging_OcrWordsCollection__SWIG_0");
        static InvokeType_VHH addProc = GetFunction<InvokeType_VHH>("SolidFramework_Imaging_OcrWordsCollection_Add");
        static InvokeType_VH deleteProc = GetFunction<InvokeType_VH>("delete_SolidFramework_Imaging_OcrWordsCollection");
        HANDLE cPtr = createProc();
        if (cPtr == nullptr) throw SolidFramework::FrameworkException("Failed to create collection");
        for (size_t i = 0; i < vector.size(); i++)  {
            addProc(cPtr, SolidFramework::FrameworkObject::GetCPtr(vector[i]));
        }
        return TemporaryWrapperPtr(new TemporaryWrapper(cPtr, true, deleteProc));
    }
    
    static std::vector<SolidFramework::Interop::ColorPtr> ColorsCollectionToVector(HANDLE cPtr, bool memoryOwn) {
        static InvokeType_UH sizeProc = GetFunction<InvokeType_UH>("SolidFramework_Interop_ColorsCollection_size");
        static InvokeType_HHI getItemProc = GetFunction<InvokeType_HHI>("SolidFramework_Interop_ColorsCollection_getitem");
        static InvokeType_VH deleteProc = GetFunction<InvokeType_VH>("delete_SolidFramework_Interop_ColorsCollection");
        return CreateVectorFromPtrCollection<SolidFramework::Interop::Color>(cPtr, memoryOwn, sizeProc, getItemProc, deleteProc);
    }
    
    static TemporaryWrapperPtr VectorToColorsCollection(const std::vector<SolidFramework::Interop::ColorPtr>& vector) {
        static InvokeType_HV createProc = GetFunction<InvokeType_HV>("new_SolidFramework_Interop_ColorsCollection__SWIG_0");
        static InvokeType_VHH addProc = GetFunction<InvokeType_VHH>("SolidFramework_Interop_ColorsCollection_Add");
        static InvokeType_VH deleteProc = GetFunction<InvokeType_VH>("delete_SolidFramework_Interop_ColorsCollection");
        HANDLE cPtr = createProc();
        if (cPtr == nullptr) throw SolidFramework::FrameworkException("Failed to create collection");
        for (size_t i = 0; i < vector.size(); i++)  {
            addProc(cPtr, SolidFramework::FrameworkObject::GetCPtr(vector[i]));
        }
        return TemporaryWrapperPtr(new TemporaryWrapper(cPtr, true, deleteProc));
    }
    
    static std::vector<SolidFramework::Interop::PointFPtr> PointFsCollectionToVector(HANDLE cPtr, bool memoryOwn) {
        static InvokeType_UH sizeProc = GetFunction<InvokeType_UH>("SolidFramework_Interop_PointFsCollection_size");
        static InvokeType_HHI getItemProc = GetFunction<InvokeType_HHI>("SolidFramework_Interop_PointFsCollection_getitem");
        static InvokeType_VH deleteProc = GetFunction<InvokeType_VH>("delete_SolidFramework_Interop_PointFsCollection");
        return CreateVectorFromPtrCollection<SolidFramework::Interop::PointF>(cPtr, memoryOwn, sizeProc, getItemProc, deleteProc);
    }
    
    static TemporaryWrapperPtr VectorToPointFsCollection(const std::vector<SolidFramework::Interop::PointFPtr>& vector) {
        static InvokeType_HV createProc = GetFunction<InvokeType_HV>("new_SolidFramework_Interop_PointFsCollection__SWIG_0");
        static InvokeType_VHH addProc = GetFunction<InvokeType_VHH>("SolidFramework_Interop_PointFsCollection_Add");
        static InvokeType_VH deleteProc = GetFunction<InvokeType_VH>("delete_SolidFramework_Interop_PointFsCollection");
        HANDLE cPtr = createProc();
        if (cPtr == nullptr) throw SolidFramework::FrameworkException("Failed to create collection");
        for (size_t i = 0; i < vector.size(); i++)  {
            addProc(cPtr, SolidFramework::FrameworkObject::GetCPtr(vector[i]));
        }
        return TemporaryWrapperPtr(new TemporaryWrapper(cPtr, true, deleteProc));
    }
    
    static std::vector<SolidFramework::Model::Layout::LayoutChunkLinePtr> LayoutChunkLinesCollectionToVector(HANDLE cPtr, bool memoryOwn) {
        static InvokeType_UH sizeProc = GetFunction<InvokeType_UH>("SolidFramework_Model_Layout_LayoutChunkLinesCollection_size");
        static InvokeType_HHI getItemProc = GetFunction<InvokeType_HHI>("SolidFramework_Model_Layout_LayoutChunkLinesCollection_getitem");
        static InvokeType_VH deleteProc = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Layout_LayoutChunkLinesCollection");
        return CreateVectorFromPtrCollection<SolidFramework::Model::Layout::LayoutChunkLine>(cPtr, memoryOwn, sizeProc, getItemProc, deleteProc);
    }
    
    static std::vector<SolidFramework::Model::Layout::LayoutChunkPtr> LayoutChunksCollectionToVector(HANDLE cPtr, bool memoryOwn) {
        static InvokeType_UH sizeProc = GetFunction<InvokeType_UH>("SolidFramework_Model_Layout_LayoutChunksCollection_size");
        static InvokeType_HHI getItemProc = GetFunction<InvokeType_HHI>("SolidFramework_Model_Layout_LayoutChunksCollection_getitem");
        static InvokeType_VH deleteProc = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Layout_LayoutChunksCollection");
        return CreateVectorFromPtrCollection<SolidFramework::Model::Layout::LayoutChunk>(cPtr, memoryOwn, sizeProc, getItemProc, deleteProc);
    }
    
    static std::vector<SolidFramework::Model::Layout::LayoutObjectPtr> LayoutObjectsCollectionToVector(HANDLE cPtr, bool memoryOwn) {
        static InvokeType_UH sizeProc = GetFunction<InvokeType_UH>("SolidFramework_Model_Layout_LayoutObjectsCollection_size");
        static InvokeType_HHI getItemProc = GetFunction<InvokeType_HHI>("SolidFramework_Model_Layout_LayoutObjectsCollection_getitem");
        static InvokeType_VH deleteProc = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Layout_LayoutObjectsCollection");
        if (cPtr == nullptr) return std::vector<SolidFramework::Model::Layout::LayoutObjectPtr>();
        unsigned int size = sizeProc(cPtr);
        std::vector<SolidFramework::Model::Layout::LayoutObjectPtr> vector(size);
        for (unsigned int i = 0; i < size; i++) {
            HANDLE newCPtr = getItemProc(cPtr, i);
            vector[i] = SolidFrameworkNative::MakeLayoutObject(newCPtr, true);
        }
        if (memoryOwn) {
            deleteProc(cPtr);
        }
        return vector;
    }
    
    static std::vector<SolidFramework::Model::Pdf::Pages::ImagePageHolderPtr> ImagePageHoldersCollectionToVector(HANDLE cPtr, bool memoryOwn) {
        static InvokeType_UH sizeProc = GetFunction<InvokeType_UH>("SolidFramework_Model_Pdf_Pages_ImagePageHoldersCollection_size");
        static InvokeType_HHI getItemProc = GetFunction<InvokeType_HHI>("SolidFramework_Model_Pdf_Pages_ImagePageHoldersCollection_getitem");
        static InvokeType_VH deleteProc = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Pdf_Pages_ImagePageHoldersCollection");
        return CreateVectorFromPtrCollection<SolidFramework::Model::Pdf::Pages::ImagePageHolder>(cPtr, memoryOwn, sizeProc, getItemProc, deleteProc);
    }
    
    static std::vector<SolidFramework::Model::Pdf::Pages::IPageHolderPtr> IPageHoldersCollectionToVector(HANDLE cPtr, bool memoryOwn) {
        static InvokeType_UH sizeProc = GetFunction<InvokeType_UH>("SolidFramework_Model_Pdf_Pages_IPageHoldersCollection_size");
        static InvokeType_HHI getItemProc = GetFunction<InvokeType_HHI>("SolidFramework_Model_Pdf_Pages_IPageHoldersCollection_getitem");
        static InvokeType_VH deleteProc = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Pdf_Pages_IPageHoldersCollection");
        if (cPtr == nullptr) return std::vector<SolidFramework::Model::Pdf::Pages::IPageHolderPtr>();
        unsigned int size = sizeProc(cPtr);
        std::vector<SolidFramework::Model::Pdf::Pages::IPageHolderPtr> vector(size);
        for (unsigned int i = 0; i < size; i++) {
            HANDLE newCPtr = getItemProc(cPtr, i);
            vector[i] = SolidFrameworkNative::MakeIPageHolder(newCPtr, true);
        }
        if (memoryOwn) {
            deleteProc(cPtr);
        }
        return vector;
    }
    
    static TemporaryWrapperPtr VectorToIPageHoldersCollection(const std::vector<SolidFramework::Model::Pdf::Pages::IPageHolderPtr>& vector) {
        static InvokeType_HV createProc = GetFunction<InvokeType_HV>("new_SolidFramework_Model_Pdf_Pages_IPageHoldersCollection__SWIG_0");
        static InvokeType_VHH addProc = GetFunction<InvokeType_VHH>("SolidFramework_Model_Pdf_Pages_IPageHoldersCollection_Add");
        static InvokeType_VH deleteProc = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Pdf_Pages_IPageHoldersCollection");
        HANDLE cPtr = createProc();
        if (cPtr == nullptr) throw SolidFramework::FrameworkException("Failed to create collection");
        for (size_t i = 0; i < vector.size(); i++)  {
            addProc(cPtr, SolidFramework::FrameworkObject::GetCPtr(vector[i]));
        }
        return TemporaryWrapperPtr(new TemporaryWrapper(cPtr, true, deleteProc));
    }
    
    static std::vector<SolidFramework::Model::Pdf::Pages::IPageSourcePtr> IPageSourcesCollectionToVector(HANDLE cPtr, bool memoryOwn) {
        static InvokeType_UH sizeProc = GetFunction<InvokeType_UH>("SolidFramework_Model_Pdf_Pages_IPageSourcesCollection_size");
        static InvokeType_HHI getItemProc = GetFunction<InvokeType_HHI>("SolidFramework_Model_Pdf_Pages_IPageSourcesCollection_getitem");
        static InvokeType_VH deleteProc = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Pdf_Pages_IPageSourcesCollection");
        if (cPtr == nullptr) return std::vector<SolidFramework::Model::Pdf::Pages::IPageSourcePtr>();
        unsigned int size = sizeProc(cPtr);
        std::vector<SolidFramework::Model::Pdf::Pages::IPageSourcePtr> vector(size);
        for (unsigned int i = 0; i < size; i++) {
            HANDLE newCPtr = getItemProc(cPtr, i);
            vector[i] = SolidFrameworkNative::MakeIPageSource(newCPtr, true);
        }
        if (memoryOwn) {
            deleteProc(cPtr);
        }
        return vector;
    }
    
    static TemporaryWrapperPtr VectorToIPageSourcesCollection(const std::vector<SolidFramework::Model::Pdf::Pages::IPageSourcePtr>& vector) {
        static InvokeType_HV createProc = GetFunction<InvokeType_HV>("new_SolidFramework_Model_Pdf_Pages_IPageSourcesCollection__SWIG_0");
        static InvokeType_VHH addProc = GetFunction<InvokeType_VHH>("SolidFramework_Model_Pdf_Pages_IPageSourcesCollection_Add");
        static InvokeType_VH deleteProc = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Pdf_Pages_IPageSourcesCollection");
        HANDLE cPtr = createProc();
        if (cPtr == nullptr) throw SolidFramework::FrameworkException("Failed to create collection");
        for (size_t i = 0; i < vector.size(); i++)  {
            addProc(cPtr, SolidFramework::FrameworkObject::GetCPtr(vector[i]));
        }
        return TemporaryWrapperPtr(new TemporaryWrapper(cPtr, true, deleteProc));
    }
    
    static std::vector<SolidFramework::Model::Pdf::Pages::PageSelectionPtr> PageSelectionsCollectionToVector(HANDLE cPtr, bool memoryOwn) {
        static InvokeType_UH sizeProc = GetFunction<InvokeType_UH>("SolidFramework_Model_Pdf_Pages_PageSelectionsCollection_size");
        static InvokeType_HHI getItemProc = GetFunction<InvokeType_HHI>("SolidFramework_Model_Pdf_Pages_PageSelectionsCollection_getitem");
        static InvokeType_VH deleteProc = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Pdf_Pages_PageSelectionsCollection");
        return CreateVectorFromPtrCollection<SolidFramework::Model::Pdf::Pages::PageSelection>(cPtr, memoryOwn, sizeProc, getItemProc, deleteProc);
    }
    
    static TemporaryWrapperPtr VectorToPageSelectionsCollection(const std::vector<SolidFramework::Model::Pdf::Pages::PageSelectionPtr>& vector) {
        static InvokeType_HV createProc = GetFunction<InvokeType_HV>("new_SolidFramework_Model_Pdf_Pages_PageSelectionsCollection__SWIG_0");
        static InvokeType_VHH addProc = GetFunction<InvokeType_VHH>("SolidFramework_Model_Pdf_Pages_PageSelectionsCollection_Add");
        static InvokeType_VH deleteProc = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Pdf_Pages_PageSelectionsCollection");
        HANDLE cPtr = createProc();
        if (cPtr == nullptr) throw SolidFramework::FrameworkException("Failed to create collection");
        for (size_t i = 0; i < vector.size(); i++)  {
            addProc(cPtr, SolidFramework::FrameworkObject::GetCPtr(vector[i]));
        }
        return TemporaryWrapperPtr(new TemporaryWrapper(cPtr, true, deleteProc));
    }
    
    static std::vector<SolidFramework::Model::Pdf::Pages::Plumbing::AnnotationPtr> AnnotationsCollectionToVector(HANDLE cPtr, bool memoryOwn) {
        static InvokeType_UH sizeProc = GetFunction<InvokeType_UH>("SolidFramework_Model_Pdf_Pages_Plumbing_AnnotationsCollection_size");
        static InvokeType_HHI getItemProc = GetFunction<InvokeType_HHI>("SolidFramework_Model_Pdf_Pages_Plumbing_AnnotationsCollection_getitem");
        static InvokeType_VH deleteProc = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Pdf_Pages_Plumbing_AnnotationsCollection");
        if (cPtr == nullptr) return std::vector<SolidFramework::Model::Pdf::Pages::Plumbing::AnnotationPtr>();
        unsigned int size = sizeProc(cPtr);
        std::vector<SolidFramework::Model::Pdf::Pages::Plumbing::AnnotationPtr> vector(size);
        for (unsigned int i = 0; i < size; i++) {
            HANDLE newCPtr = getItemProc(cPtr, i);
            vector[i] = SolidFrameworkNative::MakeAnnotation(newCPtr, true);
        }
        if (memoryOwn) {
            deleteProc(cPtr);
        }
        return vector;
    }
    
    static std::vector<SolidFramework::Model::Pdf::Pages::TransactionPtr> TransactionCollectionToVector(HANDLE cPtr, bool memoryOwn) {
        static InvokeType_UH sizeProc = GetFunction<InvokeType_UH>("SolidFramework_Model_Pdf_Pages_TransactionCollection_size");
        static InvokeType_HHI getItemProc = GetFunction<InvokeType_HHI>("SolidFramework_Model_Pdf_Pages_TransactionCollection_getitem");
        static InvokeType_VH deleteProc = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Pdf_Pages_TransactionCollection");
        return CreateVectorFromPtrCollection<SolidFramework::Model::Pdf::Pages::Transaction>(cPtr, memoryOwn, sizeProc, getItemProc, deleteProc);
    }
    
    static std::vector<SolidFramework::Model::Plumbing::CellPtr> CellsCollectionToVector(HANDLE cPtr, bool memoryOwn) {
        static InvokeType_UH sizeProc = GetFunction<InvokeType_UH>("SolidFramework_Model_Plumbing_CellsCollection_size");
        static InvokeType_HHI getItemProc = GetFunction<InvokeType_HHI>("SolidFramework_Model_Plumbing_CellsCollection_getitem");
        static InvokeType_VH deleteProc = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Plumbing_CellsCollection");
        return CreateVectorFromPtrCollection<SolidFramework::Model::Plumbing::Cell>(cPtr, memoryOwn, sizeProc, getItemProc, deleteProc);
    }
    
    static std::vector<SolidFramework::Model::Plumbing::ColumnPtr> ColumnsCollectionToVector(HANDLE cPtr, bool memoryOwn) {
        static InvokeType_UH sizeProc = GetFunction<InvokeType_UH>("SolidFramework_Model_Plumbing_ColumnsCollection_size");
        static InvokeType_HHI getItemProc = GetFunction<InvokeType_HHI>("SolidFramework_Model_Plumbing_ColumnsCollection_getitem");
        static InvokeType_VH deleteProc = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Plumbing_ColumnsCollection");
        return CreateVectorFromPtrCollection<SolidFramework::Model::Plumbing::Column>(cPtr, memoryOwn, sizeProc, getItemProc, deleteProc);
    }
    
    static std::vector<SolidFramework::Model::Plumbing::GraphicPathPtr> GraphicPathsCollectionToVector(HANDLE cPtr, bool memoryOwn) {
        static InvokeType_UH sizeProc = GetFunction<InvokeType_UH>("SolidFramework_Model_Plumbing_GraphicPathsCollection_size");
        static InvokeType_HHI getItemProc = GetFunction<InvokeType_HHI>("SolidFramework_Model_Plumbing_GraphicPathsCollection_getitem");
        static InvokeType_VH deleteProc = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Plumbing_GraphicPathsCollection");
        return CreateVectorFromPtrCollection<SolidFramework::Model::Plumbing::GraphicPath>(cPtr, memoryOwn, sizeProc, getItemProc, deleteProc);
    }
    
    static std::vector<SolidFramework::Model::Plumbing::GraphicSegmentPtr> GraphicSegmentsCollectionToVector(HANDLE cPtr, bool memoryOwn) {
        static InvokeType_UH sizeProc = GetFunction<InvokeType_UH>("SolidFramework_Model_Plumbing_GraphicSegmentsCollection_size");
        static InvokeType_HHI getItemProc = GetFunction<InvokeType_HHI>("SolidFramework_Model_Plumbing_GraphicSegmentsCollection_getitem");
        static InvokeType_VH deleteProc = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Plumbing_GraphicSegmentsCollection");
        if (cPtr == nullptr) return std::vector<SolidFramework::Model::Plumbing::GraphicSegmentPtr>();
        unsigned int size = sizeProc(cPtr);
        std::vector<SolidFramework::Model::Plumbing::GraphicSegmentPtr> vector(size);
        for (unsigned int i = 0; i < size; i++) {
            HANDLE newCPtr = getItemProc(cPtr, i);
            vector[i] = SolidFrameworkNative::MakeGraphicSegment(newCPtr, true);
        }
        if (memoryOwn) {
            deleteProc(cPtr);
        }
        return vector;
    }
    
    static std::vector<SolidFramework::Model::Plumbing::ListStylePtr> ListStylesCollectionToVector(HANDLE cPtr, bool memoryOwn) {
        static InvokeType_UH sizeProc = GetFunction<InvokeType_UH>("SolidFramework_Model_Plumbing_ListStylesCollection_size");
        static InvokeType_HHI getItemProc = GetFunction<InvokeType_HHI>("SolidFramework_Model_Plumbing_ListStylesCollection_getitem");
        static InvokeType_VH deleteProc = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Plumbing_ListStylesCollection");
        return CreateVectorFromPtrCollection<SolidFramework::Model::Plumbing::ListStyle>(cPtr, memoryOwn, sizeProc, getItemProc, deleteProc);
    }
    
    static std::vector<SolidFramework::Model::Plumbing::ParagraphPtr> ParagraphsCollectionToVector(HANDLE cPtr, bool memoryOwn) {
        static InvokeType_UH sizeProc = GetFunction<InvokeType_UH>("SolidFramework_Model_Plumbing_ParagraphsCollection_size");
        static InvokeType_HHI getItemProc = GetFunction<InvokeType_HHI>("SolidFramework_Model_Plumbing_ParagraphsCollection_getitem");
        static InvokeType_VH deleteProc = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Plumbing_ParagraphsCollection");
        return CreateVectorFromPtrCollection<SolidFramework::Model::Plumbing::Paragraph>(cPtr, memoryOwn, sizeProc, getItemProc, deleteProc);
    }
    
    static std::vector<SolidFramework::Model::Plumbing::RowPtr> RowsCollectionToVector(HANDLE cPtr, bool memoryOwn) {
        static InvokeType_UH sizeProc = GetFunction<InvokeType_UH>("SolidFramework_Model_Plumbing_RowsCollection_size");
        static InvokeType_HHI getItemProc = GetFunction<InvokeType_HHI>("SolidFramework_Model_Plumbing_RowsCollection_getitem");
        static InvokeType_VH deleteProc = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Plumbing_RowsCollection");
        return CreateVectorFromPtrCollection<SolidFramework::Model::Plumbing::Row>(cPtr, memoryOwn, sizeProc, getItemProc, deleteProc);
    }
    
    static std::vector<SolidFramework::Model::Plumbing::RunPtr> RunsCollectionToVector(HANDLE cPtr, bool memoryOwn) {
        static InvokeType_UH sizeProc = GetFunction<InvokeType_UH>("SolidFramework_Model_Plumbing_RunsCollection_size");
        static InvokeType_HHI getItemProc = GetFunction<InvokeType_HHI>("SolidFramework_Model_Plumbing_RunsCollection_getitem");
        static InvokeType_VH deleteProc = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Plumbing_RunsCollection");
        return CreateVectorFromPtrCollection<SolidFramework::Model::Plumbing::Run>(cPtr, memoryOwn, sizeProc, getItemProc, deleteProc);
    }
    
    static std::vector<SolidFramework::Model::Plumbing::SolidObjectPtr> SolidObjectsCollectionToVector(HANDLE cPtr, bool memoryOwn) {
        static InvokeType_UH sizeProc = GetFunction<InvokeType_UH>("SolidFramework_Model_Plumbing_SolidObjectsCollection_size");
        static InvokeType_HHI getItemProc = GetFunction<InvokeType_HHI>("SolidFramework_Model_Plumbing_SolidObjectsCollection_getitem");
        static InvokeType_VH deleteProc = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Plumbing_SolidObjectsCollection");
        if (cPtr == nullptr) return std::vector<SolidFramework::Model::Plumbing::SolidObjectPtr>();
        unsigned int size = sizeProc(cPtr);
        std::vector<SolidFramework::Model::Plumbing::SolidObjectPtr> vector(size);
        for (unsigned int i = 0; i < size; i++) {
            HANDLE newCPtr = getItemProc(cPtr, i);
            vector[i] = SolidFrameworkNative::MakeSolidObject(newCPtr, true);
        }
        if (memoryOwn) {
            deleteProc(cPtr);
        }
        return vector;
    }
    
    static std::vector<SolidFramework::Model::Plumbing::TabStopPtr> TabStopCollectionToVector(HANDLE cPtr, bool memoryOwn) {
        static InvokeType_UH sizeProc = GetFunction<InvokeType_UH>("SolidFramework_Model_Plumbing_TabStopCollection_size");
        static InvokeType_HHI getItemProc = GetFunction<InvokeType_HHI>("SolidFramework_Model_Plumbing_TabStopCollection_getitem");
        static InvokeType_VH deleteProc = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Plumbing_TabStopCollection");
        return CreateVectorFromPtrCollection<SolidFramework::Model::Plumbing::TabStop>(cPtr, memoryOwn, sizeProc, getItemProc, deleteProc);
    }
    
    static std::vector<int> PagesArrayToVector(HANDLE cPtr, bool memoryOwn) {
        static InvokeType_UH sizeProc = GetFunction<InvokeType_UH>("SolidFramework_PagesArray_size");
        static InvokeType_IHI getItemProc = GetFunction<InvokeType_IHI>("SolidFramework_PagesArray_getitem");
        static InvokeType_VH deleteProc = GetFunction<InvokeType_VH>("delete_SolidFramework_PagesArray");
        if (cPtr == nullptr) return std::vector<int>();
        unsigned int size = sizeProc(cPtr);
        std::vector<int> vector(size);
        for (unsigned int i = 0; i < size; i++) {
            vector[i] = getItemProc(cPtr, i);
        }
        if (memoryOwn) {
            deleteProc(cPtr);
        }
        return vector;
    }
    
    static TemporaryWrapperPtr VectorToPagesArray(const std::vector<int>& vector) {
        static InvokeType_HV createProc = GetFunction<InvokeType_HV>("new_SolidFramework_PagesArray__SWIG_0");
        static InvokeType_VHI addProc = GetFunction<InvokeType_VHI>("SolidFramework_PagesArray_Add");
        static InvokeType_VH deleteProc = GetFunction<InvokeType_VH>("delete_SolidFramework_PagesArray");
        HANDLE cPtr = createProc();
        if (cPtr == nullptr) throw SolidFramework::FrameworkException("Failed to create collection");
        for (size_t i = 0; i < vector.size(); i++)  {
            addProc(cPtr, vector[i]);
        }
        return TemporaryWrapperPtr(new TemporaryWrapper(cPtr, true, deleteProc));
    }
    
    static std::vector<SolidFramework::Pdf::Plumbing::PdfItemPtr> PdfArrayItemsCollectionToVector(HANDLE cPtr, bool memoryOwn) {
        static InvokeType_UH sizeProc = GetFunction<InvokeType_UH>("SolidFramework_Pdf_Plumbing_PdfArrayItemsCollection_size");
        static InvokeType_HHI getItemProc = GetFunction<InvokeType_HHI>("SolidFramework_Pdf_Plumbing_PdfArrayItemsCollection_getitem");
        static InvokeType_VH deleteProc = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_Plumbing_PdfArrayItemsCollection");
        if (cPtr == nullptr) return std::vector<SolidFramework::Pdf::Plumbing::PdfItemPtr>();
        unsigned int size = sizeProc(cPtr);
        std::vector<SolidFramework::Pdf::Plumbing::PdfItemPtr> vector(size);
        for (unsigned int i = 0; i < size; i++) {
            HANDLE newCPtr = getItemProc(cPtr, i);
            vector[i] = SolidFrameworkNative::MakePdfItem(newCPtr, true);
        }
        if (memoryOwn) {
            deleteProc(cPtr);
        }
        return vector;
    }
    
    static std::vector<SolidFramework::Pdf::Plumbing::PdfFontPtr> PdfFontsCollectionToVector(HANDLE cPtr, bool memoryOwn) {
        static InvokeType_UH sizeProc = GetFunction<InvokeType_UH>("SolidFramework_Pdf_Plumbing_PdfFontsCollection_size");
        static InvokeType_HHI getItemProc = GetFunction<InvokeType_HHI>("SolidFramework_Pdf_Plumbing_PdfFontsCollection_getitem");
        static InvokeType_VH deleteProc = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_Plumbing_PdfFontsCollection");
        return CreateVectorFromPtrCollection<SolidFramework::Pdf::Plumbing::PdfFont>(cPtr, memoryOwn, sizeProc, getItemProc, deleteProc);
    }
    
    static std::vector<SolidFramework::Pdf::Plumbing::PdfPagePtr> PdfPagesCollectionToVector(HANDLE cPtr, bool memoryOwn) {
        static InvokeType_UH sizeProc = GetFunction<InvokeType_UH>("SolidFramework_Pdf_Plumbing_PdfPagesCollection_size");
        static InvokeType_HHI getItemProc = GetFunction<InvokeType_HHI>("SolidFramework_Pdf_Plumbing_PdfPagesCollection_getitem");
        static InvokeType_VH deleteProc = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_Plumbing_PdfPagesCollection");
        return CreateVectorFromPtrCollection<SolidFramework::Pdf::Plumbing::PdfPage>(cPtr, memoryOwn, sizeProc, getItemProc, deleteProc);
    }
    
    static std::vector<SolidFramework::Pdf::Reports::PdfAProblemPtr> PdfAProblemsCollectionToVector(HANDLE cPtr, bool memoryOwn) {
        static InvokeType_UH sizeProc = GetFunction<InvokeType_UH>("SolidFramework_Pdf_Reports_PdfAProblemsCollection_size");
        static InvokeType_HHI getItemProc = GetFunction<InvokeType_HHI>("SolidFramework_Pdf_Reports_PdfAProblemsCollection_getitem");
        static InvokeType_VH deleteProc = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_Reports_PdfAProblemsCollection");
        return CreateVectorFromPtrCollection<SolidFramework::Pdf::Reports::PdfAProblem>(cPtr, memoryOwn, sizeProc, getItemProc, deleteProc);
    }
    
    static std::vector<SolidFramework::Pdf::Transformers::ITransformationResultPtr> ITransformationResultsCollectionToVector(HANDLE cPtr, bool memoryOwn) {
        static InvokeType_UH sizeProc = GetFunction<InvokeType_UH>("SolidFramework_Pdf_Transformers_ITransformationResultsCollection_size");
        static InvokeType_HHI getItemProc = GetFunction<InvokeType_HHI>("SolidFramework_Pdf_Transformers_ITransformationResultsCollection_getitem");
        static InvokeType_VH deleteProc = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_Transformers_ITransformationResultsCollection");
        if (cPtr == nullptr) return std::vector<SolidFramework::Pdf::Transformers::ITransformationResultPtr>();
        unsigned int size = sizeProc(cPtr);
        std::vector<SolidFramework::Pdf::Transformers::ITransformationResultPtr> vector(size);
        for (unsigned int i = 0; i < size; i++) {
            HANDLE newCPtr = getItemProc(cPtr, i);
            vector[i] = SolidFrameworkNative::MakeITransformationResult(newCPtr, true);
        }
        if (memoryOwn) {
            deleteProc(cPtr);
        }
        return vector;
    }
    
    static std::vector<std::wstring> StringsArrayToVector(HANDLE cPtr, bool memoryOwn) {
        static InvokeType_UH sizeProc = GetFunction<InvokeType_UH>("SolidFramework_StringsArray_size");
        static InvokeType_SHI getItemProc = GetFunction<InvokeType_SHI>("SolidFramework_StringsArray_getitem");
        static InvokeType_VH deleteProc = GetFunction<InvokeType_VH>("delete_SolidFramework_StringsArray");
        if (cPtr == nullptr) return std::vector<std::wstring>();
        unsigned int size = sizeProc(cPtr);
        std::vector<std::wstring> vector(size);
        for (unsigned int i = 0; i < size; i++) {
            vector[i] = UnwrapWString(getItemProc(cPtr, i));
        }
        if (memoryOwn) {
            deleteProc(cPtr);
        }
        return vector;
    }
    
    static TemporaryWrapperPtr VectorToStringsArray(const std::vector<std::wstring>& vector) {
        static InvokeType_HV createProc = GetFunction<InvokeType_HV>("new_SolidFramework_StringsArray__SWIG_0");
        static InvokeType_VHS addProc = GetFunction<InvokeType_VHS>("SolidFramework_StringsArray_Add");
        static InvokeType_VH deleteProc = GetFunction<InvokeType_VH>("delete_SolidFramework_StringsArray");
        HANDLE cPtr = createProc();
        if (cPtr == nullptr) throw SolidFramework::FrameworkException("Failed to create collection");
        for (size_t i = 0; i < vector.size(); i++)  {
            addProc(cPtr, vector[i].c_str());
        }
        return TemporaryWrapperPtr(new TemporaryWrapper(cPtr, true, deleteProc));
    }
    
}; // SolidFrameworkNative



namespace SolidFramework { 
    bool License::Allows(Plumbing::LicensePermissions permissions) {
        typedef unsigned int (SOLIDCALLBACK *InvokeType)(int);
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_License_Allows");
        return 0 != proc((int)(permissions));
    }
    
    void License::Clear() {
        static InvokeType_VV proc = GetFunction<InvokeType_VV>("SolidFramework_License_Clear");
        proc();
    }
    
    void License::CloudImport(std::wstring xml) {
        static InvokeType_VS proc = GetFunction<InvokeType_VS>("SolidFramework_License_CloudImport__SWIG_0");
        proc(xml.c_str());
    }
    
    void License::CloudImport(std::wstring name, std::wstring email, std::wstring organization, std::wstring code) {
        typedef void (SOLIDCALLBACK *InvokeType)(const wchar_t*, const wchar_t*, const wchar_t*, const wchar_t*);
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_License_CloudImport__SWIG_1");
        proc(name.c_str(), email.c_str(), organization.c_str(), code.c_str());
    }
    
    void License::CloudImport(std::wstring name, std::wstring email, std::wstring organization, std::wstring code, std::wstring applicationCode) {
        typedef void (SOLIDCALLBACK *InvokeType)(const wchar_t*, const wchar_t*, const wchar_t*, const wchar_t*, const wchar_t*);
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_License_CloudImport__SWIG_2");
        proc(name.c_str(), email.c_str(), organization.c_str(), code.c_str(), applicationCode.c_str());
    }
    
    bool License::DistributionOk() {
        typedef unsigned int (SOLIDCALLBACK *InvokeType)();
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_License_DistributionOk");
        return 0 != proc();
    }
    
    bool License::FreeOk() {
        typedef unsigned int (SOLIDCALLBACK *InvokeType)();
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_License_FreeOk");
        return 0 != proc();
    }
    
    void License::Import(std::wstring name, std::wstring email, std::wstring organization, std::wstring code) {
        typedef void (SOLIDCALLBACK *InvokeType)(const wchar_t*, const wchar_t*, const wchar_t*, const wchar_t*);
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_License_Import__SWIG_0");
        proc(name.c_str(), email.c_str(), organization.c_str(), code.c_str());
    }
    
    void License::Import(std::wstring name, std::wstring email, std::wstring organization, std::wstring code, std::wstring applicationCode) {
        typedef void (SOLIDCALLBACK *InvokeType)(const wchar_t*, const wchar_t*, const wchar_t*, const wchar_t*, const wchar_t*);
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_License_Import__SWIG_1");
        proc(name.c_str(), email.c_str(), organization.c_str(), code.c_str(), applicationCode.c_str());
    }
    
    void License::Import(std::wstring xml) {
        static InvokeType_VS proc = GetFunction<InvokeType_VS>("SolidFramework_License_Import__SWIG_2");
        proc(xml.c_str());
    }
    
    void License::LoadTemporaryLicense(std::wstring license) {
        static InvokeType_VS proc = GetFunction<InvokeType_VS>("SolidFramework_License_LoadTemporaryLicense");
        proc(license.c_str());
    }
    
    Plumbing::LicensePermissions License::GetPermissions() {
        static InvokeType_IV proc = GetFunction<InvokeType_IV>("SolidFramework_License_GetPermissions");
        return (Plumbing::LicensePermissions)proc();
    }
    
    bool License::ProfessionalOcrOk() {
        typedef unsigned int (SOLIDCALLBACK *InvokeType)();
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_License_ProfessionalOcrOk");
        return 0 != proc();
    }
    
    bool License::ProfessionalOk() {
        typedef unsigned int (SOLIDCALLBACK *InvokeType)();
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_License_ProfessionalOk");
        return 0 != proc();
    }
    
    bool License::RequiresCloudImport() {
        typedef unsigned int (SOLIDCALLBACK *InvokeType)();
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_License_RequiresCloudImport");
        return 0 != proc();
    }
    
    std::wstring License::GetTemporaryLicense() {
        static InvokeType_SV proc = GetFunction<InvokeType_SV>("SolidFramework_License_GetTemporaryLicense");
        return UnwrapWString(proc());
    }
    
    bool License::ToolsOk() {
        typedef unsigned int (SOLIDCALLBACK *InvokeType)();
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_License_ToolsOk");
        return 0 != proc();
    }
    
    Plumbing::LicenseType License::GetType() {
        static InvokeType_IV proc = GetFunction<InvokeType_IV>("SolidFramework_License_GetType");
        return (Plumbing::LicenseType)proc();
    }
    
    std::wstring License::GetUniqueMachineID() {
        static InvokeType_SV proc = GetFunction<InvokeType_SV>("SolidFramework_License_GetUniqueMachineID");
        return UnwrapWString(proc());
    }
    
    std::wstring License::WebValidate(std::wstring xml) {
        static InvokeType_SS proc = GetFunction<InvokeType_SS>("SolidFramework_License_WebValidate__SWIG_0");
        return UnwrapWString(proc(xml.c_str()));
    }
    
    std::wstring License::WebValidate(std::wstring name, std::wstring email, std::wstring organization, std::wstring code) {
        typedef const wchar_t* (SOLIDCALLBACK *InvokeType)(const wchar_t*, const wchar_t*, const wchar_t*, const wchar_t*);
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_License_WebValidate__SWIG_1");
        return UnwrapWString(proc(name.c_str(), email.c_str(), organization.c_str(), code.c_str()));
    }
    
    Plumbing::LicenseState License::Validate(const std::wstring& xml, std::wstring& newXml) {
        try {
            std::wstring temp = WebValidate(xml);
            if (temp.empty()) {
                return Plumbing::LicenseState::Valid;
            }
            newXml = temp;
            return Plumbing::LicenseState::Generated;
        } catch (FrameworkException ex) {
            std::string rawMessage = ex.GetRawMessage();
            if (rawMessage == "InvalidLicense") {
                return Plumbing::LicenseState::Invalid;
            }
            if (rawMessage == "LicenseExpired") {
                return Plumbing::LicenseState::Expired;
            }
            if (rawMessage == "WebValidateDenied" || rawMessage == "CloudImportDenied") {
                return Plumbing::LicenseState::Denied;
            }
            return Plumbing::LicenseState::WebCallFailure;
        } catch (...) {
            return Plumbing::LicenseState::WebCallFailure;
        }
    }
    

    PageRange::PageRange(const std::vector<int> & range) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("new_SolidFramework_PageRange__SWIG_3");
        HANDLE handle = proc(SolidFrameworkNative::VectorToPagesArray(range)->GetCPtr());
        Wrap(handle, true);
    }
    
    PageRange::PageRange(Plumbing::DocumentPtr document) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("new_SolidFramework_PageRange__SWIG_4");
        HANDLE handle = proc(FrameworkObject::GetCPtr(document));
        Wrap(handle, true);
    }
    
    PageRange::PageRange(Plumbing::DocumentPtr document, Plumbing::PageRanges pageRangeSet) {
        static InvokeType_HHI proc = GetFunction<InvokeType_HHI>("new_SolidFramework_PageRange__SWIG_5");
        HANDLE handle = proc(FrameworkObject::GetCPtr(document), (int)(pageRangeSet));
        Wrap(handle, true);
    }
    
    void PageRange::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_PageRange");
        FrameworkObject::Dispose(destructor);
    }
    
    bool PageRange::ContainsPage(int page, int maxPagesInDocument) {
        typedef unsigned int (SOLIDCALLBACK *InvokeType)(HANDLE, int, int);
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_PageRange_ContainsPage__SWIG_0");
        return 0 != proc(cPtr, page, maxPagesInDocument);
    }
    
    bool PageRange::ContainsPage(int page) {
        static InvokeType_UHI proc = GetFunction<InvokeType_UHI>("SolidFramework_PageRange_ContainsPage__SWIG_1");
        return 0 != proc(cPtr, page);
    }
    
    int PageRange::GetCount() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_PageRange_Count_get");
        return proc(cPtr);
    }
    
    int PageRange::GetFirst() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_PageRange_First_get");
        return proc(cPtr);
    }
    
    int PageRange::GetLast() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_PageRange_Last_get");
        return proc(cPtr);
    }
    
    PageRangePtr PageRange::Parse(std::wstring range) {
        static InvokeType_PS proc = GetFunction<InvokeType_PS>("SolidFramework_PageRange_Parse");
        HANDLE newCPtr = proc(range.c_str());
        return newCPtr == nullptr? nullptr : std::make_shared<PageRange>(newCPtr, true);
    }
    
    std::vector<int> PageRange::ToArray(int maxPagesInDocument) {
        static InvokeType_HHI proc = GetFunction<InvokeType_HHI>("SolidFramework_PageRange_ToArray");
        return SolidFrameworkNative::PagesArrayToVector(proc(cPtr, maxPagesInDocument), true);
    }
    
    std::wstring PageRange::ToString() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_PageRange_ToString");
        return UnwrapWString(proc(cPtr));
    }
    
    bool PageRange::TryParse(std::wstring range, PageRangePtr& result) {
        static InvokeType_PS proc = GetFunction<InvokeType_PS>("SolidFramework_PageRange_SafeParse");
        HANDLE newCPtr = proc(range.c_str());
        result = nullptr;
        if (newCPtr != nullptr) {
            result = PageRangePtr(new PageRange(newCPtr, true));
        }
        return result != nullptr;
    }
    

    PdfAWarningEventArgs::PdfAWarningEventArgs(std::wstring message) {
        static InvokeType_HSH proc = GetFunction<InvokeType_HSH>("new_SolidFramework_PdfAWarningEventArgs");
        HANDLE handle = proc(message.c_str(), (HANDLE)this);
        Wrap(handle, true);
    }
    
    void PdfAWarningEventArgs::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_PdfAWarningEventArgs");
        FrameworkObject::Dispose(destructor);
    }
    
    PdfAWarningEventArgsPtr PdfAWarningEventArgs::DynamicCast(WarningEventArgsPtr value) {
        return std::dynamic_pointer_cast<PdfAWarningEventArgs>(value);
    }
    

    ProgressEventArgs::ProgressEventArgs() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("new_SolidFramework_ProgressEventArgs__SWIG_0");
        HANDLE handle = proc();
        Wrap(handle, true);
    }
    
    ProgressEventArgs::ProgressEventArgs(int progress, int maxProgress, Interop::SolidErrorCodes statusCode, std::wstring statusDescription) {
        typedef HANDLE (SOLIDCALLBACK *InvokeType)(int, int, int, const wchar_t*, HANDLE);
        static InvokeType proc = GetFunction<InvokeType>("new_SolidFramework_ProgressEventArgs__SWIG_1");
        HANDLE handle = proc(progress, maxProgress, (int)(statusCode), statusDescription.c_str(), (HANDLE)this);
        Wrap(handle, true);
    }
    
    void ProgressEventArgs::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_ProgressEventArgs");
        FrameworkObject::Dispose(destructor);
    }
    
    void ProgressEventArgs::Cancel() {
        static InvokeType_VH proc = GetFunction<InvokeType_VH>("SolidFramework_ProgressEventArgs_Cancel");
        proc(cPtr);
    }
    
    bool ProgressEventArgs::IsCanceled() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_ProgressEventArgs_IsCanceled_get");
        return 0 != proc(cPtr);
    }
    
    int ProgressEventArgs::GetMaxProgress() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_ProgressEventArgs_MaxProgress_get");
        return proc(cPtr);
    }
    
    void ProgressEventArgs::SetMaxProgress(int value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_ProgressEventArgs_MaxProgress_set");
        proc(cPtr, value);
    }
    
    int ProgressEventArgs::GetProgress() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_ProgressEventArgs_Progress_get");
        return proc(cPtr);
    }
    
    void ProgressEventArgs::SetProgress(int value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_ProgressEventArgs_Progress_set");
        proc(cPtr, value);
    }
    
    Interop::SolidErrorCodes ProgressEventArgs::GetStatusCode() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_ProgressEventArgs_StatusCode_get");
        return (Interop::SolidErrorCodes)proc(cPtr);
    }
    
    void ProgressEventArgs::SetStatusCode(Interop::SolidErrorCodes value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_ProgressEventArgs_StatusCode_set");
        proc(cPtr, (int)(value));
    }
    
    std::wstring ProgressEventArgs::GetStatusDescription() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_ProgressEventArgs_GetStatusDescription");
        return proc(cPtr);
    }
    
    void ProgressEventArgs::SetStatusDescription(std::wstring value) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_ProgressEventArgs_SetStatusDescription");
        proc(cPtr, value.c_str());
    }
    

    WarningEventArgs::WarningEventArgs(std::wstring message) {
        static InvokeType_HSH proc = GetFunction<InvokeType_HSH>("new_SolidFramework_WarningEventArgs");
        HANDLE handle = proc(message.c_str(), (HANDLE)this);
        Wrap(handle, true);
    }
    
    void WarningEventArgs::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_WarningEventArgs");
        FrameworkObject::Dispose(destructor);
    }
    
    std::wstring WarningEventArgs::GetMessage() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_WarningEventArgs_GetDescription");
        return proc(cPtr);
    }
    
    void WarningEventArgs::SetMessage(std::wstring value) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_WarningEventArgs_SetDescription");
        proc(cPtr, value.c_str());
    }
    
    WarningType WarningEventArgs::GetWarning() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_WarningEventArgs_Warning_get");
        return (WarningType)proc(cPtr);
    }
    
    WarningEventArgs::WarningEventArgs() {
        // only called from derived class constructors
    }
    

} // SolidFramework

namespace SolidFramework { namespace Converters { 
    void Converter::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Converters_Converter");
        FrameworkObject::Dispose(destructor);
    }
    
    void Converter::AddSourceFile(std::wstring fullpath) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Converters_Converter_AddSourceFile__SWIG_0");
        proc(cPtr, fullpath.c_str());
    }
    
    void Converter::AddSourceFile(SolidFramework::Plumbing::DocumentPtr document) {
        static InvokeType_VHH proc = GetFunction<InvokeType_VHH>("SolidFramework_Converters_Converter_AddSourceFile__SWIG_1");
        proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(document));
    }
    
    void Converter::AddSourceFiles(const std::vector<std::wstring> & paths) {
        static InvokeType_VHH proc = GetFunction<InvokeType_VHH>("SolidFramework_Converters_Converter_AddSourceFiles");
        proc(cPtr, SolidFrameworkNative::VectorToStringsArray(paths)->GetCPtr());
    }
    
    void Converter::Cancel() {
        static InvokeType_VH proc = GetFunction<InvokeType_VH>("SolidFramework_Converters_Converter_Cancel");
        proc(cPtr);
    }
    
    bool Converter::IsCanceled() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Converters_Converter_IsCanceled");
        return 0 != proc(cPtr);
    }
    
    void Converter::ClearSourceFiles() {
        static InvokeType_VH proc = GetFunction<InvokeType_VH>("SolidFramework_Converters_Converter_ClearSourceFiles");
        proc(cPtr);
    }
    
    void Converter::Convert() {
        static InvokeType_VH proc = GetFunction<InvokeType_VH>("SolidFramework_Converters_Converter_Convert");
        proc(cPtr);
    }
    
    Plumbing::ConversionStatus Converter::ConvertTo(std::wstring fullpath) {
        static InvokeType_IHS proc = GetFunction<InvokeType_IHS>("SolidFramework_Converters_Converter_ConvertTo__SWIG_0");
        return (Plumbing::ConversionStatus)proc(cPtr, fullpath.c_str());
    }
    
    Plumbing::ConversionStatus Converter::ConvertTo(std::wstring fullpath, bool overwrite) {
        static InvokeType_IHSU proc = GetFunction<InvokeType_IHSU>("SolidFramework_Converters_Converter_ConvertTo__SWIG_1");
        return (Plumbing::ConversionStatus)proc(cPtr, fullpath.c_str(), overwrite);
    }
    
    std::wstring Converter::GetOutputDirectory() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Converters_Converter_GetOutputDirectory");
        return proc(cPtr);
    }
    
    void Converter::SetOutputDirectory(std::wstring value) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Converters_Converter_SetOutputDirectory");
        proc(cPtr, value.c_str());
    }
    
    SolidFramework::Plumbing::OverwriteMode Converter::GetOverwriteMode() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Converters_Converter_OverwriteMode_get");
        return (SolidFramework::Plumbing::OverwriteMode)proc(cPtr);
    }
    
    void Converter::SetOverwriteMode(SolidFramework::Plumbing::OverwriteMode value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Converters_Converter_OverwriteMode_set");
        proc(cPtr, (int)(value));
    }
    
    std::vector<Plumbing::IConversionResultPtr> Converter::GetResults() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Converters_Converter_GetResults");
        return SolidFrameworkNative::IConversionResultsCollectionToVector(proc(cPtr), false);
    }
    
    std::wstring Converter::GetSourceDirectory() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Converters_Converter_GetSourceDirectory");
        return proc(cPtr);
    }
    
    void Converter::SetSourceDirectory(std::wstring value) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Converters_Converter_SetSourceDirectory");
        proc(cPtr, value.c_str());
    }
    
    void Converter::FireProgress(ProgressEventArgsPtr args) {
        if (OnProgress) {
            OnProgress(args);
        }
    }
    
    void Converter::FireWarning(WarningEventArgsPtr args) {
        if (OnWarning) {
            OnWarning(args);
        }
    }
    
    Converter::Converter() {
        // only called from derived class constructors
    }
    

    PdfToDataConverter::PdfToDataConverter() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("new_SolidFramework_Converters_PdfToDataConverter");
        HANDLE handle = proc();
        static DirectorConnectType_Converter directorConnect = GetFunction<DirectorConnectType_Converter>("SolidFramework_Converters_PdfToDataConverter_director_connect");
        directorConnect(handle, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, &SolidFrameworkNative::SolidFramework_Converters_Converter_HandleProgress, &SolidFrameworkNative::SolidFramework_Converters_Converter_HandleWarning, nullptr);
        Wrap(handle, true);
        SolidFrameworkNative::SolidFramework_Converters_Converter_SetCustomData(handle, (HANDLE)this);
    }
    
    void PdfToDataConverter::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Converters_PdfToDataConverter");
        FrameworkObject::Dispose(destructor);
    }
    
    void PdfToDataConverter::AddSourceFile(std::wstring fullpath) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Converters_PdfToDataConverter_AddSourceFile__SWIG_0");
        proc(cPtr, fullpath.c_str());
    }
    
    void PdfToDataConverter::AddSourceFile(SolidFramework::Plumbing::DocumentPtr document) {
        static InvokeType_VHH proc = GetFunction<InvokeType_VHH>("SolidFramework_Converters_PdfToDataConverter_AddSourceFile__SWIG_1");
        proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(document));
    }
    
    bool PdfToDataConverter::GetAutoDetectSeparators() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Converters_PdfToDataConverter_AutoDetectSeparators_get");
        return 0 != proc(cPtr);
    }
    
    void PdfToDataConverter::SetAutoDetectSeparators(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Converters_PdfToDataConverter_AutoDetectSeparators_set");
        proc(cPtr, value);
    }
    
    void PdfToDataConverter::Convert() {
        static InvokeType_VH proc = GetFunction<InvokeType_VH>("SolidFramework_Converters_PdfToDataConverter_Convert");
        proc(cPtr);
    }
    
    Plumbing::ConversionStatus PdfToDataConverter::ConvertTo(std::wstring fullpath, bool overwrite) {
        static InvokeType_IHSU proc = GetFunction<InvokeType_IHSU>("SolidFramework_Converters_PdfToDataConverter_ConvertTo");
        return (Plumbing::ConversionStatus)proc(cPtr, fullpath.c_str(), overwrite);
    }
    
    Plumbing::DecimalSeparator PdfToDataConverter::GetDecimalSeparator() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Converters_PdfToDataConverter_DecimalSeparator_get");
        return (Plumbing::DecimalSeparator)proc(cPtr);
    }
    
    void PdfToDataConverter::SetDecimalSeparator(Plumbing::DecimalSeparator value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Converters_PdfToDataConverter_DecimalSeparator_set");
        proc(cPtr, (int)(value));
    }
    
    Plumbing::DelimiterOptions PdfToDataConverter::GetDelimiterOption() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Converters_PdfToDataConverter_DelimiterOption_get");
        return (Plumbing::DelimiterOptions)proc(cPtr);
    }
    
    void PdfToDataConverter::SetDelimiterOption(Plumbing::DelimiterOptions value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Converters_PdfToDataConverter_DelimiterOption_set");
        proc(cPtr, (int)(value));
    }
    
    bool PdfToDataConverter::GetDetectTiledPages() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Converters_PdfToDataConverter_DetectTiledPages_get");
        return 0 != proc(cPtr);
    }
    
    void PdfToDataConverter::SetDetectTiledPages(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Converters_PdfToDataConverter_DetectTiledPages_set");
        proc(cPtr, value);
    }
    
    int PdfToDataConverter::GetEncoding() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Converters_PdfToDataConverter_GetEncoding");
        return proc(cPtr);
    }
    
    void PdfToDataConverter::SetEncoding(int value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Converters_PdfToDataConverter_SetEncoding");
        proc(cPtr, value);
    }
    
    Plumbing::DataExportFormat PdfToDataConverter::GetExportFormat() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Converters_PdfToDataConverter_ExportFormat_get");
        return (Plumbing::DataExportFormat)proc(cPtr);
    }
    
    void PdfToDataConverter::SetExportFormat(Plumbing::DataExportFormat value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Converters_PdfToDataConverter_ExportFormat_set");
        proc(cPtr, (int)(value));
    }
    
    Plumbing::LineTerminator PdfToDataConverter::GetLineTerminator() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Converters_PdfToDataConverter_LineTerminator_get");
        return (Plumbing::LineTerminator)proc(cPtr);
    }
    
    void PdfToDataConverter::SetLineTerminator(Plumbing::LineTerminator value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Converters_PdfToDataConverter_LineTerminator_set");
        proc(cPtr, (int)(value));
    }
    
    Plumbing::DataDocumentType PdfToDataConverter::GetOutputType() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Converters_PdfToDataConverter_OutputType_get");
        return (Plumbing::DataDocumentType)proc(cPtr);
    }
    
    void PdfToDataConverter::SetOutputType(Plumbing::DataDocumentType value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Converters_PdfToDataConverter_OutputType_set");
        proc(cPtr, (int)(value));
    }
    
    Plumbing::ExcelTablesOnSheet PdfToDataConverter::GetSingleTable() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Converters_PdfToDataConverter_SingleTable_get");
        return (Plumbing::ExcelTablesOnSheet)proc(cPtr);
    }
    
    void PdfToDataConverter::SetSingleTable(Plumbing::ExcelTablesOnSheet value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Converters_PdfToDataConverter_SingleTable_set");
        proc(cPtr, (int)(value));
    }
    
    Plumbing::ThousandsSeparator PdfToDataConverter::GetThousandsSeparator() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Converters_PdfToDataConverter_ThousandsSeparator_get");
        return (Plumbing::ThousandsSeparator)proc(cPtr);
    }
    
    void PdfToDataConverter::SetThousandsSeparator(Plumbing::ThousandsSeparator value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Converters_PdfToDataConverter_ThousandsSeparator_set");
        proc(cPtr, (int)(value));
    }
    
    PdfToDataConverterPtr PdfToDataConverter::DynamicCast(ConverterPtr value) {
        return std::dynamic_pointer_cast<PdfToDataConverter>(value);
    }
    

    PdfToExcelConverter::PdfToExcelConverter() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("new_SolidFramework_Converters_PdfToExcelConverter");
        HANDLE handle = proc();
        static DirectorConnectType_Converter directorConnect = GetFunction<DirectorConnectType_Converter>("SolidFramework_Converters_PdfToExcelConverter_director_connect");
        directorConnect(handle, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, &SolidFrameworkNative::SolidFramework_Converters_Converter_HandleProgress, &SolidFrameworkNative::SolidFramework_Converters_Converter_HandleWarning, nullptr);
        Wrap(handle, true);
        SolidFrameworkNative::SolidFramework_Converters_Converter_SetCustomData(handle, (HANDLE)this);
    }
    
    void PdfToExcelConverter::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Converters_PdfToExcelConverter");
        FrameworkObject::Dispose(destructor);
    }
    
    void PdfToExcelConverter::AddSourceFile(std::wstring fullpath) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Converters_PdfToExcelConverter_AddSourceFile__SWIG_0");
        proc(cPtr, fullpath.c_str());
    }
    
    void PdfToExcelConverter::AddSourceFile(SolidFramework::Plumbing::DocumentPtr document) {
        static InvokeType_VHH proc = GetFunction<InvokeType_VHH>("SolidFramework_Converters_PdfToExcelConverter_AddSourceFile__SWIG_1");
        proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(document));
    }
    
    bool PdfToExcelConverter::GetAutoDetectSeparators() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Converters_PdfToExcelConverter_AutoDetectSeparators_get");
        return 0 != proc(cPtr);
    }
    
    void PdfToExcelConverter::SetAutoDetectSeparators(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Converters_PdfToExcelConverter_AutoDetectSeparators_set");
        proc(cPtr, value);
    }
    
    void PdfToExcelConverter::Convert() {
        static InvokeType_VH proc = GetFunction<InvokeType_VH>("SolidFramework_Converters_PdfToExcelConverter_Convert__SWIG_0");
        proc(cPtr);
    }
    
    Plumbing::ConversionStatus PdfToExcelConverter::Convert(std::wstring sourceFileName, std::wstring destinationFileName) {
        static InvokeType_ISS proc = GetFunction<InvokeType_ISS>("SolidFramework_Converters_PdfToExcelConverter_Convert__SWIG_1");
        return (Plumbing::ConversionStatus)proc(sourceFileName.c_str(), destinationFileName.c_str());
    }
    
    Plumbing::ConversionStatus PdfToExcelConverter::Convert(std::wstring sourceFileName, std::wstring destinationFileName, bool overwrite) {
        static InvokeType_ISSU proc = GetFunction<InvokeType_ISSU>("SolidFramework_Converters_PdfToExcelConverter_Convert__SWIG_2");
        return (Plumbing::ConversionStatus)proc(sourceFileName.c_str(), destinationFileName.c_str(), overwrite);
    }
    
    Plumbing::ConversionStatus PdfToExcelConverter::ConvertTo(std::wstring fullpath, bool overwrite) {
        static InvokeType_IHSU proc = GetFunction<InvokeType_IHSU>("SolidFramework_Converters_PdfToExcelConverter_ConvertTo");
        return (Plumbing::ConversionStatus)proc(cPtr, fullpath.c_str(), overwrite);
    }
    
    Plumbing::DecimalSeparator PdfToExcelConverter::GetDecimalSeparator() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Converters_PdfToExcelConverter_DecimalSeparator_get");
        return (Plumbing::DecimalSeparator)proc(cPtr);
    }
    
    void PdfToExcelConverter::SetDecimalSeparator(Plumbing::DecimalSeparator value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Converters_PdfToExcelConverter_DecimalSeparator_set");
        proc(cPtr, (int)(value));
    }
    
    bool PdfToExcelConverter::GetDetectTiledPages() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Converters_PdfToExcelConverter_DetectTiledPages_get");
        return 0 != proc(cPtr);
    }
    
    void PdfToExcelConverter::SetDetectTiledPages(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Converters_PdfToExcelConverter_DetectTiledPages_set");
        proc(cPtr, value);
    }
    
    Plumbing::FootnotesMode PdfToExcelConverter::GetFootnotesMode() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Converters_PdfToExcelConverter_FootnotesMode_get");
        return (Plumbing::FootnotesMode)proc(cPtr);
    }
    
    void PdfToExcelConverter::SetFootnotesMode(Plumbing::FootnotesMode value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Converters_PdfToExcelConverter_FootnotesMode_set");
        proc(cPtr, (int)(value));
    }
    
    Plumbing::HeaderAndFooterMode PdfToExcelConverter::GetHeaderAndFooterMode() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Converters_PdfToExcelConverter_HeaderAndFooterMode_get");
        return (Plumbing::HeaderAndFooterMode)proc(cPtr);
    }
    
    void PdfToExcelConverter::SetHeaderAndFooterMode(Plumbing::HeaderAndFooterMode value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Converters_PdfToExcelConverter_HeaderAndFooterMode_set");
        proc(cPtr, (int)(value));
    }
    
    bool PdfToExcelConverter::GetKeepNonTableContent() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Converters_PdfToExcelConverter_KeepNonTableContent_get");
        return 0 != proc(cPtr);
    }
    
    void PdfToExcelConverter::SetKeepNonTableContent(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Converters_PdfToExcelConverter_KeepNonTableContent_set");
        proc(cPtr, value);
    }
    
    Plumbing::MarkupAnnotConversionType PdfToExcelConverter::GetMarkupAnnotConversionType() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Converters_PdfToExcelConverter_MarkupAnnotConversionType_get");
        return (Plumbing::MarkupAnnotConversionType)proc(cPtr);
    }
    
    void PdfToExcelConverter::SetMarkupAnnotConversionType(Plumbing::MarkupAnnotConversionType value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Converters_PdfToExcelConverter_MarkupAnnotConversionType_set");
        proc(cPtr, (int)(value));
    }
    
    Plumbing::ExcelDocumentType PdfToExcelConverter::GetOutputType() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Converters_PdfToExcelConverter_OutputType_get");
        return (Plumbing::ExcelDocumentType)proc(cPtr);
    }
    
    void PdfToExcelConverter::SetOutputType(Plumbing::ExcelDocumentType value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Converters_PdfToExcelConverter_OutputType_set");
        proc(cPtr, (int)(value));
    }
    
    bool PdfToExcelConverter::GetPreserveColumnsInNonTableContent() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Converters_PdfToExcelConverter_PreserveColumnsInNonTableContent_get");
        return 0 != proc(cPtr);
    }
    
    void PdfToExcelConverter::SetPreserveColumnsInNonTableContent(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Converters_PdfToExcelConverter_PreserveColumnsInNonTableContent_set");
        proc(cPtr, value);
    }
    
    Plumbing::ExcelTablesOnSheet PdfToExcelConverter::GetSingleTable() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Converters_PdfToExcelConverter_SingleTable_get");
        return (Plumbing::ExcelTablesOnSheet)proc(cPtr);
    }
    
    void PdfToExcelConverter::SetSingleTable(Plumbing::ExcelTablesOnSheet value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Converters_PdfToExcelConverter_SingleTable_set");
        proc(cPtr, (int)(value));
    }
    
    bool PdfToExcelConverter::GetTablesFromContent() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Converters_PdfToExcelConverter_TablesFromContent_get");
        return 0 != proc(cPtr);
    }
    
    void PdfToExcelConverter::SetTablesFromContent(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Converters_PdfToExcelConverter_TablesFromContent_set");
        proc(cPtr, value);
    }
    
    bool PdfToExcelConverter::GetTextAnnotationsAsContent() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Converters_PdfToExcelConverter_TextAnnotationsAsContent_get");
        return 0 != proc(cPtr);
    }
    
    void PdfToExcelConverter::SetTextAnnotationsAsContent(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Converters_PdfToExcelConverter_TextAnnotationsAsContent_set");
        proc(cPtr, value);
    }
    
    Plumbing::ThousandsSeparator PdfToExcelConverter::GetThousandsSeparator() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Converters_PdfToExcelConverter_ThousandsSeparator_get");
        return (Plumbing::ThousandsSeparator)proc(cPtr);
    }
    
    void PdfToExcelConverter::SetThousandsSeparator(Plumbing::ThousandsSeparator value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Converters_PdfToExcelConverter_ThousandsSeparator_set");
        proc(cPtr, (int)(value));
    }
    
    PdfToExcelConverterPtr PdfToExcelConverter::DynamicCast(ConverterPtr value) {
        return std::dynamic_pointer_cast<PdfToExcelConverter>(value);
    }
    

    PdfToHtmlConverter::PdfToHtmlConverter() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("new_SolidFramework_Converters_PdfToHtmlConverter");
        HANDLE handle = proc();
        static DirectorConnectType_Converter directorConnect = GetFunction<DirectorConnectType_Converter>("SolidFramework_Converters_PdfToHtmlConverter_director_connect");
        directorConnect(handle, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, &SolidFrameworkNative::SolidFramework_Converters_Converter_HandleProgress, &SolidFrameworkNative::SolidFramework_Converters_Converter_HandleWarning, nullptr);
        Wrap(handle, true);
        SolidFrameworkNative::SolidFramework_Converters_Converter_SetCustomData(handle, (HANDLE)this);
    }
    
    void PdfToHtmlConverter::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Converters_PdfToHtmlConverter");
        FrameworkObject::Dispose(destructor);
    }
    
    void PdfToHtmlConverter::AddSourceFile(std::wstring fullpath) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Converters_PdfToHtmlConverter_AddSourceFile__SWIG_0");
        proc(cPtr, fullpath.c_str());
    }
    
    void PdfToHtmlConverter::AddSourceFile(SolidFramework::Plumbing::DocumentPtr document) {
        static InvokeType_VHH proc = GetFunction<InvokeType_VHH>("SolidFramework_Converters_PdfToHtmlConverter_AddSourceFile__SWIG_1");
        proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(document));
    }
    
    void PdfToHtmlConverter::Convert() {
        static InvokeType_VH proc = GetFunction<InvokeType_VH>("SolidFramework_Converters_PdfToHtmlConverter_Convert__SWIG_0");
        proc(cPtr);
    }
    
    Plumbing::ConversionStatus PdfToHtmlConverter::Convert(std::wstring sourceFileName, std::wstring destinationFileName) {
        static InvokeType_ISS proc = GetFunction<InvokeType_ISS>("SolidFramework_Converters_PdfToHtmlConverter_Convert__SWIG_1");
        return (Plumbing::ConversionStatus)proc(sourceFileName.c_str(), destinationFileName.c_str());
    }
    
    Plumbing::ConversionStatus PdfToHtmlConverter::Convert(std::wstring sourceFileName, std::wstring destinationFileName, bool overwrite) {
        static InvokeType_ISSU proc = GetFunction<InvokeType_ISSU>("SolidFramework_Converters_PdfToHtmlConverter_Convert__SWIG_2");
        return (Plumbing::ConversionStatus)proc(sourceFileName.c_str(), destinationFileName.c_str(), overwrite);
    }
    
    Plumbing::ConversionStatus PdfToHtmlConverter::ConvertTo(std::wstring fullpath, bool overwrite) {
        static InvokeType_IHSU proc = GetFunction<InvokeType_IHSU>("SolidFramework_Converters_PdfToHtmlConverter_ConvertTo");
        return (Plumbing::ConversionStatus)proc(cPtr, fullpath.c_str(), overwrite);
    }
    
    bool PdfToHtmlConverter::GetDetectLanguage() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Converters_PdfToHtmlConverter_DetectLanguage_get");
        return 0 != proc(cPtr);
    }
    
    void PdfToHtmlConverter::SetDetectLanguage(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Converters_PdfToHtmlConverter_DetectLanguage_set");
        proc(cPtr, value);
    }
    
    bool PdfToHtmlConverter::GetDetectLists() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Converters_PdfToHtmlConverter_DetectLists_get");
        return 0 != proc(cPtr);
    }
    
    void PdfToHtmlConverter::SetDetectLists(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Converters_PdfToHtmlConverter_DetectLists_set");
        proc(cPtr, value);
    }
    
    bool PdfToHtmlConverter::GetDetectTiledPages() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Converters_PdfToHtmlConverter_DetectTiledPages_get");
        return 0 != proc(cPtr);
    }
    
    void PdfToHtmlConverter::SetDetectTiledPages(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Converters_PdfToHtmlConverter_DetectTiledPages_set");
        proc(cPtr, value);
    }
    
    bool PdfToHtmlConverter::GetExactMode() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Converters_PdfToHtmlConverter_ExactMode_get");
        return 0 != proc(cPtr);
    }
    
    void PdfToHtmlConverter::SetExactMode(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Converters_PdfToHtmlConverter_ExactMode_set");
        proc(cPtr, value);
    }
    
    Plumbing::HeaderAndFooterMode PdfToHtmlConverter::GetHeaderAndFooterMode() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Converters_PdfToHtmlConverter_HeaderAndFooterMode_get");
        return (Plumbing::HeaderAndFooterMode)proc(cPtr);
    }
    
    void PdfToHtmlConverter::SetHeaderAndFooterMode(Plumbing::HeaderAndFooterMode value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Converters_PdfToHtmlConverter_HeaderAndFooterMode_set");
        proc(cPtr, (int)(value));
    }
    
    HtmlNavigation PdfToHtmlConverter::GetHtmlNavigation() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Converters_PdfToHtmlConverter_HtmlNavigation_get");
        return (HtmlNavigation)proc(cPtr);
    }
    
    void PdfToHtmlConverter::SetHtmlNavigation(HtmlNavigation value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Converters_PdfToHtmlConverter_HtmlNavigation_set");
        proc(cPtr, (int)(value));
    }
    
    HtmlSplittingUsing PdfToHtmlConverter::GetHtmlSplittingUsing() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Converters_PdfToHtmlConverter_HtmlSplittingUsing_get");
        return (HtmlSplittingUsing)proc(cPtr);
    }
    
    void PdfToHtmlConverter::SetHtmlSplittingUsing(HtmlSplittingUsing value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Converters_PdfToHtmlConverter_HtmlSplittingUsing_set");
        proc(cPtr, (int)(value));
    }
    
    Plumbing::HtmlImages PdfToHtmlConverter::GetImages() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Converters_PdfToHtmlConverter_Images_get");
        return (Plumbing::HtmlImages)proc(cPtr);
    }
    
    void PdfToHtmlConverter::SetImages(Plumbing::HtmlImages value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Converters_PdfToHtmlConverter_Images_set");
        proc(cPtr, (int)(value));
    }
    
    Plumbing::ImageDocumentType PdfToHtmlConverter::GetImageType() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Converters_PdfToHtmlConverter_ImageType_get");
        return (Plumbing::ImageDocumentType)proc(cPtr);
    }
    
    void PdfToHtmlConverter::SetImageType(Plumbing::ImageDocumentType value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Converters_PdfToHtmlConverter_ImageType_set");
        proc(cPtr, (int)(value));
    }
    
    bool PdfToHtmlConverter::GetKeepLineBreaks() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Converters_PdfToHtmlConverter_KeepLineBreaks_get");
        return 0 != proc(cPtr);
    }
    
    void PdfToHtmlConverter::SetKeepLineBreaks(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Converters_PdfToHtmlConverter_KeepLineBreaks_set");
        proc(cPtr, value);
    }
    
    void PdfToHtmlConverter::RebaseHtml(std::wstring path, std::wstring oldFolder, std::wstring newFolder) {
        typedef void (SOLIDCALLBACK *InvokeType)(const wchar_t*, const wchar_t*, const wchar_t*);
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_Converters_PdfToHtmlConverter_RebaseHtml");
        proc(path.c_str(), oldFolder.c_str(), newFolder.c_str());
    }
    
    int PdfToHtmlConverter::GetWidthLimit() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Converters_PdfToHtmlConverter_WidthLimit_get");
        return proc(cPtr);
    }
    
    void PdfToHtmlConverter::SetWidthLimit(int value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Converters_PdfToHtmlConverter_WidthLimit_set");
        proc(cPtr, value);
    }
    
    PdfToHtmlConverterPtr PdfToHtmlConverter::DynamicCast(ConverterPtr value) {
        return std::dynamic_pointer_cast<PdfToHtmlConverter>(value);
    }
    

    PdfToImageConverter::PdfToImageConverter() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("new_SolidFramework_Converters_PdfToImageConverter");
        HANDLE handle = proc();
        static DirectorConnectType_Converter directorConnect = GetFunction<DirectorConnectType_Converter>("SolidFramework_Converters_PdfToImageConverter_director_connect");
        directorConnect(handle, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, &SolidFrameworkNative::SolidFramework_Converters_Converter_HandleProgress, &SolidFrameworkNative::SolidFramework_Converters_Converter_HandleWarning, nullptr);
        Wrap(handle, true);
        SolidFrameworkNative::SolidFramework_Converters_Converter_SetCustomData(handle, (HANDLE)this);
    }
    
    void PdfToImageConverter::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Converters_PdfToImageConverter");
        FrameworkObject::Dispose(destructor);
    }
    
    void PdfToImageConverter::AddSourceFile(std::wstring fullpath) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Converters_PdfToImageConverter_AddSourceFile__SWIG_0");
        proc(cPtr, fullpath.c_str());
    }
    
    void PdfToImageConverter::AddSourceFile(SolidFramework::Plumbing::DocumentPtr document) {
        static InvokeType_VHH proc = GetFunction<InvokeType_VHH>("SolidFramework_Converters_PdfToImageConverter_AddSourceFile__SWIG_1");
        proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(document));
    }
    
    Plumbing::ImageConversionType PdfToImageConverter::GetConversionType() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Converters_PdfToImageConverter_ConversionType_get");
        return (Plumbing::ImageConversionType)proc(cPtr);
    }
    
    void PdfToImageConverter::SetConversionType(Plumbing::ImageConversionType value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Converters_PdfToImageConverter_ConversionType_set");
        proc(cPtr, (int)(value));
    }
    
    void PdfToImageConverter::Convert() {
        static InvokeType_VH proc = GetFunction<InvokeType_VH>("SolidFramework_Converters_PdfToImageConverter_Convert");
        proc(cPtr);
    }
    
    Plumbing::ConversionStatus PdfToImageConverter::ConvertTo(std::wstring fullpath, bool overwrite) {
        static InvokeType_IHSU proc = GetFunction<InvokeType_IHSU>("SolidFramework_Converters_PdfToImageConverter_ConvertTo");
        return (Plumbing::ConversionStatus)proc(cPtr, fullpath.c_str(), overwrite);
    }
    
    SolidFramework::Pdf::Interop::DrawingCore PdfToImageConverter::GetDrawingCore() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Converters_PdfToImageConverter_DrawingCore_get");
        return (SolidFramework::Pdf::Interop::DrawingCore)proc(cPtr);
    }
    
    void PdfToImageConverter::SetDrawingCore(SolidFramework::Pdf::Interop::DrawingCore value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Converters_PdfToImageConverter_DrawingCore_set");
        proc(cPtr, (int)(value));
    }
    
    Plumbing::ImageDocumentType PdfToImageConverter::GetOutputType() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Converters_PdfToImageConverter_OutputType_get");
        return (Plumbing::ImageDocumentType)proc(cPtr);
    }
    
    void PdfToImageConverter::SetOutputType(Plumbing::ImageDocumentType value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Converters_PdfToImageConverter_OutputType_set");
        proc(cPtr, (int)(value));
    }
    
    int PdfToImageConverter::GetPageDPI() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Converters_PdfToImageConverter_PageDPI_get");
        return proc(cPtr);
    }
    
    void PdfToImageConverter::SetPageDPI(int value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Converters_PdfToImageConverter_PageDPI_set");
        proc(cPtr, value);
    }
    
    PdfToImageConverterPtr PdfToImageConverter::DynamicCast(ConverterPtr value) {
        return std::dynamic_pointer_cast<PdfToImageConverter>(value);
    }
    

    PdfToJsonConverter::PdfToJsonConverter() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("new_SolidFramework_Converters_PdfToJsonConverter");
        HANDLE handle = proc();
        Wrap(handle, true);
    }
    
    void PdfToJsonConverter::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Converters_PdfToJsonConverter");
        FrameworkObject::Dispose(destructor);
    }
    
    void PdfToJsonConverter::AddSourceFile(std::wstring fullpath) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Converters_PdfToJsonConverter_AddSourceFile__SWIG_0");
        proc(cPtr, fullpath.c_str());
    }
    
    void PdfToJsonConverter::AddSourceFile(SolidFramework::Plumbing::DocumentPtr document) {
        static InvokeType_VHH proc = GetFunction<InvokeType_VHH>("SolidFramework_Converters_PdfToJsonConverter_AddSourceFile__SWIG_1");
        proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(document));
    }
    
    bool PdfToJsonConverter::GetAverageCharacterScaling() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Converters_PdfToJsonConverter_AverageCharacterScaling_get");
        return 0 != proc(cPtr);
    }
    
    void PdfToJsonConverter::SetAverageCharacterScaling(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Converters_PdfToJsonConverter_AverageCharacterScaling_set");
        proc(cPtr, value);
    }
    
    void PdfToJsonConverter::Convert() {
        static InvokeType_VH proc = GetFunction<InvokeType_VH>("SolidFramework_Converters_PdfToJsonConverter_Convert__SWIG_0");
        proc(cPtr);
    }
    
    Plumbing::ConversionStatus PdfToJsonConverter::Convert(std::wstring sourceFileName, std::wstring destinationFileName) {
        static InvokeType_ISS proc = GetFunction<InvokeType_ISS>("SolidFramework_Converters_PdfToJsonConverter_Convert__SWIG_1");
        return (Plumbing::ConversionStatus)proc(sourceFileName.c_str(), destinationFileName.c_str());
    }
    
    Plumbing::ConversionStatus PdfToJsonConverter::Convert(std::wstring sourceFileName, std::wstring destinationFileName, bool overwrite) {
        static InvokeType_ISSU proc = GetFunction<InvokeType_ISSU>("SolidFramework_Converters_PdfToJsonConverter_Convert__SWIG_2");
        return (Plumbing::ConversionStatus)proc(sourceFileName.c_str(), destinationFileName.c_str(), overwrite);
    }
    
    Plumbing::ConversionStatus PdfToJsonConverter::ConvertTo(std::wstring fullpath, bool overwrite) {
        static InvokeType_IHSU proc = GetFunction<InvokeType_IHSU>("SolidFramework_Converters_PdfToJsonConverter_ConvertTo");
        return (Plumbing::ConversionStatus)proc(cPtr, fullpath.c_str(), overwrite);
    }
    
    bool PdfToJsonConverter::GetDetectLanguage() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Converters_PdfToJsonConverter_DetectLanguage_get");
        return 0 != proc(cPtr);
    }
    
    void PdfToJsonConverter::SetDetectLanguage(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Converters_PdfToJsonConverter_DetectLanguage_set");
        proc(cPtr, value);
    }
    
    bool PdfToJsonConverter::GetDetectLists() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Converters_PdfToJsonConverter_DetectLists_get");
        return 0 != proc(cPtr);
    }
    
    void PdfToJsonConverter::SetDetectLists(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Converters_PdfToJsonConverter_DetectLists_set");
        proc(cPtr, value);
    }
    
    bool PdfToJsonConverter::GetDetectStyles() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Converters_PdfToJsonConverter_DetectStyles_get");
        return 0 != proc(cPtr);
    }
    
    void PdfToJsonConverter::SetDetectStyles(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Converters_PdfToJsonConverter_DetectStyles_set");
        proc(cPtr, value);
    }
    
    bool PdfToJsonConverter::GetDetectTables() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Converters_PdfToJsonConverter_DetectTables_get");
        return 0 != proc(cPtr);
    }
    
    void PdfToJsonConverter::SetDetectTables(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Converters_PdfToJsonConverter_DetectTables_set");
        proc(cPtr, value);
    }
    
    bool PdfToJsonConverter::GetDetectTaggedTables() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Converters_PdfToJsonConverter_DetectTaggedTables_get");
        return 0 != proc(cPtr);
    }
    
    void PdfToJsonConverter::SetDetectTaggedTables(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Converters_PdfToJsonConverter_DetectTaggedTables_set");
        proc(cPtr, value);
    }
    
    bool PdfToJsonConverter::GetDetectTiledPages() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Converters_PdfToJsonConverter_DetectTiledPages_get");
        return 0 != proc(cPtr);
    }
    
    void PdfToJsonConverter::SetDetectTiledPages(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Converters_PdfToJsonConverter_DetectTiledPages_set");
        proc(cPtr, value);
    }
    
    bool PdfToJsonConverter::GetDetectToc() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Converters_PdfToJsonConverter_DetectToc_get");
        return 0 != proc(cPtr);
    }
    
    void PdfToJsonConverter::SetDetectToc(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Converters_PdfToJsonConverter_DetectToc_set");
        proc(cPtr, value);
    }
    
    Plumbing::FootnotesMode PdfToJsonConverter::GetFootnotesMode() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Converters_PdfToJsonConverter_FootnotesMode_get");
        return (Plumbing::FootnotesMode)proc(cPtr);
    }
    
    void PdfToJsonConverter::SetFootnotesMode(Plumbing::FootnotesMode value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Converters_PdfToJsonConverter_FootnotesMode_set");
        proc(cPtr, (int)(value));
    }
    
    Plumbing::HeaderAndFooterMode PdfToJsonConverter::GetHeaderAndFooterMode() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Converters_PdfToJsonConverter_HeaderAndFooterMode_get");
        return (Plumbing::HeaderAndFooterMode)proc(cPtr);
    }
    
    void PdfToJsonConverter::SetHeaderAndFooterMode(Plumbing::HeaderAndFooterMode value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Converters_PdfToJsonConverter_HeaderAndFooterMode_set");
        proc(cPtr, (int)(value));
    }
    
    Plumbing::ImageAnchoringMode PdfToJsonConverter::GetImageAnchoringMode() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Converters_PdfToJsonConverter_ImageAnchoringMode_get");
        return (Plumbing::ImageAnchoringMode)proc(cPtr);
    }
    
    void PdfToJsonConverter::SetImageAnchoringMode(Plumbing::ImageAnchoringMode value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Converters_PdfToJsonConverter_ImageAnchoringMode_set");
        proc(cPtr, (int)(value));
    }
    
    bool PdfToJsonConverter::GetKeepCharacterSpacing() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Converters_PdfToJsonConverter_KeepCharacterSpacing_get");
        return 0 != proc(cPtr);
    }
    
    void PdfToJsonConverter::SetKeepCharacterSpacing(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Converters_PdfToJsonConverter_KeepCharacterSpacing_set");
        proc(cPtr, value);
    }
    
    Plumbing::MarkupAnnotConversionType PdfToJsonConverter::GetMarkupAnnotConversionType() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Converters_PdfToJsonConverter_MarkupAnnotConversionType_get");
        return (Plumbing::MarkupAnnotConversionType)proc(cPtr);
    }
    
    void PdfToJsonConverter::SetMarkupAnnotConversionType(Plumbing::MarkupAnnotConversionType value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Converters_PdfToJsonConverter_MarkupAnnotConversionType_set");
        proc(cPtr, (int)(value));
    }
    
    bool PdfToJsonConverter::GetMergeParagraphIndents() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Converters_PdfToJsonConverter_MergeParagraphIndents_get");
        return 0 != proc(cPtr);
    }
    
    void PdfToJsonConverter::SetMergeParagraphIndents(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Converters_PdfToJsonConverter_MergeParagraphIndents_set");
        proc(cPtr, value);
    }
    
    Plumbing::ReconstructionMode PdfToJsonConverter::GetReconstructionMode() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Converters_PdfToJsonConverter_ReconstructionMode_get");
        return (Plumbing::ReconstructionMode)proc(cPtr);
    }
    
    void PdfToJsonConverter::SetReconstructionMode(Plumbing::ReconstructionMode value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Converters_PdfToJsonConverter_ReconstructionMode_set");
        proc(cPtr, (int)(value));
    }
    
    bool PdfToJsonConverter::GetSupportRightToLeftWritingDirection() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Converters_PdfToJsonConverter_SupportRightToLeftWritingDirection_get");
        return 0 != proc(cPtr);
    }
    
    void PdfToJsonConverter::SetSupportRightToLeftWritingDirection(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Converters_PdfToJsonConverter_SupportRightToLeftWritingDirection_set");
        proc(cPtr, value);
    }
    
    bool PdfToJsonConverter::GetWordInstalled() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Converters_PdfToJsonConverter_GetWordInstalled");
        return 0 != proc(cPtr);
    }
    
    PdfToJsonConverterPtr PdfToJsonConverter::DynamicCast(ConverterPtr value) {
        return std::dynamic_pointer_cast<PdfToJsonConverter>(value);
    }
    

    void PdfToOfficeDocumentConverter::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Converters_PdfToOfficeDocumentConverter");
        FrameworkObject::Dispose(destructor);
    }
    
    void PdfToOfficeDocumentConverter::AddSourceFile(std::wstring fullpath) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Converters_PdfToOfficeDocumentConverter_AddSourceFile__SWIG_0");
        proc(cPtr, fullpath.c_str());
    }
    
    void PdfToOfficeDocumentConverter::AddSourceFile(SolidFramework::Plumbing::DocumentPtr document) {
        static InvokeType_VHH proc = GetFunction<InvokeType_VHH>("SolidFramework_Converters_PdfToOfficeDocumentConverter_AddSourceFile__SWIG_1");
        proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(document));
    }
    
    bool PdfToOfficeDocumentConverter::GetAutoRotate() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Converters_PdfToOfficeDocumentConverter_AutoRotate_get");
        return 0 != proc(cPtr);
    }
    
    void PdfToOfficeDocumentConverter::SetAutoRotate(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Converters_PdfToOfficeDocumentConverter_AutoRotate_set");
        proc(cPtr, value);
    }
    
    void PdfToOfficeDocumentConverter::Convert() {
        static InvokeType_VH proc = GetFunction<InvokeType_VH>("SolidFramework_Converters_PdfToOfficeDocumentConverter_Convert");
        proc(cPtr);
    }
    
    Plumbing::ConversionStatus PdfToOfficeDocumentConverter::ConvertTo(std::wstring fullpath, bool overwrite) {
        static InvokeType_IHSU proc = GetFunction<InvokeType_IHSU>("SolidFramework_Converters_PdfToOfficeDocumentConverter_ConvertTo");
        return (Plumbing::ConversionStatus)proc(cPtr, fullpath.c_str(), overwrite);
    }
    
    Plumbing::SelectedAreasPtr PdfToOfficeDocumentConverter::GetSelectedAreas() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Converters_PdfToOfficeDocumentConverter_GetSelectedAreas");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<Plumbing::SelectedAreas>(newCPtr, true);
    }
    
    void PdfToOfficeDocumentConverter::SetSelectedAreas(Plumbing::SelectedAreasPtr value) {
        static InvokeType_VHH proc = GetFunction<InvokeType_VHH>("SolidFramework_Converters_PdfToOfficeDocumentConverter_SetSelectedAreas");
        proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(value));
    }
    
    Plumbing::TextRecoveryAutomaticGNse PdfToOfficeDocumentConverter::GetTextRecoveryAutomaticGNse() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Converters_PdfToOfficeDocumentConverter_TextRecoveryAutomaticGNse_get");
        return (Plumbing::TextRecoveryAutomaticGNse)proc(cPtr);
    }
    
    void PdfToOfficeDocumentConverter::SetTextRecoveryAutomaticGNse(Plumbing::TextRecoveryAutomaticGNse value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Converters_PdfToOfficeDocumentConverter_TextRecoveryAutomaticGNse_set");
        proc(cPtr, (int)(value));
    }
    
    Plumbing::TextRecoveryEngine PdfToOfficeDocumentConverter::GetTextRecoveryEngine() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Converters_PdfToOfficeDocumentConverter_TextRecoveryEngine_get");
        return (Plumbing::TextRecoveryEngine)proc(cPtr);
    }
    
    void PdfToOfficeDocumentConverter::SetTextRecoveryEngine(Plumbing::TextRecoveryEngine value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Converters_PdfToOfficeDocumentConverter_TextRecoveryEngine_set");
        proc(cPtr, (int)(value));
    }
    
    Plumbing::TextRecoveryEngineNse PdfToOfficeDocumentConverter::GetTextRecoveryEngineNse() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Converters_PdfToOfficeDocumentConverter_TextRecoveryEngineNse_get");
        return (Plumbing::TextRecoveryEngineNse)proc(cPtr);
    }
    
    void PdfToOfficeDocumentConverter::SetTextRecoveryEngineNse(Plumbing::TextRecoveryEngineNse value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Converters_PdfToOfficeDocumentConverter_TextRecoveryEngineNse_set");
        proc(cPtr, (int)(value));
    }
    
    std::wstring PdfToOfficeDocumentConverter::GetTextRecoveryLanguage() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Converters_PdfToOfficeDocumentConverter_GetTextRecoveryLanguage");
        return proc(cPtr);
    }
    
    void PdfToOfficeDocumentConverter::SetTextRecoveryLanguage(std::wstring value) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Converters_PdfToOfficeDocumentConverter_SetTextRecoveryLanguage");
        proc(cPtr, value.c_str());
    }
    
    Plumbing::TextRecoveryNSE PdfToOfficeDocumentConverter::GetTextRecoveryNseType() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Converters_PdfToOfficeDocumentConverter_TextRecoveryNseType_get");
        return (Plumbing::TextRecoveryNSE)proc(cPtr);
    }
    
    void PdfToOfficeDocumentConverter::SetTextRecoveryNseType(Plumbing::TextRecoveryNSE value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Converters_PdfToOfficeDocumentConverter_TextRecoveryNseType_set");
        proc(cPtr, (int)(value));
    }
    
    bool PdfToOfficeDocumentConverter::GetTextRecoverySuspects() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Converters_PdfToOfficeDocumentConverter_TextRecoverySuspects_get");
        return 0 != proc(cPtr);
    }
    
    void PdfToOfficeDocumentConverter::SetTextRecoverySuspects(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Converters_PdfToOfficeDocumentConverter_TextRecoverySuspects_set");
        proc(cPtr, value);
    }
    
    Plumbing::TextRecovery PdfToOfficeDocumentConverter::GetTextRecoveryType() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Converters_PdfToOfficeDocumentConverter_TextRecoveryType_get");
        return (Plumbing::TextRecovery)proc(cPtr);
    }
    
    void PdfToOfficeDocumentConverter::SetTextRecoveryType(Plumbing::TextRecovery value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Converters_PdfToOfficeDocumentConverter_TextRecoveryType_set");
        proc(cPtr, (int)(value));
    }
    
    std::wstring PdfToOfficeDocumentConverter::GetUserProperties() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Converters_PdfToOfficeDocumentConverter_GetUserProperties");
        return proc(cPtr);
    }
    
    void PdfToOfficeDocumentConverter::SetUserProperties(std::wstring value) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Converters_PdfToOfficeDocumentConverter_SetUserProperties");
        proc(cPtr, value.c_str());
    }
    
    PdfToOfficeDocumentConverterPtr PdfToOfficeDocumentConverter::DynamicCast(ConverterPtr value) {
        return std::dynamic_pointer_cast<PdfToOfficeDocumentConverter>(value);
    }
    
    PdfToOfficeDocumentConverter::PdfToOfficeDocumentConverter() {
        // only called from derived class constructors
    }
    

    PdfToPdfAConverter::PdfToPdfAConverter() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("new_SolidFramework_Converters_PdfToPdfAConverter");
        HANDLE handle = proc();
        static DirectorConnectType_Converter directorConnect = GetFunction<DirectorConnectType_Converter>("SolidFramework_Converters_PdfToPdfAConverter_director_connect");
        directorConnect(handle, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, &SolidFrameworkNative::SolidFramework_Converters_Converter_HandleProgress, &SolidFrameworkNative::SolidFramework_Converters_Converter_HandleWarning, nullptr);
        Wrap(handle, true);
        SolidFrameworkNative::SolidFramework_Converters_Converter_SetCustomData(handle, (HANDLE)this);
    }
    
    void PdfToPdfAConverter::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Converters_PdfToPdfAConverter");
        FrameworkObject::Dispose(destructor);
    }
    
    void PdfToPdfAConverter::AddSourceFile(std::wstring fullpath) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Converters_PdfToPdfAConverter_AddSourceFile__SWIG_0");
        proc(cPtr, fullpath.c_str());
    }
    
    void PdfToPdfAConverter::AddSourceFile(SolidFramework::Plumbing::DocumentPtr document) {
        static InvokeType_VHH proc = GetFunction<InvokeType_VHH>("SolidFramework_Converters_PdfToPdfAConverter_AddSourceFile__SWIG_1");
        proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(document));
    }
    
    void PdfToPdfAConverter::Convert() {
        static InvokeType_VH proc = GetFunction<InvokeType_VH>("SolidFramework_Converters_PdfToPdfAConverter_Convert");
        proc(cPtr);
    }
    
    Plumbing::ConversionStatus PdfToPdfAConverter::ConvertTo(std::wstring fullpath, bool overwrite) {
        static InvokeType_IHSU proc = GetFunction<InvokeType_IHSU>("SolidFramework_Converters_PdfToPdfAConverter_ConvertTo");
        return (Plumbing::ConversionStatus)proc(cPtr, fullpath.c_str(), overwrite);
    }
    
    std::wstring PdfToPdfAConverter::GetExportLogPath() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Converters_PdfToPdfAConverter_GetExportLogPath");
        return proc(cPtr);
    }
    
    void PdfToPdfAConverter::SetExportLogPath(std::wstring value) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Converters_PdfToPdfAConverter_SetExportLogPath");
        proc(cPtr, value.c_str());
    }
    
    std::wstring PdfToPdfAConverter::GetLogPath() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Converters_PdfToPdfAConverter_GetLogPath");
        return proc(cPtr);
    }
    
    void PdfToPdfAConverter::SetLogPath(std::wstring value) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Converters_PdfToPdfAConverter_SetLogPath");
        proc(cPtr, value.c_str());
    }
    
    bool PdfToPdfAConverter::GetOcrAlways() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Converters_PdfToPdfAConverter_OcrAlways_get");
        return 0 != proc(cPtr);
    }
    
    void PdfToPdfAConverter::SetOcrAlways(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Converters_PdfToPdfAConverter_OcrAlways_set");
        proc(cPtr, value);
    }
    
    bool PdfToPdfAConverter::GetOcrAutoRotate() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Converters_PdfToPdfAConverter_OcrAutoRotate_get");
        return 0 != proc(cPtr);
    }
    
    void PdfToPdfAConverter::SetOcrAutoRotate(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Converters_PdfToPdfAConverter_OcrAutoRotate_set");
        proc(cPtr, value);
    }
    
    Plumbing::TextRecoveryEngine PdfToPdfAConverter::GetOcrEngine() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Converters_PdfToPdfAConverter_OcrEngine_get");
        return (Plumbing::TextRecoveryEngine)proc(cPtr);
    }
    
    void PdfToPdfAConverter::SetOcrEngine(Plumbing::TextRecoveryEngine value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Converters_PdfToPdfAConverter_OcrEngine_set");
        proc(cPtr, (int)(value));
    }
    
    SolidFramework::Imaging::Plumbing::ImageCompression PdfToPdfAConverter::GetOcrImageCompression() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Converters_PdfToPdfAConverter_OcrImageCompression_get");
        return (SolidFramework::Imaging::Plumbing::ImageCompression)proc(cPtr);
    }
    
    void PdfToPdfAConverter::SetOcrImageCompression(SolidFramework::Imaging::Plumbing::ImageCompression value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Converters_PdfToPdfAConverter_OcrImageCompression_set");
        proc(cPtr, (int)(value));
    }
    
    std::wstring PdfToPdfAConverter::GetOcrLanguage() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Converters_PdfToPdfAConverter_GetOcrLanguage");
        return proc(cPtr);
    }
    
    void PdfToPdfAConverter::SetOcrLanguage(std::wstring value) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Converters_PdfToPdfAConverter_SetOcrLanguage");
        proc(cPtr, value.c_str());
    }
    
    Plumbing::OcrType PdfToPdfAConverter::GetOcrType() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Converters_PdfToPdfAConverter_OcrType_get");
        return (Plumbing::OcrType)proc(cPtr);
    }
    
    void PdfToPdfAConverter::SetOcrType(Plumbing::OcrType value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Converters_PdfToPdfAConverter_OcrType_set");
        proc(cPtr, (int)(value));
    }
    
    SolidFramework::Pdf::Reports::PdfAReportPtr PdfToPdfAConverter::GetReport() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Converters_PdfToPdfAConverter_GetReport");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Pdf::Reports::PdfAReport>(newCPtr, true);
    }
    
    std::wstring PdfToPdfAConverter::GetReportLanguage() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Converters_PdfToPdfAConverter_GetReportLanguage");
        return proc(cPtr);
    }
    
    void PdfToPdfAConverter::SetReportLanguage(std::wstring value) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Converters_PdfToPdfAConverter_SetReportLanguage");
        proc(cPtr, value.c_str());
    }
    
    bool PdfToPdfAConverter::GetShowWarnings() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Converters_PdfToPdfAConverter_ShowWarnings_get");
        return 0 != proc(cPtr);
    }
    
    void PdfToPdfAConverter::SetShowWarnings(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Converters_PdfToPdfAConverter_ShowWarnings_set");
        proc(cPtr, value);
    }
    
    Plumbing::TextWatermarkPtr PdfToPdfAConverter::GetTextWatermark() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Converters_PdfToPdfAConverter_GetTextWatermark");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<Plumbing::TextWatermark>(newCPtr, true);
    }
    
    void PdfToPdfAConverter::SetTextWatermark(Plumbing::TextWatermarkPtr value) {
        static InvokeType_VHH proc = GetFunction<InvokeType_VHH>("SolidFramework_Converters_PdfToPdfAConverter_SetTextWatermark");
        proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(value));
    }
    
    void PdfToPdfAConverter::Validate() {
        static InvokeType_VH proc = GetFunction<InvokeType_VH>("SolidFramework_Converters_PdfToPdfAConverter_Validate__SWIG_0");
        proc(cPtr);
    }
    
    void PdfToPdfAConverter::Validate(SolidFramework::Plumbing::ValidationMode mode) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Converters_PdfToPdfAConverter_Validate__SWIG_1");
        proc(cPtr, (int)(mode));
    }
    
    SolidFramework::Plumbing::ValidationMode PdfToPdfAConverter::GetValidationMode() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Converters_PdfToPdfAConverter_ValidationMode_get");
        return (SolidFramework::Plumbing::ValidationMode)proc(cPtr);
    }
    
    void PdfToPdfAConverter::SetValidationMode(SolidFramework::Plumbing::ValidationMode value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Converters_PdfToPdfAConverter_ValidationMode_set");
        proc(cPtr, (int)(value));
    }
    
    void PdfToPdfAConverter::Verify() {
        static InvokeType_VH proc = GetFunction<InvokeType_VH>("SolidFramework_Converters_PdfToPdfAConverter_Verify");
        proc(cPtr);
    }
    
    PdfToPdfAConverterPtr PdfToPdfAConverter::DynamicCast(ConverterPtr value) {
        return std::dynamic_pointer_cast<PdfToPdfAConverter>(value);
    }
    

    PdfToPdfConverter::PdfToPdfConverter() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("new_SolidFramework_Converters_PdfToPdfConverter");
        HANDLE handle = proc();
        static DirectorConnectType_PdfToPdfConverter directorConnect = GetFunction<DirectorConnectType_PdfToPdfConverter>("SolidFramework_Converters_PdfToPdfConverter_director_connect");
        directorConnect(handle, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, &SolidFrameworkNative::SolidFramework_Converters_Converter_HandleProgress, &SolidFrameworkNative::SolidFramework_Converters_Converter_HandleWarning, nullptr, nullptr);
        Wrap(handle, true);
        SolidFrameworkNative::SolidFramework_Converters_Converter_SetCustomData(handle, (HANDLE)this);
    }
    
    void PdfToPdfConverter::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Converters_PdfToPdfConverter");
        FrameworkObject::Dispose(destructor);
    }
    
    void PdfToPdfConverter::AddSourceFile(std::wstring fullpath) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Converters_PdfToPdfConverter_AddSourceFile__SWIG_0");
        proc(cPtr, fullpath.c_str());
    }
    
    void PdfToPdfConverter::AddSourceFile(SolidFramework::Plumbing::DocumentPtr document) {
        static InvokeType_VHH proc = GetFunction<InvokeType_VHH>("SolidFramework_Converters_PdfToPdfConverter_AddSourceFile__SWIG_1");
        proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(document));
    }
    
    void PdfToPdfConverter::Convert() {
        static InvokeType_VH proc = GetFunction<InvokeType_VH>("SolidFramework_Converters_PdfToPdfConverter_Convert__SWIG_0");
        proc(cPtr);
    }
    
    Plumbing::ConversionStatus PdfToPdfConverter::Convert(std::wstring sourceFileName, std::wstring destinationFileName) {
        static InvokeType_ISS proc = GetFunction<InvokeType_ISS>("SolidFramework_Converters_PdfToPdfConverter_Convert__SWIG_1");
        return (Plumbing::ConversionStatus)proc(sourceFileName.c_str(), destinationFileName.c_str());
    }
    
    Plumbing::ConversionStatus PdfToPdfConverter::Convert(std::wstring sourceFileName, std::wstring destinationFileName, bool overwrite) {
        static InvokeType_ISSU proc = GetFunction<InvokeType_ISSU>("SolidFramework_Converters_PdfToPdfConverter_Convert__SWIG_2");
        return (Plumbing::ConversionStatus)proc(sourceFileName.c_str(), destinationFileName.c_str(), overwrite);
    }
    
    Plumbing::ConversionStatus PdfToPdfConverter::Convert(std::wstring sourceFileName, std::wstring destinationFileName, bool overwrite, std::wstring ocrLanguage) {
        typedef int (SOLIDCALLBACK *InvokeType)(const wchar_t*, const wchar_t*, unsigned int, const wchar_t*);
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_Converters_PdfToPdfConverter_Convert__SWIG_3");
        return (Plumbing::ConversionStatus)proc(sourceFileName.c_str(), destinationFileName.c_str(), overwrite, ocrLanguage.c_str());
    }
    
    Plumbing::ConversionStatus PdfToPdfConverter::ConvertTo(std::wstring fullpath, bool overwrite) {
        static InvokeType_IHSU proc = GetFunction<InvokeType_IHSU>("SolidFramework_Converters_PdfToPdfConverter_ConvertTo");
        return (Plumbing::ConversionStatus)proc(cPtr, fullpath.c_str(), overwrite);
    }
    
    bool PdfToPdfConverter::GetCreateTags() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Converters_PdfToPdfConverter_CreateTags_get");
        return 0 != proc(cPtr);
    }
    
    void PdfToPdfConverter::SetCreateTags(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Converters_PdfToPdfConverter_CreateTags_set");
        proc(cPtr, value);
    }
    
    bool PdfToPdfConverter::GetOcrAlways() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Converters_PdfToPdfConverter_OcrAlways_get");
        return 0 != proc(cPtr);
    }
    
    void PdfToPdfConverter::SetOcrAlways(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Converters_PdfToPdfConverter_OcrAlways_set");
        proc(cPtr, value);
    }
    
    bool PdfToPdfConverter::GetOcrAutoRotate() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Converters_PdfToPdfConverter_OcrAutoRotate_get");
        return 0 != proc(cPtr);
    }
    
    void PdfToPdfConverter::SetOcrAutoRotate(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Converters_PdfToPdfConverter_OcrAutoRotate_set");
        proc(cPtr, value);
    }
    
    Plumbing::TextRecoveryEngine PdfToPdfConverter::GetOcrEngine() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Converters_PdfToPdfConverter_OcrEngine_get");
        return (Plumbing::TextRecoveryEngine)proc(cPtr);
    }
    
    void PdfToPdfConverter::SetOcrEngine(Plumbing::TextRecoveryEngine value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Converters_PdfToPdfConverter_OcrEngine_set");
        proc(cPtr, (int)(value));
    }
    
    SolidFramework::Imaging::Plumbing::ImageCompression PdfToPdfConverter::GetOcrImageCompression() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Converters_PdfToPdfConverter_OcrImageCompression_get");
        return (SolidFramework::Imaging::Plumbing::ImageCompression)proc(cPtr);
    }
    
    void PdfToPdfConverter::SetOcrImageCompression(SolidFramework::Imaging::Plumbing::ImageCompression value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Converters_PdfToPdfConverter_OcrImageCompression_set");
        proc(cPtr, (int)(value));
    }
    
    std::wstring PdfToPdfConverter::GetOcrLanguage() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Converters_PdfToPdfConverter_GetOcrLanguage");
        return proc(cPtr);
    }
    
    void PdfToPdfConverter::SetOcrLanguage(std::wstring value) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Converters_PdfToPdfConverter_SetOcrLanguage");
        proc(cPtr, value.c_str());
    }
    
    Plumbing::OcrType PdfToPdfConverter::GetOcrType() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Converters_PdfToPdfConverter_OcrType_get");
        return (Plumbing::OcrType)proc(cPtr);
    }
    
    void PdfToPdfConverter::SetOcrType(Plumbing::OcrType value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Converters_PdfToPdfConverter_OcrType_set");
        proc(cPtr, (int)(value));
    }
    
    std::wstring PdfToPdfConverter::GetPassword() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Converters_PdfToPdfConverter_GetPassword");
        return proc(cPtr);
    }
    
    void PdfToPdfConverter::SetPassword(std::wstring value) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Converters_PdfToPdfConverter_SetPassword");
        proc(cPtr, value.c_str());
    }
    
    Plumbing::TextWatermarkPtr PdfToPdfConverter::GetTextWatermark() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Converters_PdfToPdfConverter_GetTextWatermark");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<Plumbing::TextWatermark>(newCPtr, true);
    }
    
    void PdfToPdfConverter::SetTextWatermark(Plumbing::TextWatermarkPtr value) {
        static InvokeType_VHH proc = GetFunction<InvokeType_VHH>("SolidFramework_Converters_PdfToPdfConverter_SetTextWatermark");
        proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(value));
    }
    
    PdfToPdfConverterPtr PdfToPdfConverter::DynamicCast(ConverterPtr value) {
        return std::dynamic_pointer_cast<PdfToPdfConverter>(value);
    }
    

    PdfToPowerPointConverter::PdfToPowerPointConverter() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("new_SolidFramework_Converters_PdfToPowerPointConverter");
        HANDLE handle = proc();
        static DirectorConnectType_Converter directorConnect = GetFunction<DirectorConnectType_Converter>("SolidFramework_Converters_PdfToPowerPointConverter_director_connect");
        directorConnect(handle, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, &SolidFrameworkNative::SolidFramework_Converters_Converter_HandleProgress, &SolidFrameworkNative::SolidFramework_Converters_Converter_HandleWarning, nullptr);
        Wrap(handle, true);
        SolidFrameworkNative::SolidFramework_Converters_Converter_SetCustomData(handle, (HANDLE)this);
    }
    
    void PdfToPowerPointConverter::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Converters_PdfToPowerPointConverter");
        FrameworkObject::Dispose(destructor);
    }
    
    void PdfToPowerPointConverter::AddSourceFile(std::wstring fullpath) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Converters_PdfToPowerPointConverter_AddSourceFile__SWIG_0");
        proc(cPtr, fullpath.c_str());
    }
    
    void PdfToPowerPointConverter::AddSourceFile(SolidFramework::Plumbing::DocumentPtr document) {
        static InvokeType_VHH proc = GetFunction<InvokeType_VHH>("SolidFramework_Converters_PdfToPowerPointConverter_AddSourceFile__SWIG_1");
        proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(document));
    }
    
    void PdfToPowerPointConverter::Convert() {
        static InvokeType_VH proc = GetFunction<InvokeType_VH>("SolidFramework_Converters_PdfToPowerPointConverter_Convert__SWIG_0");
        proc(cPtr);
    }
    
    Plumbing::ConversionStatus PdfToPowerPointConverter::Convert(std::wstring sourceFileName, std::wstring destinationFileName) {
        static InvokeType_ISS proc = GetFunction<InvokeType_ISS>("SolidFramework_Converters_PdfToPowerPointConverter_Convert__SWIG_1");
        return (Plumbing::ConversionStatus)proc(sourceFileName.c_str(), destinationFileName.c_str());
    }
    
    Plumbing::ConversionStatus PdfToPowerPointConverter::Convert(std::wstring sourceFileName, std::wstring destinationFileName, bool overwrite) {
        static InvokeType_ISSU proc = GetFunction<InvokeType_ISSU>("SolidFramework_Converters_PdfToPowerPointConverter_Convert__SWIG_2");
        return (Plumbing::ConversionStatus)proc(sourceFileName.c_str(), destinationFileName.c_str(), overwrite);
    }
    
    Plumbing::ConversionStatus PdfToPowerPointConverter::ConvertTo(std::wstring fullpath, bool overwrite) {
        static InvokeType_IHSU proc = GetFunction<InvokeType_IHSU>("SolidFramework_Converters_PdfToPowerPointConverter_ConvertTo");
        return (Plumbing::ConversionStatus)proc(cPtr, fullpath.c_str(), overwrite);
    }
    
    bool PdfToPowerPointConverter::GetDetectLists() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Converters_PdfToPowerPointConverter_DetectLists_get");
        return 0 != proc(cPtr);
    }
    
    void PdfToPowerPointConverter::SetDetectLists(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Converters_PdfToPowerPointConverter_DetectLists_set");
        proc(cPtr, value);
    }
    
    Plumbing::EmbedFontsMode PdfToPowerPointConverter::GetEmbedFontsMode() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Converters_PdfToPowerPointConverter_EmbedFontsMode_get");
        return (Plumbing::EmbedFontsMode)proc(cPtr);
    }
    
    void PdfToPowerPointConverter::SetEmbedFontsMode(Plumbing::EmbedFontsMode value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Converters_PdfToPowerPointConverter_EmbedFontsMode_set");
        proc(cPtr, (int)(value));
    }
    
    Plumbing::MarkupAnnotConversionType PdfToPowerPointConverter::GetMarkupAnnotConversionType() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Converters_PdfToPowerPointConverter_MarkupAnnotConversionType_get");
        return (Plumbing::MarkupAnnotConversionType)proc(cPtr);
    }
    
    void PdfToPowerPointConverter::SetMarkupAnnotConversionType(Plumbing::MarkupAnnotConversionType value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Converters_PdfToPowerPointConverter_MarkupAnnotConversionType_set");
        proc(cPtr, (int)(value));
    }
    
    bool PdfToPowerPointConverter::GetTextAnnotationsAsSpeakerNotes() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Converters_PdfToPowerPointConverter_TextAnnotationsAsSpeakerNotes_get");
        return 0 != proc(cPtr);
    }
    
    void PdfToPowerPointConverter::SetTextAnnotationsAsSpeakerNotes(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Converters_PdfToPowerPointConverter_TextAnnotationsAsSpeakerNotes_set");
        proc(cPtr, value);
    }
    
    PdfToPowerPointConverterPtr PdfToPowerPointConverter::DynamicCast(ConverterPtr value) {
        return std::dynamic_pointer_cast<PdfToPowerPointConverter>(value);
    }
    

    PdfToTextConverter::PdfToTextConverter() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("new_SolidFramework_Converters_PdfToTextConverter");
        HANDLE handle = proc();
        static DirectorConnectType_Converter directorConnect = GetFunction<DirectorConnectType_Converter>("SolidFramework_Converters_PdfToTextConverter_director_connect");
        directorConnect(handle, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, &SolidFrameworkNative::SolidFramework_Converters_Converter_HandleProgress, &SolidFrameworkNative::SolidFramework_Converters_Converter_HandleWarning, nullptr);
        Wrap(handle, true);
        SolidFrameworkNative::SolidFramework_Converters_Converter_SetCustomData(handle, (HANDLE)this);
    }
    
    void PdfToTextConverter::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Converters_PdfToTextConverter");
        FrameworkObject::Dispose(destructor);
    }
    
    void PdfToTextConverter::AddSourceFile(std::wstring fullpath) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Converters_PdfToTextConverter_AddSourceFile__SWIG_0");
        proc(cPtr, fullpath.c_str());
    }
    
    void PdfToTextConverter::AddSourceFile(SolidFramework::Plumbing::DocumentPtr document) {
        static InvokeType_VHH proc = GetFunction<InvokeType_VHH>("SolidFramework_Converters_PdfToTextConverter_AddSourceFile__SWIG_1");
        proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(document));
    }
    
    void PdfToTextConverter::Convert() {
        static InvokeType_VH proc = GetFunction<InvokeType_VH>("SolidFramework_Converters_PdfToTextConverter_Convert__SWIG_0");
        proc(cPtr);
    }
    
    Plumbing::ConversionStatus PdfToTextConverter::Convert(std::wstring sourceFileName, std::wstring destinationFileName) {
        static InvokeType_ISS proc = GetFunction<InvokeType_ISS>("SolidFramework_Converters_PdfToTextConverter_Convert__SWIG_1");
        return (Plumbing::ConversionStatus)proc(sourceFileName.c_str(), destinationFileName.c_str());
    }
    
    Plumbing::ConversionStatus PdfToTextConverter::Convert(std::wstring sourceFileName, std::wstring destinationFileName, bool overwrite) {
        static InvokeType_ISSU proc = GetFunction<InvokeType_ISSU>("SolidFramework_Converters_PdfToTextConverter_Convert__SWIG_2");
        return (Plumbing::ConversionStatus)proc(sourceFileName.c_str(), destinationFileName.c_str(), overwrite);
    }
    
    Plumbing::ConversionStatus PdfToTextConverter::ConvertTo(std::wstring fullpath, bool overwrite) {
        static InvokeType_IHSU proc = GetFunction<InvokeType_IHSU>("SolidFramework_Converters_PdfToTextConverter_ConvertTo");
        return (Plumbing::ConversionStatus)proc(cPtr, fullpath.c_str(), overwrite);
    }
    
    bool PdfToTextConverter::GetDetectAndRemoveFootnotes() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Converters_PdfToTextConverter_DetectAndRemoveFootnotes_get");
        return 0 != proc(cPtr);
    }
    
    void PdfToTextConverter::SetDetectAndRemoveFootnotes(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Converters_PdfToTextConverter_DetectAndRemoveFootnotes_set");
        proc(cPtr, value);
    }
    
    bool PdfToTextConverter::GetDetectAndRemoveHeadersAndFooters() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Converters_PdfToTextConverter_DetectAndRemoveHeadersAndFooters_get");
        return 0 != proc(cPtr);
    }
    
    void PdfToTextConverter::SetDetectAndRemoveHeadersAndFooters(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Converters_PdfToTextConverter_DetectAndRemoveHeadersAndFooters_set");
        proc(cPtr, value);
    }
    
    int PdfToTextConverter::GetEncoding() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Converters_PdfToTextConverter_GetEncoding");
        return proc(cPtr);
    }
    
    void PdfToTextConverter::SetEncoding(int value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Converters_PdfToTextConverter_SetEncoding");
        proc(cPtr, value);
    }
    
    bool PdfToTextConverter::GetKeepLineBreaks() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Converters_PdfToTextConverter_KeepLineBreaks_get");
        return 0 != proc(cPtr);
    }
    
    void PdfToTextConverter::SetKeepLineBreaks(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Converters_PdfToTextConverter_KeepLineBreaks_set");
        proc(cPtr, value);
    }
    
    int PdfToTextConverter::GetLineLength() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Converters_PdfToTextConverter_LineLength_get");
        return proc(cPtr);
    }
    
    void PdfToTextConverter::SetLineLength(int value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Converters_PdfToTextConverter_LineLength_set");
        proc(cPtr, value);
    }
    
    Plumbing::LineTerminator PdfToTextConverter::GetLineTerminator() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Converters_PdfToTextConverter_LineTerminator_get");
        return (Plumbing::LineTerminator)proc(cPtr);
    }
    
    void PdfToTextConverter::SetLineTerminator(Plumbing::LineTerminator value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Converters_PdfToTextConverter_LineTerminator_set");
        proc(cPtr, (int)(value));
    }
    
    bool PdfToTextConverter::GetPreserveRareUnicode() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Converters_PdfToTextConverter_PreserveRareUnicode_get");
        return 0 != proc(cPtr);
    }
    
    void PdfToTextConverter::SetPreserveRareUnicode(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Converters_PdfToTextConverter_PreserveRareUnicode_set");
        proc(cPtr, value);
    }
    
    bool PdfToTextConverter::GetTextAnnotationsAsContent() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Converters_PdfToTextConverter_TextAnnotationsAsContent_get");
        return 0 != proc(cPtr);
    }
    
    void PdfToTextConverter::SetTextAnnotationsAsContent(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Converters_PdfToTextConverter_TextAnnotationsAsContent_set");
        proc(cPtr, value);
    }
    
    PdfToTextConverterPtr PdfToTextConverter::DynamicCast(ConverterPtr value) {
        return std::dynamic_pointer_cast<PdfToTextConverter>(value);
    }
    

    PdfToWordConverter::PdfToWordConverter() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("new_SolidFramework_Converters_PdfToWordConverter");
        HANDLE handle = proc();
        static DirectorConnectType_Converter directorConnect = GetFunction<DirectorConnectType_Converter>("SolidFramework_Converters_PdfToWordConverter_director_connect");
        directorConnect(handle, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, &SolidFrameworkNative::SolidFramework_Converters_Converter_HandleProgress, &SolidFrameworkNative::SolidFramework_Converters_Converter_HandleWarning, nullptr);
        Wrap(handle, true);
        SolidFrameworkNative::SolidFramework_Converters_Converter_SetCustomData(handle, (HANDLE)this);
    }
    
    void PdfToWordConverter::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Converters_PdfToWordConverter");
        FrameworkObject::Dispose(destructor);
    }
    
    void PdfToWordConverter::AddSourceFile(std::wstring fullpath) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Converters_PdfToWordConverter_AddSourceFile__SWIG_0");
        proc(cPtr, fullpath.c_str());
    }
    
    void PdfToWordConverter::AddSourceFile(SolidFramework::Plumbing::DocumentPtr document) {
        static InvokeType_VHH proc = GetFunction<InvokeType_VHH>("SolidFramework_Converters_PdfToWordConverter_AddSourceFile__SWIG_1");
        proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(document));
    }
    
    bool PdfToWordConverter::GetAverageCharacterScaling() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Converters_PdfToWordConverter_AverageCharacterScaling_get");
        return 0 != proc(cPtr);
    }
    
    void PdfToWordConverter::SetAverageCharacterScaling(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Converters_PdfToWordConverter_AverageCharacterScaling_set");
        proc(cPtr, value);
    }
    
    void PdfToWordConverter::Convert() {
        static InvokeType_VH proc = GetFunction<InvokeType_VH>("SolidFramework_Converters_PdfToWordConverter_Convert__SWIG_0");
        proc(cPtr);
    }
    
    Plumbing::ConversionStatus PdfToWordConverter::Convert(std::wstring sourceFileName, std::wstring destinationFileName) {
        static InvokeType_ISS proc = GetFunction<InvokeType_ISS>("SolidFramework_Converters_PdfToWordConverter_Convert__SWIG_1");
        return (Plumbing::ConversionStatus)proc(sourceFileName.c_str(), destinationFileName.c_str());
    }
    
    Plumbing::ConversionStatus PdfToWordConverter::Convert(std::wstring sourceFileName, std::wstring destinationFileName, bool overwrite) {
        static InvokeType_ISSU proc = GetFunction<InvokeType_ISSU>("SolidFramework_Converters_PdfToWordConverter_Convert__SWIG_2");
        return (Plumbing::ConversionStatus)proc(sourceFileName.c_str(), destinationFileName.c_str(), overwrite);
    }
    
    Plumbing::ConversionStatus PdfToWordConverter::ConvertTo(std::wstring fullpath, bool overwrite) {
        static InvokeType_IHSU proc = GetFunction<InvokeType_IHSU>("SolidFramework_Converters_PdfToWordConverter_ConvertTo");
        return (Plumbing::ConversionStatus)proc(cPtr, fullpath.c_str(), overwrite);
    }
    
    bool PdfToWordConverter::GetDetectLanguage() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Converters_PdfToWordConverter_DetectLanguage_get");
        return 0 != proc(cPtr);
    }
    
    void PdfToWordConverter::SetDetectLanguage(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Converters_PdfToWordConverter_DetectLanguage_set");
        proc(cPtr, value);
    }
    
    bool PdfToWordConverter::GetDetectLists() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Converters_PdfToWordConverter_DetectLists_get");
        return 0 != proc(cPtr);
    }
    
    void PdfToWordConverter::SetDetectLists(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Converters_PdfToWordConverter_DetectLists_set");
        proc(cPtr, value);
    }
    
    bool PdfToWordConverter::GetDetectStyles() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Converters_PdfToWordConverter_DetectStyles_get");
        return 0 != proc(cPtr);
    }
    
    void PdfToWordConverter::SetDetectStyles(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Converters_PdfToWordConverter_DetectStyles_set");
        proc(cPtr, value);
    }
    
    bool PdfToWordConverter::GetDetectTables() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Converters_PdfToWordConverter_DetectTables_get");
        return 0 != proc(cPtr);
    }
    
    void PdfToWordConverter::SetDetectTables(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Converters_PdfToWordConverter_DetectTables_set");
        proc(cPtr, value);
    }
    
    bool PdfToWordConverter::GetDetectTaggedTables() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Converters_PdfToWordConverter_DetectTaggedTables_get");
        return 0 != proc(cPtr);
    }
    
    void PdfToWordConverter::SetDetectTaggedTables(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Converters_PdfToWordConverter_DetectTaggedTables_set");
        proc(cPtr, value);
    }
    
    bool PdfToWordConverter::GetDetectTiledPages() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Converters_PdfToWordConverter_DetectTiledPages_get");
        return 0 != proc(cPtr);
    }
    
    void PdfToWordConverter::SetDetectTiledPages(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Converters_PdfToWordConverter_DetectTiledPages_set");
        proc(cPtr, value);
    }
    
    bool PdfToWordConverter::GetDetectToc() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Converters_PdfToWordConverter_DetectToc_get");
        return 0 != proc(cPtr);
    }
    
    void PdfToWordConverter::SetDetectToc(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Converters_PdfToWordConverter_DetectToc_set");
        proc(cPtr, value);
    }
    
    Plumbing::FootnotesMode PdfToWordConverter::GetFootnotesMode() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Converters_PdfToWordConverter_FootnotesMode_get");
        return (Plumbing::FootnotesMode)proc(cPtr);
    }
    
    void PdfToWordConverter::SetFootnotesMode(Plumbing::FootnotesMode value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Converters_PdfToWordConverter_FootnotesMode_set");
        proc(cPtr, (int)(value));
    }
    
    Plumbing::HeaderAndFooterMode PdfToWordConverter::GetHeaderAndFooterMode() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Converters_PdfToWordConverter_HeaderAndFooterMode_get");
        return (Plumbing::HeaderAndFooterMode)proc(cPtr);
    }
    
    void PdfToWordConverter::SetHeaderAndFooterMode(Plumbing::HeaderAndFooterMode value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Converters_PdfToWordConverter_HeaderAndFooterMode_set");
        proc(cPtr, (int)(value));
    }
    
    Plumbing::ImageAnchoringMode PdfToWordConverter::GetImageAnchoringMode() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Converters_PdfToWordConverter_ImageAnchoringMode_get");
        return (Plumbing::ImageAnchoringMode)proc(cPtr);
    }
    
    void PdfToWordConverter::SetImageAnchoringMode(Plumbing::ImageAnchoringMode value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Converters_PdfToWordConverter_ImageAnchoringMode_set");
        proc(cPtr, (int)(value));
    }
    
    bool PdfToWordConverter::GetKeepCharacterSpacing() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Converters_PdfToWordConverter_KeepCharacterSpacing_get");
        return 0 != proc(cPtr);
    }
    
    void PdfToWordConverter::SetKeepCharacterSpacing(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Converters_PdfToWordConverter_KeepCharacterSpacing_set");
        proc(cPtr, value);
    }
    
    Plumbing::MarkupAnnotConversionType PdfToWordConverter::GetMarkupAnnotConversionType() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Converters_PdfToWordConverter_MarkupAnnotConversionType_get");
        return (Plumbing::MarkupAnnotConversionType)proc(cPtr);
    }
    
    void PdfToWordConverter::SetMarkupAnnotConversionType(Plumbing::MarkupAnnotConversionType value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Converters_PdfToWordConverter_MarkupAnnotConversionType_set");
        proc(cPtr, (int)(value));
    }
    
    bool PdfToWordConverter::GetMergeParagraphIndents() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Converters_PdfToWordConverter_MergeParagraphIndents_get");
        return 0 != proc(cPtr);
    }
    
    void PdfToWordConverter::SetMergeParagraphIndents(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Converters_PdfToWordConverter_MergeParagraphIndents_set");
        proc(cPtr, value);
    }
    
    Plumbing::WordDocumentType PdfToWordConverter::GetOutputType() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Converters_PdfToWordConverter_OutputType_get");
        return (Plumbing::WordDocumentType)proc(cPtr);
    }
    
    void PdfToWordConverter::SetOutputType(Plumbing::WordDocumentType value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Converters_PdfToWordConverter_OutputType_set");
        proc(cPtr, (int)(value));
    }
    
    Plumbing::ReconstructionMode PdfToWordConverter::GetReconstructionMode() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Converters_PdfToWordConverter_ReconstructionMode_get");
        return (Plumbing::ReconstructionMode)proc(cPtr);
    }
    
    void PdfToWordConverter::SetReconstructionMode(Plumbing::ReconstructionMode value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Converters_PdfToWordConverter_ReconstructionMode_set");
        proc(cPtr, (int)(value));
    }
    
    bool PdfToWordConverter::GetSupportRightToLeftWritingDirection() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Converters_PdfToWordConverter_SupportRightToLeftWritingDirection_get");
        return 0 != proc(cPtr);
    }
    
    void PdfToWordConverter::SetSupportRightToLeftWritingDirection(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Converters_PdfToWordConverter_SupportRightToLeftWritingDirection_set");
        proc(cPtr, value);
    }
    
    Plumbing::TargetWordFormat PdfToWordConverter::GetTargetWordFormat() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Converters_PdfToWordConverter_TargetWordFormat_get");
        return (Plumbing::TargetWordFormat)proc(cPtr);
    }
    
    void PdfToWordConverter::SetTargetWordFormat(Plumbing::TargetWordFormat value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Converters_PdfToWordConverter_TargetWordFormat_set");
        proc(cPtr, (int)(value));
    }
    
    bool PdfToWordConverter::GetWordInstalled() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Converters_PdfToWordConverter_WordInstalled_get");
        return 0 != proc(cPtr);
    }
    
    PdfToWordConverterPtr PdfToWordConverter::DynamicCast(ConverterPtr value) {
        return std::dynamic_pointer_cast<PdfToWordConverter>(value);
    }
    

    void SolidConverterPdf::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Converters_SolidConverterPdf");
        FrameworkObject::Dispose(destructor);
    }
    
    void SolidConverterPdf::AddSourceFile(std::wstring fullpath) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Converters_SolidConverterPdf_AddSourceFile__SWIG_0");
        proc(cPtr, fullpath.c_str());
    }
    
    void SolidConverterPdf::AddSourceFile(SolidFramework::Plumbing::DocumentPtr document) {
        static InvokeType_VHH proc = GetFunction<InvokeType_VHH>("SolidFramework_Converters_SolidConverterPdf_AddSourceFile__SWIG_1");
        proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(document));
    }
    
    void SolidConverterPdf::Convert() {
        static InvokeType_VH proc = GetFunction<InvokeType_VH>("SolidFramework_Converters_SolidConverterPdf_Convert");
        proc(cPtr);
    }
    
    Plumbing::ConversionStatus SolidConverterPdf::ConvertTo(std::wstring fullpath, bool overwrite) {
        static InvokeType_IHSU proc = GetFunction<InvokeType_IHSU>("SolidFramework_Converters_SolidConverterPdf_ConvertTo");
        return (Plumbing::ConversionStatus)proc(cPtr, fullpath.c_str(), overwrite);
    }
    
    bool SolidConverterPdf::GetDetectSoftHyphens() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Converters_SolidConverterPdf_DetectSoftHyphens_get");
        return 0 != proc(cPtr);
    }
    
    void SolidConverterPdf::SetDetectSoftHyphens(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Converters_SolidConverterPdf_DetectSoftHyphens_set");
        proc(cPtr, value);
    }
    
    bool SolidConverterPdf::GetGraphicsAsImages() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Converters_SolidConverterPdf_GraphicsAsImages_get");
        return 0 != proc(cPtr);
    }
    
    void SolidConverterPdf::SetGraphicsAsImages(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Converters_SolidConverterPdf_GraphicsAsImages_set");
        proc(cPtr, value);
    }
    
    Plumbing::ImageWatermarkPtr SolidConverterPdf::GetImageWatermark() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Converters_SolidConverterPdf_GetImageWatermark");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<Plumbing::ImageWatermark>(newCPtr, true);
    }
    
    void SolidConverterPdf::SetImageWatermark(Plumbing::ImageWatermarkPtr value) {
        static InvokeType_VHH proc = GetFunction<InvokeType_VHH>("SolidFramework_Converters_SolidConverterPdf_SetImageWatermark");
        proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(value));
    }
    
    bool SolidConverterPdf::GetKeepBackgroundColorText() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Converters_SolidConverterPdf_KeepBackgroundColorText_get");
        return 0 != proc(cPtr);
    }
    
    void SolidConverterPdf::SetKeepBackgroundColorText(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Converters_SolidConverterPdf_KeepBackgroundColorText_set");
        proc(cPtr, value);
    }
    
    bool SolidConverterPdf::GetKeepInvisibleText() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Converters_SolidConverterPdf_KeepInvisibleText_get");
        return 0 != proc(cPtr);
    }
    
    void SolidConverterPdf::SetKeepInvisibleText(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Converters_SolidConverterPdf_KeepInvisibleText_set");
        proc(cPtr, value);
    }
    
    bool SolidConverterPdf::GetNoRepairing() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Converters_SolidConverterPdf_NoRepairing_get");
        return 0 != proc(cPtr);
    }
    
    void SolidConverterPdf::SetNoRepairing(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Converters_SolidConverterPdf_NoRepairing_set");
        proc(cPtr, value);
    }
    
    SolidFramework::PageRangePtr SolidConverterPdf::GetPageRange() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Converters_SolidConverterPdf_GetPageRange");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::PageRange>(newCPtr, true);
    }
    
    void SolidConverterPdf::SetPageRange(SolidFramework::PageRangePtr value) {
        static InvokeType_VHH proc = GetFunction<InvokeType_VHH>("SolidFramework_Converters_SolidConverterPdf_SetPageRange");
        proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(value));
    }
    
    std::wstring SolidConverterPdf::GetPassword() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Converters_SolidConverterPdf_GetPassword");
        return proc(cPtr);
    }
    
    void SolidConverterPdf::SetPassword(std::wstring value) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Converters_SolidConverterPdf_SetPassword");
        proc(cPtr, value.c_str());
    }
    
    SolidConverterPdfPtr SolidConverterPdf::DynamicCast(ConverterPtr value) {
        return std::dynamic_pointer_cast<SolidConverterPdf>(value);
    }
    
    SolidConverterPdf::SolidConverterPdf() {
        // only called from derived class constructors
    }
    

}} // SolidFramework::Converters

namespace SolidFramework { namespace Converters { namespace Plumbing { 
    ConversionResult::ConversionResult(std::wstring source, ConversionStatus status) {
        static InvokeType_HSI proc = GetFunction<InvokeType_HSI>("new_SolidFramework_Converters_Plumbing_ConversionResult");
        HANDLE handle = proc(source.c_str(), (int)(status));
        Wrap(handle, true);
    }
    
    void ConversionResult::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Converters_Plumbing_ConversionResult");
        FrameworkObject::Dispose(destructor);
    }
    
    void ConversionResult::AddResult(std::wstring path, bool temporary) {
        static InvokeType_VHSU proc = GetFunction<InvokeType_VHSU>("SolidFramework_Converters_Plumbing_ConversionResult_AddResult");
        proc(cPtr, path.c_str(), temporary);
    }
    
    void ConversionResult::AddResultFolder(std::wstring path, bool temporary) {
        static InvokeType_VHSU proc = GetFunction<InvokeType_VHSU>("SolidFramework_Converters_Plumbing_ConversionResult_AddResultFolder");
        proc(cPtr, path.c_str(), temporary);
    }
    
    void ConversionResult::AddTemporaryFile(std::wstring path) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Converters_Plumbing_ConversionResult_AddTemporaryFile");
        proc(cPtr, path.c_str());
    }
    
    void ConversionResult::AddTemporaryFolder(std::wstring path) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Converters_Plumbing_ConversionResult_AddTemporaryFolder");
        proc(cPtr, path.c_str());
    }
    
    SolidFramework::Pdf::Transformers::OCRTransformationResultPtr ConversionResult::GetOcrStepResults() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Converters_Plumbing_ConversionResult_GetOcrStepResults");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Pdf::Transformers::OCRTransformationResult>(newCPtr, true);
    }
    
    void ConversionResult::SetOcrStepResults(SolidFramework::Pdf::Transformers::OCRTransformationResultPtr OCRStepResults) {
        static InvokeType_VHH proc = GetFunction<InvokeType_VHH>("SolidFramework_Converters_Plumbing_ConversionResult_SetOcrStepResults");
        proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(OCRStepResults));
    }
    
    std::vector<std::wstring> ConversionResult::GetPaths() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Converters_Plumbing_ConversionResult_GetPaths");
        return SolidFrameworkNative::StringsArrayToVector(proc(cPtr), false);
    }
    
    void ConversionResult::SetPaths(const std::vector<std::wstring> & value) {
        static InvokeType_VHH proc = GetFunction<InvokeType_VHH>("SolidFramework_Converters_Plumbing_ConversionResult_SetPaths");
        proc(cPtr, SolidFrameworkNative::VectorToStringsArray(value)->GetCPtr());
    }
    
    bool ConversionResult::RemoveTemporary(std::wstring path) {
        static InvokeType_UHS proc = GetFunction<InvokeType_UHS>("SolidFramework_Converters_Plumbing_ConversionResult_RemoveTemporary");
        return 0 != proc(cPtr, path.c_str());
    }
    
    std::wstring ConversionResult::GetSource() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Converters_Plumbing_ConversionResult_GetSource");
        return proc(cPtr);
    }
    
    void ConversionResult::SetSource(std::wstring value) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Converters_Plumbing_ConversionResult_SetSource");
        proc(cPtr, value.c_str());
    }
    
    ConversionStatus ConversionResult::GetStatus() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Converters_Plumbing_ConversionResult_GetStatus");
        return (ConversionStatus)proc(cPtr);
    }
    
    void ConversionResult::SetStatus(ConversionStatus value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Converters_Plumbing_ConversionResult_SetStatus");
        proc(cPtr, (int)(value));
    }
    
    ConversionResultPtr ConversionResult::DynamicCast(IConversionResultPtr value) {
        return std::dynamic_pointer_cast<ConversionResult>(value);
    }
    
    ConversionResult::ConversionResult() {
        // only called from derived class constructors
    }
    

    IConversionResult::IConversionResult() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("new_SolidFramework_Converters_Plumbing_IConversionResult");
        HANDLE handle = proc();
        Wrap(handle, true);
    }
    
    void IConversionResult::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Converters_Plumbing_IConversionResult");
        FrameworkObject::Dispose(destructor);
    }
    
    void IConversionResult::AddResult(std::wstring path, bool temporary) {
        static InvokeType_VHSU proc = GetFunction<InvokeType_VHSU>("SolidFramework_Converters_Plumbing_IConversionResult_AddResult");
        proc(cPtr, path.c_str(), temporary);
    }
    
    void IConversionResult::AddResultFolder(std::wstring path, bool temporary) {
        static InvokeType_VHSU proc = GetFunction<InvokeType_VHSU>("SolidFramework_Converters_Plumbing_IConversionResult_AddResultFolder");
        proc(cPtr, path.c_str(), temporary);
    }
    
    void IConversionResult::AddTemporaryFile(std::wstring path) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Converters_Plumbing_IConversionResult_AddTemporaryFile");
        proc(cPtr, path.c_str());
    }
    
    void IConversionResult::AddTemporaryFolder(std::wstring path) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Converters_Plumbing_IConversionResult_AddTemporaryFolder");
        proc(cPtr, path.c_str());
    }
    
    std::vector<std::wstring> IConversionResult::GetPaths() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Converters_Plumbing_IConversionResult_GetPaths");
        return SolidFrameworkNative::StringsArrayToVector(proc(cPtr), false);
    }
    
    void IConversionResult::SetPaths(const std::vector<std::wstring> & value) {
        static InvokeType_VHH proc = GetFunction<InvokeType_VHH>("SolidFramework_Converters_Plumbing_IConversionResult_SetPaths");
        proc(cPtr, SolidFrameworkNative::VectorToStringsArray(value)->GetCPtr());
    }
    
    bool IConversionResult::RemoveTemporary(std::wstring path) {
        static InvokeType_UHS proc = GetFunction<InvokeType_UHS>("SolidFramework_Converters_Plumbing_IConversionResult_RemoveTemporary");
        return 0 != proc(cPtr, path.c_str());
    }
    
    std::wstring IConversionResult::GetSource() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Converters_Plumbing_IConversionResult_GetSource");
        return proc(cPtr);
    }
    
    void IConversionResult::SetSource(std::wstring value) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Converters_Plumbing_IConversionResult_SetSource");
        proc(cPtr, value.c_str());
    }
    
    ConversionStatus IConversionResult::GetStatus() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Converters_Plumbing_IConversionResult_GetStatus");
        return (ConversionStatus)proc(cPtr);
    }
    
    void IConversionResult::SetStatus(ConversionStatus value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Converters_Plumbing_IConversionResult_SetStatus");
        proc(cPtr, (int)(value));
    }
    

    ImageWatermark::ImageWatermark() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("new_SolidFramework_Converters_Plumbing_ImageWatermark");
        HANDLE handle = proc();
        Wrap(handle, true);
    }
    
    void ImageWatermark::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Converters_Plumbing_ImageWatermark");
        FrameworkObject::Dispose(destructor);
    }
    
    std::wstring ImageWatermark::GetPath() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Converters_Plumbing_ImageWatermark_GetPath");
        return proc(cPtr);
    }
    
    void ImageWatermark::SetPath(std::wstring value) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Converters_Plumbing_ImageWatermark_SetPath");
        proc(cPtr, value.c_str());
    }
    

    void PdfAConversionResult::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Converters_Plumbing_PdfAConversionResult");
        FrameworkObject::Dispose(destructor);
    }
    
    PdfAConversionStatus PdfAConversionResult::GetPdfAStatus() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Converters_Plumbing_PdfAConversionResult_PdfAStatus_get");
        return (PdfAConversionStatus)proc(cPtr);
    }
    
    SolidFramework::Plumbing::ValidationMode PdfAConversionResult::GetPdfType() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Converters_Plumbing_PdfAConversionResult_PdfType_get");
        return (SolidFramework::Plumbing::ValidationMode)proc(cPtr);
    }
    
    PdfAConversionResultPtr PdfAConversionResult::DynamicCast(IConversionResultPtr value) {
        return std::dynamic_pointer_cast<PdfAConversionResult>(value);
    }
    

    SelectedArea::SelectedArea(int page, int x, int y, int width, int height) {
        typedef HANDLE (SOLIDCALLBACK *InvokeType)(int, int, int, int, int);
        static InvokeType proc = GetFunction<InvokeType>("new_SolidFramework_Converters_Plumbing_SelectedArea__SWIG_0");
        HANDLE handle = proc(page, x, y, width, height);
        Wrap(handle, true);
    }
    
    SelectedArea::SelectedArea(int page, SolidFramework::Interop::PointPtr location, int width, int height) {
        typedef HANDLE (SOLIDCALLBACK *InvokeType)(int, HANDLE, int, int);
        static InvokeType proc = GetFunction<InvokeType>("new_SolidFramework_Converters_Plumbing_SelectedArea__SWIG_1");
        HANDLE handle = proc(page, SolidFramework::FrameworkObject::GetCPtr(location), width, height);
        Wrap(handle, true);
    }
    
    SelectedArea::SelectedArea(int page, SolidFramework::Interop::RectanglePtr rect) {
        static InvokeType_HIH proc = GetFunction<InvokeType_HIH>("new_SolidFramework_Converters_Plumbing_SelectedArea__SWIG_2");
        HANDLE handle = proc(page, SolidFramework::FrameworkObject::GetCPtr(rect));
        Wrap(handle, true);
    }
    
    void SelectedArea::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Converters_Plumbing_SelectedArea");
        FrameworkObject::Dispose(destructor);
    }
    
    SolidFramework::Interop::RectanglePtr SelectedArea::GetArea() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Converters_Plumbing_SelectedArea_GetArea");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::Rectangle>(newCPtr, true);
    }
    
    void SelectedArea::SetArea(SolidFramework::Interop::RectanglePtr value) {
        static InvokeType_VHH proc = GetFunction<InvokeType_VHH>("SolidFramework_Converters_Plumbing_SelectedArea_SetArea");
        proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(value));
    }
    
    int SelectedArea::GetHeight() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Converters_Plumbing_SelectedArea_Height_get");
        return proc(cPtr);
    }
    
    SolidFramework::Interop::PointPtr SelectedArea::GetLocation() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Converters_Plumbing_SelectedArea_GetLocation");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::Point>(newCPtr, true);
    }
    
    int SelectedArea::GetPage() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Converters_Plumbing_SelectedArea_Page_get");
        return proc(cPtr);
    }
    
    void SelectedArea::SetPage(int value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Converters_Plumbing_SelectedArea_Page_set");
        proc(cPtr, value);
    }
    
    SelectedAreaPtr SelectedArea::Parse(std::wstring content) {
        static InvokeType_PS proc = GetFunction<InvokeType_PS>("SolidFramework_Converters_Plumbing_SelectedArea_Parse");
        HANDLE newCPtr = proc(content.c_str());
        return newCPtr == nullptr? nullptr : std::make_shared<SelectedArea>(newCPtr, true);
    }
    
    std::wstring SelectedArea::ToString() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Converters_Plumbing_SelectedArea_ToString");
        return UnwrapWString(proc(cPtr));
    }
    
    int SelectedArea::GetWidth() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Converters_Plumbing_SelectedArea_Width_get");
        return proc(cPtr);
    }
    

    SelectedAreas::SelectedAreas() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("new_SolidFramework_Converters_Plumbing_SelectedAreas");
        HANDLE handle = proc();
        Wrap(handle, true);
    }
    
    void SelectedAreas::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Converters_Plumbing_SelectedAreas");
        FrameworkObject::Dispose(destructor);
    }
    
    void SelectedAreas::Add(SelectedAreaPtr value) {
        static InvokeType_VHH proc = GetFunction<InvokeType_VHH>("SolidFramework_Converters_Plumbing_SelectedAreas_Add");
        proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(value));
    }
    
    int SelectedAreas::GetCount() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Converters_Plumbing_SelectedAreas_Count_get");
        return proc(cPtr);
    }
    
    SelectedAreasPtr SelectedAreas::Parse(std::wstring content) {
        static InvokeType_PS proc = GetFunction<InvokeType_PS>("SolidFramework_Converters_Plumbing_SelectedAreas_Parse");
        HANDLE newCPtr = proc(content.c_str());
        return newCPtr == nullptr? nullptr : std::make_shared<SelectedAreas>(newCPtr, true);
    }
    
    std::vector<SelectedAreaPtr> SelectedAreas::GetSelectedAreas() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Converters_Plumbing_SelectedAreas_GetSelectedAreas");
        return SolidFrameworkNative::SelectedAreasCollectionToVector(proc(cPtr), false);
    }
    
    std::wstring SelectedAreas::ToString() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Converters_Plumbing_SelectedAreas_ToString");
        return UnwrapWString(proc(cPtr));
    }
    

    TextWatermark::TextWatermark() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("new_SolidFramework_Converters_Plumbing_TextWatermark");
        HANDLE handle = proc();
        Wrap(handle, true);
    }
    
    void TextWatermark::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Converters_Plumbing_TextWatermark");
        FrameworkObject::Dispose(destructor);
    }
    
    int TextWatermark::GetAngle() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Converters_Plumbing_TextWatermark_Angle_get");
        return proc(cPtr);
    }
    
    void TextWatermark::SetAngle(int value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Converters_Plumbing_TextWatermark_Angle_set");
        proc(cPtr, value);
    }
    
    bool TextWatermark::GetBackground() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Converters_Plumbing_TextWatermark_Background_get");
        return 0 != proc(cPtr);
    }
    
    void TextWatermark::SetBackground(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Converters_Plumbing_TextWatermark_Background_set");
        proc(cPtr, value);
    }
    
    SolidFramework::Interop::ColorPtr TextWatermark::GetColor() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Converters_Plumbing_TextWatermark_GetColor");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::Color>(newCPtr, true);
    }
    
    void TextWatermark::SetColor(SolidFramework::Interop::ColorPtr value) {
        static InvokeType_VHH proc = GetFunction<InvokeType_VHH>("SolidFramework_Converters_Plumbing_TextWatermark_SetColor");
        proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(value));
    }
    
    bool TextWatermark::GetFill() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Converters_Plumbing_TextWatermark_Fill_get");
        return 0 != proc(cPtr);
    }
    
    void TextWatermark::SetFill(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Converters_Plumbing_TextWatermark_Fill_set");
        proc(cPtr, value);
    }
    
    float TextWatermark::GetLineWeight() {
        static InvokeType_FH proc = GetFunction<InvokeType_FH>("SolidFramework_Converters_Plumbing_TextWatermark_LineWeight_get");
        return proc(cPtr);
    }
    
    void TextWatermark::SetLineWeight(float value) {
        static InvokeType_VHF proc = GetFunction<InvokeType_VHF>("SolidFramework_Converters_Plumbing_TextWatermark_LineWeight_set");
        proc(cPtr, value);
    }
    
    TextWatermarkPtr TextWatermark::Parse(std::wstring content) {
        static InvokeType_PS proc = GetFunction<InvokeType_PS>("SolidFramework_Converters_Plumbing_TextWatermark_Parse");
        HANDLE newCPtr = proc(content.c_str());
        return newCPtr == nullptr? nullptr : std::make_shared<TextWatermark>(newCPtr, true);
    }
    
    bool TextWatermark::GetSerifFont() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Converters_Plumbing_TextWatermark_SerifFont_get");
        return 0 != proc(cPtr);
    }
    
    void TextWatermark::SetSerifFont(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Converters_Plumbing_TextWatermark_SerifFont_set");
        proc(cPtr, value);
    }
    
    bool TextWatermark::GetStroke() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Converters_Plumbing_TextWatermark_Stroke_get");
        return 0 != proc(cPtr);
    }
    
    void TextWatermark::SetStroke(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Converters_Plumbing_TextWatermark_Stroke_set");
        proc(cPtr, value);
    }
    
    std::wstring TextWatermark::GetText() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Converters_Plumbing_TextWatermark_GetText");
        return proc(cPtr);
    }
    
    void TextWatermark::SetText(std::wstring value) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Converters_Plumbing_TextWatermark_SetText");
        proc(cPtr, value.c_str());
    }
    
    std::wstring TextWatermark::ToString() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Converters_Plumbing_TextWatermark_ToString");
        return UnwrapWString(proc(cPtr));
    }
    
    std::wstring TextWatermark::GetUrl() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Converters_Plumbing_TextWatermark_GetUrl");
        return proc(cPtr);
    }
    
    void TextWatermark::SetUrl(std::wstring value) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Converters_Plumbing_TextWatermark_SetUrl");
        proc(cPtr, value.c_str());
    }
    

}}} // SolidFramework::Converters::Plumbing

namespace SolidFramework { namespace Imaging { 
    void Image::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Imaging_Image");
        FrameworkObject::Dispose(destructor);
    }
    
    void Image::GetAngles() {
        static InvokeType_VH proc = GetFunction<InvokeType_VH>("SolidFramework_Imaging_Image_GetAngles");
        proc(cPtr);
    }
    
    int Image::GetBitsPerComponent() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Imaging_Image_BitsPerComponent_get");
        return proc(cPtr);
    }
    
    std::vector<ImagePtr> Image::GetComponentImages() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Imaging_Image_GetComponentImages__SWIG_0");
        return SolidFrameworkNative::ImagesCollectionToVector(proc(cPtr), false);
    }
    
    std::vector<ImagePtr> Image::GetComponentImages(Plumbing::ImageCompression compression) {
        static InvokeType_HHI proc = GetFunction<InvokeType_HHI>("SolidFramework_Imaging_Image_GetComponentImages__SWIG_1");
        return SolidFrameworkNative::ImagesCollectionToVector(proc(cPtr, (int)(compression)), false);
    }
    
    std::vector<ImagePtr> Image::GetComponentImages(Plumbing::ImageCompression compression, Plumbing::ImageComponents components) {
        static InvokeType_HHII proc = GetFunction<InvokeType_HHII>("SolidFramework_Imaging_Image_GetComponentImages__SWIG_2");
        return SolidFrameworkNative::ImagesCollectionToVector(proc(cPtr, (int)(compression), (int)(components)), false);
    }
    
    std::vector<ImagePtr> Image::GetComponentImages(Plumbing::ImageCompression compression, Plumbing::ImageComponents components, Plumbing::ImageComponentOrder imageOrder) {
        static InvokeType_HHIII proc = GetFunction<InvokeType_HHIII>("SolidFramework_Imaging_Image_GetComponentImages__SWIG_3");
        return SolidFrameworkNative::ImagesCollectionToVector(proc(cPtr, (int)(compression), (int)(components), (int)(imageOrder)), false);
    }
    
    ImagePtr Image::Create(HANDLE native_image, SolidFramework::Pdf::Plumbing::PageStatistics statistics) {
        static InvokeType_HHI proc = GetFunction<InvokeType_HHI>("SolidFramework_Imaging_Image_Create__SWIG_0");
        HANDLE newCPtr = proc(native_image, (int)(statistics));
        return newCPtr == nullptr? nullptr : std::make_shared<Image>(newCPtr, true);
    }
    
    ImagePtr Image::Create(std::wstring path) {
        static InvokeType_PS proc = GetFunction<InvokeType_PS>("SolidFramework_Imaging_Image_Create__SWIG_1");
        HANDLE newCPtr = proc(path.c_str());
        return newCPtr == nullptr? nullptr : std::make_shared<Image>(newCPtr, true);
    }
    
    BYTE Image::GetDCTCompressionQuality() {
        static InvokeType_BH proc = GetFunction<InvokeType_BH>("SolidFramework_Imaging_Image_DCTCompressionQuality_get");
        return proc(cPtr);
    }
    
    int Image::GetDpiProcessed() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Imaging_Image_DpiProcessed_get");
        return proc(cPtr);
    }
    
    int Image::GetDpiX() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Imaging_Image_DpiX_get");
        return proc(cPtr);
    }
    
    int Image::GetDpiY() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Imaging_Image_DpiY_get");
        return proc(cPtr);
    }
    
    int Image::GetDx() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Imaging_Image_Dx_get");
        return proc(cPtr);
    }
    
    int Image::GetDy() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Imaging_Image_Dy_get");
        return proc(cPtr);
    }
    
    int Image::GetHeight() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Imaging_Image_Height_get");
        return proc(cPtr);
    }
    
    int Image::GetHeightProcessed() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Imaging_Image_HeightProcessed_get");
        return proc(cPtr);
    }
    
    ImageEncoder Image::GetImageEncoder() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Imaging_Image_ImageEncoder_get");
        return (ImageEncoder)proc(cPtr);
    }
    
    ImageType Image::GetImageType() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Imaging_Image_ImageType_get");
        return (ImageType)proc(cPtr);
    }
    
    bool Image::IsAlphaUsed() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Imaging_Image_IsAlphaUsed_get");
        return 0 != proc(cPtr);
    }
    
    bool Image::IsImageComponent() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Imaging_Image_IsImageComponent_get");
        return 0 != proc(cPtr);
    }
    
    bool Image::IsInverseComponent() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Imaging_Image_IsInverseComponent_get");
        return 0 != proc(cPtr);
    }
    
    bool Image::IsNeedingOCR() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Imaging_Image_IsNeedingOCR_get");
        return 0 != proc(cPtr);
    }
    
    bool Image::IsPaletteUsed() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Imaging_Image_IsPaletteUsed_get");
        return 0 != proc(cPtr);
    }
    
    bool Image::IsRectangleComponent() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Imaging_Image_IsRectangleComponent_get");
        return 0 != proc(cPtr);
    }
    
    bool Image::IsTextBackground() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Imaging_Image_IsTextBackground_get");
        return 0 != proc(cPtr);
    }
    
    bool Image::IsTextComponent() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Imaging_Image_IsTextComponent_get");
        return 0 != proc(cPtr);
    }
    
    bool Image::IsUnderlineComponent() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Imaging_Image_IsUnderlineComponent_get");
        return 0 != proc(cPtr);
    }
    
    bool Image::IsVectorComponent() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Imaging_Image_IsVectorComponent_get");
        return 0 != proc(cPtr);
    }
    
    std::wstring Image::GetName() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Imaging_Image_GetName");
        return UnwrapWString(proc(cPtr));
    }
    
    void Image::SetName(std::wstring value) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Imaging_Image_SetName");
        proc(cPtr, value.c_str());
    }
    
    SolidFramework::Pdf::Plumbing::PageStatistics Image::GetPageStatistics() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Imaging_Image_PageStatistics_get");
        return (SolidFramework::Pdf::Plumbing::PageStatistics)proc(cPtr);
    }
    
    SolidFramework::Interop::ColorPtr Image::GetPaletteColor(int i) {
        static InvokeType_HHI proc = GetFunction<InvokeType_HHI>("SolidFramework_Imaging_Image_GetPaletteColor");
        HANDLE newCPtr = proc(cPtr, i);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::Color>(newCPtr, true);
    }
    
    int Image::GetPaletteSize() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Imaging_Image_PaletteSize_get");
        return proc(cPtr);
    }
    
    SolidFramework::Interop::ColorPtr Image::GetPixelColor(int x, int y) {
        static InvokeType_HHII proc = GetFunction<InvokeType_HHII>("SolidFramework_Imaging_Image_GetPixelColor");
        HANDLE newCPtr = proc(cPtr, x, y);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::Color>(newCPtr, true);
    }
    
    int Image::GetTextAngle() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Imaging_Image_TextAngle_get");
        return proc(cPtr);
    }
    
    Plumbing::TextLanguage Image::GetTextLanguage() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Imaging_Image_TextLanguage_get");
        return (Plumbing::TextLanguage)proc(cPtr);
    }
    
    SolidFramework::Interop::BitmapPtr Image::ToBitmap() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Imaging_Image_ToBitmap");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::Bitmap>(newCPtr, true);
    }
    
    std::wstring Image::ToDataURI(SolidFramework::Converters::Plumbing::ImageDocumentType format) {
        static InvokeType_SHI proc = GetFunction<InvokeType_SHI>("SolidFramework_Imaging_Image_ToDataURI__SWIG_0");
        return UnwrapWString(proc(cPtr, (int)(format)));
    }
    
    std::wstring Image::ToDataURI() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Imaging_Image_ToDataURI__SWIG_1");
        return UnwrapWString(proc(cPtr));
    }
    
    int Image::GetTransparentIndex() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Imaging_Image_TransparentIndex_get");
        return proc(cPtr);
    }
    
    bool Image::GetUsesDCTCompression() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Imaging_Image_UsesDCTCompression_get");
        return 0 != proc(cPtr);
    }
    
    double Image::WasAutoRotated() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Imaging_Image_WasAutoRotated_get");
        return proc(cPtr);
    }
    
    double Image::WasDeskewed() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Imaging_Image_WasDeskewed_get");
        return proc(cPtr);
    }
    
    int Image::GetWidth() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Imaging_Image_Width_get");
        return proc(cPtr);
    }
    
    int Image::GetWidthProcessed() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Imaging_Image_WidthProcessed_get");
        return proc(cPtr);
    }
    

    IOcrRegion::IOcrRegion() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("new_SolidFramework_Imaging_IOcrRegion");
        HANDLE handle = proc();
        Wrap(handle, true);
    }
    
    void IOcrRegion::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Imaging_IOcrRegion");
        FrameworkObject::Dispose(destructor);
    }
    
    int IOcrRegion::GetDx() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Imaging_IOcrRegion_Dx_get");
        return proc(cPtr);
    }
    
    int IOcrRegion::GetDy() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Imaging_IOcrRegion_Dy_get");
        return proc(cPtr);
    }
    
    int IOcrRegion::GetHeight() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Imaging_IOcrRegion_Height_get");
        return proc(cPtr);
    }
    
    OcrRegionType IOcrRegion::GetObjectType() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Imaging_IOcrRegion_GetObjectType");
        return (OcrRegionType)proc(cPtr);
    }
    
    int IOcrRegion::GetWidth() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Imaging_IOcrRegion_Width_get");
        return proc(cPtr);
    }
    

    Ocr::Ocr() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("new_SolidFramework_Imaging_Ocr__SWIG_0");
        HANDLE handle = proc();
        Wrap(handle, true);
    }
    
    Ocr::Ocr(std::wstring languageCode) {
        static InvokeType_PS proc = GetFunction<InvokeType_PS>("new_SolidFramework_Imaging_Ocr__SWIG_1");
        HANDLE handle = proc(languageCode.c_str());
        Wrap(handle, true);
    }
    
    void Ocr::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Imaging_Ocr");
        FrameworkObject::Dispose(destructor);
    }
    
    void Ocr::AddWord(std::wstring customWord) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Imaging_Ocr_AddWord");
        proc(cPtr, customWord.c_str());
    }
    
    int Ocr::GetCount() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Imaging_Ocr_Count_get");
        return proc(cPtr);
    }
    
    std::vector<std::wstring> Ocr::GetLanguages() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("SolidFramework_Imaging_Ocr_GetLanguages");
        return SolidFrameworkNative::StringsArrayToVector(proc(), false);
    }
    
    std::wstring Ocr::NormalizeOcrLanguage(std::wstring language) {
        static InvokeType_SS proc = GetFunction<InvokeType_SS>("SolidFramework_Imaging_Ocr_NormalizeOcrLanguage");
        return UnwrapWString(proc(language.c_str()));
    }
    
    HANDLE Ocr::GetOcrEngine() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Imaging_Ocr_GetOcrEngine");
        return proc(cPtr);
    }
    
    OcrResultsPtr Ocr::Process(ImagePtr image) {
        static InvokeType_HHH proc = GetFunction<InvokeType_HHH>("SolidFramework_Imaging_Ocr_Process");
        HANDLE newCPtr = proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(image));
        return newCPtr == nullptr? nullptr : std::make_shared<OcrResults>(newCPtr, true);
    }
    
    std::wstring Ocr::GetTesseractDataDirectoryLocation() {
        static InvokeType_SV proc = GetFunction<InvokeType_SV>("SolidFramework_Imaging_Ocr_GetTesseractDataDirectoryLocation");
        return proc();
    }
    
    void Ocr::SetTesseractDataDirectoryLocation(std::wstring value) {
        static InvokeType_VS proc = GetFunction<InvokeType_VS>("SolidFramework_Imaging_Ocr_SetTesseractDataDirectoryLocation");
        proc(value.c_str());
    }
    
    bool Ocr::GetTextRecoverySuspects() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Imaging_Ocr_TextRecoverySuspects_get");
        return 0 != proc(cPtr);
    }
    
    void Ocr::SetTextRecoverySuspects(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Imaging_Ocr_TextRecoverySuspects_set");
        proc(cPtr, value);
    }
    

    void OcrGraphicRegion::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Imaging_OcrGraphicRegion");
        FrameworkObject::Dispose(destructor);
    }
    
    SolidFramework::Interop::ColorPtr OcrGraphicRegion::GetColor() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Imaging_OcrGraphicRegion_GetColor");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::Color>(newCPtr, true);
    }
    
    OcrGraphicRegionType OcrGraphicRegion::GetGraphicType() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Imaging_OcrGraphicRegion_GraphicType_get");
        return (OcrGraphicRegionType)proc(cPtr);
    }
    
    int OcrGraphicRegion::GetLineWidth() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Imaging_OcrGraphicRegion_LineWidth_get");
        return proc(cPtr);
    }
    
    OcrGraphicRegionPtr OcrGraphicRegion::DynamicCast(IOcrRegionPtr value) {
        return std::dynamic_pointer_cast<OcrGraphicRegion>(value);
    }
    

    void OcrImageRegion::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Imaging_OcrImageRegion");
        FrameworkObject::Dispose(destructor);
    }
    
    OcrImageRegionPtr OcrImageRegion::DynamicCast(IOcrRegionPtr value) {
        return std::dynamic_pointer_cast<OcrImageRegion>(value);
    }
    

    OcrLine::OcrLine(const std::vector<SolidFramework::Imaging::OcrWordPtr> & words) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("new_SolidFramework_Imaging_OcrLine");
        HANDLE handle = proc(SolidFrameworkNative::VectorToOcrWordsCollection(words)->GetCPtr());
        Wrap(handle, true);
    }
    
    void OcrLine::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Imaging_OcrLine");
        FrameworkObject::Dispose(destructor);
    }
    
    int OcrLine::GetCount() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Imaging_OcrLine_Count_get");
        return proc(cPtr);
    }
    
    std::vector<OcrWordPtr> OcrLine::GetOcrWords() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Imaging_OcrLine_GetOcrWords");
        return SolidFrameworkNative::OcrWordsCollectionToVector(proc(cPtr), false);
    }
    
    std::wstring OcrLine::ToString() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Imaging_OcrLine_ToString");
        return UnwrapWString(proc(cPtr));
    }
    

    void OcrResults::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Imaging_OcrResults");
        FrameworkObject::Dispose(destructor);
    }
    
    int OcrResults::GetCount() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Imaging_OcrResults_Count_get");
        return proc(cPtr);
    }
    
    std::vector<IOcrRegionPtr> OcrResults::GetOcrResults() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Imaging_OcrResults_GetOcrResults");
        return SolidFrameworkNative::IOcrRegionsCollectionToVector(proc(cPtr), false);
    }
    

    void OcrTextRegion::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Imaging_OcrTextRegion");
        FrameworkObject::Dispose(destructor);
    }
    
    int OcrTextRegion::GetOcrConfidentWords() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Imaging_OcrTextRegion_OcrConfidentWords_get");
        return proc(cPtr);
    }
    
    int OcrTextRegion::GetOcrLineCount() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Imaging_OcrTextRegion_OcrLineCount_get");
        return proc(cPtr);
    }
    
    std::vector<OcrLinePtr> OcrTextRegion::GetOcrLines() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Imaging_OcrTextRegion_GetOcrLines");
        return SolidFrameworkNative::OcrLinesCollectionToVector(proc(cPtr), false);
    }
    
    int OcrTextRegion::GetOcrWordCount() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Imaging_OcrTextRegion_OcrWordCount_get");
        return proc(cPtr);
    }
    
    std::wstring OcrTextRegion::GetTextLanguage() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Imaging_OcrTextRegion_GetTextLanguage");
        return UnwrapWString(proc(cPtr));
    }
    
    std::wstring OcrTextRegion::ToString() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Imaging_OcrTextRegion_ToString");
        return UnwrapWString(proc(cPtr));
    }
    
    OcrTextRegionPtr OcrTextRegion::DynamicCast(IOcrRegionPtr value) {
        return std::dynamic_pointer_cast<OcrTextRegion>(value);
    }
    

    void OcrWord::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Imaging_OcrWord");
        FrameworkObject::Dispose(destructor);
    }
    
    SolidFramework::Interop::ColorPtr OcrWord::GetBackground() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Imaging_OcrWord_GetBackground");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::Color>(newCPtr, true);
    }
    
    int OcrWord::GetBaseLine() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Imaging_OcrWord_BaseLine_get");
        return proc(cPtr);
    }
    
    bool OcrWord::GetBold() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Imaging_OcrWord_Bold_get");
        return 0 != proc(cPtr);
    }
    
    SolidFramework::Interop::ColorPtr OcrWord::GetColor() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Imaging_OcrWord_GetColor");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::Color>(newCPtr, true);
    }
    
    int OcrWord::GetConfidence() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Imaging_OcrWord_Confidence_get");
        return proc(cPtr);
    }
    
    int OcrWord::GetDx() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Imaging_OcrWord_Dx_get");
        return proc(cPtr);
    }
    
    int OcrWord::GetDy() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Imaging_OcrWord_Dy_get");
        return proc(cPtr);
    }
    
    std::wstring OcrWord::GetFont() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Imaging_OcrWord_GetFont");
        return UnwrapWString(proc(cPtr));
    }
    
    int OcrWord::GetHeight() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Imaging_OcrWord_Height_get");
        return proc(cPtr);
    }
    
    bool OcrWord::GetItalic() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Imaging_OcrWord_Italic_get");
        return 0 != proc(cPtr);
    }
    
    double OcrWord::GetPts() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Imaging_OcrWord_Pts_get");
        return proc(cPtr);
    }
    
    std::wstring OcrWord::ToString() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Imaging_OcrWord_ToString");
        return UnwrapWString(proc(cPtr));
    }
    
    int OcrWord::GetWidth() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Imaging_OcrWord_GetWidth");
        return proc(cPtr);
    }
    

}} // SolidFramework::Imaging

namespace SolidFramework { namespace Interop { 
    Bitmap::Bitmap() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("new_SolidFramework_Interop_Bitmap__SWIG_0");
        HANDLE handle = proc();
        Wrap(handle, true);
    }
    
    Bitmap::Bitmap(int width, int height, ImageType type) {
        typedef HANDLE (SOLIDCALLBACK *InvokeType)(int, int, int);
        static InvokeType proc = GetFunction<InvokeType>("new_SolidFramework_Interop_Bitmap__SWIG_1");
        HANDLE handle = proc(width, height, (int)(type));
        Wrap(handle, true);
    }
    
    void Bitmap::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Interop_Bitmap");
        FrameworkObject::Dispose(destructor);
    }
    
    HANDLE Bitmap::GetDIB() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Interop_Bitmap_GetDIB");
        return proc(cPtr);
    }
    
    int Bitmap::GetHeight() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Interop_Bitmap_Height_get");
        return proc(cPtr);
    }
    
    bool Bitmap::Load(std::wstring path) {
        static InvokeType_UHS proc = GetFunction<InvokeType_UHS>("SolidFramework_Interop_Bitmap_Load");
        return 0 != proc(cPtr, path.c_str());
    }
    
    bool Bitmap::LoadFromHandle(HANDLE handle) {
        static InvokeType_UHH proc = GetFunction<InvokeType_UHH>("SolidFramework_Interop_Bitmap_LoadFromHandle");
        return 0 != proc(cPtr, handle);
    }
    
    std::vector<ColorPtr> Bitmap::GetPalette() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Interop_Bitmap_GetPalette");
        return SolidFrameworkNative::ColorsCollectionToVector(proc(cPtr), true);
    }
    
    void Bitmap::SetPalette(const std::vector<SolidFramework::Interop::ColorPtr> & value) {
        static InvokeType_VHH proc = GetFunction<InvokeType_VHH>("SolidFramework_Interop_Bitmap_SetPalette");
        proc(cPtr, SolidFrameworkNative::VectorToColorsCollection(value)->GetCPtr());
    }
    
    bool Bitmap::ReadBits(HANDLE buffer, int stride) {
        static InvokeType_UHHI proc = GetFunction<InvokeType_UHHI>("SolidFramework_Interop_Bitmap_ReadBits");
        return 0 != proc(cPtr, buffer, stride);
    }
    
    bool Bitmap::SaveAs(std::wstring path) {
        static InvokeType_UHS proc = GetFunction<InvokeType_UHS>("SolidFramework_Interop_Bitmap_SaveAs__SWIG_0");
        return 0 != proc(cPtr, path.c_str());
    }
    
    bool Bitmap::SaveAs(std::wstring path, SolidFramework::Converters::Plumbing::ImageDocumentType format) {
        typedef unsigned int (SOLIDCALLBACK *InvokeType)(HANDLE, const wchar_t*, int);
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_Interop_Bitmap_SaveAs__SWIG_1");
        return 0 != proc(cPtr, path.c_str(), (int)(format));
    }
    
    std::wstring Bitmap::ToDataURI(SolidFramework::Converters::Plumbing::ImageDocumentType format) {
        static InvokeType_SHI proc = GetFunction<InvokeType_SHI>("SolidFramework_Interop_Bitmap_ToDataURI__SWIG_0");
        return UnwrapWString(proc(cPtr, (int)(format)));
    }
    
    std::wstring Bitmap::ToDataURI() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Interop_Bitmap_ToDataURI__SWIG_1");
        return UnwrapWString(proc(cPtr));
    }
    
    int Bitmap::GetWidth() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Interop_Bitmap_Width_get");
        return proc(cPtr);
    }
    
    bool Bitmap::WriteBits(HANDLE buffer, int stride) {
        static InvokeType_UHHI proc = GetFunction<InvokeType_UHHI>("SolidFramework_Interop_Bitmap_WriteBits");
        return 0 != proc(cPtr, buffer, stride);
    }
    
    int Bitmap::GetXDpi() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Interop_Bitmap_XDpi_get");
        return proc(cPtr);
    }
    
    void Bitmap::SetXDpi(int value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Interop_Bitmap_XDpi_set");
        proc(cPtr, value);
    }
    
    int Bitmap::GetYDpi() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Interop_Bitmap_YDpi_get");
        return proc(cPtr);
    }
    
    void Bitmap::SetYDpi(int value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Interop_Bitmap_YDpi_set");
        proc(cPtr, value);
    }
    

    Color::Color() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("new_SolidFramework_Interop_Color__SWIG_0");
        HANDLE handle = proc();
        Wrap(handle, true);
    }
    
    Color::Color(BYTE r, BYTE g, BYTE b, BYTE a) {
        typedef HANDLE (SOLIDCALLBACK *InvokeType)(BYTE, BYTE, BYTE, BYTE);
        static InvokeType proc = GetFunction<InvokeType>("new_SolidFramework_Interop_Color__SWIG_1");
        HANDLE handle = proc(r, g, b, a);
        Wrap(handle, true);
    }
    
    Color::Color(unsigned int color) {
        typedef HANDLE (SOLIDCALLBACK *InvokeType)(unsigned int);
        static InvokeType proc = GetFunction<InvokeType>("new_SolidFramework_Interop_Color__SWIG_2");
        HANDLE handle = proc(color);
        Wrap(handle, true);
    }
    
    void Color::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Interop_Color");
        FrameworkObject::Dispose(destructor);
    }
    
    int Color::Compare(ColorPtr color1, ColorPtr color2) {
        if (color1 == nullptr) {
            return color2 == nullptr? 0 : 1;
        }
        if (color2 == nullptr) {
            return -1;
        }
        static InvokeType_IHH proc = GetFunction<InvokeType_IHH>("SolidFramework_Interop_Color_Compare");
        return proc(SolidFramework::FrameworkObject::GetCPtr(color1), SolidFramework::FrameworkObject::GetCPtr(color2));
    }
    
    int Color::CompareTo(ColorPtr color) {
        if (this == nullptr) {
            return color == nullptr? 0 : 1;
        }
        if (color == nullptr) {
            return -1;
        }
        static InvokeType_IHH proc = GetFunction<InvokeType_IHH>("SolidFramework_Interop_Color_CompareTo");
        return proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(color));
    }
    
    BYTE Color::GetAlpha() {
        static InvokeType_BH proc = GetFunction<InvokeType_BH>("SolidFramework_Interop_Color_getAlpha");
        return proc(cPtr);
    }
    
    BYTE Color::GetBlue() {
        static InvokeType_BH proc = GetFunction<InvokeType_BH>("SolidFramework_Interop_Color_getBlue");
        return proc(cPtr);
    }
    
    BYTE Color::GetGreen() {
        static InvokeType_BH proc = GetFunction<InvokeType_BH>("SolidFramework_Interop_Color_getGreen");
        return proc(cPtr);
    }
    
    BYTE Color::GetRed() {
        static InvokeType_BH proc = GetFunction<InvokeType_BH>("SolidFramework_Interop_Color_getRed");
        return proc(cPtr);
    }
    
    void Color::SetAlpha(BYTE a) {
        static InvokeType_VHB proc = GetFunction<InvokeType_VHB>("SolidFramework_Interop_Color_setAlpha");
        proc(cPtr, a);
    }
    
    void Color::SetBlue(BYTE b) {
        static InvokeType_VHB proc = GetFunction<InvokeType_VHB>("SolidFramework_Interop_Color_setBlue");
        proc(cPtr, b);
    }
    
    void Color::SetGreen(BYTE g) {
        static InvokeType_VHB proc = GetFunction<InvokeType_VHB>("SolidFramework_Interop_Color_setGreen");
        proc(cPtr, g);
    }
    
    void Color::SetRed(BYTE r) {
        static InvokeType_VHB proc = GetFunction<InvokeType_VHB>("SolidFramework_Interop_Color_setRed");
        proc(cPtr, r);
    }
    

    DateTime::DateTime() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("new_SolidFramework_Interop_DateTime__SWIG_0");
        HANDLE handle = proc();
        Wrap(handle, true);
    }
    
    DateTime::DateTime(int year, int month, int day, int hour, int minute, int second, bool isLocal) {
        typedef HANDLE (SOLIDCALLBACK *InvokeType)(int, int, int, int, int, int, unsigned int);
        static InvokeType proc = GetFunction<InvokeType>("new_SolidFramework_Interop_DateTime__SWIG_1");
        HANDLE handle = proc(year, month, day, hour, minute, second, isLocal);
        Wrap(handle, true);
    }
    
    DateTime::DateTime(int year, int month, int day, int hour, int minute, int second) {
        typedef HANDLE (SOLIDCALLBACK *InvokeType)(int, int, int, int, int, int);
        static InvokeType proc = GetFunction<InvokeType>("new_SolidFramework_Interop_DateTime__SWIG_2");
        HANDLE handle = proc(year, month, day, hour, minute, second);
        Wrap(handle, true);
    }
    
    void DateTime::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Interop_DateTime");
        FrameworkObject::Dispose(destructor);
    }
    
    int DateTime::GetDay() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Interop_DateTime_getDay");
        return proc(cPtr);
    }
    
    int DateTime::GetHour() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Interop_DateTime_getHour");
        return proc(cPtr);
    }
    
    int DateTime::GetMinute() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Interop_DateTime_getMinute");
        return proc(cPtr);
    }
    
    int DateTime::GetMonth() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Interop_DateTime_getMonth");
        return proc(cPtr);
    }
    
    int DateTime::GetSecond() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Interop_DateTime_getSecond");
        return proc(cPtr);
    }
    
    int DateTime::GetYear() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Interop_DateTime_getYear");
        return proc(cPtr);
    }
    
    bool DateTime::IsLocalTime() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Interop_DateTime_isLocalTime");
        return 0 != proc(cPtr);
    }
    
    DateTimePtr DateTime::Now() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("SolidFramework_Interop_DateTime_Now");
        HANDLE newCPtr = proc();
        return newCPtr == nullptr? nullptr : std::make_shared<DateTime>(newCPtr, true);
    }
    
    DateTimePtr DateTime::Parse(std::wstring date) {
        static InvokeType_PS proc = GetFunction<InvokeType_PS>("SolidFramework_Interop_DateTime_Parse");
        HANDLE newCPtr = proc(date.c_str());
        return newCPtr == nullptr? nullptr : std::make_shared<DateTime>(newCPtr, true);
    }
    
    void DateTime::SetIsLocalTime(bool isLocal) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Interop_DateTime_setIsLocalTime");
        proc(cPtr, isLocal);
    }
    
    std::wstring DateTime::ToISOString() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Interop_DateTime_ToISOString");
        return UnwrapWString(proc(cPtr));
    }
    
    std::wstring DateTime::ToSortableString() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Interop_DateTime_ToSortableString");
        return UnwrapWString(proc(cPtr));
    }
    
    bool DateTime::TryParse(std::wstring date, SolidFramework::Interop::DateTimePtr result) {
        static InvokeType_PS proc = GetFunction<InvokeType_PS>("SolidFramework_Interop_DateTime_SafeParse");
        HANDLE newCPtr = proc(date.c_str());
        result = nullptr;
        if (newCPtr != nullptr) {
            result = SolidFramework::Interop::DateTimePtr(new SolidFramework::Interop::DateTime(newCPtr, true));
        }
        return result != nullptr;
    }
    

    Matrix::Matrix() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("new_SolidFramework_Interop_Matrix__SWIG_0");
        HANDLE handle = proc();
        Wrap(handle, true);
    }
    
    Matrix::Matrix(double A, double B, double C, double D, double Tx, double Ty) {
        typedef HANDLE (SOLIDCALLBACK *InvokeType)(double, double, double, double, double, double);
        static InvokeType proc = GetFunction<InvokeType>("new_SolidFramework_Interop_Matrix__SWIG_1");
        HANDLE handle = proc(A, B, C, D, Tx, Ty);
        Wrap(handle, true);
    }
    
    void Matrix::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Interop_Matrix");
        FrameworkObject::Dispose(destructor);
    }
    
    float Matrix::A() {
        static InvokeType_FH proc = GetFunction<InvokeType_FH>("SolidFramework_Interop_Matrix_A");
        return proc(cPtr);
    }
    
    float Matrix::B() {
        static InvokeType_FH proc = GetFunction<InvokeType_FH>("SolidFramework_Interop_Matrix_B");
        return proc(cPtr);
    }
    
    float Matrix::C() {
        static InvokeType_FH proc = GetFunction<InvokeType_FH>("SolidFramework_Interop_Matrix_C");
        return proc(cPtr);
    }
    
    void Matrix::Concat(SolidFramework::Interop::MatrixPtr mtx) {
        static InvokeType_VHH proc = GetFunction<InvokeType_VHH>("SolidFramework_Interop_Matrix_Concat");
        proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(mtx));
    }
    
    void Matrix::ConcatRight(SolidFramework::Interop::MatrixPtr mtx) {
        static InvokeType_VHH proc = GetFunction<InvokeType_VHH>("SolidFramework_Interop_Matrix_ConcatRight");
        proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(mtx));
    }
    
    float Matrix::D() {
        static InvokeType_FH proc = GetFunction<InvokeType_FH>("SolidFramework_Interop_Matrix_D");
        return proc(cPtr);
    }
    
    bool Matrix::IsFlipped() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Interop_Matrix_IsFlipped");
        return 0 != proc(cPtr);
    }
    
    bool Matrix::IsFlippedHor() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Interop_Matrix_IsFlippedHor");
        return 0 != proc(cPtr);
    }
    
    bool Matrix::IsFlippedVert() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Interop_Matrix_IsFlippedVert");
        return 0 != proc(cPtr);
    }
    
    bool Matrix::IsIdentity() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Interop_Matrix_IsIdentity");
        return 0 != proc(cPtr);
    }
    
    bool Matrix::Invert() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Interop_Matrix_Invert");
        return 0 != proc(cPtr);
    }
    
    void Matrix::Multi(double scale) {
        static InvokeType_VHD proc = GetFunction<InvokeType_VHD>("SolidFramework_Interop_Matrix_Multi__SWIG_0");
        proc(cPtr, scale);
    }
    
    void Matrix::Multi(double scaleA, double scaleB, double scaleC, double scaleD) {
        typedef void (SOLIDCALLBACK *InvokeType)(HANDLE, double, double, double, double);
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_Interop_Matrix_Multi__SWIG_1");
        proc(cPtr, scaleA, scaleB, scaleC, scaleD);
    }
    
    bool Matrix::IsOrthoBasis() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Interop_Matrix_IsOrthoBasis");
        return 0 != proc(cPtr);
    }
    
    double Matrix::GetSkew() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Interop_Matrix_GetSkew");
        return proc(cPtr);
    }
    
    SizePtr Matrix::Transform(SolidFramework::Interop::SizePtr pt) {
        static InvokeType_HHH proc = GetFunction<InvokeType_HHH>("SolidFramework_Interop_Matrix_Transform__SWIG_0");
        HANDLE newCPtr = proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(pt));
        return newCPtr == nullptr? nullptr : std::make_shared<Size>(newCPtr, true);
    }
    
    SizeFPtr Matrix::Transform(SolidFramework::Interop::SizeFPtr pt) {
        static InvokeType_HHH proc = GetFunction<InvokeType_HHH>("SolidFramework_Interop_Matrix_Transform__SWIG_1");
        HANDLE newCPtr = proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(pt));
        return newCPtr == nullptr? nullptr : std::make_shared<SizeF>(newCPtr, true);
    }
    
    PointPtr Matrix::Transform(SolidFramework::Interop::PointPtr pt) {
        static InvokeType_HHH proc = GetFunction<InvokeType_HHH>("SolidFramework_Interop_Matrix_Transform__SWIG_2");
        HANDLE newCPtr = proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(pt));
        return newCPtr == nullptr? nullptr : std::make_shared<Point>(newCPtr, true);
    }
    
    PointFPtr Matrix::Transform(SolidFramework::Interop::PointFPtr pt) {
        static InvokeType_HHH proc = GetFunction<InvokeType_HHH>("SolidFramework_Interop_Matrix_Transform__SWIG_3");
        HANDLE newCPtr = proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(pt));
        return newCPtr == nullptr? nullptr : std::make_shared<PointF>(newCPtr, true);
    }
    
    RectanglePtr Matrix::Transform(SolidFramework::Interop::RectanglePtr rc) {
        static InvokeType_HHH proc = GetFunction<InvokeType_HHH>("SolidFramework_Interop_Matrix_Transform__SWIG_4");
        HANDLE newCPtr = proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(rc));
        return newCPtr == nullptr? nullptr : std::make_shared<Rectangle>(newCPtr, true);
    }
    
    RectangleFPtr Matrix::Transform(SolidFramework::Interop::RectangleFPtr rc) {
        static InvokeType_HHH proc = GetFunction<InvokeType_HHH>("SolidFramework_Interop_Matrix_Transform__SWIG_5");
        HANDLE newCPtr = proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(rc));
        return newCPtr == nullptr? nullptr : std::make_shared<RectangleF>(newCPtr, true);
    }
    
    float Matrix::Tx() {
        static InvokeType_FH proc = GetFunction<InvokeType_FH>("SolidFramework_Interop_Matrix_Tx");
        return proc(cPtr);
    }
    
    float Matrix::Ty() {
        static InvokeType_FH proc = GetFunction<InvokeType_FH>("SolidFramework_Interop_Matrix_Ty");
        return proc(cPtr);
    }
    
    std::vector<float> Matrix::GetValues() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Interop_Matrix_GetValues");
        return SolidFrameworkNative::FloatsArrayToVector(proc(cPtr), true);
    }
    
    double Matrix::GetXRotation() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Interop_Matrix_GetXRotation");
        return proc(cPtr);
    }
    
    double Matrix::GetXScale() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Interop_Matrix_GetXScale");
        return proc(cPtr);
    }
    
    double Matrix::GetYRotation() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Interop_Matrix_GetYRotation");
        return proc(cPtr);
    }
    
    double Matrix::GetYScale() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Interop_Matrix_GetYScale");
        return proc(cPtr);
    }
    

    Point::Point() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("new_SolidFramework_Interop_Point__SWIG_0");
        HANDLE handle = proc();
        Wrap(handle, true);
    }
    
    Point::Point(SolidFramework::Interop::PointPtr value) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("new_SolidFramework_Interop_Point__SWIG_1");
        HANDLE handle = proc(SolidFramework::FrameworkObject::GetCPtr(value));
        Wrap(handle, true);
    }
    
    Point::Point(int x, int y) {
        static InvokeType_HII proc = GetFunction<InvokeType_HII>("new_SolidFramework_Interop_Point__SWIG_2");
        HANDLE handle = proc(x, y);
        Wrap(handle, true);
    }
    
    void Point::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Interop_Point");
        FrameworkObject::Dispose(destructor);
    }
    
    int Point::GetX() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Interop_Point_getX");
        return proc(cPtr);
    }
    
    int Point::GetY() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Interop_Point_getY");
        return proc(cPtr);
    }
    
    void Point::SetX(int value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Interop_Point_setX");
        proc(cPtr, value);
    }
    
    void Point::SetY(int value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Interop_Point_setY");
        proc(cPtr, value);
    }
    

    PointF::PointF() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("new_SolidFramework_Interop_PointF__SWIG_0");
        HANDLE handle = proc();
        Wrap(handle, true);
    }
    
    PointF::PointF(float x, float y) {
        typedef HANDLE (SOLIDCALLBACK *InvokeType)(float, float);
        static InvokeType proc = GetFunction<InvokeType>("new_SolidFramework_Interop_PointF__SWIG_1");
        HANDLE handle = proc(x, y);
        Wrap(handle, true);
    }
    
    void PointF::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Interop_PointF");
        FrameworkObject::Dispose(destructor);
    }
    
    int PointF::CompareTo(PointFPtr other) {
        if (this == nullptr) {
            return other == nullptr? 0 : 1;
        }
        if (other == nullptr) {
            return -1;
        }
        static InvokeType_IHH proc = GetFunction<InvokeType_IHH>("SolidFramework_Interop_PointF_CompareTo");
        return proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(other));
    }
    
    float PointF::GetX() {
        static InvokeType_FH proc = GetFunction<InvokeType_FH>("SolidFramework_Interop_PointF_getX");
        return proc(cPtr);
    }
    
    float PointF::GetY() {
        static InvokeType_FH proc = GetFunction<InvokeType_FH>("SolidFramework_Interop_PointF_getY");
        return proc(cPtr);
    }
    
    void PointF::SetX(float value) {
        static InvokeType_VHF proc = GetFunction<InvokeType_VHF>("SolidFramework_Interop_PointF_setX");
        proc(cPtr, value);
    }
    
    void PointF::SetY(float value) {
        static InvokeType_VHF proc = GetFunction<InvokeType_VHF>("SolidFramework_Interop_PointF_setY");
        proc(cPtr, value);
    }
    

    Rectangle::Rectangle() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("new_SolidFramework_Interop_Rectangle__SWIG_0");
        HANDLE handle = proc();
        Wrap(handle, true);
    }
    
    Rectangle::Rectangle(int left, int top, int right, int bottom) {
        typedef HANDLE (SOLIDCALLBACK *InvokeType)(int, int, int, int);
        static InvokeType proc = GetFunction<InvokeType>("new_SolidFramework_Interop_Rectangle__SWIG_1");
        HANDLE handle = proc(left, top, right, bottom);
        Wrap(handle, true);
    }
    
    void Rectangle::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Interop_Rectangle");
        FrameworkObject::Dispose(destructor);
    }
    
    int Rectangle::GetBottom() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Interop_Rectangle_getBottom");
        return proc(cPtr);
    }
    
    int Rectangle::GetHeight() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Interop_Rectangle_getHeight");
        return proc(cPtr);
    }
    
    int Rectangle::GetLeft() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Interop_Rectangle_getLeft");
        return proc(cPtr);
    }
    
    int Rectangle::GetRight() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Interop_Rectangle_getRight");
        return proc(cPtr);
    }
    
    int Rectangle::GetTop() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Interop_Rectangle_getTop");
        return proc(cPtr);
    }
    
    int Rectangle::GetWidth() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Interop_Rectangle_getWidth");
        return proc(cPtr);
    }
    
    void Rectangle::SetBottom(int value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Interop_Rectangle_setBottom");
        proc(cPtr, value);
    }
    
    void Rectangle::SetLeft(int value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Interop_Rectangle_setLeft");
        proc(cPtr, value);
    }
    
    void Rectangle::SetRight(int value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Interop_Rectangle_setRight");
        proc(cPtr, value);
    }
    
    void Rectangle::SetTop(int value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Interop_Rectangle_setTop");
        proc(cPtr, value);
    }
    

    RectangleF::RectangleF() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("new_SolidFramework_Interop_RectangleF__SWIG_0");
        HANDLE handle = proc();
        Wrap(handle, true);
    }
    
    RectangleF::RectangleF(double left, double top, double right, double bottom) {
        typedef HANDLE (SOLIDCALLBACK *InvokeType)(double, double, double, double);
        static InvokeType proc = GetFunction<InvokeType>("new_SolidFramework_Interop_RectangleF__SWIG_1");
        HANDLE handle = proc(left, top, right, bottom);
        Wrap(handle, true);
    }
    
    void RectangleF::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Interop_RectangleF");
        FrameworkObject::Dispose(destructor);
    }
    
    double RectangleF::GetBottom() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Interop_RectangleF_getBottom");
        return proc(cPtr);
    }
    
    double RectangleF::GetHeight() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Interop_RectangleF_getHeight");
        return proc(cPtr);
    }
    
    double RectangleF::GetLeft() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Interop_RectangleF_getLeft");
        return proc(cPtr);
    }
    
    double RectangleF::GetRight() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Interop_RectangleF_getRight");
        return proc(cPtr);
    }
    
    double RectangleF::GetTop() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Interop_RectangleF_getTop");
        return proc(cPtr);
    }
    
    double RectangleF::GetWidth() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Interop_RectangleF_getWidth");
        return proc(cPtr);
    }
    
    void RectangleF::SetBottom(double value) {
        static InvokeType_VHD proc = GetFunction<InvokeType_VHD>("SolidFramework_Interop_RectangleF_setBottom");
        proc(cPtr, value);
    }
    
    void RectangleF::SetLeft(double value) {
        static InvokeType_VHD proc = GetFunction<InvokeType_VHD>("SolidFramework_Interop_RectangleF_setLeft");
        proc(cPtr, value);
    }
    
    void RectangleF::SetRight(double value) {
        static InvokeType_VHD proc = GetFunction<InvokeType_VHD>("SolidFramework_Interop_RectangleF_setRight");
        proc(cPtr, value);
    }
    
    void RectangleF::SetTop(double value) {
        static InvokeType_VHD proc = GetFunction<InvokeType_VHD>("SolidFramework_Interop_RectangleF_setTop");
        proc(cPtr, value);
    }
    

    Size::Size() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("new_SolidFramework_Interop_Size__SWIG_0");
        HANDLE handle = proc();
        Wrap(handle, true);
    }
    
    Size::Size(SolidFramework::Interop::SizePtr value) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("new_SolidFramework_Interop_Size__SWIG_1");
        HANDLE handle = proc(SolidFramework::FrameworkObject::GetCPtr(value));
        Wrap(handle, true);
    }
    
    Size::Size(int x, int y) {
        static InvokeType_HII proc = GetFunction<InvokeType_HII>("new_SolidFramework_Interop_Size__SWIG_2");
        HANDLE handle = proc(x, y);
        Wrap(handle, true);
    }
    
    void Size::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Interop_Size");
        FrameworkObject::Dispose(destructor);
    }
    
    int Size::GetX() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Interop_Size_getX");
        return proc(cPtr);
    }
    
    int Size::GetY() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Interop_Size_getY");
        return proc(cPtr);
    }
    
    void Size::SetX(int value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Interop_Size_setX");
        proc(cPtr, value);
    }
    
    void Size::SetY(int value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Interop_Size_setY");
        proc(cPtr, value);
    }
    

    SizeF::SizeF() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("new_SolidFramework_Interop_SizeF__SWIG_0");
        HANDLE handle = proc();
        Wrap(handle, true);
    }
    
    SizeF::SizeF(float x, float y) {
        typedef HANDLE (SOLIDCALLBACK *InvokeType)(float, float);
        static InvokeType proc = GetFunction<InvokeType>("new_SolidFramework_Interop_SizeF__SWIG_1");
        HANDLE handle = proc(x, y);
        Wrap(handle, true);
    }
    
    void SizeF::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Interop_SizeF");
        FrameworkObject::Dispose(destructor);
    }
    
    float SizeF::GetX() {
        static InvokeType_FH proc = GetFunction<InvokeType_FH>("SolidFramework_Interop_SizeF_getX");
        return proc(cPtr);
    }
    
    float SizeF::GetY() {
        static InvokeType_FH proc = GetFunction<InvokeType_FH>("SolidFramework_Interop_SizeF_getY");
        return proc(cPtr);
    }
    
    void SizeF::SetX(float value) {
        static InvokeType_VHF proc = GetFunction<InvokeType_VHF>("SolidFramework_Interop_SizeF_setX");
        proc(cPtr, value);
    }
    
    void SizeF::SetY(float value) {
        static InvokeType_VHF proc = GetFunction<InvokeType_VHF>("SolidFramework_Interop_SizeF_setY");
        proc(cPtr, value);
    }
    

    TimeSpan::TimeSpan() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("new_SolidFramework_Interop_TimeSpan__SWIG_0");
        HANDLE handle = proc();
        Wrap(handle, true);
    }
    
    TimeSpan::TimeSpan(int hour, int minute, int second) {
        typedef HANDLE (SOLIDCALLBACK *InvokeType)(int, int, int);
        static InvokeType proc = GetFunction<InvokeType>("new_SolidFramework_Interop_TimeSpan__SWIG_1");
        HANDLE handle = proc(hour, minute, second);
        Wrap(handle, true);
    }
    
    TimeSpan::TimeSpan(int seconds) {
        static InvokeType_HI proc = GetFunction<InvokeType_HI>("new_SolidFramework_Interop_TimeSpan__SWIG_2");
        HANDLE handle = proc(seconds);
        Wrap(handle, true);
    }
    
    void TimeSpan::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Interop_TimeSpan");
        FrameworkObject::Dispose(destructor);
    }
    
    int TimeSpan::GetHour() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Interop_TimeSpan_getHour");
        return proc(cPtr);
    }
    
    int TimeSpan::GetMinute() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Interop_TimeSpan_getMinute");
        return proc(cPtr);
    }
    
    int TimeSpan::GetSecond() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Interop_TimeSpan_getSecond");
        return proc(cPtr);
    }
    
    int TimeSpan::GetTotalMinutes() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Interop_TimeSpan_getTotalMinutes");
        return proc(cPtr);
    }
    
    int TimeSpan::GetTotalSeconds() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Interop_TimeSpan_getTotalSeconds");
        return proc(cPtr);
    }
    

    Uri::Uri() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("new_SolidFramework_Interop_Uri__SWIG_0");
        HANDLE handle = proc();
        Wrap(handle, true);
    }
    
    Uri::Uri(std::wstring uri) {
        static InvokeType_PS proc = GetFunction<InvokeType_PS>("new_SolidFramework_Interop_Uri__SWIG_1");
        HANDLE handle = proc(uri.c_str());
        Wrap(handle, true);
    }
    
    void Uri::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Interop_Uri");
        FrameworkObject::Dispose(destructor);
    }
    
    std::wstring Uri::GetUri() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Interop_Uri_getUri");
        return proc(cPtr);
    }
    
    void Uri::SetUri(std::wstring uri) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Interop_Uri_setUri");
        proc(cPtr, uri.c_str());
    }
    

}} // SolidFramework::Interop

namespace SolidFramework { namespace Language { 
    Language::Language() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("new_SolidFramework_Language_Language");
        HANDLE handle = proc();
        Wrap(handle, true);
    }
    
    void Language::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Language_Language");
        FrameworkObject::Dispose(destructor);
    }
    
    std::wstring Language::Detect(std::wstring text, bool useEntireText) {
        typedef const wchar_t* (SOLIDCALLBACK *InvokeType)(HANDLE, const wchar_t*, unsigned int);
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_Language_Language_Detect__SWIG_0");
        return UnwrapWString(proc(cPtr, text.c_str(), useEntireText));
    }
    
    std::wstring Language::Detect(std::wstring text) {
        static InvokeType_SHS proc = GetFunction<InvokeType_SHS>("SolidFramework_Language_Language_Detect__SWIG_1");
        return UnwrapWString(proc(cPtr, text.c_str()));
    }
    
    void Language::DetectLanguages(std::wstring text, bool useEntireText, std::vector<std::wstring> & languages, std::vector<float> & scores) {
        auto languagesArray = SolidFrameworkNative::VectorToStringsArray(languages);
        auto scoresArray = SolidFrameworkNative::VectorToFloatsArray(scores);
        typedef void (SOLIDCALLBACK *InvokeType)(HANDLE, const wchar_t*, unsigned int, HANDLE, HANDLE);
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_Language_Language_DetectLanguages");
        proc(cPtr, text.c_str(), useEntireText, languagesArray->GetCPtr(), scoresArray->GetCPtr());
        languages = SolidFrameworkNative::StringsArrayToVector(languagesArray->GetCPtr(), false);
        scores = SolidFrameworkNative::FloatsArrayToVector(scoresArray->GetCPtr(), false);
    }
    
    bool Language::HarvestWords(std::wstring text, std::wstring wlanguageCode) {
        typedef unsigned int (SOLIDCALLBACK *InvokeType)(HANDLE, const wchar_t*, const wchar_t*);
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_Language_Language_HarvestWords");
        return 0 != proc(cPtr, text.c_str(), wlanguageCode.c_str());
    }
    
    bool Language::IsSoftHyphen(std::wstring firstPart, std::wstring secondPart, std::wstring wlanguageCode) {
        typedef unsigned int (SOLIDCALLBACK *InvokeType)(HANDLE, const wchar_t*, const wchar_t*, const wchar_t*);
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_Language_Language_IsSoftHyphen");
        return 0 != proc(cPtr, firstPart.c_str(), secondPart.c_str(), wlanguageCode.c_str());
    }
    
    bool Language::IsWord(std::wstring text, std::wstring wlanguageCode) {
        typedef unsigned int (SOLIDCALLBACK *InvokeType)(HANDLE, const wchar_t*, const wchar_t*);
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_Language_Language_IsWord");
        return 0 != proc(cPtr, text.c_str(), wlanguageCode.c_str());
    }
    

}} // SolidFramework::Language

namespace SolidFramework { namespace Model { 
    void BookmarksCollection::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_BookmarksCollection");
        FrameworkObject::Dispose(destructor);
    }
    
    BookmarksCollectionPtr BookmarksCollection::DynamicCast(Plumbing::SolidObjectPtr value) {
        return std::dynamic_pointer_cast<BookmarksCollection>(value);
    }
    

    void CoreModel::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_CoreModel");
        if (cPtr != nullptr) {
            Close();
        }
        FrameworkObject::Dispose(destructor);
    }
    
    void CoreModel::Close() {
        static InvokeType_VH proc = GetFunction<InvokeType_VH>("SolidFramework_Model_CoreModel_Close");
        proc(cPtr);
    }
    
    BookmarksCollectionPtr CoreModel::GetBookmarks() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_CoreModel_GetBookmarks");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<BookmarksCollection>(newCPtr, true);
    }
    
    SolidFramework::Converters::Plumbing::ConversionStatus CoreModel::GetConversionStatus() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_CoreModel_ConversionStatus_get");
        return (SolidFramework::Converters::Plumbing::ConversionStatus)proc(cPtr);
    }
    
    CoreModelPtr CoreModel::Create(std::wstring path) {
        static InvokeType_PS proc = GetFunction<InvokeType_PS>("SolidFramework_Model_CoreModel_Create__SWIG_0");
        HANDLE newCPtr = proc(path.c_str());
        return newCPtr == nullptr? nullptr : std::make_shared<CoreModel>(newCPtr, true);
    }
    
    CoreModelPtr CoreModel::Create(std::wstring path, bool noRepairing) {
        typedef HANDLE (SOLIDCALLBACK *InvokeType)(const wchar_t*, unsigned int);
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_Model_CoreModel_Create__SWIG_1");
        HANDLE newCPtr = proc(path.c_str(), noRepairing);
        return newCPtr == nullptr? nullptr : std::make_shared<CoreModel>(newCPtr, true);
    }
    
    CoreModelPtr CoreModel::Create(std::wstring path, PdfOptionsPtr options) {
        static InvokeType_HSH proc = GetFunction<InvokeType_HSH>("SolidFramework_Model_CoreModel_Create__SWIG_2");
        HANDLE newCPtr = proc(path.c_str(), SolidFramework::FrameworkObject::GetCPtr(options));
        return newCPtr == nullptr? nullptr : std::make_shared<CoreModel>(newCPtr, true);
    }
    
    CoreModelPtr CoreModel::Create(std::wstring path, PdfOptionsPtr options, bool readOnly) {
        typedef HANDLE (SOLIDCALLBACK *InvokeType)(const wchar_t*, HANDLE, unsigned int);
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_Model_CoreModel_Create__SWIG_3");
        HANDLE newCPtr = proc(path.c_str(), SolidFramework::FrameworkObject::GetCPtr(options), readOnly);
        return newCPtr == nullptr? nullptr : std::make_shared<CoreModel>(newCPtr, true);
    }
    
    CoreModelPtr CoreModel::Create(std::wstring path, PdfOptionsPtr options, bool readOnly, bool noRepairing) {
        typedef HANDLE (SOLIDCALLBACK *InvokeType)(const wchar_t*, HANDLE, unsigned int, unsigned int);
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_Model_CoreModel_Create__SWIG_4");
        HANDLE newCPtr = proc(path.c_str(), SolidFramework::FrameworkObject::GetCPtr(options), readOnly, noRepairing);
        return newCPtr == nullptr? nullptr : std::make_shared<CoreModel>(newCPtr, true);
    }
    
    Plumbing::SectionPtr CoreModel::GetDefaultSection() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_CoreModel_GetDefaultSection");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<Plumbing::Section>(newCPtr, true);
    }
    
    Plumbing::ParagraphStyleTemplatePtr CoreModel::GetDefaultStyleTemplate() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_CoreModel_GetDefaultParagraphStyleTemplate");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<Plumbing::ParagraphStyleTemplate>(newCPtr, true);
    }
    
    void CoreModel::Export(std::wstring path) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Model_CoreModel_Export__SWIG_0");
        proc(cPtr, path.c_str());
    }
    
    void CoreModel::Export(std::wstring path, bool overwrite) {
        static InvokeType_VHSU proc = GetFunction<InvokeType_VHSU>("SolidFramework_Model_CoreModel_Export__SWIG_1");
        proc(cPtr, path.c_str(), overwrite);
    }
    
    void CoreModel::Export(std::wstring path, Export::ExportOptionsPtr options) {
        static InvokeType_VHSH proc = GetFunction<InvokeType_VHSH>("SolidFramework_Model_CoreModel_Export__SWIG_2");
        proc(cPtr, path.c_str(), SolidFramework::FrameworkObject::GetCPtr(options));
    }
    
    FontsCollectionPtr CoreModel::GetFonts() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_CoreModel_GetFonts");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<FontsCollection>(newCPtr, true);
    }
    
    HyperlinksCollectionPtr CoreModel::GetHyperlinks() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_CoreModel_GetHyperlinks");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<HyperlinksCollection>(newCPtr, true);
    }
    
    std::wstring CoreModel::GetLanguage() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Model_CoreModel_GetLanguage");
        return UnwrapWString(proc(cPtr));
    }
    
    Layout::LayoutDocumentPtr CoreModel::GetLayout() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_CoreModel_GetLayout");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<Layout::LayoutDocument>(newCPtr, true);
    }
    
    ListsCollectionPtr CoreModel::GetLists() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_CoreModel_GetLists");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<ListsCollection>(newCPtr, true);
    }
    
    int CoreModel::GetPageCount() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_CoreModel_PageCount_get");
        return proc(cPtr);
    }
    
    int CoreModel::GetPageWasAutoRotated(int idx) {
        static InvokeType_IHI proc = GetFunction<InvokeType_IHI>("SolidFramework_Model_CoreModel_GetPageWasAutoRotated");
        return proc(cPtr, idx);
    }
    
    bool CoreModel::GetPageWasOCRd(int idx) {
        static InvokeType_UHI proc = GetFunction<InvokeType_UHI>("SolidFramework_Model_CoreModel_GetPageWasOCRd");
        return 0 != proc(cPtr, idx);
    }
    
    Plumbing::ParagraphPtr CoreModel::GetParagraph(HANDLE nSolidRef) {
        static InvokeType_HHH proc = GetFunction<InvokeType_HHH>("SolidFramework_Model_CoreModel_GetParagraph");
        HANDLE newCPtr = proc(cPtr, nSolidRef);
        return newCPtr == nullptr? nullptr : std::make_shared<Plumbing::Paragraph>(newCPtr, true);
    }
    
    StyleTemplatesCollectionPtr CoreModel::GetStyleTemplates() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_CoreModel_GetParagraphStyleTemplates");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<StyleTemplatesCollection>(newCPtr, true);
    }
    
    TopicPtr CoreModel::GetTopic() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_CoreModel_GetTopic");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<Topic>(newCPtr, true);
    }
    
    bool CoreModel::WasAutoRotated() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_CoreModel_WasAutoRotated_get");
        return 0 != proc(cPtr);
    }
    
    bool CoreModel::WasOCRd() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_CoreModel_WasOCRd_get");
        return 0 != proc(cPtr);
    }
    
    CoreModelPtr CoreModel::WrapNativeModel(HANDLE nativeModel) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_CoreModel_WrapNativeModel");
        HANDLE newCPtr = proc(nativeModel);
        return newCPtr == nullptr? nullptr : std::make_shared<CoreModel>(newCPtr, true);
    }
    

    void FontsCollection::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_FontsCollection");
        FrameworkObject::Dispose(destructor);
    }
    
    FontsCollectionPtr FontsCollection::DynamicCast(Plumbing::SolidObjectPtr value) {
        return std::dynamic_pointer_cast<FontsCollection>(value);
    }
    

    void HyperlinksCollection::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_HyperlinksCollection");
        FrameworkObject::Dispose(destructor);
    }
    
    HyperlinksCollectionPtr HyperlinksCollection::DynamicCast(Plumbing::SolidObjectPtr value) {
        return std::dynamic_pointer_cast<HyperlinksCollection>(value);
    }
    

    void ListsCollection::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_ListsCollection");
        FrameworkObject::Dispose(destructor);
    }
    
    ListsCollectionPtr ListsCollection::DynamicCast(Plumbing::SolidObjectPtr value) {
        return std::dynamic_pointer_cast<ListsCollection>(value);
    }
    

    PdfOptions::PdfOptions() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("new_SolidFramework_Model_PdfOptions");
        HANDLE handle = proc();
        static DirectorConnectType_PdfOptions directorConnect = GetFunction<DirectorConnectType_PdfOptions>("SolidFramework_Model_PdfOptions_director_connect");
        directorConnect(handle, nullptr, &SolidFrameworkNative::SolidFramework_Model_PdfOptions_HandleProgress, &SolidFrameworkNative::SolidFramework_Model_PdfOptions_HandleWarning);
        Wrap(handle, true);
        static InvokeType_VHH setCustomData = GetFunction<InvokeType_VHH>("SolidFramework_Model_PdfOptions_SetCustomData");
        setCustomData(handle, (HANDLE)this);
    }
    
    void PdfOptions::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_PdfOptions");
        FrameworkObject::Dispose(destructor);
    }
    
    bool PdfOptions::GetAutoDetectSeparators() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_PdfOptions_AutoDetectSeparators_get");
        return 0 != proc(cPtr);
    }
    
    void PdfOptions::SetAutoDetectSeparators(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Model_PdfOptions_AutoDetectSeparators_set");
        proc(cPtr, value);
    }
    
    bool PdfOptions::GetAutoRotate() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_PdfOptions_AutoRotate_get");
        return 0 != proc(cPtr);
    }
    
    void PdfOptions::SetAutoRotate(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Model_PdfOptions_AutoRotate_set");
        proc(cPtr, value);
    }
    
    bool PdfOptions::GetAverageCharacterScaling() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_PdfOptions_AverageCharacterScaling_get");
        return 0 != proc(cPtr);
    }
    
    void PdfOptions::SetAverageCharacterScaling(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Model_PdfOptions_AverageCharacterScaling_set");
        proc(cPtr, value);
    }
    
    SolidFramework::Converters::Plumbing::ConvertMode PdfOptions::GetConvertMode() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_PdfOptions_ConvertMode_get");
        return (SolidFramework::Converters::Plumbing::ConvertMode)proc(cPtr);
    }
    
    void PdfOptions::SetConvertMode(SolidFramework::Converters::Plumbing::ConvertMode value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Model_PdfOptions_ConvertMode_set");
        proc(cPtr, (int)(value));
    }
    
    SolidFramework::Converters::Plumbing::DecimalSeparator PdfOptions::GetDecimalSeparator() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_PdfOptions_DecimalSeparator_get");
        return (SolidFramework::Converters::Plumbing::DecimalSeparator)proc(cPtr);
    }
    
    void PdfOptions::SetDecimalSeparator(SolidFramework::Converters::Plumbing::DecimalSeparator value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Model_PdfOptions_DecimalSeparator_set");
        proc(cPtr, (int)(value));
    }
    
    bool PdfOptions::GetDetectLanguage() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_PdfOptions_DetectLanguage_get");
        return 0 != proc(cPtr);
    }
    
    void PdfOptions::SetDetectLanguage(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Model_PdfOptions_DetectLanguage_set");
        proc(cPtr, value);
    }
    
    bool PdfOptions::GetDetectLists() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_PdfOptions_DetectLists_get");
        return 0 != proc(cPtr);
    }
    
    void PdfOptions::SetDetectLists(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Model_PdfOptions_DetectLists_set");
        proc(cPtr, value);
    }
    
    bool PdfOptions::GetDetectSoftHyphens() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_PdfOptions_DetectSoftHyphens_get");
        return 0 != proc(cPtr);
    }
    
    void PdfOptions::SetDetectSoftHyphens(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Model_PdfOptions_DetectSoftHyphens_set");
        proc(cPtr, value);
    }
    
    bool PdfOptions::GetDetectTables() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_PdfOptions_DetectTables_get");
        return 0 != proc(cPtr);
    }
    
    void PdfOptions::SetDetectTables(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Model_PdfOptions_DetectTables_set");
        proc(cPtr, value);
    }
    
    bool PdfOptions::GetDetectTaggedTables() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_PdfOptions_DetectTaggedTables_get");
        return 0 != proc(cPtr);
    }
    
    void PdfOptions::SetDetectTaggedTables(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Model_PdfOptions_DetectTaggedTables_set");
        proc(cPtr, value);
    }
    
    bool PdfOptions::GetDetectTiledPages() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_PdfOptions_DetectTiledPages_get");
        return 0 != proc(cPtr);
    }
    
    void PdfOptions::SetDetectTiledPages(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Model_PdfOptions_DetectTiledPages_set");
        proc(cPtr, value);
    }
    
    bool PdfOptions::GetDetectToc() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_PdfOptions_DetectToc_get");
        return 0 != proc(cPtr);
    }
    
    void PdfOptions::SetDetectToc(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Model_PdfOptions_DetectToc_set");
        proc(cPtr, value);
    }
    
    SolidFramework::Converters::Plumbing::ExcelTablesOnSheet PdfOptions::GetExcelTablesOnSheet() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_PdfOptions_ExcelTablesOnSheet_get");
        return (SolidFramework::Converters::Plumbing::ExcelTablesOnSheet)proc(cPtr);
    }
    
    void PdfOptions::SetExcelTablesOnSheet(SolidFramework::Converters::Plumbing::ExcelTablesOnSheet value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Model_PdfOptions_ExcelTablesOnSheet_set");
        proc(cPtr, (int)(value));
    }
    
    bool PdfOptions::GetExposeSourceDocumentPagination() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_PdfOptions_ExposeSourceDocumentPagination_get");
        return 0 != proc(cPtr);
    }
    
    void PdfOptions::SetExposeSourceDocumentPagination(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Model_PdfOptions_ExposeSourceDocumentPagination_set");
        proc(cPtr, value);
    }
    
    bool PdfOptions::GetExposeTargetDocumentPagination() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_PdfOptions_ExposeTargetDocumentPagination_get");
        return 0 != proc(cPtr);
    }
    
    void PdfOptions::SetExposeTargetDocumentPagination(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Model_PdfOptions_ExposeTargetDocumentPagination_set");
        proc(cPtr, value);
    }
    
    SolidFramework::Converters::Plumbing::FootnotesMode PdfOptions::GetFootnotesMode() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_PdfOptions_FootnotesMode_get");
        return (SolidFramework::Converters::Plumbing::FootnotesMode)proc(cPtr);
    }
    
    void PdfOptions::SetFootnotesMode(SolidFramework::Converters::Plumbing::FootnotesMode value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Model_PdfOptions_FootnotesMode_set");
        proc(cPtr, (int)(value));
    }
    
    bool PdfOptions::GetGraphicsAsImages() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_PdfOptions_GraphicsAsImages_get");
        return 0 != proc(cPtr);
    }
    
    void PdfOptions::SetGraphicsAsImages(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Model_PdfOptions_GraphicsAsImages_set");
        proc(cPtr, value);
    }
    
    SolidFramework::Converters::Plumbing::HeaderAndFooterMode PdfOptions::GetHeaderAndFooterMode() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_PdfOptions_HeaderAndFooterMode_get");
        return (SolidFramework::Converters::Plumbing::HeaderAndFooterMode)proc(cPtr);
    }
    
    void PdfOptions::SetHeaderAndFooterMode(SolidFramework::Converters::Plumbing::HeaderAndFooterMode value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Model_PdfOptions_HeaderAndFooterMode_set");
        proc(cPtr, (int)(value));
    }
    
    bool PdfOptions::GetKeepBackgroundColorText() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_PdfOptions_KeepBackgroundColorText_get");
        return 0 != proc(cPtr);
    }
    
    void PdfOptions::SetKeepBackgroundColorText(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Model_PdfOptions_KeepBackgroundColorText_set");
        proc(cPtr, value);
    }
    
    bool PdfOptions::GetKeepInvisibleText() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_PdfOptions_KeepInvisibleText_get");
        return 0 != proc(cPtr);
    }
    
    void PdfOptions::SetKeepInvisibleText(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Model_PdfOptions_KeepInvisibleText_set");
        proc(cPtr, value);
    }
    
    bool PdfOptions::GetKeepLineBreaks() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_PdfOptions_KeepLineBreaks_get");
        return 0 != proc(cPtr);
    }
    
    void PdfOptions::SetKeepLineBreaks(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Model_PdfOptions_KeepLineBreaks_set");
        proc(cPtr, value);
    }
    
    bool PdfOptions::GetKeepNonTableContent() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_PdfOptions_KeepNonTableContent_get");
        return 0 != proc(cPtr);
    }
    
    void PdfOptions::SetKeepNonTableContent(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Model_PdfOptions_KeepNonTableContent_set");
        proc(cPtr, value);
    }
    
    SolidFramework::Converters::Plumbing::MarkupAnnotConversionType PdfOptions::GetMarkupAnnotConversionType() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_PdfOptions_MarkupAnnotConversionType_get");
        return (SolidFramework::Converters::Plumbing::MarkupAnnotConversionType)proc(cPtr);
    }
    
    void PdfOptions::SetMarkupAnnotConversionType(SolidFramework::Converters::Plumbing::MarkupAnnotConversionType value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Model_PdfOptions_MarkupAnnotConversionType_set");
        proc(cPtr, (int)(value));
    }
    
    bool PdfOptions::GetMergeParagraphIndents() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_PdfOptions_MergeParagraphIndents_get");
        return 0 != proc(cPtr);
    }
    
    void PdfOptions::SetMergeParagraphIndents(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Model_PdfOptions_MergeParagraphIndents_set");
        proc(cPtr, value);
    }
    
    SolidFramework::PageRangePtr PdfOptions::GetPageRange() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_PdfOptions_GetPageRange");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::PageRange>(newCPtr, true);
    }
    
    void PdfOptions::SetPageRange(SolidFramework::PageRangePtr value) {
        static InvokeType_VHH proc = GetFunction<InvokeType_VHH>("SolidFramework_Model_PdfOptions_SetPageRange");
        proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(value));
    }
    
    std::wstring PdfOptions::GetPassword() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Model_PdfOptions_GetPassword");
        return proc(cPtr);
    }
    
    void PdfOptions::SetPassword(std::wstring value) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Model_PdfOptions_SetPassword");
        proc(cPtr, value.c_str());
    }
    
    bool PdfOptions::GetPreserveColumnsInNonTableContent() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_PdfOptions_PreserveColumnsInNonTableContent_get");
        return 0 != proc(cPtr);
    }
    
    void PdfOptions::SetPreserveColumnsInNonTableContent(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Model_PdfOptions_PreserveColumnsInNonTableContent_set");
        proc(cPtr, value);
    }
    
    SolidFramework::Converters::Plumbing::ReconstructionMode PdfOptions::GetReconstructionMode() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_PdfOptions_ReconstructionMode_get");
        return (SolidFramework::Converters::Plumbing::ReconstructionMode)proc(cPtr);
    }
    
    void PdfOptions::SetReconstructionMode(SolidFramework::Converters::Plumbing::ReconstructionMode value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Model_PdfOptions_ReconstructionMode_set");
        proc(cPtr, (int)(value));
    }
    
    SolidFramework::Converters::Plumbing::SelectedAreasPtr PdfOptions::GetSelectedAreas() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_PdfOptions_GetSelectedAreas");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Converters::Plumbing::SelectedAreas>(newCPtr, true);
    }
    
    void PdfOptions::SetSelectedAreas(SolidFramework::Converters::Plumbing::SelectedAreasPtr value) {
        static InvokeType_VHH proc = GetFunction<InvokeType_VHH>("SolidFramework_Model_PdfOptions_SetSelectedAreas");
        proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(value));
    }
    
    bool PdfOptions::GetTablesFromContent() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_PdfOptions_TablesFromContent_get");
        return 0 != proc(cPtr);
    }
    
    void PdfOptions::SetTablesFromContent(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Model_PdfOptions_TablesFromContent_set");
        proc(cPtr, value);
    }
    
    SolidFramework::Converters::Plumbing::TextRecovery PdfOptions::GetTextRecovery() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_PdfOptions_TextRecovery_get");
        return (SolidFramework::Converters::Plumbing::TextRecovery)proc(cPtr);
    }
    
    void PdfOptions::SetTextRecovery(SolidFramework::Converters::Plumbing::TextRecovery value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Model_PdfOptions_TextRecovery_set");
        proc(cPtr, (int)(value));
    }
    
    SolidFramework::Converters::Plumbing::TextRecoveryEngine PdfOptions::GetTextRecoveryEngine() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_PdfOptions_TextRecoveryEngine_get");
        return (SolidFramework::Converters::Plumbing::TextRecoveryEngine)proc(cPtr);
    }
    
    void PdfOptions::SetTextRecoveryEngine(SolidFramework::Converters::Plumbing::TextRecoveryEngine value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Model_PdfOptions_TextRecoveryEngine_set");
        proc(cPtr, (int)(value));
    }
    
    SolidFramework::Converters::Plumbing::TextRecoveryEngineNse PdfOptions::GetTextRecoveryEngineNse() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_PdfOptions_TextRecoveryEngineNse_get");
        return (SolidFramework::Converters::Plumbing::TextRecoveryEngineNse)proc(cPtr);
    }
    
    void PdfOptions::SetTextRecoveryEngineNse(SolidFramework::Converters::Plumbing::TextRecoveryEngineNse value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Model_PdfOptions_TextRecoveryEngineNse_set");
        proc(cPtr, (int)(value));
    }
    
    std::wstring PdfOptions::GetTextRecoveryLanguage() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Model_PdfOptions_GetTextRecoveryLanguage");
        return proc(cPtr);
    }
    
    void PdfOptions::SetTextRecoveryLanguage(std::wstring value) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Model_PdfOptions_SetTextRecoveryLanguage");
        proc(cPtr, value.c_str());
    }
    
    SolidFramework::Converters::Plumbing::TextRecoveryNSE PdfOptions::GetTextRecoveryNSE() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_PdfOptions_TextRecoveryNSE_get");
        return (SolidFramework::Converters::Plumbing::TextRecoveryNSE)proc(cPtr);
    }
    
    void PdfOptions::SetTextRecoveryNSE(SolidFramework::Converters::Plumbing::TextRecoveryNSE value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Model_PdfOptions_TextRecoveryNSE_set");
        proc(cPtr, (int)(value));
    }
    
    SolidFramework::Converters::Plumbing::ThousandsSeparator PdfOptions::GetThousandsSeparator() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_PdfOptions_ThousandsSeparator_get");
        return (SolidFramework::Converters::Plumbing::ThousandsSeparator)proc(cPtr);
    }
    
    void PdfOptions::SetThousandsSeparator(SolidFramework::Converters::Plumbing::ThousandsSeparator value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Model_PdfOptions_ThousandsSeparator_set");
        proc(cPtr, (int)(value));
    }
    
    bool PdfOptions::GetTransformPrivateUnicode() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_PdfOptions_TransformPrivateUnicode_get");
        return 0 != proc(cPtr);
    }
    
    void PdfOptions::SetTransformPrivateUnicode(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Model_PdfOptions_TransformPrivateUnicode_set");
        proc(cPtr, value);
    }
    
    PdfOptionsPtr PdfOptions::DynamicCast(Plumbing::OptionsPtr value) {
        return std::dynamic_pointer_cast<PdfOptions>(value);
    }
    
    void PdfOptions::FireProgress(ProgressEventArgsPtr args) {
        if (OnProgress) {
            OnProgress(args);
        }
    }
    
    void PdfOptions::FireWarning(WarningEventArgsPtr args) {
        if (OnWarning) {
            OnWarning(args);
        }
    }
    

    void StyleTemplatesCollection::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_StyleTemplatesCollection");
        FrameworkObject::Dispose(destructor);
    }
    
    StyleTemplatesCollectionPtr StyleTemplatesCollection::DynamicCast(Plumbing::SolidObjectPtr value) {
        return std::dynamic_pointer_cast<StyleTemplatesCollection>(value);
    }
    

    void Topic::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Topic");
        FrameworkObject::Dispose(destructor);
    }
    
    TopicPtr Topic::DynamicCast(Plumbing::SolidObjectPtr value) {
        return std::dynamic_pointer_cast<Topic>(value);
    }
    

}} // SolidFramework::Model

namespace SolidFramework { namespace Model { namespace Export { 
    ExcelOptions::ExcelOptions() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("new_SolidFramework_Model_Export_ExcelOptions");
        HANDLE handle = proc();
        Wrap(handle, true);
    }
    
    void ExcelOptions::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Export_ExcelOptions");
        FrameworkObject::Dispose(destructor);
    }
    
    ExcelOptionsPtr ExcelOptions::DynamicCast(SolidFramework::Model::Plumbing::OptionsPtr value) {
        return std::dynamic_pointer_cast<ExcelOptions>(value);
    }
    

    ExportOptions::ExportOptions() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("new_SolidFramework_Model_Export_ExportOptions");
        HANDLE handle = proc();
        Wrap(handle, true);
    }
    
    void ExportOptions::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Export_ExportOptions");
        FrameworkObject::Dispose(destructor);
    }
    
    void ExportOptions::CheckLicense() {
        static InvokeType_VH proc = GetFunction<InvokeType_VH>("SolidFramework_Model_Export_ExportOptions_CheckLicense");
        proc(cPtr);
    }
    
    SolidFramework::Plumbing::OverwriteMode ExportOptions::GetOverwriteMode() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Export_ExportOptions_OverwriteMode_get");
        return (SolidFramework::Plumbing::OverwriteMode)proc(cPtr);
    }
    
    void ExportOptions::SetOverwriteMode(SolidFramework::Plumbing::OverwriteMode value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Model_Export_ExportOptions_OverwriteMode_set");
        proc(cPtr, (int)(value));
    }
    
    ExportOptionsPtr ExportOptions::DynamicCast(SolidFramework::Model::Plumbing::OptionsPtr value) {
        return std::dynamic_pointer_cast<ExportOptions>(value);
    }
    

    HtmlOptions::HtmlOptions() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("new_SolidFramework_Model_Export_HtmlOptions");
        HANDLE handle = proc();
        Wrap(handle, true);
    }
    
    void HtmlOptions::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Export_HtmlOptions");
        FrameworkObject::Dispose(destructor);
    }
    
    void HtmlOptions::CheckLicense() {
        static InvokeType_VH proc = GetFunction<InvokeType_VH>("SolidFramework_Model_Export_HtmlOptions_CheckLicense");
        proc(cPtr);
    }
    
    bool HtmlOptions::GetDetectLists() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_Export_HtmlOptions_DetectLists_get");
        return 0 != proc(cPtr);
    }
    
    void HtmlOptions::SetDetectLists(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Model_Export_HtmlOptions_DetectLists_set");
        proc(cPtr, value);
    }
    
    SolidFramework::Converters::Plumbing::HtmlNavigationType HtmlOptions::GetHtmlNavigationType() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Export_HtmlOptions_HtmlNavigationType_get");
        return (SolidFramework::Converters::Plumbing::HtmlNavigationType)proc(cPtr);
    }
    
    void HtmlOptions::SetHtmlNavigationType(SolidFramework::Converters::Plumbing::HtmlNavigationType value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Model_Export_HtmlOptions_HtmlNavigationType_set");
        proc(cPtr, (int)(value));
    }
    
    SolidFramework::Converters::Plumbing::HtmlImages HtmlOptions::GetImages() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Export_HtmlOptions_Images_get");
        return (SolidFramework::Converters::Plumbing::HtmlImages)proc(cPtr);
    }
    
    void HtmlOptions::SetImages(SolidFramework::Converters::Plumbing::HtmlImages value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Model_Export_HtmlOptions_Images_set");
        proc(cPtr, (int)(value));
    }
    
    SolidFramework::Converters::Plumbing::ImageDocumentType HtmlOptions::GetImageType() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Export_HtmlOptions_ImageType_get");
        return (SolidFramework::Converters::Plumbing::ImageDocumentType)proc(cPtr);
    }
    
    void HtmlOptions::SetImageType(SolidFramework::Converters::Plumbing::ImageDocumentType value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Model_Export_HtmlOptions_ImageType_set");
        proc(cPtr, (int)(value));
    }
    
    int HtmlOptions::GetWidthLimit() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Export_HtmlOptions_WidthLimit_get");
        return proc(cPtr);
    }
    
    void HtmlOptions::SetWidthLimit(int value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Model_Export_HtmlOptions_WidthLimit_set");
        proc(cPtr, value);
    }
    
    HtmlOptionsPtr HtmlOptions::DynamicCast(SolidFramework::Model::Plumbing::OptionsPtr value) {
        return std::dynamic_pointer_cast<HtmlOptions>(value);
    }
    

    PowerPointOptions::PowerPointOptions() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("new_SolidFramework_Model_Export_PowerPointOptions");
        HANDLE handle = proc();
        Wrap(handle, true);
    }
    
    void PowerPointOptions::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Export_PowerPointOptions");
        FrameworkObject::Dispose(destructor);
    }
    
    PowerPointOptionsPtr PowerPointOptions::DynamicCast(SolidFramework::Model::Plumbing::OptionsPtr value) {
        return std::dynamic_pointer_cast<PowerPointOptions>(value);
    }
    

    TxtOptions::TxtOptions() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("new_SolidFramework_Model_Export_TxtOptions");
        HANDLE handle = proc();
        Wrap(handle, true);
    }
    
    void TxtOptions::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Export_TxtOptions");
        FrameworkObject::Dispose(destructor);
    }
    
    void TxtOptions::CheckLicense() {
        static InvokeType_VH proc = GetFunction<InvokeType_VH>("SolidFramework_Model_Export_TxtOptions_CheckLicense");
        proc(cPtr);
    }
    
    int TxtOptions::GetEncoding() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Export_TxtOptions_Encoding_get");
        return proc(cPtr);
    }
    
    void TxtOptions::SetEncoding(int value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Model_Export_TxtOptions_Encoding_set");
        proc(cPtr, value);
    }
    
    int TxtOptions::GetLineLength() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Export_TxtOptions_LineLength_get");
        return proc(cPtr);
    }
    
    void TxtOptions::SetLineLength(int value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Model_Export_TxtOptions_LineLength_set");
        proc(cPtr, value);
    }
    
    SolidFramework::Converters::Plumbing::LineTerminator TxtOptions::GetLineTerminator() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Export_TxtOptions_LineTerminator_get");
        return (SolidFramework::Converters::Plumbing::LineTerminator)proc(cPtr);
    }
    
    void TxtOptions::SetLineTerminator(SolidFramework::Converters::Plumbing::LineTerminator value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Model_Export_TxtOptions_LineTerminator_set");
        proc(cPtr, (int)(value));
    }
    
    TxtOptionsPtr TxtOptions::DynamicCast(SolidFramework::Model::Plumbing::OptionsPtr value) {
        return std::dynamic_pointer_cast<TxtOptions>(value);
    }
    

    WordOptions::WordOptions() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("new_SolidFramework_Model_Export_WordOptions");
        HANDLE handle = proc();
        Wrap(handle, true);
    }
    
    void WordOptions::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Export_WordOptions");
        FrameworkObject::Dispose(destructor);
    }
    
    SolidFramework::Converters::Plumbing::TargetWordFormat WordOptions::GetTargetWordFormat() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Export_WordOptions_TargetWordFormat_get");
        return (SolidFramework::Converters::Plumbing::TargetWordFormat)proc(cPtr);
    }
    
    void WordOptions::SetTargetWordFormat(SolidFramework::Converters::Plumbing::TargetWordFormat value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Model_Export_WordOptions_TargetWordFormat_set");
        proc(cPtr, (int)(value));
    }
    
    SolidFramework::Converters::Plumbing::WordDocumentType WordOptions::GetWordDocumentType() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Export_WordOptions_WordDocumentType_get");
        return (SolidFramework::Converters::Plumbing::WordDocumentType)proc(cPtr);
    }
    
    void WordOptions::SetWordDocumentType(SolidFramework::Converters::Plumbing::WordDocumentType value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Model_Export_WordOptions_WordDocumentType_set");
        proc(cPtr, (int)(value));
    }
    
    WordOptionsPtr WordOptions::DynamicCast(SolidFramework::Model::Plumbing::OptionsPtr value) {
        return std::dynamic_pointer_cast<WordOptions>(value);
    }
    

}}} // SolidFramework::Model::Export

namespace SolidFramework { namespace Model { namespace Layout { 
    void LayoutChunk::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Layout_LayoutChunk");
        FrameworkObject::Dispose(destructor);
    }
    
    SolidFramework::Interop::RectangleFPtr LayoutChunk::GetBounds() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Layout_LayoutChunk_GetBounds");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::RectangleF>(newCPtr, true);
    }
    
    bool LayoutChunk::IsEqual(LayoutChunkPtr value) {
        static InvokeType_UHH proc = GetFunction<InvokeType_UHH>("SolidFramework_Model_Layout_LayoutChunk_IsEqual");
        return 0 != proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(value));
    }
    
    std::wstring LayoutChunk::GetText() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Model_Layout_LayoutChunk_GetText");
        return UnwrapWString(proc(cPtr));
    }
    

    void LayoutChunkLine::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Layout_LayoutChunkLine");
        FrameworkObject::Dispose(destructor);
    }
    
    SolidFramework::Interop::RectangleFPtr LayoutChunkLine::GetBounds() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Layout_LayoutChunkLine_GetBounds");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::RectangleF>(newCPtr, true);
    }
    
    std::vector<LayoutChunkPtr> LayoutChunkLine::GetChunks() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Layout_LayoutChunkLine_GetChunks");
        return SolidFrameworkNative::LayoutChunksCollectionToVector(proc(cPtr), false);
    }
    
    int LayoutChunkLine::GetCount() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Layout_LayoutChunkLine_Count_get");
        return proc(cPtr);
    }
    

    void LayoutChunkParagraph::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Layout_LayoutChunkParagraph");
        FrameworkObject::Dispose(destructor);
    }
    
    SolidFramework::Interop::RectangleFPtr LayoutChunkParagraph::GetBounds() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Layout_LayoutChunkParagraph_GetBounds");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::RectangleF>(newCPtr, true);
    }
    
    LayoutChunkParagraphPtr LayoutChunkParagraph::GetChunkParagraph(SolidFramework::Model::Plumbing::ParagraphPtr paragraph) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Layout_LayoutChunkParagraph_GetChunkParagraph");
        HANDLE newCPtr = proc(SolidFramework::FrameworkObject::GetCPtr(paragraph));
        return newCPtr == nullptr? nullptr : std::make_shared<LayoutChunkParagraph>(newCPtr, true);
    }
    
    int LayoutChunkParagraph::GetCount() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Layout_LayoutChunkParagraph_Count_get");
        return proc(cPtr);
    }
    
    std::vector<LayoutChunkLinePtr> LayoutChunkParagraph::GetLines() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Layout_LayoutChunkParagraph_GetLines");
        return SolidFrameworkNative::LayoutChunkLinesCollectionToVector(proc(cPtr), false);
    }
    
    int LayoutChunkParagraph::GetPageNumber() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Layout_LayoutChunkParagraph_PageNumber_get");
        return proc(cPtr);
    }
    
    double LayoutChunkParagraph::GetRotation() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Model_Layout_LayoutChunkParagraph_Rotation_get");
        return proc(cPtr);
    }
    

    LayoutDocument::LayoutDocument() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("new_SolidFramework_Model_Layout_LayoutDocument");
        HANDLE handle = proc();
        Wrap(handle, true);
    }
    
    void LayoutDocument::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Layout_LayoutDocument");
        FrameworkObject::Dispose(destructor);
    }
    
    int LayoutDocument::GetCount() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Layout_LayoutDocument_Count_get");
        return proc(cPtr);
    }
    
    LayoutObjectPtr LayoutDocument::FindLayoutObject(int val) {
        static InvokeType_HHI proc = GetFunction<InvokeType_HHI>("SolidFramework_Model_Layout_LayoutDocument_FindLayoutObject");
        HANDLE newCPtr = proc(cPtr, val);
        return SolidFrameworkNative::MakeLayoutObject(newCPtr, true);
    }
    
    std::vector<LayoutObjectPtr> LayoutDocument::GetLayoutObjects() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Layout_LayoutDocument_GetLayoutObjects");
        return SolidFrameworkNative::LayoutObjectsCollectionToVector(proc(cPtr), false);
    }
    

    void LayoutGraphic::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Layout_LayoutGraphic");
        FrameworkObject::Dispose(destructor);
    }
    
    int LayoutGraphic::GetID() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Layout_LayoutGraphic_GetID");
        return proc(cPtr);
    }
    
    HANDLE LayoutGraphic::GetRef() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Layout_LayoutGraphic_GetRef");
        return proc(cPtr);
    }
    
    SolidFramework::Interop::SizeFPtr LayoutGraphic::GetSize() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Layout_LayoutGraphic_GetSize");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::SizeF>(newCPtr, true);
    }
    
    SolidFramework::Interop::MatrixPtr LayoutGraphic::GetTransform() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Layout_LayoutGraphic_GetTransform");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::Matrix>(newCPtr, true);
    }
    
    LayoutGraphicPtr LayoutGraphic::DynamicCast(LayoutObjectPtr value) {
        return std::dynamic_pointer_cast<LayoutGraphic>(value);
    }
    

    void LayoutGroup::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Layout_LayoutGroup");
        FrameworkObject::Dispose(destructor);
    }
    
    int LayoutGroup::GetCount() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Layout_LayoutGroup_Count_get");
        return proc(cPtr);
    }
    
    int LayoutGroup::GetID() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Layout_LayoutGroup_GetID");
        return proc(cPtr);
    }
    
    std::vector<LayoutObjectPtr> LayoutGroup::GetLayoutObjects() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Layout_LayoutGroup_GetLayoutObjects");
        return SolidFrameworkNative::LayoutObjectsCollectionToVector(proc(cPtr), false);
    }
    
    HANDLE LayoutGroup::GetRef() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Layout_LayoutGroup_GetRef");
        return proc(cPtr);
    }
    
    SolidFramework::Interop::SizeFPtr LayoutGroup::GetSize() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Layout_LayoutGroup_GetSize");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::SizeF>(newCPtr, true);
    }
    
    SolidFramework::Interop::MatrixPtr LayoutGroup::GetTransform() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Layout_LayoutGroup_GetTransform");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::Matrix>(newCPtr, true);
    }
    
    LayoutGroupPtr LayoutGroup::DynamicCast(LayoutObjectPtr value) {
        return std::dynamic_pointer_cast<LayoutGroup>(value);
    }
    

    void LayoutImage::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Layout_LayoutImage");
        FrameworkObject::Dispose(destructor);
    }
    
    int LayoutImage::GetID() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Layout_LayoutImage_GetID");
        return proc(cPtr);
    }
    
    HANDLE LayoutImage::GetRef() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Layout_LayoutImage_GetRef");
        return proc(cPtr);
    }
    
    SolidFramework::Interop::SizeFPtr LayoutImage::GetSize() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Layout_LayoutImage_GetSize");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::SizeF>(newCPtr, true);
    }
    
    SolidFramework::Interop::MatrixPtr LayoutImage::GetTransform() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Layout_LayoutImage_GetTransform");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::Matrix>(newCPtr, true);
    }
    
    LayoutImagePtr LayoutImage::DynamicCast(LayoutObjectPtr value) {
        return std::dynamic_pointer_cast<LayoutImage>(value);
    }
    

    void LayoutObject::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Layout_LayoutObject");
        FrameworkObject::Dispose(destructor);
    }
    
    SolidFramework::Interop::RectangleFPtr LayoutObject::GetBounds() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Layout_LayoutObject_GetBounds");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::RectangleF>(newCPtr, true);
    }
    
    int LayoutObject::GetID() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Layout_LayoutObject_GetID");
        return proc(cPtr);
    }
    
    LayoutObjectType LayoutObject::GetObjectType() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Layout_LayoutObject_GetObjectType");
        return (LayoutObjectType)proc(cPtr);
    }
    
    int LayoutObject::GetPageNumber() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Layout_LayoutObject_GetPageNumber");
        return proc(cPtr);
    }
    
    HANDLE LayoutObject::GetRef() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Layout_LayoutObject_GetRef");
        return proc(cPtr);
    }
    
    SolidFramework::Interop::SizeFPtr LayoutObject::GetSize() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Layout_LayoutObject_GetSize");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::SizeF>(newCPtr, true);
    }
    
    SolidFramework::Interop::MatrixPtr LayoutObject::GetTransform() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Layout_LayoutObject_GetTransform");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::Matrix>(newCPtr, true);
    }
    
    LayoutObject::LayoutObject() {
        // only called from derived class constructors
    }
    

    void LayoutPage::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Layout_LayoutPage");
        FrameworkObject::Dispose(destructor);
    }
    
    int LayoutPage::GetCount() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Layout_LayoutPage_Count_get");
        return proc(cPtr);
    }
    
    int LayoutPage::GetID() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Layout_LayoutPage_GetID");
        return proc(cPtr);
    }
    
    std::vector<LayoutObjectPtr> LayoutPage::GetLayoutObjects() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Layout_LayoutPage_GetLayoutObjects");
        return SolidFrameworkNative::LayoutObjectsCollectionToVector(proc(cPtr), false);
    }
    
    int LayoutPage::GetPageNumber() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Layout_LayoutPage_GetPageNumber");
        return proc(cPtr);
    }
    
    HANDLE LayoutPage::GetRef() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Layout_LayoutPage_GetRef");
        return proc(cPtr);
    }
    
    SolidFramework::Interop::SizeFPtr LayoutPage::GetSize() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Layout_LayoutPage_GetSize");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::SizeF>(newCPtr, true);
    }
    
    SolidFramework::Interop::MatrixPtr LayoutPage::GetTransform() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Layout_LayoutPage_GetTransform");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::Matrix>(newCPtr, true);
    }
    
    LayoutPagePtr LayoutPage::DynamicCast(LayoutObjectPtr value) {
        return std::dynamic_pointer_cast<LayoutPage>(value);
    }
    

    void LayoutParagraph::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Layout_LayoutParagraph");
        FrameworkObject::Dispose(destructor);
    }
    
    std::wstring LayoutParagraph::GetAllText() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Model_Layout_LayoutParagraph_GetAllText");
        return UnwrapWString(proc(cPtr));
    }
    
    std::vector<LayoutChunkLinePtr> LayoutParagraph::GetChunkLines() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Layout_LayoutParagraph_GetChunkLines");
        return SolidFrameworkNative::LayoutChunkLinesCollectionToVector(proc(cPtr), false);
    }
    
    int LayoutParagraph::GetCount() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Layout_LayoutParagraph_Count_get");
        return proc(cPtr);
    }
    
    int LayoutParagraph::GetID() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Layout_LayoutParagraph_GetID");
        return proc(cPtr);
    }
    
    std::vector<LayoutObjectPtr> LayoutParagraph::GetLayoutObjects() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Layout_LayoutParagraph_GetLayoutObjects");
        return SolidFrameworkNative::LayoutObjectsCollectionToVector(proc(cPtr), false);
    }
    
    HANDLE LayoutParagraph::GetRef() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Layout_LayoutParagraph_GetRef");
        return proc(cPtr);
    }
    
    double LayoutParagraph::GetRotation() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Model_Layout_LayoutParagraph_Rotation_get");
        return proc(cPtr);
    }
    
    SolidFramework::Interop::SizeFPtr LayoutParagraph::GetSize() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Layout_LayoutParagraph_GetSize");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::SizeF>(newCPtr, true);
    }
    
    SolidFramework::Interop::MatrixPtr LayoutParagraph::GetTransform() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Layout_LayoutParagraph_GetTransform");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::Matrix>(newCPtr, true);
    }
    
    LayoutParagraphPtr LayoutParagraph::DynamicCast(LayoutObjectPtr value) {
        return std::dynamic_pointer_cast<LayoutParagraph>(value);
    }
    

    void LayoutParagraphLine::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Layout_LayoutParagraphLine");
        FrameworkObject::Dispose(destructor);
    }
    
    std::wstring LayoutParagraphLine::GetAllText() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Model_Layout_LayoutParagraphLine_GetAllText");
        return UnwrapWString(proc(cPtr));
    }
    
    int LayoutParagraphLine::GetCount() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Layout_LayoutParagraphLine_Count_get");
        return proc(cPtr);
    }
    
    int LayoutParagraphLine::GetFirstRunCharIndex() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Layout_LayoutParagraphLine_FirstRunCharIndex_get");
        return proc(cPtr);
    }
    
    int LayoutParagraphLine::GetFirstRunIndex() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Layout_LayoutParagraphLine_FirstRunIndex_get");
        return proc(cPtr);
    }
    
    int LayoutParagraphLine::GetID() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Layout_LayoutParagraphLine_GetID");
        return proc(cPtr);
    }
    
    int LayoutParagraphLine::GetLastRunCharIndex() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Layout_LayoutParagraphLine_LastRunCharIndex_get");
        return proc(cPtr);
    }
    
    int LayoutParagraphLine::GetLastRunIndex() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Layout_LayoutParagraphLine_LastRunIndex_get");
        return proc(cPtr);
    }
    
    std::vector<LayoutObjectPtr> LayoutParagraphLine::GetLayoutObjects() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Layout_LayoutParagraphLine_GetLayoutObjects");
        return SolidFrameworkNative::LayoutObjectsCollectionToVector(proc(cPtr), false);
    }
    
    HANDLE LayoutParagraphLine::GetParagraphRef() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Layout_LayoutParagraphLine_GetParagraphRef");
        return proc(cPtr);
    }
    
    HANDLE LayoutParagraphLine::GetRef() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Layout_LayoutParagraphLine_GetRef");
        return proc(cPtr);
    }
    
    SolidFramework::Interop::SizeFPtr LayoutParagraphLine::GetSize() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Layout_LayoutParagraphLine_GetSize");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::SizeF>(newCPtr, true);
    }
    
    SolidFramework::Interop::MatrixPtr LayoutParagraphLine::GetTransform() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Layout_LayoutParagraphLine_GetTransform");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::Matrix>(newCPtr, true);
    }
    
    LayoutParagraphLinePtr LayoutParagraphLine::DynamicCast(LayoutObjectPtr value) {
        return std::dynamic_pointer_cast<LayoutParagraphLine>(value);
    }
    

    void LayoutParagraphListItem::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Layout_LayoutParagraphListItem");
        FrameworkObject::Dispose(destructor);
    }
    
    SolidFramework::Interop::RectangleFPtr LayoutParagraphListItem::GetBounds() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Layout_LayoutParagraphListItem_GetBounds");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::RectangleF>(newCPtr, true);
    }
    
    SolidFramework::Interop::SizeFPtr LayoutParagraphListItem::GetSize() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Layout_LayoutParagraphListItem_GetSize");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::SizeF>(newCPtr, true);
    }
    
    std::wstring LayoutParagraphListItem::GetText() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Model_Layout_LayoutParagraphListItem_GetText");
        return UnwrapWString(proc(cPtr));
    }
    
    SolidFramework::Interop::MatrixPtr LayoutParagraphListItem::GetTransform() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Layout_LayoutParagraphListItem_GetTransform");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::Matrix>(newCPtr, true);
    }
    
    LayoutParagraphListItemPtr LayoutParagraphListItem::DynamicCast(LayoutObjectPtr value) {
        return std::dynamic_pointer_cast<LayoutParagraphListItem>(value);
    }
    

    void LayoutParagraphWord::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Layout_LayoutParagraphWord");
        FrameworkObject::Dispose(destructor);
    }
    
    SolidFramework::Interop::RectangleFPtr LayoutParagraphWord::GetBounds() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Layout_LayoutParagraphWord_GetBounds");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::RectangleF>(newCPtr, true);
    }
    
    int LayoutParagraphWord::GetFirstRunCharIndex() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Layout_LayoutParagraphWord_FirstRunCharIndex_get");
        return proc(cPtr);
    }
    
    int LayoutParagraphWord::GetFirstRunIndex() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Layout_LayoutParagraphWord_FirstRunIndex_get");
        return proc(cPtr);
    }
    
    int LayoutParagraphWord::GetID() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Layout_LayoutParagraphWord_GetID");
        return proc(cPtr);
    }
    
    int LayoutParagraphWord::GetLastRunCharIndex() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Layout_LayoutParagraphWord_LastRunCharIndex_get");
        return proc(cPtr);
    }
    
    int LayoutParagraphWord::GetLastRunIndex() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Layout_LayoutParagraphWord_LastRunIndex_get");
        return proc(cPtr);
    }
    
    HANDLE LayoutParagraphWord::GetParagraphRef() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Layout_LayoutParagraphWord_GetParagraphRef");
        return proc(cPtr);
    }
    
    HANDLE LayoutParagraphWord::GetRef() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Layout_LayoutParagraphWord_GetRef");
        return proc(cPtr);
    }
    
    SolidFramework::Interop::SizeFPtr LayoutParagraphWord::GetSize() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Layout_LayoutParagraphWord_GetSize");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::SizeF>(newCPtr, true);
    }
    
    std::wstring LayoutParagraphWord::GetText() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Model_Layout_LayoutParagraphWord_GetText");
        return UnwrapWString(proc(cPtr));
    }
    
    SolidFramework::Interop::MatrixPtr LayoutParagraphWord::GetTransform() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Layout_LayoutParagraphWord_GetTransform");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::Matrix>(newCPtr, true);
    }
    
    LayoutParagraphWordPtr LayoutParagraphWord::DynamicCast(LayoutObjectPtr value) {
        return std::dynamic_pointer_cast<LayoutParagraphWord>(value);
    }
    
    LayoutParagraphWord::LayoutParagraphWord() {
        // only called from derived class constructors
    }
    

    void LayoutTable::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Layout_LayoutTable");
        FrameworkObject::Dispose(destructor);
    }
    
    int LayoutTable::GetCount() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Layout_LayoutTable_Count_get");
        return proc(cPtr);
    }
    
    int LayoutTable::GetID() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Layout_LayoutTable_GetID");
        return proc(cPtr);
    }
    
    std::vector<LayoutObjectPtr> LayoutTable::GetLayoutObjects() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Layout_LayoutTable_GetLayoutObjects");
        return SolidFrameworkNative::LayoutObjectsCollectionToVector(proc(cPtr), false);
    }
    
    HANDLE LayoutTable::GetRef() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Layout_LayoutTable_GetRef");
        return proc(cPtr);
    }
    
    SolidFramework::Interop::SizeFPtr LayoutTable::GetSize() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Layout_LayoutTable_GetSize");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::SizeF>(newCPtr, true);
    }
    
    SolidFramework::Interop::MatrixPtr LayoutTable::GetTransform() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Layout_LayoutTable_GetTransform");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::Matrix>(newCPtr, true);
    }
    
    LayoutTablePtr LayoutTable::DynamicCast(LayoutObjectPtr value) {
        return std::dynamic_pointer_cast<LayoutTable>(value);
    }
    

    void LayoutTextBox::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Layout_LayoutTextBox");
        FrameworkObject::Dispose(destructor);
    }
    
    int LayoutTextBox::GetCount() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Layout_LayoutTextBox_Count_get");
        return proc(cPtr);
    }
    
    int LayoutTextBox::GetID() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Layout_LayoutTextBox_GetID");
        return proc(cPtr);
    }
    
    std::vector<LayoutObjectPtr> LayoutTextBox::GetLayoutObjects() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Layout_LayoutTextBox_GetLayoutObjects");
        return SolidFrameworkNative::LayoutObjectsCollectionToVector(proc(cPtr), false);
    }
    
    HANDLE LayoutTextBox::GetRef() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Layout_LayoutTextBox_GetRef");
        return proc(cPtr);
    }
    
    SolidFramework::Interop::SizeFPtr LayoutTextBox::GetSize() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Layout_LayoutTextBox_GetSize");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::SizeF>(newCPtr, true);
    }
    
    SolidFramework::Interop::MatrixPtr LayoutTextBox::GetTransform() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Layout_LayoutTextBox_GetTransform");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::Matrix>(newCPtr, true);
    }
    
    LayoutTextBoxPtr LayoutTextBox::DynamicCast(LayoutObjectPtr value) {
        return std::dynamic_pointer_cast<LayoutTextBox>(value);
    }
    

}}} // SolidFramework::Model::Layout

namespace SolidFramework { namespace Model { namespace Pdf { namespace Pages { 
    void ImagePageHolder::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Pdf_Pages_ImagePageHolder");
        FrameworkObject::Dispose(destructor);
    }
    
    IPageHolderPtr ImagePageHolder::Clone() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Pdf_Pages_ImagePageHolder_Clone");
        HANDLE newCPtr = proc(cPtr);
        return SolidFrameworkNative::MakeIPageHolder(newCPtr, true);
    }
    
    void ImagePageHolder::GenerateThumbnail() {
        static InvokeType_VH proc = GetFunction<InvokeType_VH>("SolidFramework_Model_Pdf_Pages_ImagePageHolder_GenerateThumbnail");
        proc(cPtr);
    }
    
    std::wstring ImagePageHolder::GetPath() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Model_Pdf_Pages_ImagePageHolder_GetPath");
        return proc(cPtr);
    }
    
    int ImagePageHolder::GetRotation() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Pdf_Pages_ImagePageHolder_GetRotation");
        return proc(cPtr);
    }
    
    void ImagePageHolder::SetRotation(int value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Model_Pdf_Pages_ImagePageHolder_SetRotation");
        proc(cPtr, value);
    }
    
    ImagePageHolderPtr ImagePageHolder::DynamicCast(IPageHolderPtr value) {
        return std::dynamic_pointer_cast<ImagePageHolder>(value);
    }
    

    void ImagePageSource::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Pdf_Pages_ImagePageSource");
        FrameworkObject::Dispose(destructor);
    }
    
    std::vector<ImagePageHolderPtr> ImagePageSource::GetImagePages() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Pdf_Pages_ImagePageSource_GetImagePages");
        return SolidFrameworkNative::ImagePageHoldersCollectionToVector(proc(cPtr), false);
    }
    
    SolidFramework::Pdf::Repair::RepairResult ImagePageSource::GetRepairIssue() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Pdf_Pages_ImagePageSource_RepairIssue_get");
        return (SolidFramework::Pdf::Repair::RepairResult)proc(cPtr);
    }
    
    void ImagePageSource::SetRepairIssue(SolidFramework::Pdf::Repair::RepairResult value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Model_Pdf_Pages_ImagePageSource_RepairIssue_set");
        proc(cPtr, (int)(value));
    }
    
    ImagePageSourcePtr ImagePageSource::DynamicCast(IPageSourcePtr value) {
        return std::dynamic_pointer_cast<ImagePageSource>(value);
    }
    

    IntParameter::IntParameter(int value) {
        static InvokeType_HI proc = GetFunction<InvokeType_HI>("new_SolidFramework_Model_Pdf_Pages_IntParameter");
        HANDLE handle = proc(value);
        Wrap(handle, true);
    }
    
    void IntParameter::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Pdf_Pages_IntParameter");
        FrameworkObject::Dispose(destructor);
    }
    
    int IntParameter::GetValue() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Pdf_Pages_IntParameter_Value_get");
        return proc(cPtr);
    }
    
    IntParameterPtr IntParameter::DynamicCast(ParameterPtr value) {
        return std::dynamic_pointer_cast<IntParameter>(value);
    }
    

    void IPageHolder::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Pdf_Pages_IPageHolder");
        FrameworkObject::Dispose(destructor);
    }
    
    IPageHolderPtr IPageHolder::Clone() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Pdf_Pages_IPageHolder_Clone");
        HANDLE newCPtr = proc(cPtr);
        return SolidFrameworkNative::MakeIPageHolder(newCPtr, true);
    }
    
    int IPageHolder::GetCurrentIndex() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Pdf_Pages_IPageHolder_CurrentIndex_get");
        return proc(cPtr);
    }
    
    void IPageHolder::GenerateThumbnail() {
        static InvokeType_VH proc = GetFunction<InvokeType_VH>("SolidFramework_Model_Pdf_Pages_IPageHolder_GenerateThumbnail");
        proc(cPtr);
    }
    
    float IPageHolder::GetHeight() {
        static InvokeType_FH proc = GetFunction<InvokeType_FH>("SolidFramework_Model_Pdf_Pages_IPageHolder_Height_get");
        return proc(cPtr);
    }
    
    PagesModelPtr IPageHolder::GetModel() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Pdf_Pages_IPageHolder_GetModel");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<PagesModel>(newCPtr, true);
    }
    
    int IPageHolder::GetRotation() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Pdf_Pages_IPageHolder_GetRotation");
        return proc(cPtr);
    }
    
    void IPageHolder::SetRotation(int value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Model_Pdf_Pages_IPageHolder_SetRotation");
        proc(cPtr, value);
    }
    
    SolidFramework::Interop::SizeFPtr IPageHolder::GetSize() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Pdf_Pages_IPageHolder_GetSize");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::SizeF>(newCPtr, true);
    }
    
    int IPageHolder::GetTargetID() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Pdf_Pages_IPageHolder_TargetID_get");
        return proc(cPtr);
    }
    
    void IPageHolder::SetTargetID(int value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Model_Pdf_Pages_IPageHolder_TargetID_set");
        proc(cPtr, value);
    }
    
    SolidFramework::Interop::BitmapPtr IPageHolder::GetThumbnail() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Pdf_Pages_IPageHolder_GetThumbnail");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::Bitmap>(newCPtr, true);
    }
    
    bool IPageHolder::GetThumbnailGenerated() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_Pdf_Pages_IPageHolder_ThumbnailGenerated_get");
        return 0 != proc(cPtr);
    }
    
    void IPageHolder::SetThumbnailGenerated(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Model_Pdf_Pages_IPageHolder_ThumbnailGenerated_set");
        proc(cPtr, value);
    }
    
    std::wstring IPageHolder::ToString() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Model_Pdf_Pages_IPageHolder_ToString");
        return UnwrapWString(proc(cPtr));
    }
    
    bool IPageHolder::GetVisible() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_Pdf_Pages_IPageHolder_Visible_get");
        return 0 != proc(cPtr);
    }
    
    void IPageHolder::SetVisible(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Model_Pdf_Pages_IPageHolder_Visible_set");
        proc(cPtr, value);
    }
    
    float IPageHolder::GetWidth() {
        static InvokeType_FH proc = GetFunction<InvokeType_FH>("SolidFramework_Model_Pdf_Pages_IPageHolder_Width_get");
        return proc(cPtr);
    }
    
    IPageHolder::IPageHolder() {
        // only called from derived class constructors
    }
    

    void IPageSource::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Pdf_Pages_IPageSource");
        FrameworkObject::Dispose(destructor);
    }
    
    IPageSource::IPageSource() {
        // only called from derived class constructors
    }
    

    ModelChangedEventArgs::ModelChangedEventArgs() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("new_SolidFramework_Model_Pdf_Pages_ModelChangedEventArgs__SWIG_0");
        HANDLE handle = proc();
        Wrap(handle, true);
    }
    
    ModelChangedEventArgs::ModelChangedEventArgs(ModelChangedEvent theEvent) {
        static InvokeType_HIH proc = GetFunction<InvokeType_HIH>("new_SolidFramework_Model_Pdf_Pages_ModelChangedEventArgs__SWIG_2");
        HANDLE handle = proc((int)(theEvent), (HANDLE)this);
        Wrap(handle, true);
    }
    
    void ModelChangedEventArgs::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Pdf_Pages_ModelChangedEventArgs");
        FrameworkObject::Dispose(destructor);
    }
    
    bool ModelChangedEventArgs::GetClear() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_Pdf_Pages_ModelChangedEventArgs_Clear_get");
        return 0 != proc(cPtr);
    }
    
    ModelChangedEvent ModelChangedEventArgs::GetEvent() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Pdf_Pages_ModelChangedEventArgs_Event_get");
        return (ModelChangedEvent)proc(cPtr);
    }
    
    bool ModelChangedEventArgs::GetRotation() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_Pdf_Pages_ModelChangedEventArgs_Rotation_get");
        return 0 != proc(cPtr);
    }
    

    OfferPasswordEventArgs::OfferPasswordEventArgs() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("new_SolidFramework_Model_Pdf_Pages_OfferPasswordEventArgs__SWIG_1");
        HANDLE handle = proc((HANDLE)this);
        Wrap(handle, true);
    }
    
    void OfferPasswordEventArgs::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Pdf_Pages_OfferPasswordEventArgs");
        FrameworkObject::Dispose(destructor);
    }
    
    std::wstring OfferPasswordEventArgs::GetPassword() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Model_Pdf_Pages_OfferPasswordEventArgs_GetPassword");
        return proc(cPtr);
    }
    
    void OfferPasswordEventArgs::SetPassword(std::wstring value) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Model_Pdf_Pages_OfferPasswordEventArgs_SetPassword");
        proc(cPtr, value.c_str());
    }
    
    std::wstring OfferPasswordEventArgs::GetPath() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Model_Pdf_Pages_OfferPasswordEventArgs_GetPath");
        return proc(cPtr);
    }
    
    void OfferPasswordEventArgs::SetPath(std::wstring value) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Model_Pdf_Pages_OfferPasswordEventArgs_SetPath");
        proc(cPtr, value.c_str());
    }
    
    std::wstring OfferPasswordEventArgs::GetPrompt() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Model_Pdf_Pages_OfferPasswordEventArgs_GetPrompt");
        return proc(cPtr);
    }
    
    void OfferPasswordEventArgs::SetPrompt(std::wstring value) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Model_Pdf_Pages_OfferPasswordEventArgs_SetPrompt");
        proc(cPtr, value.c_str());
    }
    
    bool OfferPasswordEventArgs::GetPasswordWasEntered() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_Pdf_Pages_OfferPasswordEventArgs_GetPasswordWasEntered");
        return 0 != proc(cPtr);
    }
    
    void OfferPasswordEventArgs::SetPasswordWasEntered(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Model_Pdf_Pages_OfferPasswordEventArgs_SetPasswordWasEntered");
        proc(cPtr, value);
    }
    

    void Operation::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Pdf_Pages_Operation");
        FrameworkObject::Dispose(destructor);
    }
    
    bool Operation::GetEnabled() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_Pdf_Pages_Operation_GetEnabled");
        return 0 != proc(cPtr);
    }
    
    bool Operation::Execute() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_Pdf_Pages_Operation_Execute__SWIG_0");
        return 0 != proc(cPtr);
    }
    
    bool Operation::Execute(ParameterPtr parameter) {
        static InvokeType_UHH proc = GetFunction<InvokeType_UHH>("SolidFramework_Model_Pdf_Pages_Operation_Execute__SWIG_1");
        return 0 != proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(parameter));
    }
    
    PagesModelPtr Operation::GetModel() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Pdf_Pages_Operation_GetModel");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<PagesModel>(newCPtr, true);
    }
    
    ModelOperation Operation::GetModelOperation() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Pdf_Pages_Operation_GetModelOperation");
        return (ModelOperation)proc(cPtr);
    }
    
    std::wstring Operation::GetName() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Model_Pdf_Pages_Operation_GetName");
        return UnwrapWString(proc(cPtr));
    }
    
    Operation::Operation() {
        // only called from derived class constructors
    }
    

    PagesChangedEventArgs::PagesChangedEventArgs() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("new_SolidFramework_Model_Pdf_Pages_PagesChangedEventArgs__SWIG_0");
        HANDLE handle = proc();
        Wrap(handle, true);
    }
    
    PagesChangedEventArgs::PagesChangedEventArgs(OperationPtr operation) {
        static InvokeType_HHH proc = GetFunction<InvokeType_HHH>("new_SolidFramework_Model_Pdf_Pages_PagesChangedEventArgs__SWIG_1");
        HANDLE handle = proc(SolidFramework::FrameworkObject::GetCPtr(operation), (HANDLE)this);
        Wrap(handle, true);
    }
    
    void PagesChangedEventArgs::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Pdf_Pages_PagesChangedEventArgs");
        FrameworkObject::Dispose(destructor);
    }
    
    OperationPtr PagesChangedEventArgs::GetOperation() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Pdf_Pages_PagesChangedEventArgs_GetOperation");
        HANDLE newCPtr = proc(cPtr);
        return SolidFrameworkNative::MakeOperation(newCPtr, true);
    }
    
    void PagesChangedEventArgs::SetOperation(OperationPtr value) {
        static InvokeType_VHH proc = GetFunction<InvokeType_VHH>("SolidFramework_Model_Pdf_Pages_PagesChangedEventArgs_SetOperation");
        proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(value));
    }
    

    PageSelection::PageSelection(IPageHolderPtr page, SolidFramework::Interop::RectangleFPtr area) {
        static InvokeType_HHH proc = GetFunction<InvokeType_HHH>("new_SolidFramework_Model_Pdf_Pages_PageSelection");
        HANDLE handle = proc(SolidFramework::FrameworkObject::GetCPtr(page), SolidFramework::FrameworkObject::GetCPtr(area));
        Wrap(handle, true);
    }
    
    void PageSelection::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Pdf_Pages_PageSelection");
        FrameworkObject::Dispose(destructor);
    }
    
    SolidFramework::Interop::RectangleFPtr PageSelection::GetArea() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Pdf_Pages_PageSelection_GetArea");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::RectangleF>(newCPtr, true);
    }
    
    IPageHolderPtr PageSelection::GetPage() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Pdf_Pages_PageSelection_GetPage");
        HANDLE newCPtr = proc(cPtr);
        return SolidFrameworkNative::MakeIPageHolder(newCPtr, true);
    }
    

    PagesModel::PagesModel() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("new_SolidFramework_Model_Pdf_Pages_PagesModelBase");
        HANDLE handle = proc();
        static DirectorConnectType_PagesModel directorConnect = GetFunction<DirectorConnectType_PagesModel>("SolidFramework_Model_Pdf_Pages_PagesModelBase_director_connect");
        directorConnect(handle, &SolidFrameworkNative::SolidFramework_Model_Pdf_Pages_PagesModel_HandleModelChanged, &SolidFrameworkNative::SolidFramework_Model_Pdf_Pages_PagesModel_HandlePagesChanged, &SolidFrameworkNative::SolidFramework_Model_Pdf_Pages_PagesModel_HandleSaveProgress, &SolidFrameworkNative::SolidFramework_Model_Pdf_Pages_PagesModel_HandlePasswordEntered, &SolidFrameworkNative::SolidFramework_Model_Pdf_Pages_PagesModel_HandleOfferPasswordPrompt, &SolidFrameworkNative::SolidFramework_Model_Pdf_Pages_PagesModel_HandleThumbnailChanged, &SolidFrameworkNative::SolidFramework_Model_Pdf_Pages_PagesModel_HandleRepairFailed, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
        Wrap(handle, true);
        static InvokeType_VHH setCustomData = GetFunction<InvokeType_VHH>("SolidFramework_Model_Pdf_Pages_PagesModelBase_SetCustomData");
        setCustomData(handle, (HANDLE)this);
    }
    
    void PagesModel::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Pdf_Pages_PagesModelBase");
        FrameworkObject::Dispose(destructor);
    }
    
    void PagesModel::Close() {
        static InvokeType_VH proc = GetFunction<InvokeType_VH>("SolidFramework_Model_Pdf_Pages_PagesModelBase_Close");
        proc(cPtr);
    }
    
    std::vector<IPageHolderPtr> PagesModel::GetAllPages() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Pdf_Pages_PagesModelBase_GetAllPages");
        return SolidFrameworkNative::IPageHoldersCollectionToVector(proc(cPtr), false);
    }
    
    bool PagesModel::CanRedo() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_Pdf_Pages_PagesModelBase_CanRedo");
        return 0 != proc(cPtr);
    }
    
    bool PagesModel::CanUndo() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_Pdf_Pages_PagesModelBase_CanUndo");
        return 0 != proc(cPtr);
    }
    
    SolidFramework::Plumbing::ValidationMode PagesModel::GetClaimedValidationMode() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Pdf_Pages_PagesModelBase_ClaimedValidationMode_get");
        return (SolidFramework::Plumbing::ValidationMode)proc(cPtr);
    }
    
    void PagesModel::SetClaimedValidationMode(SolidFramework::Plumbing::ValidationMode value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Model_Pdf_Pages_PagesModelBase_ClaimedValidationMode_set");
        proc(cPtr, (int)(value));
    }
    
    void PagesModel::Clear() {
        static InvokeType_VH proc = GetFunction<InvokeType_VH>("SolidFramework_Model_Pdf_Pages_PagesModelBase_Clear");
        proc(cPtr);
    }
    
    IPageSourcePtr PagesModel::CreateSource(std::wstring originalPath) {
        static InvokeType_HHS proc = GetFunction<InvokeType_HHS>("SolidFramework_Model_Pdf_Pages_PagesModelBase_CreateSource__SWIG_0");
        HANDLE newCPtr = proc(cPtr, originalPath.c_str());
        return SolidFrameworkNative::MakeIPageSource(newCPtr, true);
    }
    
    IPageSourcePtr PagesModel::CreateSource(std::wstring originalPath, SolidFramework::Pdf::AccessPermissions permissionsRequired, std::wstring attemptPassword) {
        typedef HANDLE (SOLIDCALLBACK *InvokeType)(HANDLE, const wchar_t*, int, const wchar_t*);
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_Model_Pdf_Pages_PagesModelBase_CreateSource__SWIG_1");
        HANDLE newCPtr = proc(cPtr, originalPath.c_str(), (int)(permissionsRequired), attemptPassword.c_str());
        return SolidFrameworkNative::MakeIPageSource(newCPtr, true);
    }
    
    IPageSourcePtr PagesModel::CreateSource(std::wstring tempCopyPath, std::wstring originalPath, SolidFramework::Pdf::AccessPermissions permissionsRequired, std::wstring attemptPassword) {
        typedef HANDLE (SOLIDCALLBACK *InvokeType)(HANDLE, const wchar_t*, const wchar_t*, int, const wchar_t*);
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_Model_Pdf_Pages_PagesModelBase_CreateSource__SWIG_2");
        HANDLE newCPtr = proc(cPtr, tempCopyPath.c_str(), originalPath.c_str(), (int)(permissionsRequired), attemptPassword.c_str());
        return SolidFrameworkNative::MakeIPageSource(newCPtr, true);
    }
    
    int PagesModel::GetCurrentPage() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Pdf_Pages_PagesModelBase_CurrentPage_get");
        return proc(cPtr);
    }
    
    void PagesModel::SetCurrentPage(int value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Model_Pdf_Pages_PagesModelBase_CurrentPage_set");
        proc(cPtr, value);
    }
    
    Plumbing::DescriptionPtr PagesModel::GetDescription() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Pdf_Pages_PagesModelBase_GetDescription");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<Plumbing::Description>(newCPtr, true);
    }
    
    void PagesModel::SetDescription(Plumbing::DescriptionPtr value) {
        static InvokeType_VHH proc = GetFunction<InvokeType_VHH>("SolidFramework_Model_Pdf_Pages_PagesModelBase_SetDescription");
        proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(value));
    }
    
    bool PagesModel::GetEmptyModelAllowed() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_Pdf_Pages_PagesModelBase_EmptyModelAllowed_get");
        return 0 != proc(cPtr);
    }
    
    void PagesModel::SetEmptyModelAllowed(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Model_Pdf_Pages_PagesModelBase_EmptyModelAllowed_set");
        proc(cPtr, value);
    }
    
    bool PagesModel::EnterOwnerAuthenticationMode() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_Pdf_Pages_PagesModelBase_EnterOwnerAuthenticationMode");
        return 0 != proc(cPtr);
    }
    
    bool PagesModel::IsEqual(PagesModelPtr value) {
        static InvokeType_UHH proc = GetFunction<InvokeType_UHH>("SolidFramework_Model_Pdf_Pages_PagesModelBase_IsEqual");
        return 0 != proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(value));
    }
    
    int PagesModel::FindPageIndex(int objectID) {
        static InvokeType_IHI proc = GetFunction<InvokeType_IHI>("SolidFramework_Model_Pdf_Pages_PagesModelBase_FindPageIndex");
        return proc(cPtr, objectID);
    }
    
    IPageHolderPtr PagesModel::FindVisiblePage(int pageIndex) {
        static InvokeType_HHI proc = GetFunction<InvokeType_HHI>("SolidFramework_Model_Pdf_Pages_PagesModelBase_FindVisiblePage");
        HANDLE newCPtr = proc(cPtr, pageIndex);
        return SolidFrameworkNative::MakeIPageHolder(newCPtr, true);
    }
    
    std::vector<SolidFramework::Pdf::Plumbing::PdfFontPtr> PagesModel::GetFonts() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Pdf_Pages_PagesModelBase_GetFonts");
        return SolidFrameworkNative::PdfFontsCollectionToVector(proc(cPtr), true);
    }
    
    int PagesModel::IndexToTargetID(int index) {
        static InvokeType_IHI proc = GetFunction<InvokeType_IHI>("SolidFramework_Model_Pdf_Pages_PagesModelBase_IndexToTargetID");
        return proc(cPtr, index);
    }
    
    Plumbing::InitialViewPtr PagesModel::GetInitialView() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Pdf_Pages_PagesModelBase_GetInitialView");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<Plumbing::InitialView>(newCPtr, true);
    }
    
    void PagesModel::SetInitialView(Plumbing::InitialViewPtr value) {
        static InvokeType_VHH proc = GetFunction<InvokeType_VHH>("SolidFramework_Model_Pdf_Pages_PagesModelBase_SetInitialView");
        proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(value));
    }
    
    void PagesModel::InsertRange(int position, const std::vector<SolidFramework::Model::Pdf::Pages::IPageSourcePtr> & collection) {
        static InvokeType_VHIH proc = GetFunction<InvokeType_VHIH>("SolidFramework_Model_Pdf_Pages_PagesModelBase_InsertRange__SWIG_0");
        proc(cPtr, position, SolidFrameworkNative::VectorToIPageSourcesCollection(collection)->GetCPtr());
    }
    
    void PagesModel::InsertRange(int insert, const std::vector<SolidFramework::Model::Pdf::Pages::IPageHolderPtr> & collection) {
        static InvokeType_VHIH proc = GetFunction<InvokeType_VHIH>("SolidFramework_Model_Pdf_Pages_PagesModelBase_InsertRange__SWIG_1");
        proc(cPtr, insert, SolidFrameworkNative::VectorToIPageHoldersCollection(collection)->GetCPtr());
    }
    
    void PagesModel::InsertRange(int position, const std::vector<std::wstring> & pageFiles, bool preferPdfAWhenCreating) {
        typedef void (SOLIDCALLBACK *InvokeType)(HANDLE, int, HANDLE, unsigned int);
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_Model_Pdf_Pages_PagesModelBase_InsertRange__SWIG_2");
        proc(cPtr, position, SolidFrameworkNative::VectorToStringsArray(pageFiles)->GetCPtr(), preferPdfAWhenCreating);
    }
    
    int PagesModel::GetInstance() {
        static InvokeType_IV proc = GetFunction<InvokeType_IV>("SolidFramework_Model_Pdf_Pages_PagesModelBase_GetInstance");
        return proc();
    }
    
    void PagesModel::SetInstance(int value) {
        static InvokeType_VI proc = GetFunction<InvokeType_VI>("SolidFramework_Model_Pdf_Pages_PagesModelBase_SetInstance");
        proc(value);
    }
    
    bool PagesModel::IsSinglePDFSource() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_Pdf_Pages_PagesModelBase_IsSinglePDFSource_get");
        return 0 != proc(cPtr);
    }
    
    bool PagesModel::IsUntitled() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_Pdf_Pages_PagesModelBase_IsUntitled_get");
        return 0 != proc(cPtr);
    }
    
    void PagesModel::SetIsUntitled(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Model_Pdf_Pages_PagesModelBase_IsUntitled_set");
        proc(cPtr, value);
    }
    
    bool PagesModel::GetModified() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_Pdf_Pages_PagesModelBase_Modified_get");
        return 0 != proc(cPtr);
    }
    
    void PagesModel::ModifySecurity(SolidFramework::Pdf::EncryptionAlgorithm algorithm, SolidFramework::Pdf::AccessPermissions permissions, std::wstring ownerPassword, std::wstring userPassword, std::wstring originalOwnerPassword) {
        typedef void (SOLIDCALLBACK *InvokeType)(HANDLE, int, int, const wchar_t*, const wchar_t*, const wchar_t*);
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_Model_Pdf_Pages_PagesModelBase_ModifySecurity");
        proc(cPtr, (int)(algorithm), (int)(permissions), ownerPassword.c_str(), userPassword.c_str(), originalOwnerPassword.c_str());
    }
    
    SolidFramework::Pdf::Plumbing::PdfNamedDestinationsCollectionPtr PagesModel::GetNamedDestinations() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Pdf_Pages_PagesModelBase_GetNamedDestinations");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Pdf::Plumbing::PdfNamedDestinationsCollection>(newCPtr, false);
    }
    
    int PagesModel::GetNextTargetID() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Pdf_Pages_PagesModelBase_GetNextTargetID");
        return proc(cPtr);
    }
    
    int PagesModel::NormalizeRotation(int rotation) {
        static InvokeType_II proc = GetFunction<InvokeType_II>("SolidFramework_Model_Pdf_Pages_PagesModelBase_NormalizeRotation");
        return proc(rotation);
    }
    
    void PagesModel::OpenPDF(std::wstring pdfPath) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Model_Pdf_Pages_PagesModelBase_OpenPDF__SWIG_0");
        proc(cPtr, pdfPath.c_str());
    }
    
    void PagesModel::OpenPDF(std::wstring pdfPath, std::wstring attemptPassword) {
        typedef void (SOLIDCALLBACK *InvokeType)(HANDLE, const wchar_t*, const wchar_t*);
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_Model_Pdf_Pages_PagesModelBase_OpenPDF__SWIG_1");
        proc(cPtr, pdfPath.c_str(), attemptPassword.c_str());
    }
    
    void PagesModel::OpenPDF(std::wstring pdfPath, SolidFramework::Pdf::AccessPermissions permissionsRequired, SolidFramework::Pdf::AuthenticationModeType authenticationRequired, std::wstring attemptPassword) {
        typedef void (SOLIDCALLBACK *InvokeType)(HANDLE, const wchar_t*, int, int, const wchar_t*);
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_Model_Pdf_Pages_PagesModelBase_OpenPDF__SWIG_2");
        proc(cPtr, pdfPath.c_str(), (int)(permissionsRequired), (int)(authenticationRequired), attemptPassword.c_str());
    }
    
    OperationPtr PagesModel::GetOperation(ModelOperation modelOperation) {
        static InvokeType_HHI proc = GetFunction<InvokeType_HHI>("SolidFramework_Model_Pdf_Pages_PagesModelBase_GetOperation");
        HANDLE newCPtr = proc(cPtr, (int)(modelOperation));
        return SolidFrameworkNative::MakeOperation(newCPtr, true);
    }
    
    std::wstring PagesModel::GetOriginalPath() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Model_Pdf_Pages_PagesModelBase_GetOriginalPath");
        return proc(cPtr);
    }
    
    void PagesModel::SetOriginalPath(std::wstring value) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Model_Pdf_Pages_PagesModelBase_SetOriginalPath");
        proc(cPtr, value.c_str());
    }
    
    HANDLE PagesModel::GetOwnerHandle() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Pdf_Pages_PagesModelBase_OwnerHandle_get");
        return proc(cPtr);
    }
    
    void PagesModel::SetOwnerHandle(HANDLE value) {
        static InvokeType_VHH proc = GetFunction<InvokeType_VHH>("SolidFramework_Model_Pdf_Pages_PagesModelBase_OwnerHandle_set");
        proc(cPtr, value);
    }
    
    int PagesModel::GetPageCount() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Pdf_Pages_PagesModelBase_PageCount_get");
        return proc(cPtr);
    }
    
    IPageHolderPtr PagesModel::GetPageHolder(int index) {
        static InvokeType_HHI proc = GetFunction<InvokeType_HHI>("SolidFramework_Model_Pdf_Pages_PagesModelBase_GetPageHolder");
        HANDLE newCPtr = proc(cPtr, index);
        return SolidFrameworkNative::MakeIPageHolder(newCPtr, true);
    }
    
    SolidFramework::Interop::RectangleFPtr PagesModel::GetPageRectangle(Plumbing::PageLayout pageLayout, int visiblePageIndex, double gutter) {
        typedef HANDLE (SOLIDCALLBACK *InvokeType)(HANDLE, int, int, double);
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_Model_Pdf_Pages_PagesModelBase_GetPageRectangle");
        HANDLE newCPtr = proc(cPtr, (int)(pageLayout), visiblePageIndex, gutter);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::RectangleF>(newCPtr, true);
    }
    
    std::vector<PageSelectionPtr> PagesModel::GetPageSelections() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Pdf_Pages_PagesModelBase_GetPageSelections");
        return SolidFrameworkNative::PageSelectionsCollectionToVector(proc(cPtr), false);
    }
    
    void PagesModel::SetPageSelections(const std::vector<SolidFramework::Model::Pdf::Pages::PageSelectionPtr> & value) {
        static InvokeType_VHH proc = GetFunction<InvokeType_VHH>("SolidFramework_Model_Pdf_Pages_PagesModelBase_SetPageSelections");
        proc(cPtr, SolidFrameworkNative::VectorToPageSelectionsCollection(value)->GetCPtr());
    }
    
    std::wstring PagesModel::GetPopupTitle() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Model_Pdf_Pages_PagesModelBase_GetPopupTitle");
        return proc(cPtr);
    }
    
    void PagesModel::SetPopupTitle(std::wstring value) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Model_Pdf_Pages_PagesModelBase_SetPopupTitle");
        proc(cPtr, value.c_str());
    }
    
    std::wstring PagesModel::GetProducer() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Model_Pdf_Pages_PagesModelBase_GetProducer");
        return proc(cPtr);
    }
    
    void PagesModel::SetProducer(std::wstring value) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Model_Pdf_Pages_PagesModelBase_SetProducer");
        proc(cPtr, value.c_str());
    }
    
    int PagesModel::GetProgressFraction() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Pdf_Pages_PagesModelBase_ProgressFraction_get");
        return proc(cPtr);
    }
    
    void PagesModel::SetProgressFraction(int value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Model_Pdf_Pages_PagesModelBase_ProgressFraction_set");
        proc(cPtr, value);
    }
    
    bool PagesModel::GetReadOnly() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_Pdf_Pages_PagesModelBase_ReadOnly_get");
        return 0 != proc(cPtr);
    }
    
    void PagesModel::SetReadOnly(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Model_Pdf_Pages_PagesModelBase_ReadOnly_set");
        proc(cPtr, value);
    }
    
    Plumbing::ReadOnlyInfoPtr PagesModel::GetReadOnlyInfo() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Pdf_Pages_PagesModelBase_GetReadOnlyInfo");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<Plumbing::ReadOnlyInfo>(newCPtr, true);
    }
    
    void PagesModel::Redo() {
        static InvokeType_VH proc = GetFunction<InvokeType_VH>("SolidFramework_Model_Pdf_Pages_PagesModelBase_Redo");
        proc(cPtr);
    }
    
    std::vector<TransactionPtr> PagesModel::GetRedoStack() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Pdf_Pages_PagesModelBase_GetRedoStack");
        return SolidFrameworkNative::TransactionCollectionToVector(proc(cPtr), false);
    }
    
    SolidFramework::Pdf::Repair::RepairResult PagesModel::GetRepairIssue() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Pdf_Pages_PagesModelBase_RepairIssue_get");
        return (SolidFramework::Pdf::Repair::RepairResult)proc(cPtr);
    }
    
    void PagesModel::SetRepairIssue(SolidFramework::Pdf::Repair::RepairResult value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Model_Pdf_Pages_PagesModelBase_RepairIssue_set");
        proc(cPtr, (int)(value));
    }
    
    std::wstring PagesModel::GetResourceString(std::wstring id) {
        static InvokeType_SHS proc = GetFunction<InvokeType_SHS>("SolidFramework_Model_Pdf_Pages_PagesModelBase_GetResourceString");
        return UnwrapWString(proc(cPtr, id.c_str()));
    }
    
    void PagesModel::Save(std::wstring pdfPath) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Model_Pdf_Pages_PagesModelBase_Save__SWIG_0");
        proc(cPtr, pdfPath.c_str());
    }
    
    bool PagesModel::Save(std::wstring pdfPath, bool selectionOnly) {
        typedef unsigned int (SOLIDCALLBACK *InvokeType)(HANDLE, const wchar_t*, unsigned int);
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_Model_Pdf_Pages_PagesModelBase_Save__SWIG_1");
        return 0 != proc(cPtr, pdfPath.c_str(), selectionOnly);
    }
    
    bool PagesModel::Save(std::wstring pdfPath, bool selectionOnly, bool copyOnly) {
        typedef unsigned int (SOLIDCALLBACK *InvokeType)(HANDLE, const wchar_t*, unsigned int, unsigned int);
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_Model_Pdf_Pages_PagesModelBase_Save__SWIG_2");
        return 0 != proc(cPtr, pdfPath.c_str(), selectionOnly, copyOnly);
    }
    
    bool PagesModel::Save(std::wstring pdfPath, bool selectionOnly, bool copyOnly, bool noOptimize) {
        typedef unsigned int (SOLIDCALLBACK *InvokeType)(HANDLE, const wchar_t*, unsigned int, unsigned int, unsigned int);
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_Model_Pdf_Pages_PagesModelBase_Save__SWIG_3");
        return 0 != proc(cPtr, pdfPath.c_str(), selectionOnly, copyOnly, noOptimize);
    }
    
    bool PagesModel::IsSecurityEditable() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_Pdf_Pages_PagesModelBase_IsSecurityEditable");
        return 0 != proc(cPtr);
    }
    
    SolidFramework::PageRangePtr PagesModel::GetSelectedPageRange() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Pdf_Pages_PagesModelBase_GetSelectedPageRange");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::PageRange>(newCPtr, true);
    }
    
    bool PagesModel::HasSelectedScannedPages() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_Pdf_Pages_PagesModelBase_HasSelectedScannedPages");
        return 0 != proc(cPtr);
    }
    
    std::vector<int> PagesModel::GetSelection() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Pdf_Pages_PagesModelBase_GetSelection");
        return SolidFrameworkNative::PagesArrayToVector(proc(cPtr), false);
    }
    
    void PagesModel::SetSelection(const std::vector<int> & value) {
        static InvokeType_VHH proc = GetFunction<InvokeType_VHH>("SolidFramework_Model_Pdf_Pages_PagesModelBase_SetSelection");
        proc(cPtr, SolidFrameworkNative::VectorToPagesArray(value)->GetCPtr());
    }
    
    SolidFramework::Pdf::PdfDocumentPtr PagesModel::GetSinglePDFSource() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Pdf_Pages_PagesModelBase_GetSinglePDFSource");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Pdf::PdfDocument>(newCPtr, true);
    }
    
    std::vector<IPageSourcePtr> PagesModel::GetSources() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Pdf_Pages_PagesModelBase_GetSources");
        return SolidFrameworkNative::IPageSourcesCollectionToVector(proc(cPtr), false);
    }
    
    int PagesModel::TargetIDtoIndex(int targetID) {
        static InvokeType_IHI proc = GetFunction<InvokeType_IHI>("SolidFramework_Model_Pdf_Pages_PagesModelBase_TargetIDtoIndex");
        return proc(cPtr, targetID);
    }
    
    int PagesModel::GetThumbnailDpi() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Pdf_Pages_PagesModelBase_GetThumbnailDpi");
        return proc(cPtr);
    }
    
    void PagesModel::SetThumbnailDpi(int value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Model_Pdf_Pages_PagesModelBase_SetThumbnailDpi");
        proc(cPtr, value);
    }
    
    void PagesModel::Undo() {
        static InvokeType_VH proc = GetFunction<InvokeType_VH>("SolidFramework_Model_Pdf_Pages_PagesModelBase_Undo");
        proc(cPtr);
    }
    
    bool PagesModel::GetUndoDisabled() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_Pdf_Pages_PagesModelBase_UndoDisabled_get");
        return 0 != proc(cPtr);
    }
    
    void PagesModel::SetUndoDisabled(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Model_Pdf_Pages_PagesModelBase_UndoDisabled_set");
        proc(cPtr, value);
    }
    
    std::vector<TransactionPtr> PagesModel::GetUndoStack() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Pdf_Pages_PagesModelBase_GetUndoStack");
        return SolidFrameworkNative::TransactionCollectionToVector(proc(cPtr), false);
    }
    
    void PagesModel::ValidateIndices(const std::vector<int> & indices) {
        static InvokeType_VHH proc = GetFunction<InvokeType_VHH>("SolidFramework_Model_Pdf_Pages_PagesModelBase_ValidateIndices");
        proc(cPtr, SolidFrameworkNative::VectorToPagesArray(indices)->GetCPtr());
    }
    
    bool PagesModel::HasVisibleNewScannedPages() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_Pdf_Pages_PagesModelBase_HasVisibleNewScannedPages");
        return 0 != proc(cPtr);
    }
    
    std::vector<IPageHolderPtr> PagesModel::GetVisiblePages() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Pdf_Pages_PagesModelBase_GetVisiblePages");
        return SolidFrameworkNative::IPageHoldersCollectionToVector(proc(cPtr), true);
    }
    
    bool PagesModel::HasVisibleScannedPages() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_Pdf_Pages_PagesModelBase_HasVisibleScannedPages");
        return 0 != proc(cPtr);
    }
    
    void PagesModel::FireModelChanged(const ModelChangedEventArgsPtr args) const {
        if (OnModelChanged) {
            OnModelChanged(args);
        }
    }
    
    void PagesModel::FirePagesChanged(const PagesChangedEventArgsPtr args) const {
        if (OnPagesChanged) {
            OnPagesChanged(args);
        }
    }
    
    void PagesModel::FireSaveProgress(const SaveProgressEventArgsPtr args) const {
        if (OnSaveProgress) {
            OnSaveProgress(args);
        }
    }
    
    void PagesModel::FirePasswordEntered(const PasswordEnteredEventArgsPtr args) const {
        if (OnPasswordEntered) {
            OnPasswordEntered(args);
        }
    }
    
    void PagesModel::FireOfferPasswordPrompt(const OfferPasswordEventArgsPtr args) const {
        if (OnOfferPasswordPrompt) {
            OnOfferPasswordPrompt(args);
        }
    }
    
    void PagesModel::FireThumbnailChanged(const ThumbnailChangedEventArgsPtr args) const {
        if (OnThumbnailChanged) {
            OnThumbnailChanged(args);
        }
    }
    
    void PagesModel::FireRepairFailed(const RepairFailedEventArgsPtr args) const {
        if (OnRepairFailed) {
            OnRepairFailed(args);
        }
    }
    

    Parameter::Parameter() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("new_SolidFramework_Model_Pdf_Pages_Parameter");
        HANDLE handle = proc();
        Wrap(handle, true);
    }
    
    void Parameter::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Pdf_Pages_Parameter");
        FrameworkObject::Dispose(destructor);
    }
    
    bool Parameter::IsEqual(ParameterPtr value) {
        static InvokeType_UHH proc = GetFunction<InvokeType_UHH>("SolidFramework_Model_Pdf_Pages_Parameter_IsEqual");
        return 0 != proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(value));
    }
    

    PasswordEnteredEventArgs::PasswordEnteredEventArgs() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("new_SolidFramework_Model_Pdf_Pages_PasswordEnteredEventArgs__SWIG_0");
        HANDLE handle = proc();
        Wrap(handle, true);
    }
    
    PasswordEnteredEventArgs::PasswordEnteredEventArgs(std::wstring password) {
        static InvokeType_HSH proc = GetFunction<InvokeType_HSH>("new_SolidFramework_Model_Pdf_Pages_PasswordEnteredEventArgs__SWIG_1");
        HANDLE handle = proc(password.c_str(), (HANDLE)this);
        Wrap(handle, true);
    }
    
    void PasswordEnteredEventArgs::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Pdf_Pages_PasswordEnteredEventArgs");
        FrameworkObject::Dispose(destructor);
    }
    
    std::wstring PasswordEnteredEventArgs::GetPassword() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Model_Pdf_Pages_PasswordEnteredEventArgs_GetPassword");
        return proc(cPtr);
    }
    
    void PasswordEnteredEventArgs::SetPassword(std::wstring value) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Model_Pdf_Pages_PasswordEnteredEventArgs_SetPassword");
        proc(cPtr, value.c_str());
    }
    

    void PdfPageHolder::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Pdf_Pages_PdfPageHolder");
        FrameworkObject::Dispose(destructor);
    }
    
    Plumbing::CommentPtr PdfPageHolder::AddHighlightComment(const std::vector<SolidFramework::Interop::PointFPtr> & location, SolidFramework::Interop::ColorPtr color, std::wstring title, std::wstring content) {
        static InvokeType_HHHHSS proc = GetFunction<InvokeType_HHHHSS>("SolidFramework_Model_Pdf_Pages_PdfPageHolder_AddHighlightComment");
        HANDLE newCPtr = proc(cPtr, SolidFrameworkNative::VectorToPointFsCollection(location)->GetCPtr(), SolidFramework::FrameworkObject::GetCPtr(color), title.c_str(), content.c_str());
        return SolidFrameworkNative::MakeComment(newCPtr, true);
    }
    
    Plumbing::CommentPtr PdfPageHolder::AddStrikeOutComment(const std::vector<SolidFramework::Interop::PointFPtr> & location, SolidFramework::Interop::ColorPtr color, std::wstring title, std::wstring content) {
        static InvokeType_HHHHSS proc = GetFunction<InvokeType_HHHHSS>("SolidFramework_Model_Pdf_Pages_PdfPageHolder_AddStrikeOutComment");
        HANDLE newCPtr = proc(cPtr, SolidFrameworkNative::VectorToPointFsCollection(location)->GetCPtr(), SolidFramework::FrameworkObject::GetCPtr(color), title.c_str(), content.c_str());
        return SolidFrameworkNative::MakeComment(newCPtr, true);
    }
    
    Plumbing::CommentPtr PdfPageHolder::AddTextComment(const std::vector<SolidFramework::Interop::PointFPtr> & location, SolidFramework::Interop::ColorPtr color, std::wstring title, std::wstring content) {
        static InvokeType_HHHHSS proc = GetFunction<InvokeType_HHHHSS>("SolidFramework_Model_Pdf_Pages_PdfPageHolder_AddTextComment__SWIG_0");
        HANDLE newCPtr = proc(cPtr, SolidFrameworkNative::VectorToPointFsCollection(location)->GetCPtr(), SolidFramework::FrameworkObject::GetCPtr(color), title.c_str(), content.c_str());
        return SolidFrameworkNative::MakeComment(newCPtr, true);
    }
    
    Plumbing::CommentPtr PdfPageHolder::AddTextComment(Plumbing::CommentPtr parent, std::wstring title, std::wstring content) {
        typedef HANDLE (SOLIDCALLBACK *InvokeType)(HANDLE, HANDLE, const wchar_t*, const wchar_t*);
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_Model_Pdf_Pages_PdfPageHolder_AddTextComment__SWIG_1");
        HANDLE newCPtr = proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(parent), title.c_str(), content.c_str());
        return SolidFrameworkNative::MakeComment(newCPtr, true);
    }
    
    Plumbing::CommentPtr PdfPageHolder::AddUnderlineComment(const std::vector<SolidFramework::Interop::PointFPtr> & location, SolidFramework::Interop::ColorPtr color, std::wstring title, std::wstring content) {
        static InvokeType_HHHHSS proc = GetFunction<InvokeType_HHHHSS>("SolidFramework_Model_Pdf_Pages_PdfPageHolder_AddUnderlineComment");
        HANDLE newCPtr = proc(cPtr, SolidFrameworkNative::VectorToPointFsCollection(location)->GetCPtr(), SolidFramework::FrameworkObject::GetCPtr(color), title.c_str(), content.c_str());
        return SolidFrameworkNative::MakeComment(newCPtr, true);
    }
    
    IPageHolderPtr PdfPageHolder::Clone() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Pdf_Pages_PdfPageHolder_Clone");
        HANDLE newCPtr = proc(cPtr);
        return SolidFrameworkNative::MakeIPageHolder(newCPtr, true);
    }
    
    Plumbing::CommentPtr PdfPageHolder::GetComment(int index) {
        static InvokeType_HHI proc = GetFunction<InvokeType_HHI>("SolidFramework_Model_Pdf_Pages_PdfPageHolder_GetComment");
        HANDLE newCPtr = proc(cPtr, index);
        return SolidFrameworkNative::MakeComment(newCPtr, true);
    }
    
    int PdfPageHolder::GetCommentCount() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Pdf_Pages_PdfPageHolder_CommentCount_get");
        return proc(cPtr);
    }
    
    bool PdfPageHolder::DeleteComment(Plumbing::CommentPtr comment) {
        static InvokeType_UHH proc = GetFunction<InvokeType_UHH>("SolidFramework_Model_Pdf_Pages_PdfPageHolder_DeleteComment");
        return 0 != proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(comment));
    }
    
    void PdfPageHolder::GenerateThumbnail() {
        static InvokeType_VH proc = GetFunction<InvokeType_VH>("SolidFramework_Model_Pdf_Pages_PdfPageHolder_GenerateThumbnail");
        proc(cPtr);
    }
    
    Plumbing::LinkPtr PdfPageHolder::GetLink(int index) {
        static InvokeType_HHI proc = GetFunction<InvokeType_HHI>("SolidFramework_Model_Pdf_Pages_PdfPageHolder_GetLink");
        HANDLE newCPtr = proc(cPtr, index);
        return SolidFrameworkNative::MakeLink(newCPtr, true);
    }
    
    int PdfPageHolder::GetLinkCount() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Pdf_Pages_PdfPageHolder_LinkCount_get");
        return proc(cPtr);
    }
    
    bool PdfPageHolder::ModifyComment(Plumbing::CommentPtr comment, const std::vector<SolidFramework::Interop::PointFPtr> & newLocation, SolidFramework::Interop::ColorPtr newColor, std::wstring newTitle, std::wstring newContent) {
        typedef unsigned int (SOLIDCALLBACK *InvokeType)(HANDLE, HANDLE, HANDLE, HANDLE, const wchar_t*, const wchar_t*);
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_Model_Pdf_Pages_PdfPageHolder_ModifyComment");
        return 0 != proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(comment), SolidFrameworkNative::VectorToPointFsCollection(newLocation)->GetCPtr(), SolidFramework::FrameworkObject::GetCPtr(newColor), newTitle.c_str(), newContent.c_str());
    }
    
    SolidFramework::Interop::RectangleFPtr PdfPageHolder::GetPdfVisibleBox() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Pdf_Pages_PdfPageHolder_GetPdfVisibleBox");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::RectangleF>(newCPtr, true);
    }
    
    int PdfPageHolder::GetRotation() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Pdf_Pages_PdfPageHolder_GetRotation");
        return proc(cPtr);
    }
    
    void PdfPageHolder::SetRotation(int value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Model_Pdf_Pages_PdfPageHolder_SetRotation");
        proc(cPtr, value);
    }
    
    PdfPageHolderPtr PdfPageHolder::DynamicCast(IPageHolderPtr value) {
        return std::dynamic_pointer_cast<PdfPageHolder>(value);
    }
    

    void PdfPageSource::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Pdf_Pages_PdfPageSource");
        FrameworkObject::Dispose(destructor);
    }
    
    PdfPageSourcePtr PdfPageSource::CreateSource(std::wstring originalPath, std::wstring tempCopyPath, PagesModelPtr model, std::wstring password, SolidFramework::Pdf::AccessPermissions permissionsRequired) {
        typedef HANDLE (SOLIDCALLBACK *InvokeType)(const wchar_t*, const wchar_t*, HANDLE, const wchar_t*, int);
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_Model_Pdf_Pages_PdfPageSource_CreateSource");
        HANDLE newCPtr = proc(originalPath.c_str(), tempCopyPath.c_str(), SolidFramework::FrameworkObject::GetCPtr(model), password.c_str(), (int)(permissionsRequired));
        return newCPtr == nullptr? nullptr : std::make_shared<PdfPageSource>(newCPtr, true);
    }
    
    std::wstring PdfPageSource::GetPassword() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Model_Pdf_Pages_PdfPageSource_GetPassword");
        return proc(cPtr);
    }
    
    SolidFramework::Pdf::Repair::RepairResult PdfPageSource::GetRepairIssue() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Pdf_Pages_PdfPageSource_RepairIssue_get");
        return (SolidFramework::Pdf::Repair::RepairResult)proc(cPtr);
    }
    
    void PdfPageSource::SetRepairIssue(SolidFramework::Pdf::Repair::RepairResult value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Model_Pdf_Pages_PdfPageSource_RepairIssue_set");
        proc(cPtr, (int)(value));
    }
    
    bool PdfPageSource::WasRepaired() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_Pdf_Pages_PdfPageSource_WasRepaired_get");
        return 0 != proc(cPtr);
    }
    
    void PdfPageSource::SetWasRepaired(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Model_Pdf_Pages_PdfPageSource_WasRepaired_set");
        proc(cPtr, value);
    }
    
    PdfPageSourcePtr PdfPageSource::DynamicCast(IPageSourcePtr value) {
        return std::dynamic_pointer_cast<PdfPageSource>(value);
    }
    

    RepairFailedEventArgs::RepairFailedEventArgs() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("new_SolidFramework_Model_Pdf_Pages_RepairFailedEventArgs__SWIG_1");
        HANDLE handle = proc((HANDLE)this);
        Wrap(handle, true);
    }
    
    void RepairFailedEventArgs::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Pdf_Pages_RepairFailedEventArgs");
        FrameworkObject::Dispose(destructor);
    }
    
    std::wstring RepairFailedEventArgs::GetPath() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Model_Pdf_Pages_RepairFailedEventArgs_GetPath");
        return proc(cPtr);
    }
    
    void RepairFailedEventArgs::SetPath(std::wstring value) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Model_Pdf_Pages_RepairFailedEventArgs_SetPath");
        proc(cPtr, value.c_str());
    }
    
    std::wstring RepairFailedEventArgs::GetReason() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Model_Pdf_Pages_RepairFailedEventArgs_GetReason");
        return proc(cPtr);
    }
    
    void RepairFailedEventArgs::SetReason(std::wstring value) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Model_Pdf_Pages_RepairFailedEventArgs_SetReason");
        proc(cPtr, value.c_str());
    }
    

    SaveProgressEventArgs::SaveProgressEventArgs() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("new_SolidFramework_Model_Pdf_Pages_SaveProgressEventArgs__SWIG_1");
        HANDLE handle = proc((HANDLE)this);
        Wrap(handle, true);
    }
    
    void SaveProgressEventArgs::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Pdf_Pages_SaveProgressEventArgs");
        FrameworkObject::Dispose(destructor);
    }
    
    int SaveProgressEventArgs::GetMaximum() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Pdf_Pages_SaveProgressEventArgs_Maximum_get");
        return proc(cPtr);
    }
    
    void SaveProgressEventArgs::SetMaximum(int value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Model_Pdf_Pages_SaveProgressEventArgs_Maximum_set");
        proc(cPtr, value);
    }
    
    ProgressPhase SaveProgressEventArgs::GetPhase() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Pdf_Pages_SaveProgressEventArgs_Phase_get");
        return (ProgressPhase)proc(cPtr);
    }
    
    void SaveProgressEventArgs::SetPhase(ProgressPhase value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Model_Pdf_Pages_SaveProgressEventArgs_Phase_set");
        proc(cPtr, (int)(value));
    }
    
    int SaveProgressEventArgs::GetTick() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Pdf_Pages_SaveProgressEventArgs_Tick_get");
        return proc(cPtr);
    }
    
    void SaveProgressEventArgs::SetTick(int value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Model_Pdf_Pages_SaveProgressEventArgs_Tick_set");
        proc(cPtr, value);
    }
    

    ThumbnailChangedEventArgs::ThumbnailChangedEventArgs() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("new_SolidFramework_Model_Pdf_Pages_ThumbnailChangedEventArgs__SWIG_1");
        HANDLE handle = proc((HANDLE)this);
        Wrap(handle, true);
    }
    
    void ThumbnailChangedEventArgs::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Pdf_Pages_ThumbnailChangedEventArgs");
        FrameworkObject::Dispose(destructor);
    }
    
    bool ThumbnailChangedEventArgs::GetInvokeRequired() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_Pdf_Pages_ThumbnailChangedEventArgs_InvokeRequired_get");
        return 0 != proc(cPtr);
    }
    
    void ThumbnailChangedEventArgs::SetInvokeRequired(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Model_Pdf_Pages_ThumbnailChangedEventArgs_InvokeRequired_set");
        proc(cPtr, value);
    }
    
    IPageHolderPtr ThumbnailChangedEventArgs::GetPageHolder() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Pdf_Pages_ThumbnailChangedEventArgs_GetPageHolder");
        HANDLE newCPtr = proc(cPtr);
        return SolidFrameworkNative::MakeIPageHolder(newCPtr, true);
    }
    
    void ThumbnailChangedEventArgs::SetPageHolder(IPageHolderPtr value) {
        static InvokeType_VHH proc = GetFunction<InvokeType_VHH>("SolidFramework_Model_Pdf_Pages_ThumbnailChangedEventArgs_SetPageHolder");
        proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(value));
    }
    

    void Transaction::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Pdf_Pages_Transaction");
        FrameworkObject::Dispose(destructor);
    }
    
    OperationPtr Transaction::GetOperation() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Pdf_Pages_Transaction_GetOperation");
        HANDLE newCPtr = proc(cPtr);
        return SolidFrameworkNative::MakeOperation(newCPtr, true);
    }
    
    void Transaction::SetOperation(OperationPtr value) {
        static InvokeType_VHH proc = GetFunction<InvokeType_VHH>("SolidFramework_Model_Pdf_Pages_Transaction_SetOperation");
        proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(value));
    }
    
    ParameterPtr Transaction::GetParameter() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Pdf_Pages_Transaction_GetParameter");
        HANDLE newCPtr = proc(cPtr);
        return SolidFrameworkNative::MakeParameter(newCPtr, true);
    }
    
    void Transaction::SetParameter(ParameterPtr value) {
        static InvokeType_VHH proc = GetFunction<InvokeType_VHH>("SolidFramework_Model_Pdf_Pages_Transaction_SetParameter");
        proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(value));
    }
    

}}}} // SolidFramework::Model::Pdf::Pages

namespace SolidFramework { namespace Model { namespace Pdf { namespace Pages { namespace Operations { 
    void AddComment::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Pdf_Pages_Operations_AddComment");
        FrameworkObject::Dispose(destructor);
    }
    
    bool AddComment::GetEnabled() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_Pdf_Pages_Operations_AddComment_GetEnabled");
        return 0 != proc(cPtr);
    }
    
    bool AddComment::Execute(SolidFramework::Model::Pdf::Pages::ParameterPtr parameter) {
        static InvokeType_UHH proc = GetFunction<InvokeType_UHH>("SolidFramework_Model_Pdf_Pages_Operations_AddComment_Execute");
        return 0 != proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(parameter));
    }
    
    SolidFramework::Model::Pdf::Pages::ModelOperation AddComment::GetModelOperation() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Pdf_Pages_Operations_AddComment_GetModelOperation");
        return (SolidFramework::Model::Pdf::Pages::ModelOperation)proc(cPtr);
    }
    
    std::wstring AddComment::GetName() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Model_Pdf_Pages_Operations_AddComment_GetName");
        return UnwrapWString(proc(cPtr));
    }
    
    AddCommentPtr AddComment::DynamicCast(SolidFramework::Model::Pdf::Pages::OperationPtr value) {
        return std::dynamic_pointer_cast<AddComment>(value);
    }
    

    AddCommentParameter::AddCommentParameter(SolidFramework::Model::Pdf::Pages::IPageHolderPtr page, SolidFramework::Model::Pdf::Pages::Plumbing::AnnotationType type, const std::vector<SolidFramework::Interop::PointFPtr> & position, SolidFramework::Interop::ColorPtr color, std::wstring title, std::wstring content) {
        typedef HANDLE (SOLIDCALLBACK *InvokeType)(HANDLE, int, HANDLE, HANDLE, const wchar_t*, const wchar_t*);
        static InvokeType proc = GetFunction<InvokeType>("new_SolidFramework_Model_Pdf_Pages_Operations_AddCommentParameter__SWIG_0");
        HANDLE handle = proc(SolidFramework::FrameworkObject::GetCPtr(page), (int)(type), SolidFrameworkNative::VectorToPointFsCollection(position)->GetCPtr(), SolidFramework::FrameworkObject::GetCPtr(color), title.c_str(), content.c_str());
        Wrap(handle, true);
    }
    
    AddCommentParameter::AddCommentParameter(SolidFramework::Model::Pdf::Pages::Plumbing::CommentPtr parentComment, std::wstring title, std::wstring content) {
        static InvokeType_HHSS proc = GetFunction<InvokeType_HHSS>("new_SolidFramework_Model_Pdf_Pages_Operations_AddCommentParameter__SWIG_1");
        HANDLE handle = proc(SolidFramework::FrameworkObject::GetCPtr(parentComment), title.c_str(), content.c_str());
        Wrap(handle, true);
    }
    
    void AddCommentParameter::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Pdf_Pages_Operations_AddCommentParameter");
        FrameworkObject::Dispose(destructor);
    }
    
    SolidFramework::Interop::ColorPtr AddCommentParameter::GetColor() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Pdf_Pages_Operations_AddCommentParameter_GetColor");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::Color>(newCPtr, true);
    }
    
    std::wstring AddCommentParameter::GetContent() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Model_Pdf_Pages_Operations_AddCommentParameter_GetContent");
        return UnwrapWString(proc(cPtr));
    }
    
    SolidFramework::Model::Pdf::Pages::IPageHolderPtr AddCommentParameter::GetIPageHolder() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Pdf_Pages_Operations_AddCommentParameter_GetIPageHolder");
        HANDLE newCPtr = proc(cPtr);
        return SolidFrameworkNative::MakeIPageHolder(newCPtr, true);
    }
    
    std::vector<SolidFramework::Interop::PointFPtr> AddCommentParameter::GetPosition() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Pdf_Pages_Operations_AddCommentParameter_GetPosition");
        return SolidFrameworkNative::PointFsCollectionToVector(proc(cPtr), true);
    }
    
    std::wstring AddCommentParameter::GetTitle() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Model_Pdf_Pages_Operations_AddCommentParameter_GetTitle");
        return UnwrapWString(proc(cPtr));
    }
    
    SolidFramework::Model::Pdf::Pages::Plumbing::AnnotationType AddCommentParameter::GetType() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Pdf_Pages_Operations_AddCommentParameter_Type_get");
        return (SolidFramework::Model::Pdf::Pages::Plumbing::AnnotationType)proc(cPtr);
    }
    
    AddCommentParameterPtr AddCommentParameter::DynamicCast(SolidFramework::Model::Pdf::Pages::ParameterPtr value) {
        return std::dynamic_pointer_cast<AddCommentParameter>(value);
    }
    

    void Copy::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Pdf_Pages_Operations_Copy");
        FrameworkObject::Dispose(destructor);
    }
    
    bool Copy::GetEnabled() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_Pdf_Pages_Operations_Copy_GetEnabled");
        return 0 != proc(cPtr);
    }
    
    bool Copy::Execute(SolidFramework::Model::Pdf::Pages::ParameterPtr parameter) {
        static InvokeType_UHH proc = GetFunction<InvokeType_UHH>("SolidFramework_Model_Pdf_Pages_Operations_Copy_Execute");
        return 0 != proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(parameter));
    }
    
    SolidFramework::Model::Pdf::Pages::ModelOperation Copy::GetModelOperation() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Pdf_Pages_Operations_Copy_GetModelOperation");
        return (SolidFramework::Model::Pdf::Pages::ModelOperation)proc(cPtr);
    }
    
    std::wstring Copy::GetName() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Model_Pdf_Pages_Operations_Copy_GetName");
        return UnwrapWString(proc(cPtr));
    }
    
    CopyPtr Copy::DynamicCast(SolidFramework::Model::Pdf::Pages::OperationPtr value) {
        return std::dynamic_pointer_cast<Copy>(value);
    }
    

    void Delete::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Pdf_Pages_Operations_Delete");
        FrameworkObject::Dispose(destructor);
    }
    
    bool Delete::GetEnabled() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_Pdf_Pages_Operations_Delete_GetEnabled");
        return 0 != proc(cPtr);
    }
    
    bool Delete::Execute() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_Pdf_Pages_Operations_Delete_Execute__SWIG_0");
        return 0 != proc(cPtr);
    }
    
    bool Delete::Execute(SolidFramework::Model::Pdf::Pages::ParameterPtr parameter) {
        static InvokeType_UHH proc = GetFunction<InvokeType_UHH>("SolidFramework_Model_Pdf_Pages_Operations_Delete_Execute__SWIG_1");
        return 0 != proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(parameter));
    }
    
    SolidFramework::Model::Pdf::Pages::ModelOperation Delete::GetModelOperation() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Pdf_Pages_Operations_Delete_GetModelOperation");
        return (SolidFramework::Model::Pdf::Pages::ModelOperation)proc(cPtr);
    }
    
    std::wstring Delete::GetName() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Model_Pdf_Pages_Operations_Delete_GetName");
        return UnwrapWString(proc(cPtr));
    }
    
    DeletePtr Delete::DynamicCast(SolidFramework::Model::Pdf::Pages::OperationPtr value) {
        return std::dynamic_pointer_cast<Delete>(value);
    }
    

    void DeleteComment::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Pdf_Pages_Operations_DeleteComment");
        FrameworkObject::Dispose(destructor);
    }
    
    bool DeleteComment::GetEnabled() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_Pdf_Pages_Operations_DeleteComment_GetEnabled");
        return 0 != proc(cPtr);
    }
    
    bool DeleteComment::Execute(SolidFramework::Model::Pdf::Pages::ParameterPtr parameter) {
        static InvokeType_UHH proc = GetFunction<InvokeType_UHH>("SolidFramework_Model_Pdf_Pages_Operations_DeleteComment_Execute");
        return 0 != proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(parameter));
    }
    
    SolidFramework::Model::Pdf::Pages::ModelOperation DeleteComment::GetModelOperation() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Pdf_Pages_Operations_DeleteComment_GetModelOperation");
        return (SolidFramework::Model::Pdf::Pages::ModelOperation)proc(cPtr);
    }
    
    std::wstring DeleteComment::GetName() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Model_Pdf_Pages_Operations_DeleteComment_GetName");
        return UnwrapWString(proc(cPtr));
    }
    
    DeleteCommentPtr DeleteComment::DynamicCast(SolidFramework::Model::Pdf::Pages::OperationPtr value) {
        return std::dynamic_pointer_cast<DeleteComment>(value);
    }
    

    DeleteCommentParameter::DeleteCommentParameter(SolidFramework::Model::Pdf::Pages::Plumbing::CommentPtr comment) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("new_SolidFramework_Model_Pdf_Pages_Operations_DeleteCommentParameter");
        HANDLE handle = proc(SolidFramework::FrameworkObject::GetCPtr(comment));
        Wrap(handle, true);
    }
    
    void DeleteCommentParameter::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Pdf_Pages_Operations_DeleteCommentParameter");
        FrameworkObject::Dispose(destructor);
    }
    
    SolidFramework::Model::Pdf::Pages::Plumbing::CommentPtr DeleteCommentParameter::GetDeletedComment() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Pdf_Pages_Operations_DeleteCommentParameter_GetDeletedComment");
        HANDLE newCPtr = proc(cPtr);
        return SolidFrameworkNative::MakeComment(newCPtr, true);
    }
    
    DeleteCommentParameterPtr DeleteCommentParameter::DynamicCast(SolidFramework::Model::Pdf::Pages::ParameterPtr value) {
        return std::dynamic_pointer_cast<DeleteCommentParameter>(value);
    }
    

    void Insert::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Pdf_Pages_Operations_Insert");
        FrameworkObject::Dispose(destructor);
    }
    
    bool Insert::GetEnabled() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_Pdf_Pages_Operations_Insert_GetEnabled");
        return 0 != proc(cPtr);
    }
    
    bool Insert::Execute(SolidFramework::Model::Pdf::Pages::ParameterPtr parameter) {
        static InvokeType_UHH proc = GetFunction<InvokeType_UHH>("SolidFramework_Model_Pdf_Pages_Operations_Insert_Execute");
        return 0 != proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(parameter));
    }
    
    SolidFramework::Model::Pdf::Pages::ModelOperation Insert::GetModelOperation() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Pdf_Pages_Operations_Insert_GetModelOperation");
        return (SolidFramework::Model::Pdf::Pages::ModelOperation)proc(cPtr);
    }
    
    std::wstring Insert::GetName() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Model_Pdf_Pages_Operations_Insert_GetName");
        return UnwrapWString(proc(cPtr));
    }
    
    InsertPtr Insert::DynamicCast(SolidFramework::Model::Pdf::Pages::OperationPtr value) {
        return std::dynamic_pointer_cast<Insert>(value);
    }
    

    InsertParameter::InsertParameter(int position, SolidFramework::Model::Pdf::Pages::IPageSourcePtr source) {
        static InvokeType_HIH proc = GetFunction<InvokeType_HIH>("new_SolidFramework_Model_Pdf_Pages_Operations_InsertParameter__SWIG_0");
        HANDLE handle = proc(position, SolidFramework::FrameworkObject::GetCPtr(source));
        Wrap(handle, true);
    }
    
    InsertParameter::InsertParameter(int position, const std::vector<SolidFramework::Model::Pdf::Pages::IPageSourcePtr> & sources) {
        static InvokeType_HIH proc = GetFunction<InvokeType_HIH>("new_SolidFramework_Model_Pdf_Pages_Operations_InsertParameter__SWIG_1");
        HANDLE handle = proc(position, SolidFrameworkNative::VectorToIPageSourcesCollection(sources)->GetCPtr());
        Wrap(handle, true);
    }
    
    void InsertParameter::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Pdf_Pages_Operations_InsertParameter");
        FrameworkObject::Dispose(destructor);
    }
    
    int InsertParameter::GetPosition() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Pdf_Pages_Operations_InsertParameter_Position_get");
        return proc(cPtr);
    }
    
    InsertParameterPtr InsertParameter::DynamicCast(SolidFramework::Model::Pdf::Pages::ParameterPtr value) {
        return std::dynamic_pointer_cast<InsertParameter>(value);
    }
    

    void ModifyComment::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Pdf_Pages_Operations_ModifyComment");
        FrameworkObject::Dispose(destructor);
    }
    
    bool ModifyComment::GetEnabled() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_Pdf_Pages_Operations_ModifyComment_GetEnabled");
        return 0 != proc(cPtr);
    }
    
    bool ModifyComment::Execute(SolidFramework::Model::Pdf::Pages::ParameterPtr parameter) {
        static InvokeType_UHH proc = GetFunction<InvokeType_UHH>("SolidFramework_Model_Pdf_Pages_Operations_ModifyComment_Execute");
        return 0 != proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(parameter));
    }
    
    SolidFramework::Model::Pdf::Pages::ModelOperation ModifyComment::GetModelOperation() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Pdf_Pages_Operations_ModifyComment_GetModelOperation");
        return (SolidFramework::Model::Pdf::Pages::ModelOperation)proc(cPtr);
    }
    
    std::wstring ModifyComment::GetName() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Model_Pdf_Pages_Operations_ModifyComment_GetName");
        return UnwrapWString(proc(cPtr));
    }
    
    ModifyCommentPtr ModifyComment::DynamicCast(SolidFramework::Model::Pdf::Pages::OperationPtr value) {
        return std::dynamic_pointer_cast<ModifyComment>(value);
    }
    

    ModifyCommentParameter::ModifyCommentParameter(SolidFramework::Model::Pdf::Pages::Plumbing::CommentPtr modifiedComment, const std::vector<SolidFramework::Interop::PointFPtr> & position, SolidFramework::Interop::ColorPtr color, std::wstring title, std::wstring content) {
        static InvokeType_HHHHSS proc = GetFunction<InvokeType_HHHHSS>("new_SolidFramework_Model_Pdf_Pages_Operations_ModifyCommentParameter__SWIG_0");
        HANDLE handle = proc(SolidFramework::FrameworkObject::GetCPtr(modifiedComment), SolidFrameworkNative::VectorToPointFsCollection(position)->GetCPtr(), SolidFramework::FrameworkObject::GetCPtr(color), title.c_str(), content.c_str());
        Wrap(handle, true);
    }
    
    ModifyCommentParameter::ModifyCommentParameter(SolidFramework::Model::Pdf::Pages::Plumbing::CommentPtr modifiedComment, const std::vector<SolidFramework::Interop::PointFPtr> & position) {
        static InvokeType_HHH proc = GetFunction<InvokeType_HHH>("new_SolidFramework_Model_Pdf_Pages_Operations_ModifyCommentParameter__SWIG_1");
        HANDLE handle = proc(SolidFramework::FrameworkObject::GetCPtr(modifiedComment), SolidFrameworkNative::VectorToPointFsCollection(position)->GetCPtr());
        Wrap(handle, true);
    }
    
    ModifyCommentParameter::ModifyCommentParameter(SolidFramework::Model::Pdf::Pages::Plumbing::CommentPtr modifiedComment, SolidFramework::Interop::ColorPtr color) {
        static InvokeType_HHH proc = GetFunction<InvokeType_HHH>("new_SolidFramework_Model_Pdf_Pages_Operations_ModifyCommentParameter__SWIG_2");
        HANDLE handle = proc(SolidFramework::FrameworkObject::GetCPtr(modifiedComment), SolidFramework::FrameworkObject::GetCPtr(color));
        Wrap(handle, true);
    }
    
    ModifyCommentParameter::ModifyCommentParameter(SolidFramework::Model::Pdf::Pages::Plumbing::CommentPtr modifiedComment, std::wstring title, std::wstring content) {
        static InvokeType_HHSS proc = GetFunction<InvokeType_HHSS>("new_SolidFramework_Model_Pdf_Pages_Operations_ModifyCommentParameter__SWIG_3");
        HANDLE handle = proc(SolidFramework::FrameworkObject::GetCPtr(modifiedComment), title.c_str(), content.c_str());
        Wrap(handle, true);
    }
    
    void ModifyCommentParameter::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Pdf_Pages_Operations_ModifyCommentParameter");
        FrameworkObject::Dispose(destructor);
    }
    
    SolidFramework::Interop::ColorPtr ModifyCommentParameter::GetColor() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Pdf_Pages_Operations_ModifyCommentParameter_GetColor");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::Color>(newCPtr, true);
    }
    
    std::wstring ModifyCommentParameter::GetContent() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Model_Pdf_Pages_Operations_ModifyCommentParameter_GetContent");
        return UnwrapWString(proc(cPtr));
    }
    
    SolidFramework::Model::Pdf::Pages::Plumbing::CommentPtr ModifyCommentParameter::GetModifiedComment() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Pdf_Pages_Operations_ModifyCommentParameter_GetModifiedComment");
        HANDLE newCPtr = proc(cPtr);
        return SolidFrameworkNative::MakeComment(newCPtr, true);
    }
    
    std::vector<SolidFramework::Interop::PointFPtr> ModifyCommentParameter::GetPosition() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Pdf_Pages_Operations_ModifyCommentParameter_GetPosition");
        return SolidFrameworkNative::PointFsCollectionToVector(proc(cPtr), true);
    }
    
    std::wstring ModifyCommentParameter::GetTitle() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Model_Pdf_Pages_Operations_ModifyCommentParameter_GetTitle");
        return UnwrapWString(proc(cPtr));
    }
    
    ModifyCommentParameterPtr ModifyCommentParameter::DynamicCast(SolidFramework::Model::Pdf::Pages::ParameterPtr value) {
        return std::dynamic_pointer_cast<ModifyCommentParameter>(value);
    }
    

    void Move::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Pdf_Pages_Operations_Move");
        FrameworkObject::Dispose(destructor);
    }
    
    bool Move::GetEnabled() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_Pdf_Pages_Operations_Move_GetEnabled");
        return 0 != proc(cPtr);
    }
    
    bool Move::Execute(SolidFramework::Model::Pdf::Pages::ParameterPtr parameter) {
        static InvokeType_UHH proc = GetFunction<InvokeType_UHH>("SolidFramework_Model_Pdf_Pages_Operations_Move_Execute");
        return 0 != proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(parameter));
    }
    
    SolidFramework::Model::Pdf::Pages::ModelOperation Move::GetModelOperation() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Pdf_Pages_Operations_Move_GetModelOperation");
        return (SolidFramework::Model::Pdf::Pages::ModelOperation)proc(cPtr);
    }
    
    std::wstring Move::GetName() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Model_Pdf_Pages_Operations_Move_GetName");
        return UnwrapWString(proc(cPtr));
    }
    
    MovePtr Move::DynamicCast(SolidFramework::Model::Pdf::Pages::OperationPtr value) {
        return std::dynamic_pointer_cast<Move>(value);
    }
    

    void Redo::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Pdf_Pages_Operations_Redo");
        FrameworkObject::Dispose(destructor);
    }
    
    bool Redo::GetEnabled() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_Pdf_Pages_Operations_Redo_GetEnabled");
        return 0 != proc(cPtr);
    }
    
    bool Redo::Execute() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_Pdf_Pages_Operations_Redo_Execute");
        return 0 != proc(cPtr);
    }
    
    SolidFramework::Model::Pdf::Pages::ModelOperation Redo::GetModelOperation() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Pdf_Pages_Operations_Redo_GetModelOperation");
        return (SolidFramework::Model::Pdf::Pages::ModelOperation)proc(cPtr);
    }
    
    std::wstring Redo::GetName() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Model_Pdf_Pages_Operations_Redo_GetName");
        return UnwrapWString(proc(cPtr));
    }
    
    std::wstring Redo::ToString() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Model_Pdf_Pages_Operations_Redo_ToString");
        return UnwrapWString(proc(cPtr));
    }
    
    RedoPtr Redo::DynamicCast(SolidFramework::Model::Pdf::Pages::OperationPtr value) {
        return std::dynamic_pointer_cast<Redo>(value);
    }
    

    void Rotate::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Pdf_Pages_Operations_Rotate");
        FrameworkObject::Dispose(destructor);
    }
    
    bool Rotate::GetEnabled() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_Pdf_Pages_Operations_Rotate_GetEnabled");
        return 0 != proc(cPtr);
    }
    
    bool Rotate::Execute(SolidFramework::Model::Pdf::Pages::ParameterPtr parameter) {
        static InvokeType_UHH proc = GetFunction<InvokeType_UHH>("SolidFramework_Model_Pdf_Pages_Operations_Rotate_Execute");
        return 0 != proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(parameter));
    }
    
    SolidFramework::Model::Pdf::Pages::ModelOperation Rotate::GetModelOperation() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Pdf_Pages_Operations_Rotate_GetModelOperation");
        return (SolidFramework::Model::Pdf::Pages::ModelOperation)proc(cPtr);
    }
    
    std::wstring Rotate::GetName() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Model_Pdf_Pages_Operations_Rotate_GetName");
        return UnwrapWString(proc(cPtr));
    }
    
    RotatePtr Rotate::DynamicCast(SolidFramework::Model::Pdf::Pages::OperationPtr value) {
        return std::dynamic_pointer_cast<Rotate>(value);
    }
    

    void Undo::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Pdf_Pages_Operations_Undo");
        FrameworkObject::Dispose(destructor);
    }
    
    bool Undo::GetEnabled() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_Pdf_Pages_Operations_Undo_GetEnabled");
        return 0 != proc(cPtr);
    }
    
    bool Undo::Execute() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_Pdf_Pages_Operations_Undo_Execute");
        return 0 != proc(cPtr);
    }
    
    SolidFramework::Model::Pdf::Pages::ModelOperation Undo::GetModelOperation() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Pdf_Pages_Operations_Undo_GetModelOperation");
        return (SolidFramework::Model::Pdf::Pages::ModelOperation)proc(cPtr);
    }
    
    std::wstring Undo::GetName() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Model_Pdf_Pages_Operations_Undo_GetName");
        return UnwrapWString(proc(cPtr));
    }
    
    std::wstring Undo::ToString() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Model_Pdf_Pages_Operations_Undo_ToString");
        return UnwrapWString(proc(cPtr));
    }
    
    UndoPtr Undo::DynamicCast(SolidFramework::Model::Pdf::Pages::OperationPtr value) {
        return std::dynamic_pointer_cast<Undo>(value);
    }
    

}}}}} // SolidFramework::Model::Pdf::Pages::Operations

namespace SolidFramework { namespace Model { namespace Pdf { namespace Pages { namespace Plumbing { 
    void Annotation::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Pdf_Pages_Plumbing_Annotation");
        FrameworkObject::Dispose(destructor);
    }
    
    bool Annotation::IsActive() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_Pdf_Pages_Plumbing_Annotation_IsActive");
        return 0 != proc(cPtr);
    }
    
    BorderStyle Annotation::GetBorderStyle() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Pdf_Pages_Plumbing_Annotation_BorderStyle_get");
        return (BorderStyle)proc(cPtr);
    }
    
    SolidFramework::Interop::RectangleFPtr Annotation::GetCanvasRectangle(SolidFramework::Interop::RectangleFPtr pageCanvasRectangle) {
        static InvokeType_HHH proc = GetFunction<InvokeType_HHH>("SolidFramework_Model_Pdf_Pages_Plumbing_Annotation_GetCanvasRectangle");
        HANDLE newCPtr = proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(pageCanvasRectangle));
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::RectangleF>(newCPtr, true);
    }
    
    SolidFramework::Interop::ColorPtr Annotation::GetColor() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Pdf_Pages_Plumbing_Annotation_GetColor");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::Color>(newCPtr, true);
    }
    
    void Annotation::Draw(HANDLE hdc, SolidFramework::Interop::RectangleFPtr screenRectangle) {
        typedef void (SOLIDCALLBACK *InvokeType)(HANDLE, HANDLE, HANDLE);
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_Model_Pdf_Pages_Plumbing_Annotation_Draw");
        proc(cPtr, hdc, SolidFramework::FrameworkObject::GetCPtr(screenRectangle));
    }
    
    SolidFramework::Model::Pdf::Pages::IPageHolderPtr Annotation::GetIPageHolder() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Pdf_Pages_Plumbing_Annotation_GetIPageHolder");
        HANDLE newCPtr = proc(cPtr);
        return SolidFrameworkNative::MakeIPageHolder(newCPtr, true);
    }
    
    double Annotation::GetLineWidth() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Model_Pdf_Pages_Plumbing_Annotation_LineWidth_get");
        return proc(cPtr);
    }
    
    SolidFramework::Interop::DateTimePtr Annotation::GetModificationDate() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Pdf_Pages_Plumbing_Annotation_GetModificationDate");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::DateTime>(newCPtr, true);
    }
    
    std::wstring Annotation::GetText() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Model_Pdf_Pages_Plumbing_Annotation_GetText");
        return proc(cPtr);
    }
    
    std::wstring Annotation::ToString() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Model_Pdf_Pages_Plumbing_Annotation_ToString");
        return UnwrapWString(proc(cPtr));
    }
    
    Annotation::Annotation() {
        // only called from derived class constructors
    }
    

    void Comment::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Pdf_Pages_Plumbing_Comment");
        FrameworkObject::Dispose(destructor);
    }
    
    std::vector<AnnotationPtr> Comment::GetChildren() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Pdf_Pages_Plumbing_Comment_GetChildren");
        return SolidFrameworkNative::AnnotationsCollectionToVector(proc(cPtr), false);
    }
    
    int Comment::GetCount() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Pdf_Pages_Plumbing_Comment_Count_get");
        return proc(cPtr);
    }
    
    SolidFramework::Interop::DateTimePtr Comment::GetCreationDate() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Pdf_Pages_Plumbing_Comment_GetCreationDate");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::DateTime>(newCPtr, true);
    }
    
    std::wstring Comment::GetLabel() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Model_Pdf_Pages_Plumbing_Comment_GetLabel");
        return proc(cPtr);
    }
    
    std::vector<SolidFramework::Interop::PointFPtr> Comment::GetLocation() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Pdf_Pages_Plumbing_Comment_GetLocation");
        return SolidFrameworkNative::PointFsCollectionToVector(proc(cPtr), true);
    }
    
    CommentPtr Comment::DynamicCast(AnnotationPtr value) {
        return std::dynamic_pointer_cast<Comment>(value);
    }
    
    Comment::Comment() {
        // only called from derived class constructors
    }
    

    Description::Description() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("new_SolidFramework_Model_Pdf_Pages_Plumbing_Description");
        HANDLE handle = proc();
        Wrap(handle, true);
    }
    
    void Description::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Pdf_Pages_Plumbing_Description");
        FrameworkObject::Dispose(destructor);
    }
    
    std::wstring Description::GetAuthor() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Model_Pdf_Pages_Plumbing_Description_GetAuthor");
        return proc(cPtr);
    }
    
    void Description::SetAuthor(std::wstring value) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Model_Pdf_Pages_Plumbing_Description_SetAuthor");
        proc(cPtr, value.c_str());
    }
    
    std::wstring Description::GetKeywords() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Model_Pdf_Pages_Plumbing_Description_GetKeywords");
        return proc(cPtr);
    }
    
    void Description::SetKeywords(std::wstring value) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Model_Pdf_Pages_Plumbing_Description_SetKeywords");
        proc(cPtr, value.c_str());
    }
    
    bool Description::GetModified() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_Pdf_Pages_Plumbing_Description_Modified_get");
        return 0 != proc(cPtr);
    }
    
    void Description::SetModified(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Model_Pdf_Pages_Plumbing_Description_Modified_set");
        proc(cPtr, value);
    }
    
    std::wstring Description::GetSubject() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Model_Pdf_Pages_Plumbing_Description_GetSubject");
        return proc(cPtr);
    }
    
    void Description::SetSubject(std::wstring value) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Model_Pdf_Pages_Plumbing_Description_SetSubject");
        proc(cPtr, value.c_str());
    }
    
    std::wstring Description::GetTitle() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Model_Pdf_Pages_Plumbing_Description_GetTitle");
        return proc(cPtr);
    }
    
    void Description::SetTitle(std::wstring value) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Model_Pdf_Pages_Plumbing_Description_SetTitle");
        proc(cPtr, value.c_str());
    }
    

    GoToLink::GoToLink() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("new_SolidFramework_Model_Pdf_Pages_Plumbing_GoToLink");
        HANDLE handle = proc();
        Wrap(handle, true);
    }
    
    void GoToLink::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Pdf_Pages_Plumbing_GoToLink");
        FrameworkObject::Dispose(destructor);
    }
    
    int GoToLink::GetIndex() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Pdf_Pages_Plumbing_GoToLink_Index_get");
        return proc(cPtr);
    }
    
    GoToLinkPtr GoToLink::DynamicCast(AnnotationPtr value) {
        return std::dynamic_pointer_cast<GoToLink>(value);
    }
    

    HighlightComment::HighlightComment() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("new_SolidFramework_Model_Pdf_Pages_Plumbing_HighlightComment");
        HANDLE handle = proc();
        Wrap(handle, true);
    }
    
    void HighlightComment::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Pdf_Pages_Plumbing_HighlightComment");
        FrameworkObject::Dispose(destructor);
    }
    
    std::wstring HighlightComment::ToString() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Model_Pdf_Pages_Plumbing_HighlightComment_ToString");
        return UnwrapWString(proc(cPtr));
    }
    
    HighlightCommentPtr HighlightComment::DynamicCast(AnnotationPtr value) {
        return std::dynamic_pointer_cast<HighlightComment>(value);
    }
    

    void InitialView::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Pdf_Pages_Plumbing_InitialView");
        FrameworkObject::Dispose(destructor);
    }
    
    bool InitialView::GetCenterWindowOnScreen() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_Pdf_Pages_Plumbing_InitialView_CenterWindowOnScreen_get");
        return 0 != proc(cPtr);
    }
    
    void InitialView::SetCenterWindowOnScreen(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Model_Pdf_Pages_Plumbing_InitialView_CenterWindowOnScreen_set");
        proc(cPtr, value);
    }
    
    bool InitialView::GetDisplayDocumentTitle() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_Pdf_Pages_Plumbing_InitialView_DisplayDocumentTitle_get");
        return 0 != proc(cPtr);
    }
    
    void InitialView::SetDisplayDocumentTitle(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Model_Pdf_Pages_Plumbing_InitialView_DisplayDocumentTitle_set");
        proc(cPtr, value);
    }
    
    bool InitialView::GetHideMenuBar() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_Pdf_Pages_Plumbing_InitialView_HideMenuBar_get");
        return 0 != proc(cPtr);
    }
    
    void InitialView::SetHideMenuBar(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Model_Pdf_Pages_Plumbing_InitialView_HideMenuBar_set");
        proc(cPtr, value);
    }
    
    bool InitialView::GetHideToolBars() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_Pdf_Pages_Plumbing_InitialView_HideToolBars_get");
        return 0 != proc(cPtr);
    }
    
    void InitialView::SetHideToolBars(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Model_Pdf_Pages_Plumbing_InitialView_HideToolBars_set");
        proc(cPtr, value);
    }
    
    bool InitialView::GetHideWindowControls() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_Pdf_Pages_Plumbing_InitialView_HideWindowControls_get");
        return 0 != proc(cPtr);
    }
    
    void InitialView::SetHideWindowControls(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Model_Pdf_Pages_Plumbing_InitialView_HideWindowControls_set");
        proc(cPtr, value);
    }
    
    PageLayout InitialView::GetInitialPageLayout() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Pdf_Pages_Plumbing_InitialView_InitialPageLayout_get");
        return (PageLayout)proc(cPtr);
    }
    
    void InitialView::SetInitialPageLayout(PageLayout value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Model_Pdf_Pages_Plumbing_InitialView_InitialPageLayout_set");
        proc(cPtr, (int)(value));
    }
    
    bool InitialView::IsDefault(SolidFramework::Model::Pdf::Pages::PagesModelPtr model) {
        static InvokeType_UHH proc = GetFunction<InvokeType_UHH>("SolidFramework_Model_Pdf_Pages_Plumbing_InitialView_GetIsDefault");
        return 0 != proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(model));
    }
    
    PageMagnification InitialView::GetMagnification() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Pdf_Pages_Plumbing_InitialView_Magnification_get");
        return (PageMagnification)proc(cPtr);
    }
    
    void InitialView::SetMagnification(PageMagnification value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Model_Pdf_Pages_Plumbing_InitialView_Magnification_set");
        proc(cPtr, (int)(value));
    }
    
    bool InitialView::GetModified() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_Pdf_Pages_Plumbing_InitialView_Modified_get");
        return 0 != proc(cPtr);
    }
    
    void InitialView::SetModified(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Model_Pdf_Pages_Plumbing_InitialView_Modified_set");
        proc(cPtr, value);
    }
    
    NavigationTab InitialView::GetNavigationTab() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Pdf_Pages_Plumbing_InitialView_NavigationTab_get");
        return (NavigationTab)proc(cPtr);
    }
    
    void InitialView::SetNavigationTab(NavigationTab value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Model_Pdf_Pages_Plumbing_InitialView_NavigationTab_set");
        proc(cPtr, (int)(value));
    }
    
    bool InitialView::GetOpenInFullScreenMode() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_Pdf_Pages_Plumbing_InitialView_OpenInFullScreenMode_get");
        return 0 != proc(cPtr);
    }
    
    void InitialView::SetOpenInFullScreenMode(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Model_Pdf_Pages_Plumbing_InitialView_OpenInFullScreenMode_set");
        proc(cPtr, value);
    }
    
    int InitialView::GetOpenToPageTargetID() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Pdf_Pages_Plumbing_InitialView_OpenToPageTargetID_get");
        return proc(cPtr);
    }
    
    void InitialView::SetOpenToPageTargetID(int value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Model_Pdf_Pages_Plumbing_InitialView_OpenToPageTargetID_set");
        proc(cPtr, value);
    }
    
    double InitialView::GetPercent() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Model_Pdf_Pages_Plumbing_InitialView_Percent_get");
        return proc(cPtr);
    }
    
    void InitialView::SetPercent(double value) {
        static InvokeType_VHD proc = GetFunction<InvokeType_VHD>("SolidFramework_Model_Pdf_Pages_Plumbing_InitialView_Percent_set");
        proc(cPtr, value);
    }
    
    bool InitialView::GetResizeWindowToInitialPage() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_Pdf_Pages_Plumbing_InitialView_ResizeWindowToInitialPage_get");
        return 0 != proc(cPtr);
    }
    
    void InitialView::SetResizeWindowToInitialPage(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Model_Pdf_Pages_Plumbing_InitialView_ResizeWindowToInitialPage_set");
        proc(cPtr, value);
    }
    

    void Link::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Pdf_Pages_Plumbing_Link");
        FrameworkObject::Dispose(destructor);
    }
    
    LinkPtr Link::DynamicCast(AnnotationPtr value) {
        return std::dynamic_pointer_cast<Link>(value);
    }
    
    Link::Link() {
        // only called from derived class constructors
    }
    

    ReadOnlyInfo::ReadOnlyInfo() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("new_SolidFramework_Model_Pdf_Pages_Plumbing_ReadOnlyInfo");
        HANDLE handle = proc();
        Wrap(handle, true);
    }
    
    void ReadOnlyInfo::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Pdf_Pages_Plumbing_ReadOnlyInfo");
        FrameworkObject::Dispose(destructor);
    }
    
    SolidFramework::Pdf::AccessPermissions ReadOnlyInfo::GetAccessPermissions() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Pdf_Pages_Plumbing_ReadOnlyInfo_AccessPermissions_get");
        return (SolidFramework::Pdf::AccessPermissions)proc(cPtr);
    }
    
    SolidFramework::Pdf::AuthenticationModeType ReadOnlyInfo::GetAuthenticationMode() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Pdf_Pages_Plumbing_ReadOnlyInfo_AuthenticationMode_get");
        return (SolidFramework::Pdf::AuthenticationModeType)proc(cPtr);
    }
    
    SolidFramework::Interop::DateTimePtr ReadOnlyInfo::GetCreationDate() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Pdf_Pages_Plumbing_ReadOnlyInfo_GetCreationDate");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::DateTime>(newCPtr, true);
    }
    
    std::wstring ReadOnlyInfo::GetCreator() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Model_Pdf_Pages_Plumbing_ReadOnlyInfo_GetCreator");
        return proc(cPtr);
    }
    
    SolidFramework::Pdf::EncryptionAlgorithm ReadOnlyInfo::GetEncryptionAlgorithm() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Pdf_Pages_Plumbing_ReadOnlyInfo_EncryptionAlgorithm_get");
        return (SolidFramework::Pdf::EncryptionAlgorithm)proc(cPtr);
    }
    
    double ReadOnlyInfo::GetFirstPageHeight() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Model_Pdf_Pages_Plumbing_ReadOnlyInfo_FirstPageHeight_get");
        return proc(cPtr);
    }
    
    double ReadOnlyInfo::GetFirstPageWidth() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Model_Pdf_Pages_Plumbing_ReadOnlyInfo_FirstPageWidth_get");
        return proc(cPtr);
    }
    
    bool ReadOnlyInfo::IsLinearized() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_Pdf_Pages_Plumbing_ReadOnlyInfo_IsLinearized_get");
        return 0 != proc(cPtr);
    }
    
    bool ReadOnlyInfo::IsTagged() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_Pdf_Pages_Plumbing_ReadOnlyInfo_IsTagged_get");
        return 0 != proc(cPtr);
    }
    
    SolidFramework::Interop::DateTimePtr ReadOnlyInfo::GetModificationDate() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Pdf_Pages_Plumbing_ReadOnlyInfo_GetModificationDate");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::DateTime>(newCPtr, true);
    }
    
    std::wstring ReadOnlyInfo::GetOpenPassword() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Model_Pdf_Pages_Plumbing_ReadOnlyInfo_GetOpenPassword");
        return proc(cPtr);
    }
    
    std::wstring ReadOnlyInfo::GetProducer() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Model_Pdf_Pages_Plumbing_ReadOnlyInfo_GetProducer");
        return proc(cPtr);
    }
    
    int ReadOnlyInfo::GetSizeInBytes() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Pdf_Pages_Plumbing_ReadOnlyInfo_SizeInBytes_get");
        return proc(cPtr);
    }
    
    std::wstring ReadOnlyInfo::GetVersion() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Model_Pdf_Pages_Plumbing_ReadOnlyInfo_GetVersion");
        return proc(cPtr);
    }
    
    bool ReadOnlyInfo::WasRepaired() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_Pdf_Pages_Plumbing_ReadOnlyInfo_WasRepaired_get");
        return 0 != proc(cPtr);
    }
    

    StrikeOutComment::StrikeOutComment() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("new_SolidFramework_Model_Pdf_Pages_Plumbing_StrikeOutComment");
        HANDLE handle = proc();
        Wrap(handle, true);
    }
    
    void StrikeOutComment::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Pdf_Pages_Plumbing_StrikeOutComment");
        FrameworkObject::Dispose(destructor);
    }
    
    std::wstring StrikeOutComment::ToString() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Model_Pdf_Pages_Plumbing_StrikeOutComment_ToString");
        return UnwrapWString(proc(cPtr));
    }
    
    StrikeOutCommentPtr StrikeOutComment::DynamicCast(AnnotationPtr value) {
        return std::dynamic_pointer_cast<StrikeOutComment>(value);
    }
    

    TextComment::TextComment() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("new_SolidFramework_Model_Pdf_Pages_Plumbing_TextComment");
        HANDLE handle = proc();
        Wrap(handle, true);
    }
    
    void TextComment::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Pdf_Pages_Plumbing_TextComment");
        FrameworkObject::Dispose(destructor);
    }
    
    std::wstring TextComment::ToString() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Model_Pdf_Pages_Plumbing_TextComment_ToString");
        return UnwrapWString(proc(cPtr));
    }
    
    TextCommentPtr TextComment::DynamicCast(AnnotationPtr value) {
        return std::dynamic_pointer_cast<TextComment>(value);
    }
    

    UnderlineComment::UnderlineComment() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("new_SolidFramework_Model_Pdf_Pages_Plumbing_UnderlineComment");
        HANDLE handle = proc();
        Wrap(handle, true);
    }
    
    void UnderlineComment::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Pdf_Pages_Plumbing_UnderlineComment");
        FrameworkObject::Dispose(destructor);
    }
    
    std::wstring UnderlineComment::ToString() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Model_Pdf_Pages_Plumbing_UnderlineComment_ToString");
        return UnwrapWString(proc(cPtr));
    }
    
    UnderlineCommentPtr UnderlineComment::DynamicCast(AnnotationPtr value) {
        return std::dynamic_pointer_cast<UnderlineComment>(value);
    }
    

    URILink::URILink() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("new_SolidFramework_Model_Pdf_Pages_Plumbing_URILink");
        HANDLE handle = proc();
        Wrap(handle, true);
    }
    
    void URILink::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Pdf_Pages_Plumbing_URILink");
        FrameworkObject::Dispose(destructor);
    }
    
    bool URILink::IsSafe() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_Pdf_Pages_Plumbing_URILink_IsSafe_get");
        return 0 != proc(cPtr);
    }
    
    std::wstring URILink::GetURI() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Model_Pdf_Pages_Plumbing_URILink_GetURI");
        return proc(cPtr);
    }
    
    void URILink::SetURI(std::wstring value) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Model_Pdf_Pages_Plumbing_URILink_SetURI");
        proc(cPtr, value.c_str());
    }
    
    URILinkPtr URILink::DynamicCast(AnnotationPtr value) {
        return std::dynamic_pointer_cast<URILink>(value);
    }
    

}}}}} // SolidFramework::Model::Pdf::Pages::Plumbing

namespace SolidFramework { namespace Model { namespace Plumbing { 
    void BezierGraphicSegment::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Plumbing_BezierGraphicSegment");
        FrameworkObject::Dispose(destructor);
    }
    
    SolidFramework::Interop::PointFPtr BezierGraphicSegment::GetEndHandle() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Plumbing_BezierGraphicSegment_GetEndHandle");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::PointF>(newCPtr, true);
    }
    
    SolidFramework::Interop::PointFPtr BezierGraphicSegment::GetStartHandle() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Plumbing_BezierGraphicSegment_GetStartHandle");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::PointF>(newCPtr, true);
    }
    
    BezierGraphicSegmentPtr BezierGraphicSegment::DynamicCast(GraphicSegmentPtr value) {
        return std::dynamic_pointer_cast<BezierGraphicSegment>(value);
    }
    

    void Bookmark::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Plumbing_Bookmark");
        FrameworkObject::Dispose(destructor);
    }
    
    BookmarkPtr Bookmark::GetChild(int index) {
        static InvokeType_HHI proc = GetFunction<InvokeType_HHI>("SolidFramework_Model_Plumbing_Bookmark_GetChild");
        HANDLE newCPtr = proc(cPtr, index);
        return newCPtr == nullptr? nullptr : std::make_shared<Bookmark>(newCPtr, true);
    }
    
    int Bookmark::GetCount() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Plumbing_Bookmark_Count_get");
        return proc(cPtr);
    }
    
    bool Bookmark::IsHidden() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_Plumbing_Bookmark_IsHidden_get");
        return 0 != proc(cPtr);
    }
    
    std::wstring Bookmark::GetName() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Model_Plumbing_Bookmark_GetName");
        return UnwrapWString(proc(cPtr));
    }
    
    int Bookmark::GetPageNumber() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Plumbing_Bookmark_PageNumber_get");
        return proc(cPtr);
    }
    
    BookmarkType Bookmark::GetType() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Plumbing_Bookmark_Type_get");
        return (BookmarkType)proc(cPtr);
    }
    
    BookmarkPtr Bookmark::DynamicCast(SolidObjectPtr value) {
        return std::dynamic_pointer_cast<Bookmark>(value);
    }
    

    void Borders::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Plumbing_Borders");
        FrameworkObject::Dispose(destructor);
    }
    
    LineStylePtr Borders::GetBottom() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Plumbing_Borders_GetBottom");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<LineStyle>(newCPtr, true);
    }
    
    bool Borders::IsEqual(BordersPtr value) {
        static InvokeType_UHH proc = GetFunction<InvokeType_UHH>("SolidFramework_Model_Plumbing_Borders_IsEqual");
        return 0 != proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(value));
    }
    
    LineStylePtr Borders::GetLeft() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Plumbing_Borders_GetLeft");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<LineStyle>(newCPtr, true);
    }
    
    LineStylePtr Borders::GetRight() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Plumbing_Borders_GetRight");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<LineStyle>(newCPtr, true);
    }
    
    LineStylePtr Borders::GetTop() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Plumbing_Borders_GetTop");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<LineStyle>(newCPtr, true);
    }
    

    void Cell::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Plumbing_Cell");
        FrameworkObject::Dispose(destructor);
    }
    
    BordersPtr Cell::GetBorders() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Plumbing_Cell_GetBorders");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<Borders>(newCPtr, true);
    }
    
    CellVerticalAlignment Cell::GetCellVerticalAlignment() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Plumbing_Cell_CellVerticalAlignment_get");
        return (CellVerticalAlignment)proc(cPtr);
    }
    
    void Cell::SetCellVerticalAlignment(CellVerticalAlignment value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Model_Plumbing_Cell_CellVerticalAlignment_set");
        proc(cPtr, (int)(value));
    }
    
    int Cell::GetColSpan() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Plumbing_Cell_ColSpan_get");
        return proc(cPtr);
    }
    
    SolidFramework::Interop::ColorPtr Cell::GetFillColor() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Plumbing_Cell_GetFillColor");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::Color>(newCPtr, true);
    }
    
    std::wstring Cell::GetFormat() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Model_Plumbing_Cell_GetFormat");
        return proc(cPtr);
    }
    
    bool Cell::HasFillColor() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_Plumbing_Cell_HasFillColor_get");
        return 0 != proc(cPtr);
    }
    
    HyperlinkPtr Cell::GetHyperlink() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Plumbing_Cell_GetHyperlink");
        HANDLE newCPtr = proc(cPtr);
        return SolidFrameworkNative::MakeHyperlink(newCPtr, true);
    }
    
    int Cell::GetRowSpan() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Plumbing_Cell_RowSpan_get");
        return proc(cPtr);
    }
    
    std::wstring Cell::GetText() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Model_Plumbing_Cell_GetText");
        return proc(cPtr);
    }
    
    TextDirection Cell::GetTextDirection() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Plumbing_Cell_TextDirection_get");
        return (TextDirection)proc(cPtr);
    }
    
    TextType Cell::GetTextType() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Plumbing_Cell_TextType_get");
        return (TextType)proc(cPtr);
    }
    
    void Cell::SetTextType(TextType value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Model_Plumbing_Cell_TextType_set");
        proc(cPtr, (int)(value));
    }
    
    std::wstring Cell::GetValue() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Model_Plumbing_Cell_GetValue");
        return proc(cPtr);
    }
    
    double Cell::GetWidth() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Model_Plumbing_Cell_Width_get");
        return proc(cPtr);
    }
    
    CellPtr Cell::DynamicCast(SolidObjectPtr value) {
        return std::dynamic_pointer_cast<Cell>(value);
    }
    

    void Column::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Plumbing_Column");
        FrameworkObject::Dispose(destructor);
    }
    
    double Column::GetSpacing() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Model_Plumbing_Column_Spacing_get");
        return proc(cPtr);
    }
    
    void Column::SetSpacing(double value) {
        static InvokeType_VHD proc = GetFunction<InvokeType_VHD>("SolidFramework_Model_Plumbing_Column_Spacing_set");
        proc(cPtr, value);
    }
    
    double Column::GetWidth() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Model_Plumbing_Column_Width_get");
        return proc(cPtr);
    }
    
    void Column::SetWidth(double value) {
        static InvokeType_VHD proc = GetFunction<InvokeType_VHD>("SolidFramework_Model_Plumbing_Column_Width_set");
        proc(cPtr, value);
    }
    

    void Columns::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Plumbing_Columns");
        FrameworkObject::Dispose(destructor);
    }
    
    std::vector<ColumnPtr> Columns::GetColumns() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Plumbing_Columns_GetColumns");
        return SolidFrameworkNative::ColumnsCollectionToVector(proc(cPtr), false);
    }
    
    int Columns::GetCount() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Plumbing_Columns_Count_get");
        return proc(cPtr);
    }
    
    std::wstring Columns::ToString() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Model_Plumbing_Columns_ToString");
        return UnwrapWString(proc(cPtr));
    }
    

    void ExternalHyperlink::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Plumbing_ExternalHyperlink");
        FrameworkObject::Dispose(destructor);
    }
    
    std::wstring ExternalHyperlink::GetDestination() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Model_Plumbing_ExternalHyperlink_GetDestination");
        return UnwrapWString(proc(cPtr));
    }
    
    SolidFramework::Interop::UriPtr ExternalHyperlink::GetUrl() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Plumbing_ExternalHyperlink_GetUrl");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::Uri>(newCPtr, true);
    }
    
    ExternalHyperlinkPtr ExternalHyperlink::DynamicCast(SolidObjectPtr value) {
        return std::dynamic_pointer_cast<ExternalHyperlink>(value);
    }
    

    void FillStyle::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Plumbing_FillStyle");
        FrameworkObject::Dispose(destructor);
    }
    
    bool FillStyle::IsEqual(FillStylePtr value) {
        static InvokeType_UHH proc = GetFunction<InvokeType_UHH>("SolidFramework_Model_Plumbing_FillStyle_IsEqual");
        return 0 != proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(value));
    }
    
    FillStyleType FillStyle::GetFillType() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Plumbing_FillStyle_FillType_get");
        return (FillStyleType)proc(cPtr);
    }
    
    FillStyle::FillStyle() {
        // only called from derived class constructors
    }
    

    void Font::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Plumbing_Font");
        FrameworkObject::Dispose(destructor);
    }
    
    bool Font::GetBold() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_Plumbing_Font_Bold_get");
        return 0 != proc(cPtr);
    }
    
    bool Font::GetEastAsiaFont() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_Plumbing_Font_EastAsiaFont_get");
        return 0 != proc(cPtr);
    }
    
    double Font::GetGlyphWidth(int cid) {
        typedef double (SOLIDCALLBACK *InvokeType)(HANDLE, int);
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_Model_Plumbing_Font_GetGlyphWidth");
        return proc(cPtr, cid);
    }
    
    bool Font::GetItalic() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_Plumbing_Font_Italic_get");
        return 0 != proc(cPtr);
    }
    
    std::wstring Font::GetName() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Model_Plumbing_Font_GetName");
        return UnwrapWString(proc(cPtr));
    }
    
    std::wstring Font::TransformPrivateToUnicode(std::wstring text) {
        static InvokeType_SHS proc = GetFunction<InvokeType_SHS>("SolidFramework_Model_Plumbing_Font_TransformPrivateToUnicode");
        return UnwrapWString(proc(cPtr, text.c_str()));
    }
    
    FontPtr Font::DynamicCast(SolidObjectPtr value) {
        return std::dynamic_pointer_cast<Font>(value);
    }
    

    void GradientFill::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Plumbing_GradientFill");
        FrameworkObject::Dispose(destructor);
    }
    
    double GradientFill::GetAngle() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Model_Plumbing_GradientFill_Angle_get");
        return proc(cPtr);
    }
    
    double GradientFill::GetFocus() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Model_Plumbing_GradientFill_Focus_get");
        return proc(cPtr);
    }
    
    SolidFramework::Interop::ColorPtr GradientFill::GetFrom() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Plumbing_GradientFill_GetFromColor");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::Color>(newCPtr, true);
    }
    
    SolidFramework::Interop::ColorPtr GradientFill::GetTo() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Plumbing_GradientFill_GetToColor");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::Color>(newCPtr, true);
    }
    
    GradientFillPtr GradientFill::DynamicCast(FillStylePtr value) {
        return std::dynamic_pointer_cast<GradientFill>(value);
    }
    

    void Graphic::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Plumbing_Graphic");
        FrameworkObject::Dispose(destructor);
    }
    
    int Graphic::GetCount() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Plumbing_Graphic_Count_get");
        return proc(cPtr);
    }
    
    std::vector<GraphicPathPtr> Graphic::GetPaths() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Plumbing_Graphic_GetPaths");
        return SolidFrameworkNative::GraphicPathsCollectionToVector(proc(cPtr), false);
    }
    
    GraphicPtr Graphic::DynamicCast(SolidObjectPtr value) {
        return std::dynamic_pointer_cast<Graphic>(value);
    }
    

    void GraphicPath::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Plumbing_GraphicPath");
        FrameworkObject::Dispose(destructor);
    }
    
    int GraphicPath::GetCount() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Plumbing_GraphicPath_Count_get");
        return proc(cPtr);
    }
    
    bool GraphicPath::IsClosed() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_Plumbing_GraphicPath_IsClosed_get");
        return 0 != proc(cPtr);
    }
    
    std::vector<GraphicSegmentPtr> GraphicPath::GetSegments() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Plumbing_GraphicPath_GetSegments");
        return SolidFrameworkNative::GraphicSegmentsCollectionToVector(proc(cPtr), false);
    }
    

    void GraphicSegment::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Plumbing_GraphicSegment");
        FrameworkObject::Dispose(destructor);
    }
    
    SolidFramework::Interop::PointFPtr GraphicSegment::GetEnd() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Plumbing_GraphicSegment_GetEnd");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::PointF>(newCPtr, true);
    }
    
    bool GraphicSegment::IsEqual(GraphicSegmentPtr value) {
        static InvokeType_UHH proc = GetFunction<InvokeType_UHH>("SolidFramework_Model_Plumbing_GraphicSegment_IsEqual");
        return 0 != proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(value));
    }
    
    SolidFramework::Interop::PointFPtr GraphicSegment::GetStart() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Plumbing_GraphicSegment_GetStart");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::PointF>(newCPtr, true);
    }
    
    GraphicSegment::GraphicSegment() {
        // only called from derived class constructors
    }
    

    void Group::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Plumbing_Group");
        FrameworkObject::Dispose(destructor);
    }
    
    int Group::GetCount() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Plumbing_Group_Count_get");
        return proc(cPtr);
    }
    
    std::vector<SolidObjectPtr> Group::GetSolidObjects() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Plumbing_Group_GetSolidObjects");
        return SolidFrameworkNative::SolidObjectsCollectionToVector(proc(cPtr), false);
    }
    
    GroupPtr Group::DynamicCast(SolidObjectPtr value) {
        return std::dynamic_pointer_cast<Group>(value);
    }
    

    void HeaderFooter::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Plumbing_HeaderFooter");
        FrameworkObject::Dispose(destructor);
    }
    
    bool HeaderFooter::IsHeader() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_Plumbing_HeaderFooter_IsHeader_get");
        return 0 != proc(cPtr);
    }
    
    double HeaderFooter::GetOffset() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Model_Plumbing_HeaderFooter_Offset_get");
        return proc(cPtr);
    }
    
    HeaderFooterPtr HeaderFooter::DynamicCast(SolidObjectPtr value) {
        return std::dynamic_pointer_cast<HeaderFooter>(value);
    }
    

    void HeaderFooterBlockCollection::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Plumbing_HeaderFooterBlockCollection");
        FrameworkObject::Dispose(destructor);
    }
    
    PageNumberFormat HeaderFooterBlockCollection::GetPageNumberFormat() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Plumbing_HeaderFooterBlockCollection_PageNumberFormat_get");
        return (PageNumberFormat)proc(cPtr);
    }
    
    int HeaderFooterBlockCollection::GetPageNumberStartValue() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Plumbing_HeaderFooterBlockCollection_PageNumberStartValue_get");
        return proc(cPtr);
    }
    
    HeaderFooterBlockCollectionPtr HeaderFooterBlockCollection::DynamicCast(SolidObjectPtr value) {
        return std::dynamic_pointer_cast<HeaderFooterBlockCollection>(value);
    }
    

    void Hyperlink::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Plumbing_Hyperlink");
        FrameworkObject::Dispose(destructor);
    }
    
    int Hyperlink::Compare(HyperlinkPtr first, HyperlinkPtr second) {
        if (first == nullptr) {
            return second == nullptr? 0 : 1;
        }
        if (second == nullptr) {
            return -1;
        }
        static InvokeType_IHH proc = GetFunction<InvokeType_IHH>("SolidFramework_Model_Plumbing_Hyperlink_Compare");
        return proc(SolidFramework::FrameworkObject::GetCPtr(first), SolidFramework::FrameworkObject::GetCPtr(second));
    }
    
    HyperlinkPtr Hyperlink::DynamicCast(SolidObjectPtr value) {
        return std::dynamic_pointer_cast<Hyperlink>(value);
    }
    
    Hyperlink::Hyperlink() {
        // only called from derived class constructors
    }
    

    void ImageFill::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Plumbing_ImageFill");
        FrameworkObject::Dispose(destructor);
    }
    
    SolidFramework::Imaging::ImagePtr ImageFill::GetImage() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Plumbing_ImageFill_GetImage");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Imaging::Image>(newCPtr, true);
    }
    
    ImageFillPtr ImageFill::DynamicCast(FillStylePtr value) {
        return std::dynamic_pointer_cast<ImageFill>(value);
    }
    

    void ImageShape::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Plumbing_ImageShape");
        FrameworkObject::Dispose(destructor);
    }
    
    SolidFramework::Imaging::ImagePtr ImageShape::GetImage() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Plumbing_ImageShape_GetImage");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Imaging::Image>(newCPtr, true);
    }
    
    ImageShapePtr ImageShape::DynamicCast(SolidObjectPtr value) {
        return std::dynamic_pointer_cast<ImageShape>(value);
    }
    

    void InternalHyperlink::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Plumbing_InternalHyperlink");
        FrameworkObject::Dispose(destructor);
    }
    
    BookmarkPtr InternalHyperlink::GetBookmark() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Plumbing_InternalHyperlink_GetBookmark");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<Bookmark>(newCPtr, true);
    }
    
    InternalHyperlinkPtr InternalHyperlink::DynamicCast(SolidObjectPtr value) {
        return std::dynamic_pointer_cast<InternalHyperlink>(value);
    }
    

    void LineStyle::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Plumbing_LineStyle");
        FrameworkObject::Dispose(destructor);
    }
    
    SolidFramework::Interop::ColorPtr LineStyle::GetColor() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Plumbing_LineStyle_GetColor");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::Color>(newCPtr, true);
    }
    
    LineCompoundType LineStyle::GetCompoundType() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Plumbing_LineStyle_CompoundType_get");
        return (LineCompoundType)proc(cPtr);
    }
    
    LineDashType LineStyle::GetDashType() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Plumbing_LineStyle_DashType_get");
        return (LineDashType)proc(cPtr);
    }
    
    bool LineStyle::IsEqual(LineStylePtr value) {
        static InvokeType_UHH proc = GetFunction<InvokeType_UHH>("SolidFramework_Model_Plumbing_LineStyle_IsEqual");
        return 0 != proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(value));
    }
    
    double LineStyle::GetWeight() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Model_Plumbing_LineStyle_Weight_get");
        return proc(cPtr);
    }
    

    void List::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Plumbing_List");
        FrameworkObject::Dispose(destructor);
    }
    
    std::vector<ParagraphPtr> List::GetAttachedParagraphs() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Plumbing_List_GetAttachedParagraphs");
        return SolidFrameworkNative::ParagraphsCollectionToVector(proc(cPtr), false);
    }
    
    int List::CalculateNumericValue(ParagraphPtr paragraph) {
        static InvokeType_IHH proc = GetFunction<InvokeType_IHH>("SolidFramework_Model_Plumbing_List_CalculateNumericValue");
        return proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(paragraph));
    }
    
    int List::GetCount() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Plumbing_List_Count_get");
        return proc(cPtr);
    }
    
    std::wstring List::GenerateLevelText(ParagraphPtr paragraph) {
        typedef const wchar_t* (SOLIDCALLBACK *InvokeType)(HANDLE, HANDLE);
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_Model_Plumbing_List_GenerateLevelText");
        return UnwrapWString(proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(paragraph)));
    }
    
    ListLevelsPtr List::GetListLevels() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Plumbing_List_GetListLevels");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<ListLevels>(newCPtr, true);
    }
    
    SolidFramework::Model::TopicPtr List::GetTopic() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Plumbing_List_GetTopic");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Model::Topic>(newCPtr, true);
    }
    
    ListPtr List::DynamicCast(SolidObjectPtr value) {
        return std::dynamic_pointer_cast<List>(value);
    }
    

    void ListLevels::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Plumbing_ListLevels");
        FrameworkObject::Dispose(destructor);
    }
    
    int ListLevels::GetCount() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Plumbing_ListLevels_Count_get");
        return proc(cPtr);
    }
    
    ListStylePtr ListLevels::GetListStyle(int level) {
        static InvokeType_HHI proc = GetFunction<InvokeType_HHI>("SolidFramework_Model_Plumbing_ListLevels_GetListStyle");
        HANDLE newCPtr = proc(cPtr, level);
        return newCPtr == nullptr? nullptr : std::make_shared<ListStyle>(newCPtr, true);
    }
    
    std::vector<ListStylePtr> ListLevels::GetListStyles() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Plumbing_ListLevels_GetListStyles");
        return SolidFrameworkNative::ListStylesCollectionToVector(proc(cPtr), false);
    }
    
    ListLevelsPtr ListLevels::DynamicCast(SolidObjectPtr value) {
        return std::dynamic_pointer_cast<ListLevels>(value);
    }
    

    void ListStyle::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Plumbing_ListStyle");
        FrameworkObject::Dispose(destructor);
    }
    
    ListFormat ListStyle::GetFormat() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Plumbing_ListStyle_Format_get");
        return (ListFormat)proc(cPtr);
    }
    
    int ListStyle::GetStartValue() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Plumbing_ListStyle_StartValue_get");
        return proc(cPtr);
    }
    
    std::wstring ListStyle::GetTemplate() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Model_Plumbing_ListStyle_GetTemplate");
        return UnwrapWString(proc(cPtr));
    }
    
    ListStylePtr ListStyle::DynamicCast(StylePtr value) {
        return std::dynamic_pointer_cast<ListStyle>(value);
    }
    

    void Margins::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Plumbing_Margins");
        FrameworkObject::Dispose(destructor);
    }
    
    double Margins::GetBottom() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Model_Plumbing_Margins_Bottom_get");
        return proc(cPtr);
    }
    
    double Margins::GetLeft() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Model_Plumbing_Margins_Left_get");
        return proc(cPtr);
    }
    
    double Margins::GetRight() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Model_Plumbing_Margins_Right_get");
        return proc(cPtr);
    }
    
    double Margins::GetTop() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Model_Plumbing_Margins_Top_get");
        return proc(cPtr);
    }
    
    std::wstring Margins::ToString() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Model_Plumbing_Margins_ToString");
        return UnwrapWString(proc(cPtr));
    }
    

    void MatrixSO::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Plumbing_MatrixSO");
        FrameworkObject::Dispose(destructor);
    }
    
    double MatrixSO::GetRotation() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Model_Plumbing_MatrixSO_Rotation_get");
        return proc(cPtr);
    }
    
    double MatrixSO::GetSkew() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Model_Plumbing_MatrixSO_Skew_get");
        return proc(cPtr);
    }
    
    double MatrixSO::GetXOffset() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Model_Plumbing_MatrixSO_XOffset_get");
        return proc(cPtr);
    }
    
    double MatrixSO::GetXScale() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Model_Plumbing_MatrixSO_XScale_get");
        return proc(cPtr);
    }
    
    double MatrixSO::GetYOffset() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Model_Plumbing_MatrixSO_YOffset_get");
        return proc(cPtr);
    }
    
    double MatrixSO::GetYScale() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Model_Plumbing_MatrixSO_YScale_get");
        return proc(cPtr);
    }
    

    Options::Options() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("new_SolidFramework_Model_Plumbing_Options");
        HANDLE handle = proc();
        Wrap(handle, true);
    }
    
    void Options::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Plumbing_Options");
        FrameworkObject::Dispose(destructor);
    }
    

    void Paragraph::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Plumbing_Paragraph");
        FrameworkObject::Dispose(destructor);
    }
    
    ListPtr Paragraph::GetAttachedList() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Plumbing_Paragraph_GetAttachedList");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<List>(newCPtr, true);
    }
    
    int Paragraph::GetAttachedListLevel() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Plumbing_Paragraph_AttachedListLevel_get");
        return proc(cPtr);
    }
    
    int Paragraph::GetCount() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Plumbing_Paragraph_Count_get");
        return proc(cPtr);
    }
    
    std::vector<RunPtr> Paragraph::GetRuns() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Plumbing_Paragraph_GetRuns");
        return SolidFrameworkNative::RunsCollectionToVector(proc(cPtr), false);
    }
    
    ParagraphStylePtr Paragraph::GetStyle() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Plumbing_Paragraph_GetParagraphStyle");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<ParagraphStyle>(newCPtr, true);
    }
    
    std::vector<TabStopPtr> Paragraph::GetTabStops() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Plumbing_Paragraph_GetTabStops");
        return SolidFrameworkNative::TabStopCollectionToVector(proc(cPtr), false);
    }
    
    ParagraphPtr Paragraph::DynamicCast(SolidObjectPtr value) {
        return std::dynamic_pointer_cast<Paragraph>(value);
    }
    

    void ParagraphStyle::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Plumbing_ParagraphStyle");
        FrameworkObject::Dispose(destructor);
    }
    
    double ParagraphStyle::GetAfterSpacing() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Model_Plumbing_ParagraphStyle_AfterSpacing_get");
        return proc(cPtr);
    }
    
    Alignment ParagraphStyle::GetAlignment() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Plumbing_ParagraphStyle_Alignment_get");
        return (Alignment)proc(cPtr);
    }
    
    double ParagraphStyle::GetBeforeSpacing() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Model_Plumbing_ParagraphStyle_BeforeSpacing_get");
        return proc(cPtr);
    }
    
    double ParagraphStyle::GetFirstLineIndent() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Model_Plumbing_ParagraphStyle_FirstLineIndent_get");
        return proc(cPtr);
    }
    
    double ParagraphStyle::GetLeftIndent() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Model_Plumbing_ParagraphStyle_LeftIndent_get");
        return proc(cPtr);
    }
    
    double ParagraphStyle::GetLineSpacing() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Model_Plumbing_ParagraphStyle_LineSpacing_get");
        return proc(cPtr);
    }
    
    LineSpacingType ParagraphStyle::GetLineSpacingType() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Plumbing_ParagraphStyle_LineSpacingType_get");
        return (LineSpacingType)proc(cPtr);
    }
    
    int ParagraphStyle::GetOutlineLevel() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Plumbing_ParagraphStyle_OutlineLevel_get");
        return proc(cPtr);
    }
    
    double ParagraphStyle::GetRightIndent() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Model_Plumbing_ParagraphStyle_RightIndent_get");
        return proc(cPtr);
    }
    
    ParagraphStylePtr ParagraphStyle::DynamicCast(StylePtr value) {
        return std::dynamic_pointer_cast<ParagraphStyle>(value);
    }
    
    ParagraphStyle::ParagraphStyle() {
        // only called from derived class constructors
    }
    

    void ParagraphStyleTemplate::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Plumbing_ParagraphStyleTemplate");
        FrameworkObject::Dispose(destructor);
    }
    
    double ParagraphStyleTemplate::GetAfterSpacing() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Model_Plumbing_ParagraphStyleTemplate_AfterSpacing_get");
        return proc(cPtr);
    }
    
    Alignment ParagraphStyleTemplate::GetAlignment() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Plumbing_ParagraphStyleTemplate_Alignment_get");
        return (Alignment)proc(cPtr);
    }
    
    double ParagraphStyleTemplate::GetBeforeSpacing() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Model_Plumbing_ParagraphStyleTemplate_BeforeSpacing_get");
        return proc(cPtr);
    }
    
    double ParagraphStyleTemplate::GetFirstLineIndent() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Model_Plumbing_ParagraphStyleTemplate_FirstLineIndent_get");
        return proc(cPtr);
    }
    
    double ParagraphStyleTemplate::GetLeftIndent() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Model_Plumbing_ParagraphStyleTemplate_LeftIndent_get");
        return proc(cPtr);
    }
    
    double ParagraphStyleTemplate::GetLineSpacing() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Model_Plumbing_ParagraphStyleTemplate_LineSpacing_get");
        return proc(cPtr);
    }
    
    LineSpacingType ParagraphStyleTemplate::GetLineSpacingType() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Plumbing_ParagraphStyleTemplate_LineSpacingType_get");
        return (LineSpacingType)proc(cPtr);
    }
    
    int ParagraphStyleTemplate::GetOutlineLevel() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Plumbing_ParagraphStyleTemplate_OutlineLevel_get");
        return proc(cPtr);
    }
    
    double ParagraphStyleTemplate::GetRightIndent() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Model_Plumbing_ParagraphStyleTemplate_RightIndent_get");
        return proc(cPtr);
    }
    
    bool ParagraphStyleTemplate::GetRightToLeftWriting() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_Plumbing_ParagraphStyleTemplate_RightToLeftWriting_get");
        return 0 != proc(cPtr);
    }
    
    ParagraphStylePtr ParagraphStyleTemplate::GetStyle() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Plumbing_ParagraphStyleTemplate_GetParagraphStyle");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<ParagraphStyle>(newCPtr, true);
    }
    
    ParagraphStyleTemplatePtr ParagraphStyleTemplate::DynamicCast(SolidObjectPtr value) {
        return std::dynamic_pointer_cast<ParagraphStyleTemplate>(value);
    }
    

    void Row::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Plumbing_Row");
        FrameworkObject::Dispose(destructor);
    }
    
    double Row::CalculateExcelHeight() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Model_Plumbing_Row_CalculateExcelHeight");
        return proc(cPtr);
    }
    
    double Row::CalculateWordHeight() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Model_Plumbing_Row_CalculateWordHeight");
        return proc(cPtr);
    }
    
    std::vector<CellPtr> Row::GetCells() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Plumbing_Row_GetCells");
        return SolidFrameworkNative::CellsCollectionToVector(proc(cPtr), false);
    }
    
    int Row::GetCount() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Plumbing_Row_Count_get");
        return proc(cPtr);
    }
    
    double Row::GetHeight() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Model_Plumbing_Row_Height_get");
        return proc(cPtr);
    }
    
    RowHeightRule Row::GetHeightRule() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Plumbing_Row_HeightRule_get");
        return (RowHeightRule)proc(cPtr);
    }
    

    void Run::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Plumbing_Run");
        FrameworkObject::Dispose(destructor);
    }
    
    BookmarkPtr Run::GetBookmarkEnd() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Plumbing_Run_GetBookmarkEnd");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<Bookmark>(newCPtr, true);
    }
    
    BookmarkPtr Run::GetBookmarkStart() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Plumbing_Run_GetBookmarkStart");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<Bookmark>(newCPtr, true);
    }
    
    HyperlinkPtr Run::GetHyperlink() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Plumbing_Run_GetHyperlink");
        HANDLE newCPtr = proc(cPtr);
        return SolidFrameworkNative::MakeHyperlink(newCPtr, true);
    }
    
    ShapePtr Run::GetInlineShape() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Plumbing_Run_GetInlineShape");
        HANDLE newCPtr = proc(cPtr);
        return SolidFrameworkNative::MakeShape(newCPtr, true);
    }
    
    bool Run::IsPageNumberField() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_Plumbing_Run_IsPageNumberField_get");
        return 0 != proc(cPtr);
    }
    
    TextStylePtr Run::GetStyle() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Plumbing_Run_GetTextStyle");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<TextStyle>(newCPtr, true);
    }
    
    std::wstring Run::GetText() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Model_Plumbing_Run_GetText");
        return UnwrapWString(proc(cPtr));
    }
    

    void Section::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Plumbing_Section");
        FrameworkObject::Dispose(destructor);
    }
    
    ColumnsPtr Section::GetColumns() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Plumbing_Section_GetColumns");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<Columns>(newCPtr, true);
    }
    
    bool Section::HasDifferentHeaderFooterForEvenPages() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_Plumbing_Section_HasDifferentHeaderFooterForEvenPages_get");
        return 0 != proc(cPtr);
    }
    
    bool Section::HasPageBackgroundColor() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_Plumbing_Section_HasPageBackgroundColor_get");
        return 0 != proc(cPtr);
    }
    
    bool Section::IsSoftPageBreak() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_Plumbing_Section_IsSoftPageBreak_get");
        return 0 != proc(cPtr);
    }
    
    MarginsPtr Section::GetMargins() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Plumbing_Section_GetMargins");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<Margins>(newCPtr, true);
    }
    
    int Section::GetOriginalPageNumber() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Plumbing_Section_GetOriginalPageNumber");
        return proc(cPtr);
    }
    
    SolidFramework::Interop::ColorPtr Section::GetPageBackgroundColor() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Plumbing_Section_GetPageBackgroundColor");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::Color>(newCPtr, true);
    }
    
    LineStylePtr Section::GetPageBorder() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Plumbing_Section_GetPageBorder");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<LineStyle>(newCPtr, true);
    }
    
    MarginsPtr Section::GetPageBorderMargins() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Plumbing_Section_GetPageBorderMargins");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<Margins>(newCPtr, true);
    }
    
    double Section::GetPageHeight() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Model_Plumbing_Section_PageHeight_get");
        return proc(cPtr);
    }
    
    double Section::GetPageWidth() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Model_Plumbing_Section_PageWidth_get");
        return proc(cPtr);
    }
    
    SectionBreak Section::GetSectionBreak() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Plumbing_Section_SectionBreak_get");
        return (SectionBreak)proc(cPtr);
    }
    
    SlideLayout Section::GetSlideLayout() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Plumbing_Section_SlideLayout_get");
        return (SlideLayout)proc(cPtr);
    }
    
    TextDirection Section::GetTextDirection() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Plumbing_Section_TextDirection_get");
        return (TextDirection)proc(cPtr);
    }
    
    SectionPtr Section::DynamicCast(SolidObjectPtr value) {
        return std::dynamic_pointer_cast<Section>(value);
    }
    

    void Shape::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Plumbing_Shape");
        FrameworkObject::Dispose(destructor);
    }
    
    std::wstring Shape::GetAltText() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Model_Plumbing_Shape_GetAltText");
        return UnwrapWString(proc(cPtr));
    }
    
    FillStylePtr Shape::GetFill() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Plumbing_Shape_GetFillStyle");
        HANDLE newCPtr = proc(cPtr);
        return SolidFrameworkNative::MakeFillStyle(newCPtr, true);
    }
    
    double Shape::GetHeight() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Model_Plumbing_Shape_Height_get");
        return proc(cPtr);
    }
    
    HorizontalAlignment Shape::GetHorizontalAlignment() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Plumbing_Shape_HorizontalAlignment_get");
        return (HorizontalAlignment)proc(cPtr);
    }
    
    HorizontalAnchoring Shape::GetHorizontalAnchoring() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Plumbing_Shape_HorizontalAnchoring_get");
        return (HorizontalAnchoring)proc(cPtr);
    }
    
    LineStylePtr Shape::GetLine() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Plumbing_Shape_GetLineStyle");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<LineStyle>(newCPtr, true);
    }
    
    MatrixSOPtr Shape::GetTransform() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Plumbing_Shape_GetTransform");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<MatrixSO>(newCPtr, true);
    }
    
    VerticalAlignment Shape::GetVerticalAlignment() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Plumbing_Shape_VerticalAlignment_get");
        return (VerticalAlignment)proc(cPtr);
    }
    
    VerticalAnchoring Shape::GetVerticalAnchoring() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Plumbing_Shape_VerticalAnchoring_get");
        return (VerticalAnchoring)proc(cPtr);
    }
    
    double Shape::GetWidth() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Model_Plumbing_Shape_Width_get");
        return proc(cPtr);
    }
    
    WrappingType Shape::GetWrappingType() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Plumbing_Shape_WrappingType_get");
        return (WrappingType)proc(cPtr);
    }
    
    ShapePtr Shape::DynamicCast(SolidObjectPtr value) {
        return std::dynamic_pointer_cast<Shape>(value);
    }
    
    Shape::Shape() {
        // only called from derived class constructors
    }
    

    void SolidCollection::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Plumbing_SolidCollection");
        FrameworkObject::Dispose(destructor);
    }
    
    int SolidCollection::GetCount() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Plumbing_SolidCollection_Count_get");
        return proc(cPtr);
    }
    
    std::vector<SolidObjectPtr> SolidCollection::GetSolidObjects() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Plumbing_SolidCollection_GetSolidObjects");
        return SolidFrameworkNative::SolidObjectsCollectionToVector(proc(cPtr), false);
    }
    
    SolidCollectionPtr SolidCollection::DynamicCast(SolidObjectPtr value) {
        return std::dynamic_pointer_cast<SolidCollection>(value);
    }
    
    SolidCollection::SolidCollection() {
        // only called from derived class constructors
    }
    

    void SolidFill::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Plumbing_SolidFill");
        FrameworkObject::Dispose(destructor);
    }
    
    SolidFramework::Interop::ColorPtr SolidFill::GetColor() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Plumbing_SolidFill_GetColor");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::Color>(newCPtr, true);
    }
    
    SolidFillPtr SolidFill::DynamicCast(FillStylePtr value) {
        return std::dynamic_pointer_cast<SolidFill>(value);
    }
    

    void SolidObject::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Plumbing_SolidObject");
        FrameworkObject::Dispose(destructor);
    }
    
    int SolidObject::GetID() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Plumbing_SolidObject_GetID");
        return proc(cPtr);
    }
    
    SolidObjectType SolidObject::GetObjectType() {
        return (SolidObjectType)SolidFrameworkNative::SolidFramework_Model_Plumbing_SolidObject_GetObjectType(cPtr);
    }
    
    std::wstring SolidObject::ToString() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Model_Plumbing_SolidObject_ToString");
        return UnwrapWString(proc(cPtr));
    }
    
    SolidObject::SolidObject() {
        // only called from derived class constructors
    }
    

    void Style::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Plumbing_Style");
        FrameworkObject::Dispose(destructor);
    }
    
    Style::Style() {
        // only called from derived class constructors
    }
    

    void StyleTemplate::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Plumbing_StyleTemplate");
        FrameworkObject::Dispose(destructor);
    }
    
    std::wstring StyleTemplate::GetName() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Model_Plumbing_StyleTemplate_GetName");
        return UnwrapWString(proc(cPtr));
    }
    
    StyleTemplatePtr StyleTemplate::GetStyleBasedOn() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Plumbing_StyleTemplate_GetStyleBasedOn");
        HANDLE newCPtr = proc(cPtr);
        return SolidFrameworkNative::MakeStyleTemplate(newCPtr, true);
    }
    
    StyleTemplatePtr StyleTemplate::DynamicCast(SolidObjectPtr value) {
        return std::dynamic_pointer_cast<StyleTemplate>(value);
    }
    
    StyleTemplate::StyleTemplate() {
        // only called from derived class constructors
    }
    

    void Table::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Plumbing_Table");
        FrameworkObject::Dispose(destructor);
    }
    
    CellPtr Table::GetCell(int ri, int ci) {
        static InvokeType_HHII proc = GetFunction<InvokeType_HHII>("SolidFramework_Model_Plumbing_Table_GetCell");
        HANDLE newCPtr = proc(cPtr, ri, ci);
        return newCPtr == nullptr? nullptr : std::make_shared<Cell>(newCPtr, true);
    }
    
    double Table::GetCellSpacing() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Model_Plumbing_Table_CellSpacing_get");
        return proc(cPtr);
    }
    
    int Table::GetRowCount() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Plumbing_Table_RowCount_get");
        return proc(cPtr);
    }
    
    std::vector<RowPtr> Table::GetRows() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Plumbing_Table_GetRows");
        return SolidFrameworkNative::RowsCollectionToVector(proc(cPtr), false);
    }
    
    TablePtr Table::DynamicCast(SolidObjectPtr value) {
        return std::dynamic_pointer_cast<Table>(value);
    }
    

    void TabStop::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Plumbing_TabStop");
        FrameworkObject::Dispose(destructor);
    }
    
    TabAlignment TabStop::GetAlignment() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Plumbing_TabStop_Alignment_get");
        return (TabAlignment)proc(cPtr);
    }
    
    bool TabStop::IsEqual(TabStopPtr value) {
        static InvokeType_UHH proc = GetFunction<InvokeType_UHH>("SolidFramework_Model_Plumbing_TabStop_IsEqual");
        return 0 != proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(value));
    }
    
    TabLeader TabStop::GetLeader() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Plumbing_TabStop_Leader_get");
        return (TabLeader)proc(cPtr);
    }
    
    int TabStop::GetNativeHashCode() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Plumbing_TabStop_GetNativeHashCode");
        return proc(cPtr);
    }
    
    double TabStop::GetTabPosition() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Model_Plumbing_TabStop_TabPosition_get");
        return proc(cPtr);
    }
    

    void TextBox::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Plumbing_TextBox");
        FrameworkObject::Dispose(destructor);
    }
    
    int TextBox::GetCount() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Plumbing_TextBox_Count_get");
        return proc(cPtr);
    }
    
    bool TextBox::IsAnnotation() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_Plumbing_TextBox_IsAnnotation_get");
        return 0 != proc(cPtr);
    }
    
    MarginsPtr TextBox::GetMargins() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Plumbing_TextBox_GetMargins");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<Margins>(newCPtr, true);
    }
    
    int TextBox::GetRotation() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Plumbing_TextBox_Rotation_get");
        return proc(cPtr);
    }
    
    std::vector<SolidObjectPtr> TextBox::GetSolidObjects() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Plumbing_TextBox_GetSolidObjects");
        return SolidFrameworkNative::SolidObjectsCollectionToVector(proc(cPtr), false);
    }
    
    TextDirection TextBox::GetTextDirection() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Plumbing_TextBox_TextDirection_get");
        return (TextDirection)proc(cPtr);
    }
    
    TextBoxPtr TextBox::DynamicCast(SolidObjectPtr value) {
        return std::dynamic_pointer_cast<TextBox>(value);
    }
    

    void TextStyle::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Plumbing_TextStyle");
        FrameworkObject::Dispose(destructor);
    }
    
    StyleTemplatePtr TextStyle::GetBasedOn() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Plumbing_TextStyle_GetBasedOn");
        HANDLE newCPtr = proc(cPtr);
        return SolidFrameworkNative::MakeStyleTemplate(newCPtr, true);
    }
    
    CharacterPosition TextStyle::GetCharacterPosition() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Plumbing_TextStyle_CharacterPosition_get");
        return (CharacterPosition)proc(cPtr);
    }
    
    double TextStyle::GetCharSpacing() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Model_Plumbing_TextStyle_CharSpacing_get");
        return proc(cPtr);
    }
    
    FontPtr TextStyle::GetFont() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Plumbing_TextStyle_GetFont");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<Font>(newCPtr, true);
    }
    
    double TextStyle::GetFontSize() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Model_Plumbing_TextStyle_FontSize_get");
        return proc(cPtr);
    }
    
    int TextStyle::GetFontWeight() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Plumbing_TextStyle_FontWeight_get");
        return proc(cPtr);
    }
    
    bool TextStyle::HasHighlightColor() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_Plumbing_TextStyle_HasHighlightColor_get");
        return 0 != proc(cPtr);
    }
    
    bool TextStyle::HasTextColor() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_Plumbing_TextStyle_HasTextColor_get");
        return 0 != proc(cPtr);
    }
    
    bool TextStyle::HasUnderlineColor() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_Plumbing_TextStyle_HasUnderlineColor_get");
        return 0 != proc(cPtr);
    }
    
    SolidFramework::Interop::ColorPtr TextStyle::GetHighlightColor() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Plumbing_TextStyle_GetHighlightColor");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::Color>(newCPtr, true);
    }
    
    bool TextStyle::GetItalic() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_Plumbing_TextStyle_Italic_get");
        return 0 != proc(cPtr);
    }
    
    bool TextStyle::GetRightToLeft() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_Plumbing_TextStyle_RightToLeft_get");
        return 0 != proc(cPtr);
    }
    
    bool TextStyle::GetSmallCaps() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Model_Plumbing_TextStyle_SmallCaps_get");
        return 0 != proc(cPtr);
    }
    
    SolidFramework::Interop::ColorPtr TextStyle::GetTextColor() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Plumbing_TextStyle_GetTextColor");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::Color>(newCPtr, true);
    }
    
    double TextStyle::GetTextRise() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Model_Plumbing_TextStyle_TextRise_get");
        return proc(cPtr);
    }
    
    double TextStyle::GetTextScale() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Model_Plumbing_TextStyle_TextScale_get");
        return proc(cPtr);
    }
    
    SolidFramework::Interop::ColorPtr TextStyle::GetUnderlineColor() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Plumbing_TextStyle_GetUnderlineColor");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::Color>(newCPtr, true);
    }
    
    BorderLineType TextStyle::GetUnderlineType() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Model_Plumbing_TextStyle_UnderlineType_get");
        return (BorderLineType)proc(cPtr);
    }
    
    double TextStyle::GetWordSpacing() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Model_Plumbing_TextStyle_WordSpacing_get");
        return proc(cPtr);
    }
    
    TextStylePtr TextStyle::DynamicCast(StylePtr value) {
        return std::dynamic_pointer_cast<TextStyle>(value);
    }
    
    TextStyle::TextStyle() {
        // only called from derived class constructors
    }
    

    void TextStyleTemplate::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Plumbing_TextStyleTemplate");
        FrameworkObject::Dispose(destructor);
    }
    
    TextStylePtr TextStyleTemplate::GetStyle() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Model_Plumbing_TextStyleTemplate_GetTextStyle");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<TextStyle>(newCPtr, true);
    }
    
    TextStyleTemplatePtr TextStyleTemplate::DynamicCast(SolidObjectPtr value) {
        return std::dynamic_pointer_cast<TextStyleTemplate>(value);
    }
    
    TextStyleTemplate::TextStyleTemplate() {
        // only called from derived class constructors
    }
    

    void TOC::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Model_Plumbing_TOC");
        FrameworkObject::Dispose(destructor);
    }
    
    TOCPtr TOC::DynamicCast(SolidObjectPtr value) {
        return std::dynamic_pointer_cast<TOC>(value);
    }
    

}}} // SolidFramework::Model::Plumbing

namespace SolidFramework { namespace Pdf { 
    void Catalog::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_Catalog");
        FrameworkObject::Dispose(destructor);
    }
    
    SolidFramework::Plumbing::ICatalogPtr Catalog::Create(PdfDocumentPtr doc) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_Catalog_Create");
        HANDLE newCPtr = proc(SolidFramework::FrameworkObject::GetCPtr(doc));
        return SolidFrameworkNative::MakeICatalog(newCPtr, true);
    }
    
    int Catalog::GetCurrentPage() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Pdf_Catalog_GetCurrentPage");
        return proc(cPtr);
    }
    
    void Catalog::SetCurrentPage(int value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Pdf_Catalog_SetCurrentPage");
        proc(cPtr, value);
    }
    
    Plumbing::PdfDictionaryPtr Catalog::GetDests() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_Catalog_GetDests");
        HANDLE newCPtr = proc(cPtr);
        return SolidFrameworkNative::MakePdfDictionary(newCPtr, true);
    }
    
    bool Catalog::IsTagged() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Pdf_Catalog_IsTagged_get");
        return 0 != proc(cPtr);
    }
    
    Plumbing::PdfDictionaryPtr Catalog::GetNames() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_Catalog_GetNames");
        HANDLE newCPtr = proc(cPtr);
        return SolidFrameworkNative::MakePdfDictionary(newCPtr, true);
    }
    
    Plumbing::PdfItemPtr Catalog::GetOpenAction() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_Catalog_GetOpenAction");
        HANDLE newCPtr = proc(cPtr);
        return SolidFrameworkNative::MakePdfItem(newCPtr, true);
    }
    
    void Catalog::SetOpenAction(Plumbing::PdfItemPtr value) {
        static InvokeType_VHH proc = GetFunction<InvokeType_VHH>("SolidFramework_Pdf_Catalog_SetOpenAction");
        proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(value));
    }
    
    OptionalContentPropertiesPtr Catalog::GetOptionalContentProperties() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_Catalog_GetOptionalContentProperties");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<OptionalContentProperties>(newCPtr, true);
    }
    
    Plumbing::PdfOutlinesPtr Catalog::GetOutlines() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_Catalog_GetOutlines");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<Plumbing::PdfOutlines>(newCPtr, true);
    }
    
    PageLayout Catalog::GetPageLayout() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Pdf_Catalog_PageLayout_get");
        return (PageLayout)proc(cPtr);
    }
    
    void Catalog::SetPageLayout(PageLayout value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Pdf_Catalog_PageLayout_set");
        proc(cPtr, (int)(value));
    }
    
    PageMode Catalog::GetPageMode() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Pdf_Catalog_PageMode_get");
        return (PageMode)proc(cPtr);
    }
    
    void Catalog::SetPageMode(PageMode value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Pdf_Catalog_PageMode_set");
        proc(cPtr, (int)(value));
    }
    
    SolidFramework::Plumbing::IPagesCollectionPtr Catalog::GetPages() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_Catalog_GetPages");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Plumbing::IPagesCollection>(newCPtr, true);
    }
    
    void Catalog::RemoveViewerPreferences() {
        static InvokeType_VH proc = GetFunction<InvokeType_VH>("SolidFramework_Pdf_Catalog_RemoveViewerPreferences");
        proc(cPtr);
    }
    
    std::wstring Catalog::GetType() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Pdf_Catalog_GetType");
        return UnwrapWString(proc(cPtr));
    }
    
    std::wstring Catalog::GetVersion() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Pdf_Catalog_GetVersion");
        return UnwrapWString(proc(cPtr));
    }
    
    ViewerPreferencesPtr Catalog::GetViewerPreferences() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_Catalog_GetViewerPreferences__SWIG_0");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<ViewerPreferences>(newCPtr, true);
    }
    
    ViewerPreferencesPtr Catalog::GetViewerPreferences(bool create) {
        typedef HANDLE (SOLIDCALLBACK *InvokeType)(HANDLE, unsigned int);
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_Pdf_Catalog_GetViewerPreferences__SWIG_1");
        HANDLE newCPtr = proc(cPtr, create);
        return newCPtr == nullptr? nullptr : std::make_shared<ViewerPreferences>(newCPtr, true);
    }
    
    CatalogPtr Catalog::DynamicCast(SolidFramework::Plumbing::ICatalogPtr value) {
        return std::dynamic_pointer_cast<Catalog>(value);
    }
    

    void Info::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_Info");
        FrameworkObject::Dispose(destructor);
    }
    
    std::wstring Info::GetAuthor() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Pdf_Info_GetAuthor");
        return UnwrapWString(proc(cPtr));
    }
    
    void Info::SetAuthor(std::wstring value) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Pdf_Info_SetAuthor");
        proc(cPtr, value.c_str());
    }
    
    InfoPtr Info::Create(PdfDocumentPtr document) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_Info_Create");
        HANDLE newCPtr = proc(SolidFramework::FrameworkObject::GetCPtr(document));
        return newCPtr == nullptr? nullptr : std::make_shared<Info>(newCPtr, true);
    }
    
    Plumbing::PdfDatePtr Info::GetCreationDate() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_Info_GetCreationDate");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<Plumbing::PdfDate>(newCPtr, true);
    }
    
    void Info::SetCreationDate(Plumbing::PdfDatePtr value) {
        static InvokeType_VHH proc = GetFunction<InvokeType_VHH>("SolidFramework_Pdf_Info_SetCreationDate");
        proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(value));
    }
    
    std::wstring Info::GetCreator() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Pdf_Info_GetCreator");
        return UnwrapWString(proc(cPtr));
    }
    
    void Info::SetCreator(std::wstring value) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Pdf_Info_SetCreator");
        proc(cPtr, value.c_str());
    }
    
    std::wstring Info::GetKeywords() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Pdf_Info_GetKeywords");
        return UnwrapWString(proc(cPtr));
    }
    
    void Info::SetKeywords(std::wstring value) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Pdf_Info_SetKeywords");
        proc(cPtr, value.c_str());
    }
    
    Plumbing::PdfDatePtr Info::GetModificationDate() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_Info_GetModificationDate");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<Plumbing::PdfDate>(newCPtr, true);
    }
    
    void Info::SetModificationDate(Plumbing::PdfDatePtr value) {
        static InvokeType_VHH proc = GetFunction<InvokeType_VHH>("SolidFramework_Pdf_Info_SetModificationDate");
        proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(value));
    }
    
    std::wstring Info::GetProducer() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Pdf_Info_GetProducer");
        return UnwrapWString(proc(cPtr));
    }
    
    void Info::SetProducer(std::wstring value) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Pdf_Info_SetProducer");
        proc(cPtr, value.c_str());
    }
    
    std::wstring Info::GetSubject() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Pdf_Info_GetSubject");
        return UnwrapWString(proc(cPtr));
    }
    
    void Info::SetSubject(std::wstring value) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Pdf_Info_SetSubject");
        proc(cPtr, value.c_str());
    }
    
    std::wstring Info::GetTitle() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Pdf_Info_GetTitle");
        return UnwrapWString(proc(cPtr));
    }
    
    void Info::SetTitle(std::wstring value) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Pdf_Info_SetTitle");
        proc(cPtr, value.c_str());
    }
    

    void OptionalContentProperties::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_OptionalContentProperties");
        FrameworkObject::Dispose(destructor);
    }
    
    Plumbing::PdfArrayPtr OptionalContentProperties::GetAlternateConfigurations() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_OptionalContentProperties_GetAlternateConfigurations");
        HANDLE newCPtr = proc(cPtr);
        return SolidFrameworkNative::MakePdfArray(newCPtr, true);
    }
    
    OptionalContentPropertiesPtr OptionalContentProperties::Create(CatalogPtr catalog) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_OptionalContentProperties_Create");
        HANDLE newCPtr = proc(SolidFramework::FrameworkObject::GetCPtr(catalog));
        return newCPtr == nullptr? nullptr : std::make_shared<OptionalContentProperties>(newCPtr, true);
    }
    
    Plumbing::PdfDictionaryPtr OptionalContentProperties::GetDefault() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_OptionalContentProperties_GetDefault");
        HANDLE newCPtr = proc(cPtr);
        return SolidFrameworkNative::MakePdfDictionary(newCPtr, true);
    }
    
    Plumbing::PdfArrayPtr OptionalContentProperties::GetOptionalContentGroups() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_OptionalContentProperties_GetOptionalContentGroups");
        HANDLE newCPtr = proc(cPtr);
        return SolidFrameworkNative::MakePdfArray(newCPtr, true);
    }
    

    PdfCertificateSecurityHandler::PdfCertificateSecurityHandler() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("new_SolidFramework_Pdf_PdfCertificateSecurityHandler");
        HANDLE handle = proc();
        Wrap(handle, true);
    }
    
    void PdfCertificateSecurityHandler::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_PdfCertificateSecurityHandler");
        FrameworkObject::Dispose(destructor);
    }
    
    PdfCertificateSecurityHandlerPtr PdfCertificateSecurityHandler::DynamicCast(PdfSecurityHandlerPtr value) {
        return std::dynamic_pointer_cast<PdfCertificateSecurityHandler>(value);
    }
    

    PdfDocument::PdfDocument() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("new_SolidFramework_Pdf_PdfDocument__SWIG_0");
        HANDLE handle = proc();
        Wrap(handle, true);
    }
    
    PdfDocument::PdfDocument(std::wstring path) {
        static InvokeType_PS proc = GetFunction<InvokeType_PS>("new_SolidFramework_Pdf_PdfDocument__SWIG_1");
        HANDLE handle = proc(path.c_str());
        Wrap(handle, true);
    }
    
    void PdfDocument::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_PdfDocument");
        FrameworkObject::Dispose(destructor);
    }
    
    SolidFramework::Interop::NativeError PdfDocument::Close() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Pdf_PdfDocument_Close");
        return (SolidFramework::Interop::NativeError)proc(cPtr);
    }
    
    SolidFramework::Interop::NativeError PdfDocument::AddTextWatermark(std::wstring strWatermarkText, std::wstring strURL, bool bSerifFont, float angle, bool bFill, int dwColor, bool bStroke, float lineWeight, int zorder, int vpos, int hpos) {
        typedef int (SOLIDCALLBACK *InvokeType)(HANDLE, const wchar_t*, const wchar_t*, unsigned int, float, unsigned int, int, unsigned int, float, int, int, int);
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_Pdf_PdfDocument_AddTextWatermark");
        return (SolidFramework::Interop::NativeError)proc(cPtr, strWatermarkText.c_str(), strURL.c_str(), bSerifFont, angle, bFill, dwColor, bStroke, lineWeight, zorder, vpos, hpos);
    }
    
    void PdfDocument::Append(PdfDocumentPtr pdfDocumentToAppend) {
        static InvokeType_VHH proc = GetFunction<InvokeType_VHH>("SolidFramework_Pdf_PdfDocument_Append");
        proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(pdfDocumentToAppend));
    }
    
    AuthenticationModeType PdfDocument::GetAuthenticationMode() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Pdf_PdfDocument_AuthenticationMode_get");
        return (AuthenticationModeType)proc(cPtr);
    }
    
    SolidFramework::Plumbing::ICatalogPtr PdfDocument::GetCatalog() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_PdfDocument_GetCatalog");
        HANDLE newCPtr = proc(cPtr);
        return SolidFrameworkNative::MakeICatalog(newCPtr, true);
    }
    
    Plumbing::PdfDictionaryPtr PdfDocument::GetCatalogDictionary() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_PdfDocument_GetCatalogDictionary");
        HANDLE newCPtr = proc(cPtr);
        return SolidFrameworkNative::MakePdfDictionary(newCPtr, true);
    }
    
    bool PdfDocument::CheckOwnerPassword(std::wstring password) {
        static InvokeType_UHS proc = GetFunction<InvokeType_UHS>("SolidFramework_Pdf_PdfDocument_CheckOwnerPassword");
        return 0 != proc(cPtr, password.c_str());
    }
    
    bool PdfDocument::CheckUserPassword(std::wstring password) {
        static InvokeType_UHS proc = GetFunction<InvokeType_UHS>("SolidFramework_Pdf_PdfDocument_CheckUserPassword");
        return 0 != proc(cPtr, password.c_str());
    }
    
    void PdfDocument::DecodeAllStreams() {
        static InvokeType_VH proc = GetFunction<InvokeType_VH>("SolidFramework_Pdf_PdfDocument_DecodeAllStreams");
        proc(cPtr);
    }
    
    EncryptionAlgorithm PdfDocument::GetEncryptionAlgorithm() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Pdf_PdfDocument_EncryptionAlgorithm_get");
        return (EncryptionAlgorithm)proc(cPtr);
    }
    
    SolidFramework::Interop::NativeError PdfDocument::EnterOwnerAuthenticationMode(std::wstring ownerPassword) {
        static InvokeType_IHS proc = GetFunction<InvokeType_IHS>("SolidFramework_Pdf_PdfDocument_EnterOwnerAuthenticationMode");
        return (SolidFramework::Interop::NativeError)proc(cPtr, ownerPassword.c_str());
    }
    
    std::wstring PdfDocument::GetExtension() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Pdf_PdfDocument_GetExtension");
        return proc(cPtr);
    }
    
    std::vector<Plumbing::PdfFontPtr> PdfDocument::GetFonts() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_PdfDocument_GetFonts");
        return SolidFrameworkNative::PdfFontsCollectionToVector(proc(cPtr), true);
    }
    
    Plumbing::PdfArrayPtr PdfDocument::GetID() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_PdfDocument_GetID");
        HANDLE newCPtr = proc(cPtr);
        return SolidFrameworkNative::MakePdfArray(newCPtr, true);
    }
    
    Plumbing::PdfArrayPtr PdfDocument::GetIDArray() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_PdfDocument_GetIDArray");
        HANDLE newCPtr = proc(cPtr);
        return SolidFrameworkNative::MakePdfArray(newCPtr, true);
    }
    
    InfoPtr PdfDocument::GetInfo() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_PdfDocument_GetInfo");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<Info>(newCPtr, true);
    }
    
    Plumbing::PdfDictionaryPtr PdfDocument::GetInfoDictionary() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_PdfDocument_GetInfoDictionary");
        HANDLE newCPtr = proc(cPtr);
        return SolidFrameworkNative::MakePdfDictionary(newCPtr, true);
    }
    
    std::wstring PdfDocument::InquireUserPasswordByOwnerPassword(std::wstring ownerPassword) {
        static InvokeType_SHS proc = GetFunction<InvokeType_SHS>("SolidFramework_Pdf_PdfDocument_InquireUserPasswordByOwnerPassword");
        return UnwrapWString(proc(cPtr, ownerPassword.c_str()));
    }
    
    bool PdfDocument::IsLinearized() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Pdf_PdfDocument_IsLinearized_get");
        return 0 != proc(cPtr);
    }
    
    bool PdfDocument::IsModified() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Pdf_PdfDocument_IsModified_get");
        return 0 != proc(cPtr);
    }
    
    HANDLE PdfDocument::NativeDataBase() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_PdfDocument_NativeDataBase");
        return proc(cPtr);
    }
    
    void PdfDocument::SetOutputID(const std::vector<BYTE> & permanentIdentifier, const std::vector<BYTE> & changingIdentifier) {
        typedef void (SOLIDCALLBACK *InvokeType)(HANDLE, HANDLE, HANDLE);
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_Pdf_PdfDocument_SetOutputID");
        proc(cPtr, SolidFrameworkNative::VectorToBytesArray(permanentIdentifier)->GetCPtr(), SolidFrameworkNative::VectorToBytesArray(changingIdentifier)->GetCPtr());
    }
    
    int PdfDocument::GetPageCount() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Pdf_PdfDocument_PageCount_get");
        return proc(cPtr);
    }
    
    std::vector<Plumbing::PdfPagePtr> PdfDocument::GetPages() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_PdfDocument_GetPages__SWIG_0");
        return SolidFrameworkNative::PdfPagesCollectionToVector(proc(cPtr), false);
    }
    
    std::vector<Plumbing::PdfPagePtr> PdfDocument::GetPages(int nFrom, int nTo) {
        static InvokeType_HHII proc = GetFunction<InvokeType_HHII>("SolidFramework_Pdf_PdfDocument_GetPages__SWIG_1");
        return SolidFrameworkNative::PdfPagesCollectionToVector(proc(cPtr, nFrom, nTo), true);
    }
    
    bool PdfDocument::IsPdfLoaded() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Pdf_PdfDocument_IsPdfLoaded");
        return 0 != proc(cPtr);
    }
    
    AccessPermissions PdfDocument::GetPermissions() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Pdf_PdfDocument_Permissions_get");
        return (AccessPermissions)proc(cPtr);
    }
    
    SolidFramework::Interop::NativeError PdfDocument::RecognizeDocument(std::wstring OCRLang, SolidFramework::Converters::Plumbing::TextRecovery textRecovery, SolidFramework::Converters::Plumbing::OcrType ocrType, bool pageAutoRotate, SolidFramework::Imaging::Plumbing::ImageCompression imageCompression) {
        typedef int (SOLIDCALLBACK *InvokeType)(HANDLE, const wchar_t*, int, int, unsigned int, int);
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_Pdf_PdfDocument_RecognizeDocument");
        return (SolidFramework::Interop::NativeError)proc(cPtr, OCRLang.c_str(), (int)(textRecovery), (int)(ocrType), pageAutoRotate, (int)(imageCompression));
    }
    
    void PdfDocument::RemoveStructTreeRoot() {
        static InvokeType_VH proc = GetFunction<InvokeType_VH>("SolidFramework_Pdf_PdfDocument_RemoveStructTreeRoot");
        proc(cPtr);
    }
    
    Plumbing::PdfDictionaryPtr PdfDocument::GetRoot() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_PdfDocument_GetRoot");
        HANDLE newCPtr = proc(cPtr);
        return SolidFrameworkNative::MakePdfDictionary(newCPtr, true);
    }
    
    void PdfDocument::Save(SolidFramework::Plumbing::OverwriteMode mode, SolidFramework::PageRangePtr pageRange) {
        static InvokeType_VHIH proc = GetFunction<InvokeType_VHIH>("SolidFramework_Pdf_PdfDocument_Save");
        proc(cPtr, (int)(mode), SolidFramework::FrameworkObject::GetCPtr(pageRange));
    }
    
    void PdfDocument::SaveAsProtected(std::wstring path, PdfSecurityHandlerPtr securityHandler, SolidFramework::Plumbing::OverwriteMode overwriteMode, bool shouldOptimize) {
        typedef void (SOLIDCALLBACK *InvokeType)(HANDLE, const wchar_t*, HANDLE, int, unsigned int);
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_Pdf_PdfDocument_SaveAsProtected__SWIG_0");
        proc(cPtr, path.c_str(), SolidFramework::FrameworkObject::GetCPtr(securityHandler), (int)(overwriteMode), shouldOptimize);
    }
    
    void PdfDocument::SaveAsProtected(std::wstring path, PdfSecurityHandlerPtr securityHandler, SolidFramework::Plumbing::OverwriteMode overwriteMode) {
        typedef void (SOLIDCALLBACK *InvokeType)(HANDLE, const wchar_t*, HANDLE, int);
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_Pdf_PdfDocument_SaveAsProtected__SWIG_1");
        proc(cPtr, path.c_str(), SolidFramework::FrameworkObject::GetCPtr(securityHandler), (int)(overwriteMode));
    }
    
    void PdfDocument::SaveAsProtected(std::wstring path, PdfSecurityHandlerPtr securityHandler) {
        static InvokeType_VHSH proc = GetFunction<InvokeType_VHSH>("SolidFramework_Pdf_PdfDocument_SaveAsProtected__SWIG_2");
        proc(cPtr, path.c_str(), SolidFramework::FrameworkObject::GetCPtr(securityHandler));
    }
    
    void PdfDocument::SaveProtected(PdfSecurityHandlerPtr securityHandler, SolidFramework::Plumbing::OverwriteMode overwriteMode, bool shouldOptimize) {
        typedef void (SOLIDCALLBACK *InvokeType)(HANDLE, HANDLE, int, unsigned int);
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_Pdf_PdfDocument_SaveProtected__SWIG_0");
        proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(securityHandler), (int)(overwriteMode), shouldOptimize);
    }
    
    void PdfDocument::SaveProtected(PdfSecurityHandlerPtr securityHandler, SolidFramework::Plumbing::OverwriteMode overwriteMode) {
        typedef void (SOLIDCALLBACK *InvokeType)(HANDLE, HANDLE, int);
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_Pdf_PdfDocument_SaveProtected__SWIG_1");
        proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(securityHandler), (int)(overwriteMode));
    }
    
    void PdfDocument::SaveProtected(PdfSecurityHandlerPtr securityHandler) {
        static InvokeType_VHH proc = GetFunction<InvokeType_VHH>("SolidFramework_Pdf_PdfDocument_SaveProtected__SWIG_2");
        proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(securityHandler));
    }
    
    Plumbing::PdfDictionaryPtr PdfDocument::GetTrailer() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_PdfDocument_GetTrailer");
        HANDLE newCPtr = proc(cPtr);
        return SolidFrameworkNative::MakePdfDictionary(newCPtr, true);
    }
    
    std::wstring PdfDocument::GetVersion() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Pdf_PdfDocument_GetVersion");
        return UnwrapWString(proc(cPtr));
    }
    
    PdfDocumentPtr PdfDocument::DynamicCast(SolidFramework::Plumbing::DocumentPtr value) {
        return std::dynamic_pointer_cast<PdfDocument>(value);
    }
    

    PdfPasswordSecurityHandler::PdfPasswordSecurityHandler() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("new_SolidFramework_Pdf_PdfPasswordSecurityHandler");
        HANDLE handle = proc();
        Wrap(handle, true);
    }
    
    void PdfPasswordSecurityHandler::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_PdfPasswordSecurityHandler");
        FrameworkObject::Dispose(destructor);
    }
    
    EncryptionAlgorithm PdfPasswordSecurityHandler::GetEncryptionAlgorithm() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Pdf_PdfPasswordSecurityHandler_EncryptionAlgorithm_get");
        return (EncryptionAlgorithm)proc(cPtr);
    }
    
    void PdfPasswordSecurityHandler::SetEncryptionAlgorithm(EncryptionAlgorithm value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Pdf_PdfPasswordSecurityHandler_EncryptionAlgorithm_set");
        proc(cPtr, (int)(value));
    }
    
    std::wstring PdfPasswordSecurityHandler::GetOpenPassword() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Pdf_PdfPasswordSecurityHandler_GetOpenPassword");
        return proc(cPtr);
    }
    
    void PdfPasswordSecurityHandler::SetOpenPassword(std::wstring value) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Pdf_PdfPasswordSecurityHandler_SetOpenPassword");
        proc(cPtr, value.c_str());
    }
    
    std::wstring PdfPasswordSecurityHandler::GetOwnerPassword() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Pdf_PdfPasswordSecurityHandler_GetOwnerPassword");
        return proc(cPtr);
    }
    
    void PdfPasswordSecurityHandler::SetOwnerPassword(std::wstring value) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Pdf_PdfPasswordSecurityHandler_SetOwnerPassword");
        proc(cPtr, value.c_str());
    }
    
    AccessPermissions PdfPasswordSecurityHandler::GetPermissions() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Pdf_PdfPasswordSecurityHandler_Permissions_get");
        return (AccessPermissions)proc(cPtr);
    }
    
    void PdfPasswordSecurityHandler::SetPermissions(AccessPermissions value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Pdf_PdfPasswordSecurityHandler_Permissions_set");
        proc(cPtr, (int)(value));
    }
    
    PdfPasswordSecurityHandlerPtr PdfPasswordSecurityHandler::DynamicCast(PdfSecurityHandlerPtr value) {
        return std::dynamic_pointer_cast<PdfPasswordSecurityHandler>(value);
    }
    

    void PdfSecurityHandler::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_PdfSecurityHandler");
        FrameworkObject::Dispose(destructor);
    }
    
    EncryptionAlgorithm PdfSecurityHandler::GetEncryptionAlgorithm() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Pdf_PdfSecurityHandler_EncryptionAlgorithm_get");
        return (EncryptionAlgorithm)proc(cPtr);
    }
    
    bool PdfSecurityHandler::IsOwner() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Pdf_PdfSecurityHandler_IsOwner_get");
        return 0 != proc(cPtr);
    }
    
    AccessPermissions PdfSecurityHandler::GetPermissions() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Pdf_PdfSecurityHandler_Permissions_get");
        return (AccessPermissions)proc(cPtr);
    }
    
    PdfSecurityHandler::PdfSecurityHandler() {
        // only called from derived class constructors
    }
    

    void ViewerPreferences::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_ViewerPreferences");
        FrameworkObject::Dispose(destructor);
    }
    
    bool ViewerPreferences::GetCenterWindow() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Pdf_ViewerPreferences_CenterWindow_get");
        return 0 != proc(cPtr);
    }
    
    void ViewerPreferences::SetCenterWindow(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Pdf_ViewerPreferences_CenterWindow_set");
        proc(cPtr, value);
    }
    
    ViewerPreferencesPtr ViewerPreferences::Create(CatalogPtr catalog) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_ViewerPreferences_Create");
        HANDLE newCPtr = proc(SolidFramework::FrameworkObject::GetCPtr(catalog));
        return newCPtr == nullptr? nullptr : std::make_shared<ViewerPreferences>(newCPtr, true);
    }
    
    bool ViewerPreferences::GetDisplayDocumentTitle() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Pdf_ViewerPreferences_DisplayDocumentTitle_get");
        return 0 != proc(cPtr);
    }
    
    void ViewerPreferences::SetDisplayDocumentTitle(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Pdf_ViewerPreferences_DisplayDocumentTitle_set");
        proc(cPtr, value);
    }
    
    Duplex ViewerPreferences::GetDuplex() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Pdf_ViewerPreferences_Duplex_get");
        return (Duplex)proc(cPtr);
    }
    
    void ViewerPreferences::SetDuplex(Duplex value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Pdf_ViewerPreferences_Duplex_set");
        proc(cPtr, (int)(value));
    }
    
    bool ViewerPreferences::GetFitWindow() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Pdf_ViewerPreferences_FitWindow_get");
        return 0 != proc(cPtr);
    }
    
    void ViewerPreferences::SetFitWindow(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Pdf_ViewerPreferences_FitWindow_set");
        proc(cPtr, value);
    }
    
    bool ViewerPreferences::GetHideMenuBar() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Pdf_ViewerPreferences_HideMenuBar_get");
        return 0 != proc(cPtr);
    }
    
    void ViewerPreferences::SetHideMenuBar(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Pdf_ViewerPreferences_HideMenuBar_set");
        proc(cPtr, value);
    }
    
    bool ViewerPreferences::GetHideToolBar() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Pdf_ViewerPreferences_HideToolBar_get");
        return 0 != proc(cPtr);
    }
    
    void ViewerPreferences::SetHideToolBar(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Pdf_ViewerPreferences_HideToolBar_set");
        proc(cPtr, value);
    }
    
    bool ViewerPreferences::GetHideWindowUI() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Pdf_ViewerPreferences_HideWindowUI_get");
        return 0 != proc(cPtr);
    }
    
    void ViewerPreferences::SetHideWindowUI(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Pdf_ViewerPreferences_HideWindowUI_set");
        proc(cPtr, value);
    }
    
    NonFullScreenPageMode ViewerPreferences::GetNonFullScreenPageMode() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Pdf_ViewerPreferences_NonFullScreenPageMode_get");
        return (NonFullScreenPageMode)proc(cPtr);
    }
    
    void ViewerPreferences::SetNonFullScreenPageMode(NonFullScreenPageMode value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Pdf_ViewerPreferences_NonFullScreenPageMode_set");
        proc(cPtr, (int)(value));
    }
    
    int ViewerPreferences::GetNumberOfCopies() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Pdf_ViewerPreferences_NumberOfCopies_get");
        return proc(cPtr);
    }
    
    void ViewerPreferences::SetNumberOfCopies(int value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Pdf_ViewerPreferences_NumberOfCopies_set");
        proc(cPtr, value);
    }
    
    bool ViewerPreferences::GetPickTrayByPdfSize() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Pdf_ViewerPreferences_PickTrayByPdfSize_get");
        return 0 != proc(cPtr);
    }
    
    void ViewerPreferences::SetPickTrayByPdfSize(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Pdf_ViewerPreferences_PickTrayByPdfSize_set");
        proc(cPtr, value);
    }
    
    PageBoundary ViewerPreferences::GetPrintArea() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Pdf_ViewerPreferences_PrintArea_get");
        return (PageBoundary)proc(cPtr);
    }
    
    void ViewerPreferences::SetPrintArea(PageBoundary value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Pdf_ViewerPreferences_PrintArea_set");
        proc(cPtr, (int)(value));
    }
    
    PageBoundary ViewerPreferences::GetPrintClip() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Pdf_ViewerPreferences_PrintClip_get");
        return (PageBoundary)proc(cPtr);
    }
    
    void ViewerPreferences::SetPrintClip(PageBoundary value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Pdf_ViewerPreferences_PrintClip_set");
        proc(cPtr, (int)(value));
    }
    
    SolidFramework::PageRangePtr ViewerPreferences::GetPrintPageRange() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_ViewerPreferences_GetPrintPageRange");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::PageRange>(newCPtr, true);
    }
    
    void ViewerPreferences::SetPrintPageRange(SolidFramework::PageRangePtr value) {
        static InvokeType_VHH proc = GetFunction<InvokeType_VHH>("SolidFramework_Pdf_ViewerPreferences_SetPrintPageRange");
        proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(value));
    }
    
    PrintScaling ViewerPreferences::GetPrintScaling() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Pdf_ViewerPreferences_PrintScaling_get");
        return (PrintScaling)proc(cPtr);
    }
    
    void ViewerPreferences::SetPrintScaling(PrintScaling value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Pdf_ViewerPreferences_PrintScaling_set");
        proc(cPtr, (int)(value));
    }
    
    ReadingDirection ViewerPreferences::GetReadingDirection() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Pdf_ViewerPreferences_ReadingDirection_get");
        return (ReadingDirection)proc(cPtr);
    }
    
    void ViewerPreferences::SetReadingDirection(ReadingDirection value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Pdf_ViewerPreferences_ReadingDirection_set");
        proc(cPtr, (int)(value));
    }
    
    PageBoundary ViewerPreferences::GetViewArea() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Pdf_ViewerPreferences_ViewArea_get");
        return (PageBoundary)proc(cPtr);
    }
    
    void ViewerPreferences::SetViewArea(PageBoundary value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Pdf_ViewerPreferences_ViewArea_set");
        proc(cPtr, (int)(value));
    }
    
    PageBoundary ViewerPreferences::GetViewClip() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Pdf_ViewerPreferences_ViewClip_get");
        return (PageBoundary)proc(cPtr);
    }
    
    void ViewerPreferences::SetViewClip(PageBoundary value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Pdf_ViewerPreferences_ViewClip_set");
        proc(cPtr, (int)(value));
    }
    

}} // SolidFramework::Pdf

namespace SolidFramework { namespace Pdf { namespace Plumbing { 
    PdfAction::PdfAction(SolidFramework::Pdf::PdfDocumentPtr pdfDocument) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("new_SolidFramework_Pdf_Plumbing_PdfAction");
        HANDLE handle = proc(SolidFramework::FrameworkObject::GetCPtr(pdfDocument));
        Wrap(handle, true);
    }
    
    void PdfAction::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_Plumbing_PdfAction");
        FrameworkObject::Dispose(destructor);
    }
    
    PdfItemPtr PdfAction::GetD() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_Plumbing_PdfAction_GetD");
        HANDLE newCPtr = proc(cPtr);
        return SolidFrameworkNative::MakePdfItem(newCPtr, true);
    }
    
    std::wstring PdfAction::GetS() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Pdf_Plumbing_PdfAction_GetS");
        return UnwrapWString(proc(cPtr));
    }
    
    PdfActionPtr PdfAction::DynamicCast(PdfItemPtr value) {
        return std::dynamic_pointer_cast<PdfAction>(value);
    }
    
    PdfAction::PdfAction() {
        // only called from derived class constructors
    }
    

    PdfAnnot::PdfAnnot(SolidFramework::Pdf::PdfDocumentPtr pdfDocument) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("new_SolidFramework_Pdf_Plumbing_PdfAnnot");
        HANDLE handle = proc(SolidFramework::FrameworkObject::GetCPtr(pdfDocument));
        Wrap(handle, true);
    }
    
    void PdfAnnot::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_Plumbing_PdfAnnot");
        FrameworkObject::Dispose(destructor);
    }
    
    std::wstring PdfAnnot::GetContents() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Pdf_Plumbing_PdfAnnot_GetContents");
        return UnwrapWString(proc(cPtr));
    }
    
    std::wstring PdfAnnot::GetModified() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Pdf_Plumbing_PdfAnnot_GetModified");
        return UnwrapWString(proc(cPtr));
    }
    
    PdfObjectPtr PdfAnnot::GetParentAnnotationObj() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_Plumbing_PdfAnnot_GetParentAnnotationObj");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<PdfObject>(newCPtr, true);
    }
    
    SolidFramework::Interop::RectangleFPtr PdfAnnot::GetRect() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_Plumbing_PdfAnnot_GetRect");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::RectangleF>(newCPtr, true);
    }
    
    std::wstring PdfAnnot::GetSubtype() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Pdf_Plumbing_PdfAnnot_GetSubtype");
        return UnwrapWString(proc(cPtr));
    }
    
    PdfAnnotPtr PdfAnnot::DynamicCast(PdfItemPtr value) {
        return std::dynamic_pointer_cast<PdfAnnot>(value);
    }
    
    PdfAnnot::PdfAnnot() {
        // only called from derived class constructors
    }
    

    PdfArray::PdfArray(SolidFramework::Pdf::PdfDocumentPtr pdfDocument) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("new_SolidFramework_Pdf_Plumbing_PdfArray");
        HANDLE handle = proc(SolidFramework::FrameworkObject::GetCPtr(pdfDocument));
        Wrap(handle, true);
    }
    
    void PdfArray::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_Plumbing_PdfArray");
        FrameworkObject::Dispose(destructor);
    }
    
    int PdfArray::Add(PdfItemPtr value) {
        static InvokeType_IHH proc = GetFunction<InvokeType_IHH>("SolidFramework_Pdf_Plumbing_PdfArray_Add__SWIG_0");
        return proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(value));
    }
    
    void PdfArray::Add(PdfItemPtr value, int index) {
        typedef void (SOLIDCALLBACK *InvokeType)(HANDLE, HANDLE, int);
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_Pdf_Plumbing_PdfArray_Add__SWIG_1");
        proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(value), index);
    }
    
    void PdfArray::Clear() {
        static InvokeType_VH proc = GetFunction<InvokeType_VH>("SolidFramework_Pdf_Plumbing_PdfArray_Clear");
        proc(cPtr);
    }
    
    int PdfArray::GetCount() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Pdf_Plumbing_PdfArray_Count_get");
        return proc(cPtr);
    }
    
    bool PdfArray::IsFixedSize() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Pdf_Plumbing_PdfArray_IsFixedSize");
        return 0 != proc(cPtr);
    }
    
    PdfItemPtr PdfArray::GetItem(int index) {
        static InvokeType_HHI proc = GetFunction<InvokeType_HHI>("SolidFramework_Pdf_Plumbing_PdfArray_Get");
        HANDLE newCPtr = proc(cPtr, index);
        return SolidFrameworkNative::MakePdfItem(newCPtr, true);
    }
    
    std::vector<PdfItemPtr> PdfArray::GetPdfArrayItemsCollection() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_Plumbing_PdfArray_GetPdfArrayItemsCollection");
        return SolidFrameworkNative::PdfArrayItemsCollectionToVector(proc(cPtr), true);
    }
    
    bool PdfArray::IsReadOnly() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Pdf_Plumbing_PdfArray_IsReadOnly");
        return 0 != proc(cPtr);
    }
    
    bool PdfArray::IsSynchronized() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Pdf_Plumbing_PdfArray_IsSynchronized");
        return 0 != proc(cPtr);
    }
    
    std::wstring PdfArray::ToString() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Pdf_Plumbing_PdfArray_ToString");
        return UnwrapWString(proc(cPtr));
    }
    
    PdfArrayPtr PdfArray::DynamicCast(PdfItemPtr value) {
        return std::dynamic_pointer_cast<PdfArray>(value);
    }
    
    PdfArray::PdfArray() {
        // only called from derived class constructors
    }
    

    PdfBoolean::PdfBoolean(SolidFramework::Pdf::PdfDocumentPtr pdfDocument) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("new_SolidFramework_Pdf_Plumbing_PdfBoolean__SWIG_1");
        HANDLE handle = proc(SolidFramework::FrameworkObject::GetCPtr(pdfDocument));
        Wrap(handle, true);
    }
    
    void PdfBoolean::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_Plumbing_PdfBoolean");
        FrameworkObject::Dispose(destructor);
    }
    
    std::wstring PdfBoolean::ToString() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Pdf_Plumbing_PdfBoolean_ToString");
        return UnwrapWString(proc(cPtr));
    }
    
    bool PdfBoolean::GetValue() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Pdf_Plumbing_PdfBoolean_Value_get");
        return 0 != proc(cPtr);
    }
    
    void PdfBoolean::SetValue(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Pdf_Plumbing_PdfBoolean_Value_set");
        proc(cPtr, value);
    }
    
    PdfBooleanPtr PdfBoolean::DynamicCast(PdfItemPtr value) {
        return std::dynamic_pointer_cast<PdfBoolean>(value);
    }
    

    PdfCaretAnnot::PdfCaretAnnot(SolidFramework::Pdf::PdfDocumentPtr pdfDocument) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("new_SolidFramework_Pdf_Plumbing_PdfCaretAnnot");
        HANDLE handle = proc(SolidFramework::FrameworkObject::GetCPtr(pdfDocument));
        Wrap(handle, true);
    }
    
    void PdfCaretAnnot::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_Plumbing_PdfCaretAnnot");
        FrameworkObject::Dispose(destructor);
    }
    
    PdfCaretAnnotPtr PdfCaretAnnot::DynamicCast(PdfItemPtr value) {
        return std::dynamic_pointer_cast<PdfCaretAnnot>(value);
    }
    

    PdfCircleAnnot::PdfCircleAnnot(SolidFramework::Pdf::PdfDocumentPtr pdfDocument) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("new_SolidFramework_Pdf_Plumbing_PdfCircleAnnot");
        HANDLE handle = proc(SolidFramework::FrameworkObject::GetCPtr(pdfDocument));
        Wrap(handle, true);
    }
    
    void PdfCircleAnnot::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_Plumbing_PdfCircleAnnot");
        FrameworkObject::Dispose(destructor);
    }
    
    PdfCircleAnnotPtr PdfCircleAnnot::DynamicCast(PdfItemPtr value) {
        return std::dynamic_pointer_cast<PdfCircleAnnot>(value);
    }
    

    PdfDate::PdfDate() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("new_SolidFramework_Pdf_Plumbing_PdfDate__SWIG_0");
        HANDLE handle = proc();
        Wrap(handle, true);
    }
    
    PdfDate::PdfDate(SolidFramework::Interop::DateTimePtr date) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("new_SolidFramework_Pdf_Plumbing_PdfDate__SWIG_1");
        HANDLE handle = proc(SolidFramework::FrameworkObject::GetCPtr(date));
        Wrap(handle, true);
    }
    
    PdfDate::PdfDate(std::wstring date) {
        static InvokeType_PS proc = GetFunction<InvokeType_PS>("new_SolidFramework_Pdf_Plumbing_PdfDate__SWIG_2");
        HANDLE handle = proc(date.c_str());
        Wrap(handle, true);
    }
    
    void PdfDate::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_Plumbing_PdfDate");
        FrameworkObject::Dispose(destructor);
    }
    
    SolidFramework::Interop::DateTimePtr PdfDate::GetDateTime() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_Plumbing_PdfDate_GetDateTime");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::DateTime>(newCPtr, true);
    }
    
    void PdfDate::SetDateTime(SolidFramework::Interop::DateTimePtr value) {
        static InvokeType_VHH proc = GetFunction<InvokeType_VHH>("SolidFramework_Pdf_Plumbing_PdfDate_SetDateTime");
        proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(value));
    }
    
    bool PdfDate::IsValid() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Pdf_Plumbing_PdfDate_IsValid_get");
        return 0 != proc(cPtr);
    }
    
    PdfDatePtr PdfDate::Parse(std::wstring date) {
        static InvokeType_PS proc = GetFunction<InvokeType_PS>("SolidFramework_Pdf_Plumbing_PdfDate_Parse");
        HANDLE newCPtr = proc(date.c_str());
        return newCPtr == nullptr? nullptr : std::make_shared<PdfDate>(newCPtr, true);
    }
    
    std::wstring PdfDate::ToString() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Pdf_Plumbing_PdfDate_ToString");
        return UnwrapWString(proc(cPtr));
    }
    
    std::wstring PdfDate::ToStringXmp() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Pdf_Plumbing_PdfDate_ToStringXmp");
        return UnwrapWString(proc(cPtr));
    }
    
    bool PdfDate::TryParse(std::wstring date, PdfDatePtr& result) {
        static InvokeType_PS proc = GetFunction<InvokeType_PS>("SolidFramework_Pdf_Plumbing_PdfDate_SafeParse");
        HANDLE newCPtr = proc(date.c_str());
        result = nullptr;
        if (newCPtr != nullptr) {
            result = PdfDatePtr(new PdfDate(newCPtr, true));
        }
        return result != nullptr;
    }
    
    SolidFramework::Interop::TimeSpanPtr PdfDate::GetUtcOffset() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_Plumbing_PdfDate_GetUtcOffset");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::TimeSpan>(newCPtr, true);
    }
    
    void PdfDate::SetUtcOffset(SolidFramework::Interop::TimeSpanPtr value) {
        static InvokeType_VHH proc = GetFunction<InvokeType_VHH>("SolidFramework_Pdf_Plumbing_PdfDate_SetUtcOffset");
        proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(value));
    }
    
    bool PdfDate::GetUtcOffsetDefined() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Pdf_Plumbing_PdfDate_GetUtcOffsetDefined");
        return 0 != proc(cPtr);
    }
    
    void PdfDate::SetUtcOffsetDefined(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Pdf_Plumbing_PdfDate_SetUtcOffsetDefined");
        proc(cPtr, value);
    }
    

    PdfDictionary::PdfDictionary(SolidFramework::Pdf::PdfDocumentPtr pdfDocument) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("new_SolidFramework_Pdf_Plumbing_PdfDictionary");
        HANDLE handle = proc(SolidFramework::FrameworkObject::GetCPtr(pdfDocument));
        Wrap(handle, true);
    }
    
    void PdfDictionary::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_Plumbing_PdfDictionary");
        FrameworkObject::Dispose(destructor);
    }
    
    void PdfDictionary::Add(std::wstring key, PdfItemPtr value) {
        static InvokeType_VHSH proc = GetFunction<InvokeType_VHSH>("SolidFramework_Pdf_Plumbing_PdfDictionary_Add");
        proc(cPtr, key.c_str(), SolidFramework::FrameworkObject::GetCPtr(value));
    }
    
    void PdfDictionary::Clear() {
        static InvokeType_VH proc = GetFunction<InvokeType_VH>("SolidFramework_Pdf_Plumbing_PdfDictionary_Clear");
        proc(cPtr);
    }
    
    bool PdfDictionary::ClearMetadata() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Pdf_Plumbing_PdfDictionary_ClearMetadata");
        return 0 != proc(cPtr);
    }
    
    bool PdfDictionary::ContainsKey(std::wstring key) {
        static InvokeType_UHS proc = GetFunction<InvokeType_UHS>("SolidFramework_Pdf_Plumbing_PdfDictionary_ContainsKey");
        return 0 != proc(cPtr, key.c_str());
    }
    
    int PdfDictionary::GetCount() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Pdf_Plumbing_PdfDictionary_Count_get");
        return proc(cPtr);
    }
    
    PdfItemPtr PdfDictionary::GetDictionaryValueByIndex(int i) {
        static InvokeType_HHI proc = GetFunction<InvokeType_HHI>("SolidFramework_Pdf_Plumbing_PdfDictionary_GetDictionaryValueByIndex");
        HANDLE newCPtr = proc(cPtr, i);
        return SolidFrameworkNative::MakePdfItem(newCPtr, true);
    }
    
    PdfItemPtr PdfDictionary::GetItem(std::wstring key) {
        static InvokeType_HHS proc = GetFunction<InvokeType_HHS>("SolidFramework_Pdf_Plumbing_PdfDictionary_GetDictionaryValue");
        HANDLE newCPtr = proc(cPtr, key.c_str());
        return SolidFrameworkNative::MakePdfItem(newCPtr, true);
    }
    
    PdfDictionaryItemsCollectionPtr PdfDictionary::GetPdfDictionaryItemsCollection() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_Plumbing_PdfDictionary_GetPdfDictionaryItemsCollection");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<PdfDictionaryItemsCollection>(newCPtr, true);
    }
    
    std::wstring PdfDictionary::ToString() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Pdf_Plumbing_PdfDictionary_ToString");
        return UnwrapWString(proc(cPtr));
    }
    
    std::wstring PdfDictionary::GetType() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Pdf_Plumbing_PdfDictionary_GetType");
        return UnwrapWString(proc(cPtr));
    }
    
    PdfDictionaryPtr PdfDictionary::DynamicCast(PdfItemPtr value) {
        return std::dynamic_pointer_cast<PdfDictionary>(value);
    }
    
    PdfDictionary::PdfDictionary() {
        // only called from derived class constructors
    }
    

    PdfDictionaryItemsCollection::PdfDictionaryItemsCollection() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("new_SolidFramework_Pdf_Plumbing_PdfDictionaryItemsCollection__SWIG_0");
        HANDLE handle = proc();
        Wrap(handle, true);
    }
    
    PdfDictionaryItemsCollection::PdfDictionaryItemsCollection(PdfDictionaryItemsCollectionPtr other) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("new_SolidFramework_Pdf_Plumbing_PdfDictionaryItemsCollection__SWIG_1");
        HANDLE handle = proc(SolidFramework::FrameworkObject::GetCPtr(other));
        Wrap(handle, true);
    }
    
    void PdfDictionaryItemsCollection::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_Plumbing_PdfDictionaryItemsCollection");
        FrameworkObject::Dispose(destructor);
    }
    
    void PdfDictionaryItemsCollection::Add(std::wstring key, PdfItemPtr val) {
        static InvokeType_VHSH proc = GetFunction<InvokeType_VHSH>("SolidFramework_Pdf_Plumbing_PdfDictionaryItemsCollection_Add");
        proc(cPtr, key.c_str(), SolidFramework::FrameworkObject::GetCPtr(val));
    }
    
    void PdfDictionaryItemsCollection::Clear() {
        static InvokeType_VH proc = GetFunction<InvokeType_VH>("SolidFramework_Pdf_Plumbing_PdfDictionaryItemsCollection_Clear");
        proc(cPtr);
    }
    
    bool PdfDictionaryItemsCollection::ContainsKey(std::wstring key) {
        static InvokeType_UHS proc = GetFunction<InvokeType_UHS>("SolidFramework_Pdf_Plumbing_PdfDictionaryItemsCollection_ContainsKey");
        return 0 != proc(cPtr, key.c_str());
    }
    
    int PdfDictionaryItemsCollection::GetCount() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Pdf_Plumbing_PdfDictionaryItemsCollection_size");
        return (int)proc(cPtr);
    }
    
    bool PdfDictionaryItemsCollection::Empty() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Pdf_Plumbing_PdfDictionaryItemsCollection_empty");
        return 0 != proc(cPtr);
    }
    
    PdfItemPtr PdfDictionaryItemsCollection::GetItem(std::wstring key) {
        static InvokeType_HHS proc = GetFunction<InvokeType_HHS>("SolidFramework_Pdf_Plumbing_PdfDictionaryItemsCollection_getitem");
        HANDLE newCPtr = proc(cPtr, key.c_str());
        return SolidFrameworkNative::MakePdfItem(newCPtr, true);
    }
    
    bool PdfDictionaryItemsCollection::Remove(std::wstring key) {
        static InvokeType_UHS proc = GetFunction<InvokeType_UHS>("SolidFramework_Pdf_Plumbing_PdfDictionaryItemsCollection_Remove");
        return 0 != proc(cPtr, key.c_str());
    }
    

    PdfFileAttachmentAnnot::PdfFileAttachmentAnnot(SolidFramework::Pdf::PdfDocumentPtr pdfDocument) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("new_SolidFramework_Pdf_Plumbing_PdfFileAttachmentAnnot");
        HANDLE handle = proc(SolidFramework::FrameworkObject::GetCPtr(pdfDocument));
        Wrap(handle, true);
    }
    
    void PdfFileAttachmentAnnot::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_Plumbing_PdfFileAttachmentAnnot");
        FrameworkObject::Dispose(destructor);
    }
    
    PdfFileAttachmentAnnotPtr PdfFileAttachmentAnnot::DynamicCast(PdfItemPtr value) {
        return std::dynamic_pointer_cast<PdfFileAttachmentAnnot>(value);
    }
    

    PdfFont::PdfFont(SolidFramework::Pdf::PdfDocumentPtr pdfDocument) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("new_SolidFramework_Pdf_Plumbing_PdfFont");
        HANDLE handle = proc(SolidFramework::FrameworkObject::GetCPtr(pdfDocument));
        Wrap(handle, true);
    }
    
    void PdfFont::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_Plumbing_PdfFont");
        FrameworkObject::Dispose(destructor);
    }
    
    std::wstring PdfFont::GetBaseFontName() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Pdf_Plumbing_PdfFont_GetBaseFontName");
        return UnwrapWString(proc(cPtr));
    }
    
    bool PdfFont::IsBuiltIn() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Pdf_Plumbing_PdfFont_IsBuiltIn");
        return 0 != proc(cPtr);
    }
    
    bool PdfFont::IsEmbedded() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Pdf_Plumbing_PdfFont_IsEmbedded");
        return 0 != proc(cPtr);
    }
    
    FontStatus PdfFont::GetPdfFontStatus() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Pdf_Plumbing_PdfFont_PdfFontStatus_get");
        return (FontStatus)proc(cPtr);
    }
    
    FontType PdfFont::GetPdfFontType() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Pdf_Plumbing_PdfFont_PdfFontType_get");
        return (FontType)proc(cPtr);
    }
    
    bool PdfFont::IsPredefinedEncoding() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Pdf_Plumbing_PdfFont_IsPredefinedEncoding");
        return 0 != proc(cPtr);
    }
    
    bool PdfFont::IsSubset() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Pdf_Plumbing_PdfFont_IsSubset");
        return 0 != proc(cPtr);
    }
    
    bool PdfFont::IsSymbolic() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Pdf_Plumbing_PdfFont_IsSymbolic");
        return 0 != proc(cPtr);
    }
    
    bool PdfFont::HasToUnicode() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Pdf_Plumbing_PdfFont_HasToUnicode");
        return 0 != proc(cPtr);
    }
    
    PdfFontPtr PdfFont::DynamicCast(PdfItemPtr value) {
        return std::dynamic_pointer_cast<PdfFont>(value);
    }
    

    PdfFreeTextAnnot::PdfFreeTextAnnot(SolidFramework::Pdf::PdfDocumentPtr pdfDocument) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("new_SolidFramework_Pdf_Plumbing_PdfFreeTextAnnot");
        HANDLE handle = proc(SolidFramework::FrameworkObject::GetCPtr(pdfDocument));
        Wrap(handle, true);
    }
    
    void PdfFreeTextAnnot::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_Plumbing_PdfFreeTextAnnot");
        FrameworkObject::Dispose(destructor);
    }
    
    PdfFreeTextAnnotPtr PdfFreeTextAnnot::DynamicCast(PdfItemPtr value) {
        return std::dynamic_pointer_cast<PdfFreeTextAnnot>(value);
    }
    

    PdfGoTo::PdfGoTo(SolidFramework::Pdf::PdfDocumentPtr pdfDocument) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("new_SolidFramework_Pdf_Plumbing_PdfGoTo");
        HANDLE handle = proc(SolidFramework::FrameworkObject::GetCPtr(pdfDocument));
        Wrap(handle, true);
    }
    
    void PdfGoTo::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_Plumbing_PdfGoTo");
        FrameworkObject::Dispose(destructor);
    }
    
    PdfGoToPtr PdfGoTo::DynamicCast(PdfItemPtr value) {
        return std::dynamic_pointer_cast<PdfGoTo>(value);
    }
    

    PdfHighlightAnnot::PdfHighlightAnnot(SolidFramework::Pdf::PdfDocumentPtr pdfDocument) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("new_SolidFramework_Pdf_Plumbing_PdfHighlightAnnot");
        HANDLE handle = proc(SolidFramework::FrameworkObject::GetCPtr(pdfDocument));
        Wrap(handle, true);
    }
    
    void PdfHighlightAnnot::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_Plumbing_PdfHighlightAnnot");
        FrameworkObject::Dispose(destructor);
    }
    
    PdfHighlightAnnotPtr PdfHighlightAnnot::DynamicCast(PdfItemPtr value) {
        return std::dynamic_pointer_cast<PdfHighlightAnnot>(value);
    }
    

    PdfInkAnnot::PdfInkAnnot(SolidFramework::Pdf::PdfDocumentPtr pdfDocument) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("new_SolidFramework_Pdf_Plumbing_PdfInkAnnot");
        HANDLE handle = proc(SolidFramework::FrameworkObject::GetCPtr(pdfDocument));
        Wrap(handle, true);
    }
    
    void PdfInkAnnot::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_Plumbing_PdfInkAnnot");
        FrameworkObject::Dispose(destructor);
    }
    
    PdfInkAnnotPtr PdfInkAnnot::DynamicCast(PdfItemPtr value) {
        return std::dynamic_pointer_cast<PdfInkAnnot>(value);
    }
    

    void PdfItem::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_Plumbing_PdfItem");
        FrameworkObject::Dispose(destructor);
    }
    
    HANDLE PdfItem::GetHandle() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_Plumbing_PdfItem_GetHandle");
        return proc(cPtr);
    }
    
    PdfItemPtr PdfItem::GetIndirectionItem(PdfItemPtr item) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_Plumbing_PdfItem_GetIndirectionItem__SWIG_0");
        HANDLE newCPtr = proc(SolidFramework::FrameworkObject::GetCPtr(item));
        return SolidFrameworkNative::MakePdfItem(newCPtr, true);
    }
    
    PdfItemPtr PdfItem::GetIndirectionItem(PdfItemPtr item, std::wstring forceType) {
        static InvokeType_HHS proc = GetFunction<InvokeType_HHS>("SolidFramework_Pdf_Plumbing_PdfItem_GetIndirectionItem__SWIG_1");
        HANDLE newCPtr = proc(SolidFramework::FrameworkObject::GetCPtr(item), forceType.c_str());
        return SolidFrameworkNative::MakePdfItem(newCPtr, true);
    }
    
    PdfObjectPtr PdfItem::GetIndirectionObject(PdfItemPtr item) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_Plumbing_PdfItem_GetIndirectionObject");
        HANDLE newCPtr = proc(SolidFramework::FrameworkObject::GetCPtr(item));
        return newCPtr == nullptr? nullptr : std::make_shared<PdfObject>(newCPtr, true);
    }
    
    PdfObjectType PdfItem::GetPdfObjectType() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Pdf_Plumbing_PdfItem_PdfObjectType_get");
        return (PdfObjectType)proc(cPtr);
    }
    
    std::wstring PdfItem::ToString() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Pdf_Plumbing_PdfItem_ToString");
        return UnwrapWString(proc(cPtr));
    }
    
    PdfItem::PdfItem() {
        // only called from derived class constructors
    }
    

    PdfKids::PdfKids(SolidFramework::Pdf::PdfDocumentPtr pdfDocument) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("new_SolidFramework_Pdf_Plumbing_PdfKids");
        HANDLE handle = proc(SolidFramework::FrameworkObject::GetCPtr(pdfDocument));
        Wrap(handle, true);
    }
    
    void PdfKids::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_Plumbing_PdfKids");
        FrameworkObject::Dispose(destructor);
    }
    
    PdfKidsPtr PdfKids::DynamicCast(PdfItemPtr value) {
        return std::dynamic_pointer_cast<PdfKids>(value);
    }
    

    PdfLineAnnot::PdfLineAnnot(SolidFramework::Pdf::PdfDocumentPtr pdfDocument) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("new_SolidFramework_Pdf_Plumbing_PdfLineAnnot");
        HANDLE handle = proc(SolidFramework::FrameworkObject::GetCPtr(pdfDocument));
        Wrap(handle, true);
    }
    
    void PdfLineAnnot::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_Plumbing_PdfLineAnnot");
        FrameworkObject::Dispose(destructor);
    }
    
    PdfLineAnnotPtr PdfLineAnnot::DynamicCast(PdfItemPtr value) {
        return std::dynamic_pointer_cast<PdfLineAnnot>(value);
    }
    

    PdfLink::PdfLink(SolidFramework::Pdf::PdfDocumentPtr pdfDocument) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("new_SolidFramework_Pdf_Plumbing_PdfLink");
        HANDLE handle = proc(SolidFramework::FrameworkObject::GetCPtr(pdfDocument));
        Wrap(handle, true);
    }
    
    void PdfLink::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_Plumbing_PdfLink");
        FrameworkObject::Dispose(destructor);
    }
    
    PdfActionPtr PdfLink::GetA() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_Plumbing_PdfLink_GetA");
        HANDLE newCPtr = proc(cPtr);
        return SolidFrameworkNative::MakePdfAction(newCPtr, true);
    }
    
    PdfItemPtr PdfLink::GetDest() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_Plumbing_PdfLink_GetDest");
        HANDLE newCPtr = proc(cPtr);
        return SolidFrameworkNative::MakePdfItem(newCPtr, true);
    }
    
    PdfLinkPtr PdfLink::DynamicCast(PdfItemPtr value) {
        return std::dynamic_pointer_cast<PdfLink>(value);
    }
    

    PdfMarkupAnnot::PdfMarkupAnnot(SolidFramework::Pdf::PdfDocumentPtr pdfDocument) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("new_SolidFramework_Pdf_Plumbing_PdfMarkupAnnot");
        HANDLE handle = proc(SolidFramework::FrameworkObject::GetCPtr(pdfDocument));
        Wrap(handle, true);
    }
    
    void PdfMarkupAnnot::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_Plumbing_PdfMarkupAnnot");
        FrameworkObject::Dispose(destructor);
    }
    
    std::wstring PdfMarkupAnnot::GetCreationDate() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Pdf_Plumbing_PdfMarkupAnnot_GetCreationDate");
        return UnwrapWString(proc(cPtr));
    }
    
    PdfObjectPtr PdfMarkupAnnot::GetParentAnnotationObj() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_Plumbing_PdfMarkupAnnot_GetParentAnnotationObj");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<PdfObject>(newCPtr, true);
    }
    
    std::wstring PdfMarkupAnnot::GetRC() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Pdf_Plumbing_PdfMarkupAnnot_GetRC");
        return UnwrapWString(proc(cPtr));
    }
    
    std::wstring PdfMarkupAnnot::GetSubj() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Pdf_Plumbing_PdfMarkupAnnot_GetSubj");
        return UnwrapWString(proc(cPtr));
    }
    
    std::wstring PdfMarkupAnnot::GetT() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Pdf_Plumbing_PdfMarkupAnnot_GetT");
        return UnwrapWString(proc(cPtr));
    }
    
    PdfMarkupAnnotPtr PdfMarkupAnnot::DynamicCast(PdfItemPtr value) {
        return std::dynamic_pointer_cast<PdfMarkupAnnot>(value);
    }
    
    PdfMarkupAnnot::PdfMarkupAnnot() {
        // only called from derived class constructors
    }
    

    PdfName::PdfName(SolidFramework::Pdf::PdfDocumentPtr pdfDocument) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("new_SolidFramework_Pdf_Plumbing_PdfName");
        HANDLE handle = proc(SolidFramework::FrameworkObject::GetCPtr(pdfDocument));
        Wrap(handle, true);
    }
    
    void PdfName::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_Plumbing_PdfName");
        FrameworkObject::Dispose(destructor);
    }
    
    std::wstring PdfName::ToString() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Pdf_Plumbing_PdfName_ToString");
        return UnwrapWString(proc(cPtr));
    }
    
    std::wstring PdfName::GetValue() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Pdf_Plumbing_PdfName_GetValue");
        return UnwrapWString(proc(cPtr));
    }
    
    void PdfName::SetValue(std::wstring value) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Pdf_Plumbing_PdfName_SetValue");
        proc(cPtr, value.c_str());
    }
    
    PdfNamePtr PdfName::DynamicCast(PdfItemPtr value) {
        return std::dynamic_pointer_cast<PdfName>(value);
    }
    

    PdfNamedDestinationsCollection::PdfNamedDestinationsCollection() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("new_SolidFramework_Pdf_Plumbing_PdfNamedDestinationsCollection__SWIG_0");
        HANDLE handle = proc();
        Wrap(handle, true);
    }
    
    PdfNamedDestinationsCollection::PdfNamedDestinationsCollection(PdfNamedDestinationsCollectionPtr other) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("new_SolidFramework_Pdf_Plumbing_PdfNamedDestinationsCollection__SWIG_1");
        HANDLE handle = proc(SolidFramework::FrameworkObject::GetCPtr(other));
        Wrap(handle, true);
    }
    
    void PdfNamedDestinationsCollection::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_Plumbing_PdfNamedDestinationsCollection");
        FrameworkObject::Dispose(destructor);
    }
    
    void PdfNamedDestinationsCollection::Add(std::wstring key, PdfArrayPtr val) {
        static InvokeType_VHSH proc = GetFunction<InvokeType_VHSH>("SolidFramework_Pdf_Plumbing_PdfNamedDestinationsCollection_Add");
        proc(cPtr, key.c_str(), SolidFramework::FrameworkObject::GetCPtr(val));
    }
    
    void PdfNamedDestinationsCollection::Clear() {
        static InvokeType_VH proc = GetFunction<InvokeType_VH>("SolidFramework_Pdf_Plumbing_PdfNamedDestinationsCollection_Clear");
        proc(cPtr);
    }
    
    bool PdfNamedDestinationsCollection::ContainsKey(std::wstring key) {
        static InvokeType_UHS proc = GetFunction<InvokeType_UHS>("SolidFramework_Pdf_Plumbing_PdfNamedDestinationsCollection_ContainsKey");
        return 0 != proc(cPtr, key.c_str());
    }
    
    int PdfNamedDestinationsCollection::GetCount() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Pdf_Plumbing_PdfNamedDestinationsCollection_size");
        return (int)proc(cPtr);
    }
    
    bool PdfNamedDestinationsCollection::Empty() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Pdf_Plumbing_PdfNamedDestinationsCollection_empty");
        return 0 != proc(cPtr);
    }
    
    PdfArrayPtr PdfNamedDestinationsCollection::GetItem(std::wstring key) {
        static InvokeType_HHS proc = GetFunction<InvokeType_HHS>("SolidFramework_Pdf_Plumbing_PdfNamedDestinationsCollection_getitem");
        HANDLE newCPtr = proc(cPtr, key.c_str());
        return SolidFrameworkNative::MakePdfArray(newCPtr, true);
    }
    
    bool PdfNamedDestinationsCollection::Remove(std::wstring key) {
        static InvokeType_UHS proc = GetFunction<InvokeType_UHS>("SolidFramework_Pdf_Plumbing_PdfNamedDestinationsCollection_Remove");
        return 0 != proc(cPtr, key.c_str());
    }
    

    PdfNull::PdfNull(SolidFramework::Pdf::PdfDocumentPtr pdfDocument) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("new_SolidFramework_Pdf_Plumbing_PdfNull");
        HANDLE handle = proc(SolidFramework::FrameworkObject::GetCPtr(pdfDocument));
        Wrap(handle, true);
    }
    
    void PdfNull::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_Plumbing_PdfNull");
        FrameworkObject::Dispose(destructor);
    }
    
    std::wstring PdfNull::ToString() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Pdf_Plumbing_PdfNull_ToString");
        return UnwrapWString(proc(cPtr));
    }
    
    PdfNullPtr PdfNull::DynamicCast(PdfItemPtr value) {
        return std::dynamic_pointer_cast<PdfNull>(value);
    }
    

    PdfNumber::PdfNumber(SolidFramework::Pdf::PdfDocumentPtr pdfDocument) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("new_SolidFramework_Pdf_Plumbing_PdfNumber");
        HANDLE handle = proc(SolidFramework::FrameworkObject::GetCPtr(pdfDocument));
        Wrap(handle, true);
    }
    
    void PdfNumber::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_Plumbing_PdfNumber");
        FrameworkObject::Dispose(destructor);
    }
    
    std::wstring PdfNumber::ToString() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Pdf_Plumbing_PdfNumber_ToString");
        return UnwrapWString(proc(cPtr));
    }
    
    double PdfNumber::GetValue() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Pdf_Plumbing_PdfNumber_Value_get");
        return proc(cPtr);
    }
    
    void PdfNumber::SetValue(double value) {
        static InvokeType_VHD proc = GetFunction<InvokeType_VHD>("SolidFramework_Pdf_Plumbing_PdfNumber_Value_set");
        proc(cPtr, value);
    }
    
    PdfNumberPtr PdfNumber::DynamicCast(PdfItemPtr value) {
        return std::dynamic_pointer_cast<PdfNumber>(value);
    }
    

    PdfObject::PdfObject(SolidFramework::Pdf::PdfDocumentPtr pdfDocument) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("new_SolidFramework_Pdf_Plumbing_PdfObject");
        HANDLE handle = proc(SolidFramework::FrameworkObject::GetCPtr(pdfDocument));
        Wrap(handle, true);
    }
    
    void PdfObject::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_Plumbing_PdfObject");
        FrameworkObject::Dispose(destructor);
    }
    
    int PdfObject::GetGenerationId() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Pdf_Plumbing_PdfObject_GenerationId_get");
        return proc(cPtr);
    }
    
    PdfItemPtr PdfObject::GetItem() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_Plumbing_PdfObject_GetItem");
        HANDLE newCPtr = proc(cPtr);
        return SolidFrameworkNative::MakePdfItem(newCPtr, true);
    }
    
    int PdfObject::GetObjectId() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Pdf_Plumbing_PdfObject_ObjectId_get");
        return proc(cPtr);
    }
    
    std::wstring PdfObject::ToString() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Pdf_Plumbing_PdfObject_ToString");
        return UnwrapWString(proc(cPtr));
    }
    
    PdfObjectPtr PdfObject::DynamicCast(PdfItemPtr value) {
        return std::dynamic_pointer_cast<PdfObject>(value);
    }
    

    PdfOutlineItem::PdfOutlineItem(SolidFramework::Pdf::PdfDocumentPtr pdfDocument) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("new_SolidFramework_Pdf_Plumbing_PdfOutlineItem");
        HANDLE handle = proc(SolidFramework::FrameworkObject::GetCPtr(pdfDocument));
        Wrap(handle, true);
    }
    
    void PdfOutlineItem::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_Plumbing_PdfOutlineItem");
        FrameworkObject::Dispose(destructor);
    }
    
    PdfActionPtr PdfOutlineItem::GetA() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_Plumbing_PdfOutlineItem_GetA");
        HANDLE newCPtr = proc(cPtr);
        return SolidFrameworkNative::MakePdfAction(newCPtr, true);
    }
    
    PdfItemPtr PdfOutlineItem::GetDest() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_Plumbing_PdfOutlineItem_GetDest");
        HANDLE newCPtr = proc(cPtr);
        return SolidFrameworkNative::MakePdfItem(newCPtr, true);
    }
    
    PdfOutlineItemPtr PdfOutlineItem::GetFirst() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_Plumbing_PdfOutlineItem_GetFirst");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<PdfOutlineItem>(newCPtr, true);
    }
    
    PdfOutlineItemPtr PdfOutlineItem::GetLast() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_Plumbing_PdfOutlineItem_GetLast");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<PdfOutlineItem>(newCPtr, true);
    }
    
    PdfOutlineItemPtr PdfOutlineItem::GetNext() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_Plumbing_PdfOutlineItem_GetNext");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<PdfOutlineItem>(newCPtr, true);
    }
    
    int PdfOutlineItem::GetOpenItems() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Pdf_Plumbing_PdfOutlineItem_OpenItems_get");
        return proc(cPtr);
    }
    
    PdfItemPtr PdfOutlineItem::GetParent() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_Plumbing_PdfOutlineItem_GetParent");
        HANDLE newCPtr = proc(cPtr);
        return SolidFrameworkNative::MakePdfItem(newCPtr, true);
    }
    
    PdfOutlineItemPtr PdfOutlineItem::GetPrev() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_Plumbing_PdfOutlineItem_GetPrev");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<PdfOutlineItem>(newCPtr, true);
    }
    
    std::wstring PdfOutlineItem::GetTitle() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Pdf_Plumbing_PdfOutlineItem_GetTitle");
        return UnwrapWString(proc(cPtr));
    }
    
    PdfOutlineItemPtr PdfOutlineItem::DynamicCast(PdfItemPtr value) {
        return std::dynamic_pointer_cast<PdfOutlineItem>(value);
    }
    

    PdfOutlines::PdfOutlines(SolidFramework::Pdf::PdfDocumentPtr pdfDocument) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("new_SolidFramework_Pdf_Plumbing_PdfOutlines");
        HANDLE handle = proc(SolidFramework::FrameworkObject::GetCPtr(pdfDocument));
        Wrap(handle, true);
    }
    
    void PdfOutlines::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_Plumbing_PdfOutlines");
        FrameworkObject::Dispose(destructor);
    }
    
    PdfOutlineItemPtr PdfOutlines::GetFirst() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_Plumbing_PdfOutlines_GetFirst");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<PdfOutlineItem>(newCPtr, true);
    }
    
    PdfOutlineItemPtr PdfOutlines::GetLast() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_Plumbing_PdfOutlines_GetLast");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<PdfOutlineItem>(newCPtr, true);
    }
    
    int PdfOutlines::GetOpenItems() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Pdf_Plumbing_PdfOutlines_OpenItems_get");
        return proc(cPtr);
    }
    
    PdfOutlinesPtr PdfOutlines::DynamicCast(PdfItemPtr value) {
        return std::dynamic_pointer_cast<PdfOutlines>(value);
    }
    

    PdfPage::PdfPage(SolidFramework::Pdf::PdfDocumentPtr pdfDocument) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("new_SolidFramework_Pdf_Plumbing_PdfPage");
        HANDLE handle = proc(SolidFramework::FrameworkObject::GetCPtr(pdfDocument));
        Wrap(handle, true);
    }
    
    void PdfPage::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_Plumbing_PdfPage");
        FrameworkObject::Dispose(destructor);
    }
    
    PdfArrayPtr PdfPage::GetAnnots() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_Plumbing_PdfPage_GetAnnots");
        HANDLE newCPtr = proc(cPtr);
        return SolidFrameworkNative::MakePdfArray(newCPtr, true);
    }
    
    void PdfPage::SetAnnots(PdfArrayPtr value) {
        static InvokeType_VHH proc = GetFunction<InvokeType_VHH>("SolidFramework_Pdf_Plumbing_PdfPage_SetAnnots");
        proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(value));
    }
    
    PdfRectanglePtr PdfPage::GetArtBox() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_Plumbing_PdfPage_GetArtBox");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<PdfRectangle>(newCPtr, true);
    }
    
    int PdfPage::AutoRotate() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Pdf_Plumbing_PdfPage_AutoRotate");
        return proc(cPtr);
    }
    
    PdfRectanglePtr PdfPage::GetBleedBox() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_Plumbing_PdfPage_GetBleedBox");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<PdfRectangle>(newCPtr, true);
    }
    
    int PdfPage::BuildLayout() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Pdf_Plumbing_PdfPage_BuildLayout");
        return proc(cPtr);
    }
    
    bool PdfPage::CanSelectTextWithRect(SolidFramework::Interop::RectangleFPtr selectionRect) {
        static InvokeType_UHH proc = GetFunction<InvokeType_UHH>("SolidFramework_Pdf_Plumbing_PdfPage_CanSelectTextWithRect");
        return 0 != proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(selectionRect));
    }
    
    PdfItemPtr PdfPage::GetContents() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_Plumbing_PdfPage_GetContents");
        HANDLE newCPtr = proc(cPtr);
        return SolidFrameworkNative::MakePdfItem(newCPtr, true);
    }
    
    PdfRectanglePtr PdfPage::GetCropBox() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_Plumbing_PdfPage_GetCropBox");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<PdfRectangle>(newCPtr, true);
    }
    
    void PdfPage::SetCropBox(PdfRectanglePtr value) {
        static InvokeType_VHH proc = GetFunction<InvokeType_VHH>("SolidFramework_Pdf_Plumbing_PdfPage_SetCropBox");
        proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(value));
    }
    
    double PdfPage::Deskew() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Pdf_Plumbing_PdfPage_Deskew");
        return proc(cPtr);
    }
    
    SolidFramework::Interop::BitmapPtr PdfPage::DrawBitmap(int nDpi) {
        static InvokeType_HHI proc = GetFunction<InvokeType_HHI>("SolidFramework_Pdf_Plumbing_PdfPage_DrawBitmap__SWIG_0");
        HANDLE newCPtr = proc(cPtr, nDpi);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::Bitmap>(newCPtr, true);
    }
    
    SolidFramework::Interop::BitmapPtr PdfPage::DrawBitmap(int nDpi, SolidFramework::Pdf::Interop::DrawingCore drawingCore, SolidFramework::Pdf::Interop::CacheDrawingType cache) {
        static InvokeType_HHIII proc = GetFunction<InvokeType_HHIII>("SolidFramework_Pdf_Plumbing_PdfPage_DrawBitmap__SWIG_1");
        HANDLE newCPtr = proc(cPtr, nDpi, (int)(drawingCore), (int)(cache));
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::Bitmap>(newCPtr, true);
    }
    
    bool PdfPage::HasInvisibleText() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Pdf_Plumbing_PdfPage_HasInvisibleText_get");
        return 0 != proc(cPtr);
    }
    
    bool PdfPage::IsScanned() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Pdf_Plumbing_PdfPage_IsScanned_get");
        return 0 != proc(cPtr);
    }
    
    bool PdfPage::IsVectorText() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Pdf_Plumbing_PdfPage_IsVectorText_get");
        return 0 != proc(cPtr);
    }
    
    PdfDatePtr PdfPage::GetLastModified() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_Plumbing_PdfPage_GetLastModified");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<PdfDate>(newCPtr, true);
    }
    
    int PdfPage::LoadPage() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Pdf_Plumbing_PdfPage_LoadPage");
        return proc(cPtr);
    }
    
    PdfRectanglePtr PdfPage::GetMediaBox() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_Plumbing_PdfPage_GetMediaBox");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<PdfRectangle>(newCPtr, true);
    }
    
    void PdfPage::SetMediaBox(PdfRectanglePtr value) {
        static InvokeType_VHH proc = GetFunction<InvokeType_VHH>("SolidFramework_Pdf_Plumbing_PdfPage_SetMediaBox");
        proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(value));
    }
    
    PdfReferencePtr PdfPage::GetOriginalReference() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_Plumbing_PdfPage_GetOriginalReference");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<PdfReference>(newCPtr, true);
    }
    
    PageStatistics PdfPage::GetPageStatistics() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Pdf_Plumbing_PdfPage_GetPageStatistics");
        return (PageStatistics)proc(cPtr);
    }
    
    PdfPagesPtr PdfPage::GetParent() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_Plumbing_PdfPage_GetParent");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<PdfPages>(newCPtr, true);
    }
    
    int PdfPage::ResetLayout() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Pdf_Plumbing_PdfPage_ResetLayout");
        return proc(cPtr);
    }
    
    PdfDictionaryPtr PdfPage::GetResources() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_Plumbing_PdfPage_GetResources");
        HANDLE newCPtr = proc(cPtr);
        return SolidFrameworkNative::MakePdfDictionary(newCPtr, true);
    }
    
    int PdfPage::GetRotation() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Pdf_Plumbing_PdfPage_Rotation_get");
        return proc(cPtr);
    }
    
    void PdfPage::SetRotation(int value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Pdf_Plumbing_PdfPage_Rotation_set");
        proc(cPtr, value);
    }
    
    SolidFramework::Imaging::ImagePtr PdfPage::GetScannedImage() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_Plumbing_PdfPage_GetScannedImage");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Imaging::Image>(newCPtr, true);
    }
    
    int PdfPage::SearchText(std::wstring searchText, bool matchCase, bool matchWholeWord) {
        typedef int (SOLIDCALLBACK *InvokeType)(HANDLE, const wchar_t*, unsigned int, unsigned int);
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_Pdf_Plumbing_PdfPage_SearchText");
        return proc(cPtr, searchText.c_str(), matchCase, matchWholeWord);
    }
    
    std::wstring PdfPage::GetSelectedText() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Pdf_Plumbing_PdfPage_GetSelectedText");
        return UnwrapWString(proc(cPtr));
    }
    
    bool PdfPage::SelectText(SolidFramework::Interop::RectangleFPtr selectionRect, bool resetPreviousSelection) {
        static InvokeType_UHHU proc = GetFunction<InvokeType_UHHU>("SolidFramework_Pdf_Plumbing_PdfPage_SelectText");
        return 0 != proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(selectionRect), resetPreviousSelection);
    }
    
    bool PdfPage::SelectTextWithRect(SolidFramework::Interop::RectangleFPtr selectionRect, bool resetPreviousSelection) {
        static InvokeType_UHHU proc = GetFunction<InvokeType_UHHU>("SolidFramework_Pdf_Plumbing_PdfPage_SelectTextWithRect");
        return 0 != proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(selectionRect), resetPreviousSelection);
    }
    
    SolidFramework::Interop::RectangleFPtr PdfPage::SetTextSelection(int index) {
        static InvokeType_HHI proc = GetFunction<InvokeType_HHI>("SolidFramework_Pdf_Plumbing_PdfPage_SetTextSelection");
        HANDLE newCPtr = proc(cPtr, index);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::RectangleF>(newCPtr, true);
    }
    
    PdfRectanglePtr PdfPage::GetTrimBox() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_Plumbing_PdfPage_GetTrimBox");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<PdfRectangle>(newCPtr, true);
    }
    
    double PdfPage::GetUserUnit() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Pdf_Plumbing_PdfPage_UserUnit_get");
        return proc(cPtr);
    }
    
    void PdfPage::SetUserUnit(double value) {
        static InvokeType_VHD proc = GetFunction<InvokeType_VHD>("SolidFramework_Pdf_Plumbing_PdfPage_UserUnit_set");
        proc(cPtr, value);
    }
    
    SolidFramework::Interop::RectangleFPtr PdfPage::GetVisibleBox() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_Plumbing_PdfPage_GetVisibleBox");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::RectangleF>(newCPtr, true);
    }
    
    PdfPagePtr PdfPage::DynamicCast(PdfItemPtr value) {
        return std::dynamic_pointer_cast<PdfPage>(value);
    }
    

    PdfPages::PdfPages(SolidFramework::Pdf::PdfDocumentPtr pdfDocument) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("new_SolidFramework_Pdf_Plumbing_PdfPages");
        HANDLE handle = proc(SolidFramework::FrameworkObject::GetCPtr(pdfDocument));
        Wrap(handle, true);
    }
    
    void PdfPages::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_Plumbing_PdfPages");
        FrameworkObject::Dispose(destructor);
    }
    
    PdfRectanglePtr PdfPages::GetArtBox() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_Plumbing_PdfPages_GetArtBox");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<PdfRectangle>(newCPtr, true);
    }
    
    PdfRectanglePtr PdfPages::GetBleedBox() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_Plumbing_PdfPages_GetBleedBox");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<PdfRectangle>(newCPtr, true);
    }
    
    PdfRectanglePtr PdfPages::GetCropBox() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_Plumbing_PdfPages_GetCropBox");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<PdfRectangle>(newCPtr, true);
    }
    
    PdfKidsPtr PdfPages::GetKids() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_Plumbing_PdfPages_GetKids");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<PdfKids>(newCPtr, true);
    }
    
    PdfRectanglePtr PdfPages::GetMediaBox() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_Plumbing_PdfPages_GetMediaBox");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<PdfRectangle>(newCPtr, true);
    }
    
    int PdfPages::GetPageCount() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Pdf_Plumbing_PdfPages_PageCount_get");
        return proc(cPtr);
    }
    
    PdfPagesPtr PdfPages::GetParent() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_Plumbing_PdfPages_GetParent");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<PdfPages>(newCPtr, true);
    }
    
    PdfDictionaryPtr PdfPages::GetResources() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_Plumbing_PdfPages_GetResources");
        HANDLE newCPtr = proc(cPtr);
        return SolidFrameworkNative::MakePdfDictionary(newCPtr, true);
    }
    
    int PdfPages::GetRotation() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Pdf_Plumbing_PdfPages_GetRotation");
        return proc(cPtr);
    }
    
    PdfRectanglePtr PdfPages::GetTrimBox() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_Plumbing_PdfPages_GetTrimBox");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<PdfRectangle>(newCPtr, true);
    }
    
    PdfPagesPtr PdfPages::DynamicCast(PdfItemPtr value) {
        return std::dynamic_pointer_cast<PdfPages>(value);
    }
    
    PdfPagesPtr PdfPages::DynamicCast(SolidFramework::Plumbing::IPagesCollectionPtr value) {
        return std::dynamic_pointer_cast<PdfPages>(value);
    }
    

    PdfPolygonAnnot::PdfPolygonAnnot(SolidFramework::Pdf::PdfDocumentPtr pdfDocument) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("new_SolidFramework_Pdf_Plumbing_PdfPolygonAnnot");
        HANDLE handle = proc(SolidFramework::FrameworkObject::GetCPtr(pdfDocument));
        Wrap(handle, true);
    }
    
    void PdfPolygonAnnot::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_Plumbing_PdfPolygonAnnot");
        FrameworkObject::Dispose(destructor);
    }
    
    PdfPolygonAnnotPtr PdfPolygonAnnot::DynamicCast(PdfItemPtr value) {
        return std::dynamic_pointer_cast<PdfPolygonAnnot>(value);
    }
    

    PdfPolyLineAnnot::PdfPolyLineAnnot(SolidFramework::Pdf::PdfDocumentPtr pdfDocument) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("new_SolidFramework_Pdf_Plumbing_PdfPolyLineAnnot");
        HANDLE handle = proc(SolidFramework::FrameworkObject::GetCPtr(pdfDocument));
        Wrap(handle, true);
    }
    
    void PdfPolyLineAnnot::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_Plumbing_PdfPolyLineAnnot");
        FrameworkObject::Dispose(destructor);
    }
    
    PdfPolyLineAnnotPtr PdfPolyLineAnnot::DynamicCast(PdfItemPtr value) {
        return std::dynamic_pointer_cast<PdfPolyLineAnnot>(value);
    }
    

    PdfPopupAnnot::PdfPopupAnnot(SolidFramework::Pdf::PdfDocumentPtr pdfDocument) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("new_SolidFramework_Pdf_Plumbing_PdfPopupAnnot");
        HANDLE handle = proc(SolidFramework::FrameworkObject::GetCPtr(pdfDocument));
        Wrap(handle, true);
    }
    
    void PdfPopupAnnot::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_Plumbing_PdfPopupAnnot");
        FrameworkObject::Dispose(destructor);
    }
    
    PdfObjectPtr PdfPopupAnnot::GetParentAnnotationObj() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_Plumbing_PdfPopupAnnot_GetParentAnnotationObj");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<PdfObject>(newCPtr, true);
    }
    
    PdfPopupAnnotPtr PdfPopupAnnot::DynamicCast(PdfItemPtr value) {
        return std::dynamic_pointer_cast<PdfPopupAnnot>(value);
    }
    

    PdfRectangle::PdfRectangle(SolidFramework::Pdf::PdfDocumentPtr pdfDocument) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("new_SolidFramework_Pdf_Plumbing_PdfRectangle__SWIG_0");
        HANDLE handle = proc(SolidFramework::FrameworkObject::GetCPtr(pdfDocument));
        Wrap(handle, true);
    }
    
    PdfRectangle::PdfRectangle(SolidFramework::Pdf::PdfDocumentPtr pdfDocument, double x, double y, double width, double height) {
        typedef HANDLE (SOLIDCALLBACK *InvokeType)(HANDLE, double, double, double, double);
        static InvokeType proc = GetFunction<InvokeType>("new_SolidFramework_Pdf_Plumbing_PdfRectangle__SWIG_1");
        HANDLE handle = proc(SolidFramework::FrameworkObject::GetCPtr(pdfDocument), x, y, width, height);
        Wrap(handle, true);
    }
    
    void PdfRectangle::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_Plumbing_PdfRectangle");
        FrameworkObject::Dispose(destructor);
    }
    
    double PdfRectangle::GetBottom() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Pdf_Plumbing_PdfRectangle_Bottom_get");
        return proc(cPtr);
    }
    
    double PdfRectangle::GetLeft() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Pdf_Plumbing_PdfRectangle_Left_get");
        return proc(cPtr);
    }
    
    double PdfRectangle::GetRight() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Pdf_Plumbing_PdfRectangle_Right_get");
        return proc(cPtr);
    }
    
    double PdfRectangle::GetTop() {
        static InvokeType_DH proc = GetFunction<InvokeType_DH>("SolidFramework_Pdf_Plumbing_PdfRectangle_Top_get");
        return proc(cPtr);
    }
    
    PdfRectanglePtr PdfRectangle::DynamicCast(PdfItemPtr value) {
        return std::dynamic_pointer_cast<PdfRectangle>(value);
    }
    

    PdfReference::PdfReference(SolidFramework::Pdf::PdfDocumentPtr pdfDocument) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("new_SolidFramework_Pdf_Plumbing_PdfReference");
        HANDLE handle = proc(SolidFramework::FrameworkObject::GetCPtr(pdfDocument));
        Wrap(handle, true);
    }
    
    void PdfReference::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_Plumbing_PdfReference");
        FrameworkObject::Dispose(destructor);
    }
    
    int PdfReference::GetGenerationId() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Pdf_Plumbing_PdfReference_GenerationId_get");
        return proc(cPtr);
    }
    
    void PdfReference::SetGenerationId(int value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Pdf_Plumbing_PdfReference_GenerationId_set");
        proc(cPtr, value);
    }
    
    PdfObjectPtr PdfReference::GetObject() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_Plumbing_PdfReference_GetObject");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<PdfObject>(newCPtr, true);
    }
    
    int PdfReference::GetObjectId() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Pdf_Plumbing_PdfReference_ObjectId_get");
        return proc(cPtr);
    }
    
    void PdfReference::SetObjectId(int value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Pdf_Plumbing_PdfReference_ObjectId_set");
        proc(cPtr, value);
    }
    
    std::wstring PdfReference::ToString() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Pdf_Plumbing_PdfReference_ToString");
        return UnwrapWString(proc(cPtr));
    }
    
    PdfReferencePtr PdfReference::DynamicCast(PdfItemPtr value) {
        return std::dynamic_pointer_cast<PdfReference>(value);
    }
    

    PdfSoundAnnot::PdfSoundAnnot(SolidFramework::Pdf::PdfDocumentPtr pdfDocument) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("new_SolidFramework_Pdf_Plumbing_PdfSoundAnnot");
        HANDLE handle = proc(SolidFramework::FrameworkObject::GetCPtr(pdfDocument));
        Wrap(handle, true);
    }
    
    void PdfSoundAnnot::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_Plumbing_PdfSoundAnnot");
        FrameworkObject::Dispose(destructor);
    }
    
    PdfSoundAnnotPtr PdfSoundAnnot::DynamicCast(PdfItemPtr value) {
        return std::dynamic_pointer_cast<PdfSoundAnnot>(value);
    }
    

    PdfSquareAnnot::PdfSquareAnnot(SolidFramework::Pdf::PdfDocumentPtr pdfDocument) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("new_SolidFramework_Pdf_Plumbing_PdfSquareAnnot");
        HANDLE handle = proc(SolidFramework::FrameworkObject::GetCPtr(pdfDocument));
        Wrap(handle, true);
    }
    
    void PdfSquareAnnot::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_Plumbing_PdfSquareAnnot");
        FrameworkObject::Dispose(destructor);
    }
    
    PdfSquareAnnotPtr PdfSquareAnnot::DynamicCast(PdfItemPtr value) {
        return std::dynamic_pointer_cast<PdfSquareAnnot>(value);
    }
    

    PdfSquigglyAnnot::PdfSquigglyAnnot(SolidFramework::Pdf::PdfDocumentPtr pdfDocument) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("new_SolidFramework_Pdf_Plumbing_PdfSquigglyAnnot");
        HANDLE handle = proc(SolidFramework::FrameworkObject::GetCPtr(pdfDocument));
        Wrap(handle, true);
    }
    
    void PdfSquigglyAnnot::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_Plumbing_PdfSquigglyAnnot");
        FrameworkObject::Dispose(destructor);
    }
    
    PdfSquigglyAnnotPtr PdfSquigglyAnnot::DynamicCast(PdfItemPtr value) {
        return std::dynamic_pointer_cast<PdfSquigglyAnnot>(value);
    }
    

    PdfStampAnnot::PdfStampAnnot(SolidFramework::Pdf::PdfDocumentPtr pdfDocument) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("new_SolidFramework_Pdf_Plumbing_PdfStampAnnot");
        HANDLE handle = proc(SolidFramework::FrameworkObject::GetCPtr(pdfDocument));
        Wrap(handle, true);
    }
    
    void PdfStampAnnot::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_Plumbing_PdfStampAnnot");
        FrameworkObject::Dispose(destructor);
    }
    
    PdfStampAnnotPtr PdfStampAnnot::DynamicCast(PdfItemPtr value) {
        return std::dynamic_pointer_cast<PdfStampAnnot>(value);
    }
    

    PdfStreamObject::PdfStreamObject(SolidFramework::Pdf::PdfDocumentPtr pdfDocument) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("new_SolidFramework_Pdf_Plumbing_PdfStreamObject");
        HANDLE handle = proc(SolidFramework::FrameworkObject::GetCPtr(pdfDocument));
        Wrap(handle, true);
    }
    
    void PdfStreamObject::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_Plumbing_PdfStreamObject");
        FrameworkObject::Dispose(destructor);
    }
    
    HANDLE PdfStreamObject::GetData() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_Plumbing_PdfStreamObject_Data_get");
        return proc(cPtr);
    }
    
    std::wstring PdfStreamObject::GetDataAsString() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Pdf_Plumbing_PdfStreamObject_GetDataAsString");
        return UnwrapWString(proc(cPtr));
    }
    
    int PdfStreamObject::GetDataLength() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Pdf_Plumbing_PdfStreamObject_DataLength_get");
        return proc(cPtr);
    }
    
    int PdfStreamObject::GetLength() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Pdf_Plumbing_PdfStreamObject_Length_get");
        return proc(cPtr);
    }
    
    SolidFramework::Interop::BitmapPtr PdfStreamObject::ToBitmap(SolidFramework::Pdf::PdfDocumentPtr document) {
        static InvokeType_HHH proc = GetFunction<InvokeType_HHH>("SolidFramework_Pdf_Plumbing_PdfStreamObject_ToBitmap");
        HANDLE newCPtr = proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(document));
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Interop::Bitmap>(newCPtr, true);
    }
    
    PdfStreamObjectPtr PdfStreamObject::DynamicCast(PdfItemPtr value) {
        return std::dynamic_pointer_cast<PdfStreamObject>(value);
    }
    

    PdfStrikeOutAnnot::PdfStrikeOutAnnot(SolidFramework::Pdf::PdfDocumentPtr pdfDocument) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("new_SolidFramework_Pdf_Plumbing_PdfStrikeOutAnnot");
        HANDLE handle = proc(SolidFramework::FrameworkObject::GetCPtr(pdfDocument));
        Wrap(handle, true);
    }
    
    void PdfStrikeOutAnnot::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_Plumbing_PdfStrikeOutAnnot");
        FrameworkObject::Dispose(destructor);
    }
    
    PdfStrikeOutAnnotPtr PdfStrikeOutAnnot::DynamicCast(PdfItemPtr value) {
        return std::dynamic_pointer_cast<PdfStrikeOutAnnot>(value);
    }
    

    PdfString::PdfString(SolidFramework::Pdf::PdfDocumentPtr pdfDocument) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("new_SolidFramework_Pdf_Plumbing_PdfString");
        HANDLE handle = proc(SolidFramework::FrameworkObject::GetCPtr(pdfDocument));
        Wrap(handle, true);
    }
    
    void PdfString::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_Plumbing_PdfString");
        FrameworkObject::Dispose(destructor);
    }
    
    std::vector<BYTE> PdfString::GetRawValue() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_Plumbing_PdfString_GetRawValue");
        return SolidFrameworkNative::BytesArrayToVector(proc(cPtr), true);
    }
    
    void PdfString::SetRawValue(const std::vector<BYTE> & value) {
        static InvokeType_VHH proc = GetFunction<InvokeType_VHH>("SolidFramework_Pdf_Plumbing_PdfString_SetRawValue");
        proc(cPtr, SolidFrameworkNative::VectorToBytesArray(value)->GetCPtr());
    }
    
    std::wstring PdfString::ToString() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Pdf_Plumbing_PdfString_ToString");
        return UnwrapWString(proc(cPtr));
    }
    
    std::wstring PdfString::GetValue() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Pdf_Plumbing_PdfString_GetValue");
        return UnwrapWString(proc(cPtr));
    }
    
    void PdfString::SetValue(std::wstring value) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Pdf_Plumbing_PdfString_SetValue");
        proc(cPtr, value.c_str());
    }
    
    PdfStringPtr PdfString::DynamicCast(PdfItemPtr value) {
        return std::dynamic_pointer_cast<PdfString>(value);
    }
    

    PdfTextAnnot::PdfTextAnnot(SolidFramework::Pdf::PdfDocumentPtr pdfDocument) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("new_SolidFramework_Pdf_Plumbing_PdfTextAnnot");
        HANDLE handle = proc(SolidFramework::FrameworkObject::GetCPtr(pdfDocument));
        Wrap(handle, true);
    }
    
    void PdfTextAnnot::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_Plumbing_PdfTextAnnot");
        FrameworkObject::Dispose(destructor);
    }
    
    PdfTextAnnotPtr PdfTextAnnot::DynamicCast(PdfItemPtr value) {
        return std::dynamic_pointer_cast<PdfTextAnnot>(value);
    }
    

    PdfUnderlineAnnot::PdfUnderlineAnnot(SolidFramework::Pdf::PdfDocumentPtr pdfDocument) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("new_SolidFramework_Pdf_Plumbing_PdfUnderlineAnnot");
        HANDLE handle = proc(SolidFramework::FrameworkObject::GetCPtr(pdfDocument));
        Wrap(handle, true);
    }
    
    void PdfUnderlineAnnot::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_Plumbing_PdfUnderlineAnnot");
        FrameworkObject::Dispose(destructor);
    }
    
    PdfUnderlineAnnotPtr PdfUnderlineAnnot::DynamicCast(PdfItemPtr value) {
        return std::dynamic_pointer_cast<PdfUnderlineAnnot>(value);
    }
    

    PdfUriDictionary::PdfUriDictionary(SolidFramework::Pdf::PdfDocumentPtr pdfDocument) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("new_SolidFramework_Pdf_Plumbing_PdfUriDictionary");
        HANDLE handle = proc(SolidFramework::FrameworkObject::GetCPtr(pdfDocument));
        Wrap(handle, true);
    }
    
    void PdfUriDictionary::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_Plumbing_PdfUriDictionary");
        FrameworkObject::Dispose(destructor);
    }
    
    std::wstring PdfUriDictionary::GetUri() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Pdf_Plumbing_PdfUriDictionary_GetUri");
        return UnwrapWString(proc(cPtr));
    }
    
    PdfUriDictionaryPtr PdfUriDictionary::DynamicCast(PdfItemPtr value) {
        return std::dynamic_pointer_cast<PdfUriDictionary>(value);
    }
    

}}} // SolidFramework::Pdf::Plumbing

namespace SolidFramework { namespace Pdf { namespace Repair { 
    void RepairXRef::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_Repair_RepairXRef");
        FrameworkObject::Dispose(destructor);
    }
    
    bool RepairXRef::IsPDF(std::wstring path) {
        typedef unsigned int (SOLIDCALLBACK *InvokeType)(const wchar_t*);
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_Pdf_Repair_RepairXRef_IsPDF");
        return 0 != proc(path.c_str());
    }
    
    RepairResult RepairXRef::Rebuild(std::wstring originalPath, std::wstring repairedPath) {
        static InvokeType_ISS proc = GetFunction<InvokeType_ISS>("SolidFramework_Pdf_Repair_RepairXRef_Rebuild__SWIG_0");
        return (RepairResult)proc(originalPath.c_str(), repairedPath.c_str());
    }
    
    RepairResult RepairXRef::Rebuild(std::wstring originalPath, std::wstring repairedPath, bool overwrite) {
        static InvokeType_ISSU proc = GetFunction<InvokeType_ISSU>("SolidFramework_Pdf_Repair_RepairXRef_Rebuild__SWIG_1");
        return (RepairResult)proc(originalPath.c_str(), repairedPath.c_str(), overwrite);
    }
    
    int RepairXRef::GetVersion() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Pdf_Repair_RepairXRef_Version_get");
        return proc(cPtr);
    }
    
    void RepairXRef::SetVersion(int value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Pdf_Repair_RepairXRef_Version_set");
        proc(cPtr, value);
    }
    

}}} // SolidFramework::Pdf::Repair

namespace SolidFramework { namespace Pdf { namespace Reports { 
    void PdfAProblem::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_Reports_PdfAProblem");
        FrameworkObject::Dispose(destructor);
    }
    
    std::wstring PdfAProblem::GetClause() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Pdf_Reports_PdfAProblem_GetClause");
        return proc(cPtr);
    }
    
    std::wstring PdfAProblem::GetDescription() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Pdf_Reports_PdfAProblem_GetDescription");
        return proc(cPtr);
    }
    
    std::wstring PdfAProblem::GetLocation() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Pdf_Reports_PdfAProblem_GetLocation");
        return UnwrapWString(proc(cPtr));
    }
    
    bool PdfAProblem::GetMatched() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Pdf_Reports_PdfAProblem_Matched_get");
        return 0 != proc(cPtr);
    }
    
    void PdfAProblem::SetMatched(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Pdf_Reports_PdfAProblem_Matched_set");
        proc(cPtr, value);
    }
    
    std::wstring PdfAProblem::GetSection() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Pdf_Reports_PdfAProblem_GetSection");
        return proc(cPtr);
    }
    
    std::wstring PdfAProblem::GetSeverity() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Pdf_Reports_PdfAProblem_GetSeverity");
        return proc(cPtr);
    }
    
    std::wstring PdfAProblem::GetStandard() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Pdf_Reports_PdfAProblem_GetStandard");
        return proc(cPtr);
    }
    
    std::wstring PdfAProblem::ToString() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Pdf_Reports_PdfAProblem_ToString");
        return UnwrapWString(proc(cPtr));
    }
    

    PdfAProblemClass::PdfAProblemClass() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("new_SolidFramework_Pdf_Reports_PdfAProblemClass");
        HANDLE handle = proc();
        Wrap(handle, true);
    }
    
    void PdfAProblemClass::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_Reports_PdfAProblemClass");
        FrameworkObject::Dispose(destructor);
    }
    
    std::wstring PdfAProblemClass::Compare(PdfAProblemClassPtr other, bool showWarnings) {
        static InvokeType_SHHU proc = GetFunction<InvokeType_SHHU>("SolidFramework_Pdf_Reports_PdfAProblemClass_Compare");
        return UnwrapWString(proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(other), showWarnings));
    }
    
    int PdfAProblemClass::GetCount() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Pdf_Reports_PdfAProblemClass_Count_get");
        return proc(cPtr);
    }
    
    std::vector<PdfAProblemPtr> PdfAProblemClass::GetProblems() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_Reports_PdfAProblemClass_GetPdfAProblems");
        return SolidFrameworkNative::PdfAProblemsCollectionToVector(proc(cPtr), false);
    }
    

    PdfAProblemClassesCollection::PdfAProblemClassesCollection() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("new_SolidFramework_Pdf_Reports_PdfAProblemClassesCollection__SWIG_0");
        HANDLE handle = proc();
        Wrap(handle, true);
    }
    
    PdfAProblemClassesCollection::PdfAProblemClassesCollection(PdfAProblemClassesCollectionPtr other) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("new_SolidFramework_Pdf_Reports_PdfAProblemClassesCollection__SWIG_1");
        HANDLE handle = proc(SolidFramework::FrameworkObject::GetCPtr(other));
        Wrap(handle, true);
    }
    
    void PdfAProblemClassesCollection::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_Reports_PdfAProblemClassesCollection");
        FrameworkObject::Dispose(destructor);
    }
    
    void PdfAProblemClassesCollection::Add(std::wstring key, PdfAProblemClassPtr val) {
        static InvokeType_VHSH proc = GetFunction<InvokeType_VHSH>("SolidFramework_Pdf_Reports_PdfAProblemClassesCollection_Add");
        proc(cPtr, key.c_str(), SolidFramework::FrameworkObject::GetCPtr(val));
    }
    
    void PdfAProblemClassesCollection::Clear() {
        static InvokeType_VH proc = GetFunction<InvokeType_VH>("SolidFramework_Pdf_Reports_PdfAProblemClassesCollection_Clear");
        proc(cPtr);
    }
    
    bool PdfAProblemClassesCollection::ContainsKey(std::wstring key) {
        static InvokeType_UHS proc = GetFunction<InvokeType_UHS>("SolidFramework_Pdf_Reports_PdfAProblemClassesCollection_ContainsKey");
        return 0 != proc(cPtr, key.c_str());
    }
    
    int PdfAProblemClassesCollection::GetCount() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Pdf_Reports_PdfAProblemClassesCollection_size");
        return (int)proc(cPtr);
    }
    
    bool PdfAProblemClassesCollection::Empty() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Pdf_Reports_PdfAProblemClassesCollection_empty");
        return 0 != proc(cPtr);
    }
    
    PdfAProblemClassPtr PdfAProblemClassesCollection::GetItem(std::wstring key) {
        static InvokeType_HHS proc = GetFunction<InvokeType_HHS>("SolidFramework_Pdf_Reports_PdfAProblemClassesCollection_getitem");
        HANDLE newCPtr = proc(cPtr, key.c_str());
        return newCPtr == nullptr? nullptr : std::make_shared<PdfAProblemClass>(newCPtr, true);
    }
    
    bool PdfAProblemClassesCollection::Remove(std::wstring key) {
        static InvokeType_UHS proc = GetFunction<InvokeType_UHS>("SolidFramework_Pdf_Reports_PdfAProblemClassesCollection_Remove");
        return 0 != proc(cPtr, key.c_str());
    }
    

    PdfAReport::PdfAReport(std::wstring path) {
        static InvokeType_PS proc = GetFunction<InvokeType_PS>("new_SolidFramework_Pdf_Reports_PdfAReport");
        HANDLE handle = proc(path.c_str());
        Wrap(handle, true);
    }
    
    void PdfAReport::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_Reports_PdfAReport");
        FrameworkObject::Dispose(destructor);
    }
    
    std::wstring PdfAReport::GetApplicationName() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Pdf_Reports_PdfAReport_GetApplicationName");
        return UnwrapWString(proc(cPtr));
    }
    
    int PdfAReport::GetCount() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Pdf_Reports_PdfAReport_Count_get");
        return proc(cPtr);
    }
    
    bool PdfAReport::GetEncrypted() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Pdf_Reports_PdfAReport_Encrypted_get");
        return 0 != proc(cPtr);
    }
    
    std::vector<std::wstring> PdfAReport::GetKeys() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_Reports_PdfAReport_GetKeys");
        return SolidFrameworkNative::StringsArrayToVector(proc(cPtr), true);
    }
    
    bool PdfAReport::GetMissingStructTreeRoot() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Pdf_Reports_PdfAReport_MissingStructTreeRoot_get");
        return 0 != proc(cPtr);
    }
    
    SolidFramework::Plumbing::ValidationMode PdfAReport::GetMode() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Pdf_Reports_PdfAReport_Mode_get");
        return (SolidFramework::Plumbing::ValidationMode)proc(cPtr);
    }
    
    PdfAResultsCollectionPtr PdfAReport::GetResults() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_Reports_PdfAReport_GetResults");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<PdfAResultsCollection>(newCPtr, false);
    }
    
    void PdfAReport::Save(std::wstring path) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Pdf_Reports_PdfAReport_Save");
        proc(cPtr, path.c_str());
    }
    

    void PdfAResult::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_Reports_PdfAResult");
        FrameworkObject::Dispose(destructor);
    }
    
    PdfAProblemClassesCollectionPtr PdfAResult::GetClasses() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_Reports_PdfAResult_GetClasses");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<PdfAProblemClassesCollection>(newCPtr, false);
    }
    
    std::wstring PdfAResult::Compare(PdfAResultPtr other, bool showWarnings) {
        static InvokeType_SHHU proc = GetFunction<InvokeType_SHHU>("SolidFramework_Pdf_Reports_PdfAResult_Compare");
        return UnwrapWString(proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(other), showWarnings));
    }
    
    int PdfAResult::GetCount() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Pdf_Reports_PdfAResult_Count_get");
        return proc(cPtr);
    }
    
    bool PdfAResult::GetEncrypted() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Pdf_Reports_PdfAResult_Encrypted_get");
        return 0 != proc(cPtr);
    }
    
    std::vector<std::wstring> PdfAResult::GetKeys() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_Reports_PdfAResult_GetKeys");
        return SolidFrameworkNative::StringsArrayToVector(proc(cPtr), true);
    }
    
    bool PdfAResult::GetMissingStructTreeRoot() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Pdf_Reports_PdfAResult_MissingStructTreeRoot_get");
        return 0 != proc(cPtr);
    }
    

    PdfAResultsCollection::PdfAResultsCollection() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("new_SolidFramework_Pdf_Reports_PdfAResultsCollection__SWIG_0");
        HANDLE handle = proc();
        Wrap(handle, true);
    }
    
    PdfAResultsCollection::PdfAResultsCollection(PdfAResultsCollectionPtr other) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("new_SolidFramework_Pdf_Reports_PdfAResultsCollection__SWIG_1");
        HANDLE handle = proc(SolidFramework::FrameworkObject::GetCPtr(other));
        Wrap(handle, true);
    }
    
    void PdfAResultsCollection::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_Reports_PdfAResultsCollection");
        FrameworkObject::Dispose(destructor);
    }
    
    void PdfAResultsCollection::Add(std::wstring key, PdfAResultPtr val) {
        static InvokeType_VHSH proc = GetFunction<InvokeType_VHSH>("SolidFramework_Pdf_Reports_PdfAResultsCollection_Add");
        proc(cPtr, key.c_str(), SolidFramework::FrameworkObject::GetCPtr(val));
    }
    
    void PdfAResultsCollection::Clear() {
        static InvokeType_VH proc = GetFunction<InvokeType_VH>("SolidFramework_Pdf_Reports_PdfAResultsCollection_Clear");
        proc(cPtr);
    }
    
    bool PdfAResultsCollection::ContainsKey(std::wstring key) {
        static InvokeType_UHS proc = GetFunction<InvokeType_UHS>("SolidFramework_Pdf_Reports_PdfAResultsCollection_ContainsKey");
        return 0 != proc(cPtr, key.c_str());
    }
    
    int PdfAResultsCollection::GetCount() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Pdf_Reports_PdfAResultsCollection_size");
        return (int)proc(cPtr);
    }
    
    bool PdfAResultsCollection::Empty() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Pdf_Reports_PdfAResultsCollection_empty");
        return 0 != proc(cPtr);
    }
    
    PdfAResultPtr PdfAResultsCollection::GetItem(std::wstring key) {
        static InvokeType_HHS proc = GetFunction<InvokeType_HHS>("SolidFramework_Pdf_Reports_PdfAResultsCollection_getitem");
        HANDLE newCPtr = proc(cPtr, key.c_str());
        return newCPtr == nullptr? nullptr : std::make_shared<PdfAResult>(newCPtr, true);
    }
    
    bool PdfAResultsCollection::Remove(std::wstring key) {
        static InvokeType_UHS proc = GetFunction<InvokeType_UHS>("SolidFramework_Pdf_Reports_PdfAResultsCollection_Remove");
        return 0 != proc(cPtr, key.c_str());
    }
    

}}} // SolidFramework::Pdf::Reports

namespace SolidFramework { namespace Pdf { namespace Transformers { 
    ImageWatermarkTransformer::ImageWatermarkTransformer() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("new_SolidFramework_Pdf_Transformers_ImageWatermarkTransformer__SWIG_0");
        HANDLE handle = proc();
        Wrap(handle, true);
    }
    
    ImageWatermarkTransformer::ImageWatermarkTransformer(SolidFramework::Converters::Plumbing::ImageWatermarkPtr watermark) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("new_SolidFramework_Pdf_Transformers_ImageWatermarkTransformer__SWIG_1");
        HANDLE handle = proc(SolidFramework::FrameworkObject::GetCPtr(watermark));
        Wrap(handle, true);
    }
    
    void ImageWatermarkTransformer::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_Transformers_ImageWatermarkTransformer");
        FrameworkObject::Dispose(destructor);
    }
    
    SolidFramework::Converters::Plumbing::ImageWatermarkPtr ImageWatermarkTransformer::GetImageWatermark() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_Transformers_ImageWatermarkTransformer_GetImageWatermark");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Converters::Plumbing::ImageWatermark>(newCPtr, true);
    }
    
    void ImageWatermarkTransformer::SetImageWatermark(SolidFramework::Converters::Plumbing::ImageWatermarkPtr value) {
        static InvokeType_VHH proc = GetFunction<InvokeType_VHH>("SolidFramework_Pdf_Transformers_ImageWatermarkTransformer_SetImageWatermark");
        proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(value));
    }
    
    ImageWatermarkTransformerPtr ImageWatermarkTransformer::DynamicCast(PdfTransformerPtr value) {
        return std::dynamic_pointer_cast<ImageWatermarkTransformer>(value);
    }
    

    void IRecognize::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_Transformers_IRecognize");
        FrameworkObject::Dispose(destructor);
    }
    
    bool IRecognize::GetOcrAlways() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Pdf_Transformers_IRecognize_OcrAlways_get");
        return 0 != proc(cPtr);
    }
    
    void IRecognize::SetOcrAlways(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Pdf_Transformers_IRecognize_OcrAlways_set");
        proc(cPtr, value);
    }
    
    bool IRecognize::GetOcrAutoRotate() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Pdf_Transformers_IRecognize_OcrAutoRotate_get");
        return 0 != proc(cPtr);
    }
    
    void IRecognize::SetOcrAutoRotate(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Pdf_Transformers_IRecognize_OcrAutoRotate_set");
        proc(cPtr, value);
    }
    
    SolidFramework::Converters::Plumbing::TextRecoveryEngine IRecognize::GetOcrEngine() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Pdf_Transformers_IRecognize_OcrEngine_get");
        return (SolidFramework::Converters::Plumbing::TextRecoveryEngine)proc(cPtr);
    }
    
    void IRecognize::SetOcrEngine(SolidFramework::Converters::Plumbing::TextRecoveryEngine value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Pdf_Transformers_IRecognize_OcrEngine_set");
        proc(cPtr, (int)(value));
    }
    
    SolidFramework::Imaging::Plumbing::ImageCompression IRecognize::GetOcrImageCompression() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Pdf_Transformers_IRecognize_OcrImageCompression_get");
        return (SolidFramework::Imaging::Plumbing::ImageCompression)proc(cPtr);
    }
    
    void IRecognize::SetOcrImageCompression(SolidFramework::Imaging::Plumbing::ImageCompression value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Pdf_Transformers_IRecognize_OcrImageCompression_set");
        proc(cPtr, (int)(value));
    }
    
    std::wstring IRecognize::GetOcrLanguage() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Pdf_Transformers_IRecognize_GetOcrLanguage");
        return proc(cPtr);
    }
    
    void IRecognize::SetOcrLanguage(std::wstring value) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Pdf_Transformers_IRecognize_SetOcrLanguage");
        proc(cPtr, value.c_str());
    }
    
    SolidFramework::Converters::Plumbing::OcrType IRecognize::GetOcrType() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Pdf_Transformers_IRecognize_OcrType_get");
        return (SolidFramework::Converters::Plumbing::OcrType)proc(cPtr);
    }
    
    void IRecognize::SetOcrType(SolidFramework::Converters::Plumbing::OcrType value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Pdf_Transformers_IRecognize_OcrType_set");
        proc(cPtr, (int)(value));
    }
    
    SolidFramework::Interop::NativeError IRecognize::Recognize() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Pdf_Transformers_IRecognize_Recognize");
        return (SolidFramework::Interop::NativeError)proc(cPtr);
    }
    
    IRecognizePtr IRecognize::DynamicCast(PdfTransformerPtr value) {
        return std::dynamic_pointer_cast<IRecognize>(value);
    }
    
    IRecognize::IRecognize() {
        // only called from derived class constructors
    }
    

    ITransformationResult::ITransformationResult() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("new_SolidFramework_Pdf_Transformers_ITransformationResult");
        HANDLE handle = proc();
        Wrap(handle, true);
    }
    
    void ITransformationResult::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_Transformers_ITransformationResult");
        FrameworkObject::Dispose(destructor);
    }
    
    SolidFramework::Pdf::PdfDocumentPtr ITransformationResult::GetDocument() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_Transformers_ITransformationResult_GetDocument");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Pdf::PdfDocument>(newCPtr, true);
    }
    
    bool ITransformationResult::IsEqual(ITransformationResultPtr value) {
        static InvokeType_UHH proc = GetFunction<InvokeType_UHH>("SolidFramework_Pdf_Transformers_ITransformationResult_IsEqual");
        return 0 != proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(value));
    }
    
    SolidFramework::Interop::NativeError ITransformationResult::GetStatus() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Pdf_Transformers_ITransformationResult_GetStatus");
        return (SolidFramework::Interop::NativeError)proc(cPtr);
    }
    

    void OCRTransformationResult::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_Transformers_OCRTransformationResult");
        FrameworkObject::Dispose(destructor);
    }
    
    int OCRTransformationResult::GetDocumentConfidentWordCount() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Pdf_Transformers_OCRTransformationResult_GetDocumentConfidentWordCount");
        return proc(cPtr);
    }
    
    int OCRTransformationResult::GetDocumentGraphicCount() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Pdf_Transformers_OCRTransformationResult_GetDocumentGraphicCount");
        return proc(cPtr);
    }
    
    int OCRTransformationResult::GetDocumentImageCount() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Pdf_Transformers_OCRTransformationResult_GetDocumentImageCount");
        return proc(cPtr);
    }
    
    int OCRTransformationResult::GetDocumentWordCount() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Pdf_Transformers_OCRTransformationResult_GetDocumentWordCount");
        return proc(cPtr);
    }
    
    int OCRTransformationResult::GetOCRPageDetailsPageCount() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Pdf_Transformers_OCRTransformationResult_GetOCRPageDetailsPageCount");
        return proc(cPtr);
    }
    
    int OCRTransformationResult::GetPageConfidentWordCount(int nPageIndex) {
        static InvokeType_IHI proc = GetFunction<InvokeType_IHI>("SolidFramework_Pdf_Transformers_OCRTransformationResult_GetPageConfidentWordCount");
        return proc(cPtr, nPageIndex);
    }
    
    int OCRTransformationResult::GetPageGraphicCount(int nPageIndex) {
        static InvokeType_IHI proc = GetFunction<InvokeType_IHI>("SolidFramework_Pdf_Transformers_OCRTransformationResult_GetPageGraphicCount");
        return proc(cPtr, nPageIndex);
    }
    
    int OCRTransformationResult::GetPageImageCount(int nPageIndex) {
        static InvokeType_IHI proc = GetFunction<InvokeType_IHI>("SolidFramework_Pdf_Transformers_OCRTransformationResult_GetPageImageCount");
        return proc(cPtr, nPageIndex);
    }
    
    int OCRTransformationResult::GetPageWordCount(int nPageIndex) {
        static InvokeType_IHI proc = GetFunction<InvokeType_IHI>("SolidFramework_Pdf_Transformers_OCRTransformationResult_GetPageWordCount");
        return proc(cPtr, nPageIndex);
    }
    
    TransformationResultPtr OCRTransformationResult::DynamicCast(ITransformationResultPtr value) {
        return std::dynamic_pointer_cast<TransformationResult>(value);
    }
    

    OcrTransformer::OcrTransformer() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("new_SolidFramework_Pdf_Transformers_OcrTransformer");
        HANDLE handle = proc();
        Wrap(handle, true);
    }
    
    void OcrTransformer::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_Transformers_OcrTransformer");
        FrameworkObject::Dispose(destructor);
    }
    
    SolidFramework::Interop::NativeError OcrTransformer::Recognize() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Pdf_Transformers_OcrTransformer_Recognize");
        return (SolidFramework::Interop::NativeError)proc(cPtr);
    }
    
    OcrTransformerPtr OcrTransformer::DynamicCast(PdfTransformerPtr value) {
        return std::dynamic_pointer_cast<OcrTransformer>(value);
    }
    

    PasswordSecurityTransformer::PasswordSecurityTransformer() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("new_SolidFramework_Pdf_Transformers_PasswordSecurityTransformer__SWIG_0");
        HANDLE handle = proc();
        Wrap(handle, true);
    }
    
    PasswordSecurityTransformer::PasswordSecurityTransformer(std::wstring ownerPassword, SolidFramework::Pdf::EncryptionAlgorithm algorithm) {
        static InvokeType_HSI proc = GetFunction<InvokeType_HSI>("new_SolidFramework_Pdf_Transformers_PasswordSecurityTransformer__SWIG_1");
        HANDLE handle = proc(ownerPassword.c_str(), (int)(algorithm));
        Wrap(handle, true);
    }
    
    PasswordSecurityTransformer::PasswordSecurityTransformer(std::wstring ownerPassword, std::wstring userPassword, SolidFramework::Pdf::EncryptionAlgorithm algorithm, SolidFramework::Pdf::AccessPermissions permission) {
        typedef HANDLE (SOLIDCALLBACK *InvokeType)(const wchar_t*, const wchar_t*, int, int);
        static InvokeType proc = GetFunction<InvokeType>("new_SolidFramework_Pdf_Transformers_PasswordSecurityTransformer__SWIG_2");
        HANDLE handle = proc(ownerPassword.c_str(), userPassword.c_str(), (int)(algorithm), (int)(permission));
        Wrap(handle, true);
    }
    
    void PasswordSecurityTransformer::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_Transformers_PasswordSecurityTransformer");
        FrameworkObject::Dispose(destructor);
    }
    
    SolidFramework::Pdf::EncryptionAlgorithm PasswordSecurityTransformer::GetEncryptionAlgorithm() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Pdf_Transformers_PasswordSecurityTransformer_EncryptionAlgorithm_get");
        return (SolidFramework::Pdf::EncryptionAlgorithm)proc(cPtr);
    }
    
    void PasswordSecurityTransformer::SetEncryptionAlgorithm(SolidFramework::Pdf::EncryptionAlgorithm value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Pdf_Transformers_PasswordSecurityTransformer_EncryptionAlgorithm_set");
        proc(cPtr, (int)(value));
    }
    
    std::wstring PasswordSecurityTransformer::GetOwnerPassword() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Pdf_Transformers_PasswordSecurityTransformer_GetOwnerPassword");
        return proc(cPtr);
    }
    
    void PasswordSecurityTransformer::SetOwnerPassword(std::wstring value) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Pdf_Transformers_PasswordSecurityTransformer_SetOwnerPassword");
        proc(cPtr, value.c_str());
    }
    
    SolidFramework::Pdf::AccessPermissions PasswordSecurityTransformer::GetPermission() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Pdf_Transformers_PasswordSecurityTransformer_Permission_get");
        return (SolidFramework::Pdf::AccessPermissions)proc(cPtr);
    }
    
    void PasswordSecurityTransformer::SetPermission(SolidFramework::Pdf::AccessPermissions value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Pdf_Transformers_PasswordSecurityTransformer_Permission_set");
        proc(cPtr, (int)(value));
    }
    
    std::wstring PasswordSecurityTransformer::GetUserPassword() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Pdf_Transformers_PasswordSecurityTransformer_GetUserPassword");
        return proc(cPtr);
    }
    
    void PasswordSecurityTransformer::SetUserPassword(std::wstring value) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Pdf_Transformers_PasswordSecurityTransformer_SetUserPassword");
        proc(cPtr, value.c_str());
    }
    
    PasswordSecurityTransformerPtr PasswordSecurityTransformer::DynamicCast(PdfTransformerPtr value) {
        return std::dynamic_pointer_cast<PasswordSecurityTransformer>(value);
    }
    

    void PdfTransformer::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_Transformers_PdfTransformer");
        FrameworkObject::Dispose(destructor);
    }
    
    void PdfTransformer::AddDocument(SolidFramework::Pdf::PdfDocumentPtr document) {
        static InvokeType_VHH proc = GetFunction<InvokeType_VHH>("SolidFramework_Pdf_Transformers_PdfTransformer_AddDocument");
        proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(document));
    }
    
    void PdfTransformer::Clear() {
        static InvokeType_VH proc = GetFunction<InvokeType_VH>("SolidFramework_Pdf_Transformers_PdfTransformer_Clear");
        proc(cPtr);
    }
    
    void PdfTransformer::ImportFromFile(std::wstring path) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Pdf_Transformers_PdfTransformer_ImportFromFile");
        proc(cPtr, path.c_str());
    }
    
    void PdfTransformer::ImportFromFolder(std::wstring path) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Pdf_Transformers_PdfTransformer_ImportFromFolder");
        proc(cPtr, path.c_str());
    }
    
    std::wstring PdfTransformer::GetOutputFolder() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Pdf_Transformers_PdfTransformer_GetOutputFolder");
        return proc(cPtr);
    }
    
    void PdfTransformer::SetOutputFolder(std::wstring value) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Pdf_Transformers_PdfTransformer_SetOutputFolder");
        proc(cPtr, value.c_str());
    }
    
    SolidFramework::Plumbing::OverwriteMode PdfTransformer::GetOverwriteMode() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Pdf_Transformers_PdfTransformer_OverwriteMode_get");
        return (SolidFramework::Plumbing::OverwriteMode)proc(cPtr);
    }
    
    void PdfTransformer::SetOverwriteMode(SolidFramework::Plumbing::OverwriteMode value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Pdf_Transformers_PdfTransformer_OverwriteMode_set");
        proc(cPtr, (int)(value));
    }
    
    std::vector<ITransformationResultPtr> PdfTransformer::GetResults() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_Transformers_PdfTransformer_GetResults");
        return SolidFrameworkNative::ITransformationResultsCollectionToVector(proc(cPtr), false);
    }
    
    void PdfTransformer::Transform() {
        static InvokeType_VH proc = GetFunction<InvokeType_VH>("SolidFramework_Pdf_Transformers_PdfTransformer_Transform__SWIG_0");
        proc(cPtr);
    }
    
    ITransformationResultPtr PdfTransformer::Transform(SolidFramework::Pdf::PdfDocumentPtr document, PdfTransformerPtr transformer) {
        static InvokeType_HHH proc = GetFunction<InvokeType_HHH>("SolidFramework_Pdf_Transformers_PdfTransformer_Transform__SWIG_1");
        HANDLE newCPtr = proc(SolidFramework::FrameworkObject::GetCPtr(document), SolidFramework::FrameworkObject::GetCPtr(transformer));
        return SolidFrameworkNative::MakeITransformationResult(newCPtr, true);
    }
    
    PdfTransformer::PdfTransformer() {
        // only called from derived class constructors
    }
    

    TextWatermarkTransformer::TextWatermarkTransformer() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("new_SolidFramework_Pdf_Transformers_TextWatermarkTransformer__SWIG_0");
        HANDLE handle = proc();
        Wrap(handle, true);
    }
    
    TextWatermarkTransformer::TextWatermarkTransformer(SolidFramework::Converters::Plumbing::TextWatermarkPtr watermark) {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("new_SolidFramework_Pdf_Transformers_TextWatermarkTransformer__SWIG_1");
        HANDLE handle = proc(SolidFramework::FrameworkObject::GetCPtr(watermark));
        Wrap(handle, true);
    }
    
    void TextWatermarkTransformer::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_Transformers_TextWatermarkTransformer");
        FrameworkObject::Dispose(destructor);
    }
    
    SolidFramework::Converters::Plumbing::TextWatermarkPtr TextWatermarkTransformer::GetTextWatermark() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_Transformers_TextWatermarkTransformer_GetTextWatermark");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Converters::Plumbing::TextWatermark>(newCPtr, true);
    }
    
    void TextWatermarkTransformer::SetTextWatermark(SolidFramework::Converters::Plumbing::TextWatermarkPtr value) {
        static InvokeType_VHH proc = GetFunction<InvokeType_VHH>("SolidFramework_Pdf_Transformers_TextWatermarkTransformer_SetTextWatermark");
        proc(cPtr, SolidFramework::FrameworkObject::GetCPtr(value));
    }
    
    TextWatermarkTransformerPtr TextWatermarkTransformer::DynamicCast(PdfTransformerPtr value) {
        return std::dynamic_pointer_cast<TextWatermarkTransformer>(value);
    }
    

    TransformationResult::TransformationResult(SolidFramework::Pdf::PdfDocumentPtr doc, SolidFramework::Interop::NativeError stat) {
        static InvokeType_HHI proc = GetFunction<InvokeType_HHI>("new_SolidFramework_Pdf_Transformers_TransformationResult");
        HANDLE handle = proc(SolidFramework::FrameworkObject::GetCPtr(doc), (int)(stat));
        Wrap(handle, true);
    }
    
    void TransformationResult::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Pdf_Transformers_TransformationResult");
        FrameworkObject::Dispose(destructor);
    }
    
    SolidFramework::Pdf::PdfDocumentPtr TransformationResult::GetDocument() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Pdf_Transformers_TransformationResult_GetDocument");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<SolidFramework::Pdf::PdfDocument>(newCPtr, true);
    }
    
    SolidFramework::Interop::NativeError TransformationResult::GetStatus() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Pdf_Transformers_TransformationResult_GetStatus");
        return (SolidFramework::Interop::NativeError)proc(cPtr);
    }
    
    TransformationResultPtr TransformationResult::DynamicCast(ITransformationResultPtr value) {
        return std::dynamic_pointer_cast<TransformationResult>(value);
    }
    
    TransformationResult::TransformationResult() {
        // only called from derived class constructors
    }
    

}}} // SolidFramework::Pdf::Transformers

namespace SolidFramework { namespace Platform { 
    std::vector<std::wstring> Directory::GetContents(std::wstring path, bool allDirectories) {
        typedef HANDLE (SOLIDCALLBACK *InvokeType)(const wchar_t*, unsigned int);
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_Platform_Directory_GetContents");
        return SolidFrameworkNative::StringsArrayToVector(proc(path.c_str(), allDirectories), true);
    }
    
    bool Directory::CreateDirectory(std::wstring path) {
        typedef unsigned int (SOLIDCALLBACK *InvokeType)(const wchar_t*);
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_Platform_Directory_CreateDirectory");
        return 0 != proc(path.c_str());
    }
    
    std::wstring Directory::GetCurrentDirectory() {
        static InvokeType_SV proc = GetFunction<InvokeType_SV>("SolidFramework_Platform_Directory_GetCurrentDirectory");
        return UnwrapWString(proc());
    }
    
    bool Directory::Delete(std::wstring path, bool recursive) {
        typedef unsigned int (SOLIDCALLBACK *InvokeType)(const wchar_t*, unsigned int);
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_Platform_Directory_Delete__SWIG_0");
        return 0 != proc(path.c_str(), recursive);
    }
    
    bool Directory::Delete(std::wstring path) {
        typedef unsigned int (SOLIDCALLBACK *InvokeType)(const wchar_t*);
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_Platform_Directory_Delete__SWIG_1");
        return 0 != proc(path.c_str());
    }
    
    bool Directory::Exists(std::wstring path) {
        typedef unsigned int (SOLIDCALLBACK *InvokeType)(const wchar_t*);
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_Platform_Directory_Exists");
        return 0 != proc(path.c_str());
    }
    
    std::vector<std::wstring> Directory::GetFiles(std::wstring path, bool allDirectories) {
        typedef HANDLE (SOLIDCALLBACK *InvokeType)(const wchar_t*, unsigned int);
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_Platform_Directory_GetFiles");
        return SolidFrameworkNative::StringsArrayToVector(proc(path.c_str(), allDirectories), true);
    }
    
    std::vector<std::wstring> Directory::GetFolders(std::wstring path, bool allDirectories) {
        typedef HANDLE (SOLIDCALLBACK *InvokeType)(const wchar_t*, unsigned int);
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_Platform_Directory_GetFolders");
        return SolidFrameworkNative::StringsArrayToVector(proc(path.c_str(), allDirectories), true);
    }
    

    bool File::Copy(std::wstring source, std::wstring destination, bool overwrite) {
        typedef unsigned int (SOLIDCALLBACK *InvokeType)(const wchar_t*, const wchar_t*, unsigned int);
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_Platform_File_Copy__SWIG_0");
        return 0 != proc(source.c_str(), destination.c_str(), overwrite);
    }
    
    bool File::Copy(std::wstring source, std::wstring destination) {
        typedef unsigned int (SOLIDCALLBACK *InvokeType)(const wchar_t*, const wchar_t*);
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_Platform_File_Copy__SWIG_1");
        return 0 != proc(source.c_str(), destination.c_str());
    }
    
    bool File::Delete(std::wstring path) {
        typedef unsigned int (SOLIDCALLBACK *InvokeType)(const wchar_t*);
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_Platform_File_Delete");
        return 0 != proc(path.c_str());
    }
    
    bool File::Exists(std::wstring path) {
        typedef unsigned int (SOLIDCALLBACK *InvokeType)(const wchar_t*);
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_Platform_File_Exists");
        return 0 != proc(path.c_str());
    }
    
    bool File::Move(std::wstring source, std::wstring destination, bool overwrite) {
        typedef unsigned int (SOLIDCALLBACK *InvokeType)(const wchar_t*, const wchar_t*, unsigned int);
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_Platform_File_Move__SWIG_0");
        return 0 != proc(source.c_str(), destination.c_str(), overwrite);
    }
    
    bool File::Move(std::wstring source, std::wstring destination) {
        typedef unsigned int (SOLIDCALLBACK *InvokeType)(const wchar_t*, const wchar_t*);
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_Platform_File_Move__SWIG_1");
        return 0 != proc(source.c_str(), destination.c_str());
    }
    
    std::wstring File::ReadAllText(std::wstring path) {
        static InvokeType_SS proc = GetFunction<InvokeType_SS>("SolidFramework_Platform_File_ReadAllText");
        return UnwrapWString(proc(path.c_str()));
    }
    
    bool File::WriteAllText(std::wstring path, std::wstring text) {
        typedef unsigned int (SOLIDCALLBACK *InvokeType)(const wchar_t*, const wchar_t*);
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_Platform_File_WriteAllText");
        return 0 != proc(path.c_str(), text.c_str());
    }
    

    std::wstring Path::ChangeExtension(std::wstring path, std::wstring newExtension) {
        static InvokeType_SSS proc = GetFunction<InvokeType_SSS>("SolidFramework_Platform_Path_ChangeExtension");
        return UnwrapWString(proc(path.c_str(), newExtension.c_str()));
    }
    
    std::wstring Path::ChangeFileName(std::wstring path, std::wstring newFileName) {
        static InvokeType_SSS proc = GetFunction<InvokeType_SSS>("SolidFramework_Platform_Path_ChangeFileName");
        return UnwrapWString(proc(path.c_str(), newFileName.c_str()));
    }
    
    std::wstring Path::Combine(std::wstring first, std::wstring second) {
        static InvokeType_SSS proc = GetFunction<InvokeType_SSS>("SolidFramework_Platform_Path_Combine");
        return UnwrapWString(proc(first.c_str(), second.c_str()));
    }
    
    std::wstring Path::GetDirectoryName(std::wstring path) {
        static InvokeType_SS proc = GetFunction<InvokeType_SS>("SolidFramework_Platform_Path_GetDirectoryName");
        return UnwrapWString(proc(path.c_str()));
    }
    
    std::wstring Path::GetExecutablePath() {
        static InvokeType_SV proc = GetFunction<InvokeType_SV>("SolidFramework_Platform_Path_GetExecutablePath");
        return UnwrapWString(proc());
    }
    
    std::wstring Path::GetExtension(std::wstring path) {
        static InvokeType_SS proc = GetFunction<InvokeType_SS>("SolidFramework_Platform_Path_GetExtension");
        return UnwrapWString(proc(path.c_str()));
    }
    
    std::wstring Path::GetFileName(std::wstring path) {
        static InvokeType_SS proc = GetFunction<InvokeType_SS>("SolidFramework_Platform_Path_GetFileName");
        return UnwrapWString(proc(path.c_str()));
    }
    
    std::wstring Path::GetFileNameWithoutExtension(std::wstring path) {
        static InvokeType_SS proc = GetFunction<InvokeType_SS>("SolidFramework_Platform_Path_GetFileNameWithoutExtension");
        return UnwrapWString(proc(path.c_str()));
    }
    
    std::wstring Path::GetFullPath(std::wstring path) {
        static InvokeType_SS proc = GetFunction<InvokeType_SS>("SolidFramework_Platform_Path_GetFullPath");
        return UnwrapWString(proc(path.c_str()));
    }
    
    std::wstring Path::RemoveBackslash(std::wstring path) {
        static InvokeType_SS proc = GetFunction<InvokeType_SS>("SolidFramework_Platform_Path_RemoveBackslash");
        return UnwrapWString(proc(path.c_str()));
    }
    
    bool Path::IsRooted(std::wstring path) {
        typedef unsigned int (SOLIDCALLBACK *InvokeType)(const wchar_t*);
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_Platform_Path_IsRooted");
        return 0 != proc(path.c_str());
    }
    

    std::wstring Platform::GetAppDataDirectory() {
        static InvokeType_SV proc = GetFunction<InvokeType_SV>("SolidFramework_Platform_Platform_GetAppDataDirectory");
        return UnwrapWString(proc());
    }
    
    void Platform::SetAppDataDirectory(std::wstring path) {
        static InvokeType_VS proc = GetFunction<InvokeType_VS>("SolidFramework_Platform_Platform_SetAppDataDirectory");
        proc(path.c_str());
    }
    
    std::wstring Platform::GetBuild() {
        static InvokeType_SV proc = GetFunction<InvokeType_SV>("SolidFramework_Platform_Platform_GetBuild");
        return UnwrapWString(proc());
    }
    
    std::wstring Platform::GetCMapDirectory() {
        static InvokeType_SV proc = GetFunction<InvokeType_SV>("SolidFramework_Platform_Platform_GetCMapDirectory");
        return UnwrapWString(proc());
    }
    
    void Platform::SetCMapDirectory(std::wstring path) {
        static InvokeType_VS proc = GetFunction<InvokeType_VS>("SolidFramework_Platform_Platform_SetCMapDirectory");
        proc(path.c_str());
    }
    
    std::wstring Platform::GetColorProfileDirectory() {
        static InvokeType_SV proc = GetFunction<InvokeType_SV>("SolidFramework_Platform_Platform_GetColorProfileDirectory");
        return UnwrapWString(proc());
    }
    
    void Platform::SetColorProfileDirectory(std::wstring path) {
        static InvokeType_VS proc = GetFunction<InvokeType_VS>("SolidFramework_Platform_Platform_SetColorProfileDirectory");
        proc(path.c_str());
    }
    
    std::wstring Platform::GetFontDirectory() {
        static InvokeType_SV proc = GetFunction<InvokeType_SV>("SolidFramework_Platform_Platform_GetFontDirectory");
        return UnwrapWString(proc());
    }
    
    void Platform::SetFontDirectory(std::wstring path) {
        static InvokeType_VS proc = GetFunction<InvokeType_VS>("SolidFramework_Platform_Platform_SetFontDirectory");
        proc(path.c_str());
    }
    
    std::wstring Platform::GetFontsDataBaseFile() {
        static InvokeType_SV proc = GetFunction<InvokeType_SV>("SolidFramework_Platform_Platform_GetFontsDataBaseFile");
        return UnwrapWString(proc());
    }
    
    void Platform::SetFontsDataBaseFile(std::wstring path) {
        static InvokeType_VS proc = GetFunction<InvokeType_VS>("SolidFramework_Platform_Platform_SetFontsDataBaseFile");
        proc(path.c_str());
    }
    
    std::wstring Platform::GetFullVersion() {
        static InvokeType_SV proc = GetFunction<InvokeType_SV>("SolidFramework_Platform_Platform_GetFullVersion");
        return UnwrapWString(proc());
    }
    
    std::wstring Platform::GetLogFilename() {
        static InvokeType_SV proc = GetFunction<InvokeType_SV>("SolidFramework_Platform_Platform_GetLogFilename");
        return UnwrapWString(proc());
    }
    
    void Platform::SetLogFilename(std::wstring path) {
        static InvokeType_VS proc = GetFunction<InvokeType_VS>("SolidFramework_Platform_Platform_SetLogFilename");
        proc(path.c_str());
    }
    
    void Platform::SupportPlatformIndependentPaths(bool value) {
        supportPlatformIndependentPaths = value;
        if (initialized) {
            typedef void (SOLIDCALLBACK *InvokeType)(unsigned int);
            static InvokeType proc = GetFunction<InvokeType>("SolidFramework_Platform_Platform_SupportPlatformIndependentPaths");
            proc(value);
        } else {
            initializedWithPlatformIndependentPaths = true;
        }
    }
    
    std::wstring Platform::GetTempDirectory() {
        static InvokeType_SV proc = GetFunction<InvokeType_SV>("SolidFramework_Platform_Platform_GetTempDirectory");
        return UnwrapWString(proc());
    }
    
    void Platform::SetTempFileIsAboutToBeCreatedCallback(Platform::TempFileIsAboutToBeCreatedCallback callback) {
        static ConnectType_TempFileIsAboutToBeCreated proc = GetFunction<ConnectType_TempFileIsAboutToBeCreated>("SolidFramework_Platform_Platform_SetNativeTempFileIsAboutToBeCreatedCallback");
        proc(callback);
    }
    
    void Platform::Terminate() {
        static InvokeType_VV proc = GetFunction<InvokeType_VV>("SolidFramework_Platform_Platform_Terminate");
        proc();
    }
    
    std::wstring Platform::GetVersion() {
        static InvokeType_SV proc = GetFunction<InvokeType_SV>("SolidFramework_Platform_Platform_GetVersion");
        return UnwrapWString(proc());
    }
    
    SolidFramework::Plumbing::WarningLevelType Platform::GetWarningLevel() {
        static InvokeType_IV proc = GetFunction<InvokeType_IV>("SolidFramework_Platform_Platform_GetWarningLevel");
        return (SolidFramework::Plumbing::WarningLevelType)proc();
    }
    
    bool Platform::SetWarningLevel(SolidFramework::Plumbing::WarningLevelType val) {
        typedef unsigned int (SOLIDCALLBACK *InvokeType)(int);
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_Platform_Platform_SetWarningLevel");
        return 0 != proc((int)(val));
    }
    
    void Platform::WriteToLog(std::wstring content) {
        static InvokeType_VS proc = GetFunction<InvokeType_VS>("SolidFramework_Platform_Platform_WriteToLog");
        proc(content.c_str());
    }
    
    bool Platform::SetSupportDirectory(const std::wstring path) {
        DLLWrapper::libraryPath = path;
        return true;
    }
    
    void Platform::OutputDebugString(const std::wstring message) {
        #if defined __SOLIDWINDOWS__
        ::OutputDebugString(message.c_str());
        #else
        // TODO: support OutputDebugString for other platforms
        #endif
    }
    

}} // SolidFramework::Platform

namespace SolidFramework { namespace Plumbing { 
    void Document::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Plumbing_Document");
        FrameworkObject::Dispose(destructor);
    }
    
    SolidFramework::Interop::NativeError Document::Close() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Plumbing_Document_Close");
        return (SolidFramework::Interop::NativeError)proc(cPtr);
    }
    
    ICatalogPtr Document::GetCatalog() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Plumbing_Document_GetCatalog");
        HANDLE newCPtr = proc(cPtr);
        return SolidFrameworkNative::MakeICatalog(newCPtr, true);
    }
    
    void Document::Create() {
        static InvokeType_VH proc = GetFunction<InvokeType_VH>("SolidFramework_Plumbing_Document_Create");
        proc(cPtr);
    }
    
    bool Document::GetDirty() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Plumbing_Document_Dirty_get");
        return 0 != proc(cPtr);
    }
    
    void Document::SetDirty(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Plumbing_Document_Dirty_set");
        proc(cPtr, value);
    }
    
    std::wstring Document::GetExtension() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Plumbing_Document_GetExtension");
        return proc(cPtr);
    }
    
    bool Document::IsLoaded() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Plumbing_Document_IsLoaded_get");
        return 0 != proc(cPtr);
    }
    
    void Document::Open() {
        static InvokeType_VH proc = GetFunction<InvokeType_VH>("SolidFramework_Plumbing_Document_Open__SWIG_0");
        proc(cPtr);
    }
    
    void Document::Open(std::wstring password) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Plumbing_Document_Open__SWIG_1");
        proc(cPtr, password.c_str());
    }
    
    OverwriteMode Document::GetOverwriteMode() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Plumbing_Document_OverwriteMode_get");
        return (OverwriteMode)proc(cPtr);
    }
    
    void Document::SetOverwriteMode(OverwriteMode value) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Plumbing_Document_OverwriteMode_set");
        proc(cPtr, (int)(value));
    }
    
    std::wstring Document::GetPath() {
        static InvokeType_SH proc = GetFunction<InvokeType_SH>("SolidFramework_Plumbing_Document_GetPath");
        return proc(cPtr);
    }
    
    void Document::SetPath(std::wstring value) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Plumbing_Document_SetPath");
        proc(cPtr, value.c_str());
    }
    
    bool Document::GetRequiresOptimization() {
        static InvokeType_UH proc = GetFunction<InvokeType_UH>("SolidFramework_Plumbing_Document_RequiresOptimization_get");
        return 0 != proc(cPtr);
    }
    
    void Document::SetRequiresOptimization(bool value) {
        static InvokeType_VHU proc = GetFunction<InvokeType_VHU>("SolidFramework_Plumbing_Document_RequiresOptimization_set");
        proc(cPtr, value);
    }
    
    void Document::Save() {
        static InvokeType_VH proc = GetFunction<InvokeType_VH>("SolidFramework_Plumbing_Document_Save__SWIG_0");
        proc(cPtr);
    }
    
    void Document::Save(OverwriteMode mode) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Plumbing_Document_Save__SWIG_1");
        proc(cPtr, (int)(mode));
    }
    
    void Document::Save(OverwriteMode mode, bool useOptimizer) {
        typedef void (SOLIDCALLBACK *InvokeType)(HANDLE, int, unsigned int);
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_Plumbing_Document_Save__SWIG_2");
        proc(cPtr, (int)(mode), useOptimizer);
    }
    
    void Document::SaveAs(std::wstring path) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Plumbing_Document_SaveAs__SWIG_0");
        proc(cPtr, path.c_str());
    }
    
    void Document::SaveAs(std::wstring path, OverwriteMode mode) {
        static InvokeType_VHSI proc = GetFunction<InvokeType_VHSI>("SolidFramework_Plumbing_Document_SaveAs__SWIG_1");
        proc(cPtr, path.c_str(), (int)(mode));
    }
    
    void Document::SaveAs(std::wstring path, OverwriteMode mode, bool useOptimizer) {
        typedef void (SOLIDCALLBACK *InvokeType)(HANDLE, const wchar_t*, int, unsigned int);
        static InvokeType proc = GetFunction<InvokeType>("SolidFramework_Plumbing_Document_SaveAs__SWIG_2");
        proc(cPtr, path.c_str(), (int)(mode), useOptimizer);
    }
    
    void Document::SaveOptimized() {
        static InvokeType_VH proc = GetFunction<InvokeType_VH>("SolidFramework_Plumbing_Document_SaveOptimized__SWIG_0");
        proc(cPtr);
    }
    
    void Document::SaveOptimized(OverwriteMode mode) {
        static InvokeType_VHI proc = GetFunction<InvokeType_VHI>("SolidFramework_Plumbing_Document_SaveOptimized__SWIG_1");
        proc(cPtr, (int)(mode));
    }
    
    void Document::SaveOptimizedAs(std::wstring path) {
        static InvokeType_VHS proc = GetFunction<InvokeType_VHS>("SolidFramework_Plumbing_Document_SaveOptimizedAs__SWIG_0");
        proc(cPtr, path.c_str());
    }
    
    void Document::SaveOptimizedAs(std::wstring path, OverwriteMode mode) {
        static InvokeType_VHSI proc = GetFunction<InvokeType_VHSI>("SolidFramework_Plumbing_Document_SaveOptimizedAs__SWIG_1");
        proc(cPtr, path.c_str(), (int)(mode));
    }
    
    Document::Document() {
        // only called from derived class constructors
    }
    

    ICatalog::ICatalog() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("new_SolidFramework_Plumbing_ICatalog");
        HANDLE handle = proc();
        Wrap(handle, true);
    }
    
    void ICatalog::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Plumbing_ICatalog");
        FrameworkObject::Dispose(destructor);
    }
    
    int ICatalog::GetCurrentPage() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Plumbing_ICatalog_GetCurrentPage");
        return proc(cPtr);
    }
    
    IPagesCollectionPtr ICatalog::GetPages() {
        static InvokeType_HH proc = GetFunction<InvokeType_HH>("SolidFramework_Plumbing_ICatalog_GetPages");
        HANDLE newCPtr = proc(cPtr);
        return newCPtr == nullptr? nullptr : std::make_shared<IPagesCollection>(newCPtr, true);
    }
    

    IPagesCollection::IPagesCollection() {
        static InvokeType_HV proc = GetFunction<InvokeType_HV>("new_SolidFramework_Plumbing_IPagesCollection");
        HANDLE handle = proc();
        Wrap(handle, true);
    }
    
    void IPagesCollection::Dispose() {
        static InvokeType_VH destructor = GetFunction<InvokeType_VH>("delete_SolidFramework_Plumbing_IPagesCollection");
        FrameworkObject::Dispose(destructor);
    }
    
    int IPagesCollection::GetPageCount() {
        static InvokeType_IH proc = GetFunction<InvokeType_IH>("SolidFramework_Plumbing_IPagesCollection_PageCount_get");
        return proc(cPtr);
    }
    
    IPagesCollectionPtr IPagesCollection::DynamicCast(SolidFramework::Pdf::Plumbing::PdfPagesPtr value) {
        return std::dynamic_pointer_cast<IPagesCollection>(value);
    }
    

}} // SolidFramework::Plumbing

