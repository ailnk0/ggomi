/*
 * Version: 10.0.14502
 */

#ifndef __SOLIDFRAMEWORK_H__
#define __SOLIDFRAMEWORK_H__

#include <map>
#include <vector>
#include <string>
#include <memory>
#include <stdlib.h>
#include <functional>
#include <stdexcept>

#if (!defined(__SOLIDWINDOWS__) && !defined(__SOLIDMAC__) && !defined(__SOLIDLINUX__))
// the platform isn't explicitly set, so default to the platform the compiler is running on
#if (defined(__LINUX__) || defined(__linux__) || defined(__linux))
#define __SOLIDLINUX__
#elif (defined(__APPLE__) || defined(__MACH__))
#define __SOLIDMAC__
#elif (defined(_WIN32) || defined(_WIN64))
#define __SOLIDWINDOWS__
#else
NOT_IMPLEMENTED
#endif
#endif


#if defined(__SOLIDWINDOWS__)
#include <stdio.h>
#include <locale.h>
#include <wchar.h>
#include <windows.h>

#define SOLIDCALLBACK __cdecl
#elif (defined(__SOLIDLINUX__) || defined(__SOLIDMAC__))
#define __SOLIDPOSIX__

#include <errno.h>
#include <assert.h>
#include <dlfcn.h>
#include <pthread.h>
#include <codecvt>
#include <locale>
#include <sys/stat.h>

#ifdef __SOLIDLINUX__
#include <string.h>
#endif

#define SOLIDCALLBACK
#else
NOT_IMPLEMENTED
#endif


typedef void* HANDLE;
typedef unsigned char BYTE;


namespace SolidFramework {

    // Loads SolidFramework or returns false if it couldn't load the DLLs
    // if path is set it should be the path to the SolidFramework DLLs
    bool Initialize(std::wstring path=L"");

    // Sets whether SolidFramework should accept both '\' and '/' as directory separators (defaults to false)
    void SupportPlatformIndependentPaths(bool value);

    // Gets the name of the platform SolidFramework has been configured to target (Win32, Win64, Linux or OSX)
    // The target platform can be changed by explicitly defining __SOLIDLINUX__, __SOLIDMAC__ or __SOLIDWINDOWS__
    std::wstring GetTargetPlatformName();

    class FrameworkException: public std::runtime_error
    {
    private:
        std::string rawMessage;
    public:
        FrameworkException();
        FrameworkException(const char* msg);
        FrameworkException(const char* msg, const char* rawMessage);

        std::string GetRawMessage();
    };

    class InvalidLicenseException: public FrameworkException
    {
    public:
        InvalidLicenseException(const char* msg, const char* rawMessage): FrameworkException(msg, rawMessage) {}
    };

    class OperationCancelledException: public FrameworkException
    {
    public:
        OperationCancelledException(const char* msg, const char* rawMessage): FrameworkException(msg, rawMessage) {}
    };

    class BadDataException: public FrameworkException
    {
    public:
        BadDataException(const char* msg, const char* rawMessage): FrameworkException(msg, rawMessage) {}
    };

    class UnknownFrameworkError: public FrameworkException
    {
    public:
        UnknownFrameworkError(const char* msg, const char* rawMessage): FrameworkException(msg, rawMessage) {}
    };

    class FrameworkIOException: public FrameworkException
    {
    public:
        FrameworkIOException(const char* msg, const char* rawMessage): FrameworkException(msg, rawMessage) {}
    };

    class FileNotFoundException: public FrameworkIOException
    {
    private:
        std::wstring path;
    public:
        FileNotFoundException(const char* msg, const char* rawMessage, const wchar_t* path): FrameworkIOException(msg, rawMessage)
        {
            this->path = path;
        }

        std::wstring GetPath();
    };

    class PathTooLongException: public FrameworkIOException
    {
    public:
        PathTooLongException(const char* msg, const char* rawMessage): FrameworkIOException(msg, rawMessage) {}
    };

    class DirectoryNotFoundException: public FrameworkIOException
    {
    public:
        DirectoryNotFoundException(const char* msg, const char* rawMessage): FrameworkIOException(msg, rawMessage) {}
    };

    class FileAlreadyExistsException: public FrameworkIOException
    {
    public:
        FileAlreadyExistsException(const char* msg, const char* rawMessage): FrameworkIOException(msg, rawMessage) {}
    };

    class FormatException: public FrameworkException
    {
    public:
        FormatException(const char* msg, const char* rawMessage): FrameworkException(msg, rawMessage) {}
    };

    class ArgumentException: public FrameworkException
    {
    public:
        ArgumentException(const char* msg, const char* rawMessage): FrameworkException(msg, rawMessage) {}
    };

    class InvalidOperationException: public FrameworkException
    {
    public:
        InvalidOperationException(const char* msg, const char* rawMessage): FrameworkException(msg, rawMessage) {}
    };

    class PasswordRequiredException: public FrameworkException
    {
    public:
        PasswordRequiredException(const char* msg, const char* rawMessage): FrameworkException(msg, rawMessage) {}
    };

} // SolidFramework

namespace SolidFramework { namespace Platform { namespace Plumbing {

    class Logging
    {
    private:
        Logging();
        bool isLogging;
        std::wstring loggingPath;
        unsigned long rotationSize;
      #if defined(__SOLIDWINDOWS__)
        HANDLE hMutex;
      #elif defined(__SOLIDPOSIX__)
        pthread_mutex_t hMutex;
      #else
        NOT_IMPLEMENTED
      #endif
    public:
        ~Logging();
        /// <summary>
        /// Gets the singleton instance.
        /// </summary>
        static Logging& GetInstance();
        static void SetLogPath(const std::wstring& path);
        static void WriteLine(const std::wstring& content);
        static void SetRotationSize(const unsigned long rotationSize);
    };

}}} // SolidFramework::Platform::Plumbing

#define __SOLIDFRAMEWORK_ENUM_FLAGS__(EnumType) \
inline EnumType operator ~ (EnumType a) { return (EnumType)(~((std::underlying_type<EnumType>::type)a)); } \
inline EnumType operator | (EnumType a, EnumType b) { return (EnumType)(((std::underlying_type<EnumType>::type)a) | ((std::underlying_type<EnumType>::type)b)); } \
inline EnumType operator & (EnumType a, EnumType b) { return (EnumType)(((std::underlying_type<EnumType>::type)a) & ((std::underlying_type<EnumType>::type)b)); } \
inline EnumType operator ^ (EnumType a, EnumType b) { return (EnumType)(((std::underlying_type<EnumType>::type)a) ^ ((std::underlying_type<EnumType>::type)b)); } \
inline EnumType& operator |= (EnumType& a, EnumType b) { return a = a | b; } \
inline EnumType& operator &= (EnumType& a, EnumType b) { return a = a & b; } \
inline EnumType& operator ^= (EnumType& a, EnumType b) { return a = a ^ b; }

namespace SolidFramework {

    class FrameworkObject;
    typedef std::shared_ptr<FrameworkObject> FrameworkObjectPtr;

    class FrameworkObject
    {
        typedef void (SOLIDCALLBACK *DestructorCallback)(HANDLE);
    protected:
        HANDLE cPtr;
        bool ownsMemory;
        void Wrap(HANDLE handle, bool ownsObject);
        void Dispose(DestructorCallback destructor);
    public:
        FrameworkObject();

        virtual void Dispose();
        virtual ~FrameworkObject() { Dispose(); }

        static HANDLE GetCPtr(FrameworkObjectPtr owner)
        {
            return owner == nullptr? nullptr : owner->cPtr;
        }
    };

} // SolidFramework


namespace SolidFramework { 

    enum class WarningEventArgsType {
        WarningEventArgs = 0,
        PdfAWarningEventArgs = 1,
    }; // WarningEventArgsType

    // Represents types of Warning Type.
    enum class WarningType {
        // Document contains Non-standard encoded text and TextRecoveryNSE is set to Never.
        NSEText = 0,
        // Document contains CJK fonts and no CJK fonts were found locally to substitute.
        CJKFont = 1,
    }; // WarningType

    class License;
    class PageRange;
    class PdfAWarningEventArgs;
    class ProgressEventArgs;
    class WarningEventArgs;

    typedef std::shared_ptr <License> LicensePtr;
    typedef std::shared_ptr <PageRange> PageRangePtr;
    typedef std::shared_ptr <PdfAWarningEventArgs> PdfAWarningEventArgsPtr;
    typedef std::shared_ptr <ProgressEventArgs> ProgressEventArgsPtr;
    typedef std::shared_ptr <WarningEventArgs> WarningEventArgsPtr;

} // SolidFramework

namespace SolidFramework { namespace Converters { 

    enum class ConverterType {
        Converter = 0,
        PdfToPdfConverter = 1,
        PdfToPdfAConverter = 2,
        SolidConverterPdf = 3,
        PdfToImageConverter = 4,
        PdfToOfficeDocumentConverter = 5,
        PdfToWordConverter = 6,
        PdfToExcelConverter = 7,
        PdfToTextConverter = 8,
        PdfToDataConverter = 9,
        PdfToPowerPointConverter = 10,
        PdfToHtmlConverter = 11,
        PdfToJsonConverter = 12,
    }; // ConverterType

    // HTML Navigation.
    enum class HtmlNavigation {
        // navigation of HTML is absent.
        SingleWithoutNavigation = 0,
        // HTML with Embedded Navigation.
        SingleWithEmbeddedNavigation = 1,
        // HTML into multiple files.
        SplitIntoMultipleFiles = 2,
    }; // HtmlNavigation

    // HTML with Embedded Navigation: pages, bookmarks, headings.
    enum class HtmlSplittingUsing {
        // HTML with Embedded Navigation: Pages.
        Pages = 1,
        // HTML with Embedded Navigation: Bookmarks.
        Bookmarks = 2,
        // HTML with Embedded Navigation: Headings.
        Headings = 4,
    }; // HtmlSplittingUsing
    __SOLIDFRAMEWORK_ENUM_FLAGS__(HtmlSplittingUsing);

    class Converter;
    class PdfToDataConverter;
    class PdfToExcelConverter;
    class PdfToHtmlConverter;
    class PdfToImageConverter;
    class PdfToJsonConverter;
    class PdfToOfficeDocumentConverter;
    class PdfToPdfAConverter;
    class PdfToPdfConverter;
    class PdfToPowerPointConverter;
    class PdfToTextConverter;
    class PdfToWordConverter;
    class SolidConverterPdf;

    typedef std::shared_ptr <Converter> ConverterPtr;
    typedef std::shared_ptr <PdfToDataConverter> PdfToDataConverterPtr;
    typedef std::shared_ptr <PdfToExcelConverter> PdfToExcelConverterPtr;
    typedef std::shared_ptr <PdfToHtmlConverter> PdfToHtmlConverterPtr;
    typedef std::shared_ptr <PdfToImageConverter> PdfToImageConverterPtr;
    typedef std::shared_ptr <PdfToJsonConverter> PdfToJsonConverterPtr;
    typedef std::shared_ptr <PdfToOfficeDocumentConverter> PdfToOfficeDocumentConverterPtr;
    typedef std::shared_ptr <PdfToPdfAConverter> PdfToPdfAConverterPtr;
    typedef std::shared_ptr <PdfToPdfConverter> PdfToPdfConverterPtr;
    typedef std::shared_ptr <PdfToPowerPointConverter> PdfToPowerPointConverterPtr;
    typedef std::shared_ptr <PdfToTextConverter> PdfToTextConverterPtr;
    typedef std::shared_ptr <PdfToWordConverter> PdfToWordConverterPtr;
    typedef std::shared_ptr <SolidConverterPdf> SolidConverterPdfPtr;

}} // SolidFramework::Converters

namespace SolidFramework { namespace Converters { namespace Plumbing { 

    enum class ConversionResultType {
        IConversionResult = 0,
        ConversionResult = 1,
        PdfAConversionResult = 2,
    }; // ConversionResultType

    // Enum that describes the result of an attempt to convert PDF files to Data, Excel, HTML, Images, PDFA, PowerPoint, Text and Word in a single line of code.
    enum class ConversionStatus {
        // Successful conversion.
        Success = 0,
        // Conversion was canceled.
        Canceled = 1,
        // An Internal Error occurred.
        InternalError = 2,
        // Conversion failed.
        Fail = 3,
        // Unavailable action.
        UnavailableAction = 4,
        // Bad input data.
        BadData = 5,
        // I/O Error
        IOError = 6,
        // Output file exists and is locked or readonly.
        IOFileLocked = 7,
        // Invalid page range was specified.
        InvalidPagesRange = 8,
        // Not enough RAM to perform the task.
        NotEnoughMemory = 9,
        // PDF file is password protected.
        FileHasCopyProtection = 10,
        // Unsupported encryption handler.
        UnsupportedEncryptionHandler = 11,
        // Missing security certificate.
        MissingCertificate = 12,
        // Conversion was canceled.
        OCRCanceled = 13,
        // Conversion was canceled because the output file already exists.
        CanceledExists = 14,
        // No tables were found to extract. This will occur when converting to Excel while ignoring non-table content, if only non-table content exists.
        NoTablesToExtract = 15,
        // No images were found to extract.
        NoImagesToExtract = 16,
        // The source document has PDFA Errors, but it was posible to correct these in the reconstructed file. For further details look at the PdfAConversionStatus.
        PdfAError = 20,
        // The source document has fatal PDFA Errors. This prevented conversion from occurring.
        PdfAFatalError = 21,
        // Document already loaded.
        AlreadyLoaded = 30,
        // Wrong security password specified.
        WrongPassword = 31,
        // Conversion status is No User No Owner.
        NoUserNoOwner = 32,
        // Conversion status is No User Owner.
        NoUserOwner = 33,
        // Conversion status is User No Owner.
        UserNoOwner = 34,
        // Conversion status is User Owner.
        UserOwner = 35,
        // Invalid License used.
        InvalidLicense = 36,
        // No Bpp Conversion.
        NoBppConversion = 150,
        // No Gray Scale.
        NoGrayscale = 151,
        // Unsupported mode.
        PSDUnsupportedMode = 152,
        // Unknown conversion status (not set yet).
        Unknown = 200,
        // Conversion failure (an exception that was not caught by SolidFramework)
        ConversionFailure = 252,
        // Option error (argument valid, setting as an option failed)
        OptionError = 253,
        // License error
        LicenseError = 254,
        // Invalid argument
        InvalidArgument = 255,
    }; // ConversionStatus

    enum class ConvertMode {
        Document = 0,
        Images = 1,
        Tables = 2,
        Presentation = 3,
    }; // ConvertMode

    // Data output document types.
    enum class DataDocumentType {
        // CSV
        Csv = 0,
    }; // DataDocumentType

    // The data format to export to.
    enum class DataExportFormat {
        // Export to csv or delimited text file
        Text = 0,
        // Export to xlsx (without styles)
        Excel = 1,
        // Export to MySQL compatible SQL statements
        MySQL = 2,
        // Export to MS SQL Server compatible SQL statements
        MSSQL = 3,
    }; // DataExportFormat

    // The decimal separator used in numeric reconstruction for Excel files.
    enum class DecimalSeparator {
        // Sets decimal separator to period
        Period = 0,
        // Sets decimal separator to comma
        Comma = 1,
    }; // DecimalSeparator

    // The delimiter used in data files.
    enum class DelimiterOptions {
        // Sets delimiter to comma
        Comma = 0,
        // Sets delimiter to semicolon
        Semicolon = 1,
        // Sets delimiter to tab
        Tab = 2,
    }; // DelimiterOptions

    enum class EmbedFontsMode {
        NoFontEmbedding = 0,
        EmbedEditableFonts = 2,
    }; // EmbedFontsMode

    // Excel output document types.
    enum class ExcelDocumentType {
        // Sets format to Xls.
        Xls = 0,
        // Sets format to XlsX.
        XlsX = 1,
    }; // ExcelDocumentType

    enum class ExcelTablesOnSheet {
        PlaceEachTableOnOwnSheet = 0,
        PlaceAllTablesOnSingleSheet = 1,
        PlaceTablesForEachPageOnSingleSheet = 2,
    }; // ExcelTablesOnSheet

    // Sets footnotes handling mode when converting PDFs
    enum class FootnotesMode {
        // Recover footnotes in the output document
        Detect = 0,
        // Recognize footnotes but place them in the body of the output document
        Ignore = 1,
        // Remove footnotes from the output document
        Remove = 2,
    }; // FootnotesMode

    // Sets Headers and Footers handling mode when converting PDFs
    enum class HeaderAndFooterMode {
        // Recover Headers and Footers in the output document
        Detect = 0,
        // Recognize Headers and Footers but place them in the body of the output document
        Ignore = 1,
        // Remove Headers and Footers from the output document
        Remove = 2,
    }; // HeaderAndFooterMode

    // Sets images handling mode when converting to HTML format
    enum class HtmlImages {
        // Default value
        Default = 0,
        // Include linked images into output HTML file
        Link = 0,
        // Do not include images into output HTML file
        Ignore = 1,
        // Embed images into output HTML file
        Embed = 2,
    }; // HtmlImages

    // Html navigation types.
    enum class HtmlNavigationType {
        HtmlNavigationNoNavigation = 0,
        HtmlNavigationPages = 1,
        HtmlNavigationHeadings = 2,
        HtmlNavigationBookmarks = 3,
        HtmlNavigationPagesHeadings = 4,
        HtmlNavigationPagesBookmarks = 5,
        HtmlNavigationHeadingsBookmark = 6,
        HtmlNavigationPagesHeadingsBookmarks = 7,
        HtmlNavigationSplitFileByPages = 8,
        HtmlNavigationSplitFileByHeadings = 9,
        HtmlNavigationSplitFileByBookmarks = 10,
    }; // HtmlNavigationType

    // Sets images anchoring mode in the output Word file
    enum class ImageAnchoringMode {
        // Use automatic anchoring
        Automatic = 0,
        // Anchor images to paragraph
        Paragraph = 1,
        // Anchor images to page
        Page = 2,
        // Remove images from the output file
        RemoveImages = 3,
    }; // ImageAnchoringMode

    // Represents types of Image conversion type
    enum class ImageConversionType {
        // Extract all images from pdf file
        ExtractImages = 0,
        // Create image pages from pdf pages
        ExtractPages = 1,
    }; // ImageConversionType

    // Sets converted images format
    enum class ImageDocumentType {
        // Use optimal - the most appropriate format for each image
        Default = 0,
        // BMP images
        Bmp = 1,
        // Jpeg images
        Jpeg = 2,
        // Png images
        Png = 3,
        // Tiff images
        Tiff = 4,
        // Gif images
        Gif = 5,
    }; // ImageDocumentType

    // The line terminator used in text based files.
    enum class LineTerminator {
        // Set the terminator based on current platform. Allows .Net to determine and set the terminator based on the platform detected.
        Platform = 0,
        // Set the terminator to 0x0D 0x0A (Windows default).
        Windows = 1,
        // Set the terminator to 0x0A (OSX default).
        OSX = 2,
    }; // LineTerminator

    // Sets annotations recovering mode when converting to Word format.
    enum class MarkupAnnotConversionType {
        // Do not recover annotations
        Never = 0,
        // Recover annotations to textboxes
        Textbox = 1,
        // Recover annotations to Word comments
        Comment = 2,
    }; // MarkupAnnotConversionType

    // Represents OCR type for the converted document, i.e. Creating a Searchable Text Layer.
    enum class OcrType {
        // Full OCR
        FullOcr = 0,
        // Create Searchable Text Layer
        CreateSearchableTextLayer = 1,
        // None
        None = 2,
    }; // OcrType

    // Represents types of PDFA conversion status
    enum class PdfAConversionStatus {
        // Document is PDFA compliant
        Compliant = 0,
        // Fail conversion
        Failure = 1,
        // All errors could be fixed
        ErrorsFixed = 2,
        // Not all errors could be fixed
        ErrorsNotFixed = 3,
        // Document has fatal PDFA errors
        ErrorsFatal = 4,
    }; // PdfAConversionStatus

    // Reconstruction mode when converting to Word format
    enum class ReconstructionMode {
        // Use Flowing mode
        Flowing = 0,
        // Use Exact mode
        Exact = 1,
        // Use Continuous mode
        Continuous = 2,
        // Use Plain text
        PlainText = 3,
        // No columns legacy mode
        NoColumns = 4,
    }; // ReconstructionMode

    // Target MS Word Compatibility Version (OOXML file format)
    enum class TargetWordFormat {
        // Not set
        Automatic = 0,
        // MS Word 2007
        Word2007 = 12,
        // MS Word 2010
        Word2010 = 14,
        // MS Word 2013, MS Word 2016, MS Word 2019, MS Word 365, MS Word Online
        Word2013 = 15,
    }; // TargetWordFormat

    // Represents types of Text recovering
    enum class TextRecovery {
        // Never use
        Never = 0,
        // Use always
        Always = 1,
        // Scanned page only
        Automatic = 2,
        // Default value is Never
        Default = 2,
    }; // TextRecovery

    // Configure Automatic GNSE features
    enum class TextRecoveryAutomaticGNse {
        // No automatic GNSE features
        None = 0,
        // Logo glyphs as vector graphics
        Logos = 1,
        // Use GNSE to correct font styles
        Styles = 2,
        // Icon font glyphs as vector graphics
        Icons = 4,
        // Fully automatic correction of common symbolic fonts
        Symbols = 8,
        // Barcode fonts as vector graphics
        Barcodes = 16,
        // Fully automatic correction of ligatures like ff, fl fi, ffi, ti, etc.
        Ligatures = 32,
        // AllCap font detection and correction
        AllCaps = 64,
        // SmallCap font detection and correction
        SmallCaps = 128,
        // Fully automatic correction of alphanumeric glyphs (including automatic by DQ)
        AlphaNum = 256,
        // Fully automatic correction of ligatures like ff, fl fi, ffi, ti, etc.
        ExoticLigatures = 512,
        // All automatic GNSE features
        All = 2047,
    }; // TextRecoveryAutomaticGNse

    // Represents types of Optical Character Recognition (OCR) Text recovery engine, i.e. SolidOCR
    enum class TextRecoveryEngine {
        // Automatic chose from available engines
        Automatic = 0,
        // Default value is Automatic
        Default = 0,
        // Use Microsoft Office Document Imaging if present
        MODI = 1,
        // Use Solid OCR
        SolidOCR = 2,
        // Reserved for and alternate engine (IRIS)
        IRIS = 3,
    }; // TextRecoveryEngine

    // Represents types of Text recovery engine
    enum class TextRecoveryEngineNse {
        // Default value is Automatic
        Default = 0,
        // Automatic chose from available engines
        Automatic = 0,
        // Use NSE-capable OCR engine (like MODI) - never used because NSE would be "off" if MODI is absent
        OCR = 1,
        // Use Solid NSE
        SolidNSE = 2,
    }; // TextRecoveryEngineNse

    // Represents types of Text recovery using Non Standard Encoding (NSE)
    enum class TextRecoveryNSE {
        // Never use NSE
        Never = 0,
        // Every characters
        Always = 1,
        // Problem characters only
        Automatic = 2,
        // Default value is Automatic
        Default = 2,
    }; // TextRecoveryNSE

    // The thousands separator used in numeric reconstruction for Excel files.
    enum class ThousandsSeparator {
        // Sets thousands separator to comma
        Comma = 0,
        // Sets thousands separator to period
        Period = 1,
        // Sets thousands separator to space
        Space = 2,
    }; // ThousandsSeparator

    // Word output document types.
    enum class WordDocumentType {
        // Sets format to Microsoft Office XML.
        WordML = 0,
        // Sets format to RTF.
        Rtf = 1,
        // Sets format to TXT.
        Txt = 2,
        // Sets format to Doc.
        Doc = 3,
        // Sets format to DocX.
        DocX = 4,
        // Sets format to JSON.
        Json = 5,
    }; // WordDocumentType

    class ConversionResult;
    class IConversionResult;
    class ImageWatermark;
    class PdfAConversionResult;
    class SelectedArea;
    class SelectedAreas;
    class TextWatermark;

    typedef std::shared_ptr <ConversionResult> ConversionResultPtr;
    typedef std::shared_ptr <IConversionResult> IConversionResultPtr;
    typedef std::shared_ptr <ImageWatermark> ImageWatermarkPtr;
    typedef std::shared_ptr <PdfAConversionResult> PdfAConversionResultPtr;
    typedef std::shared_ptr <SelectedArea> SelectedAreaPtr;
    typedef std::shared_ptr <SelectedAreas> SelectedAreasPtr;
    typedef std::shared_ptr <TextWatermark> TextWatermarkPtr;

}}} // SolidFramework::Converters::Plumbing

namespace SolidFramework { namespace Imaging { 

    // Represents types of Image encoder
    enum class ImageEncoder {
        // Undefined Image Encoder
        None = 0,
        // Image Encoder is ASCII Hex
        ASCIIHexEncode = 1,
        // Image Encoder is LZW
        LZWEncode = 2,
        // Image Encoder is Flate
        FlateEncode = 3,
        // Image Encoder is ASCII85
        ASCII85Encode = 4,
        // Runs length encoder
        RunLengthEncode = 5,
        // Image Encoder is JBIG2
        JBIG2Encode = 6,
        // Image Encoder is DCT
        DCTEncode = 7,
        // Image Encoder is J2K
        J2KEncode = 8,
        // Image Encoder is CCITT Fax (Group3)
        CCITTFaxEncodeGroup3 = 9,
        // Image Encoder is CCITT Fax (Group4)
        CCITTFaxEncodeGroup4 = 10,
        // Automatic (Optimize size)
        AutomaticSizeOptimize = 11,
        // Automatic (Optimize speed)
        AutomaticSpeedOptimize = 12,
        // Automatic
        Automatic = 13,
    }; // ImageEncoder

    // Represents types of Image type such as colour, greyscale etc.
    enum class ImageType {
        // Undefined Image Type
        None = 0,
        // Image Type is Color
        Color = 1,
        // Image Type is Gray Scale
        Grayscale = 2,
        // Image Type is monochrome
        Monochrome = 3,
    }; // ImageType

    // Represents types of OCR graphic region
    enum class OcrGraphicRegionType {
        // Region is unknown
        Unknown = 0,
        // Region is rectangle
        Rectangle = 1,
        // Region is rectangle
        FilledRectangle = 2,
        // Region is line
        Line = 3,
    }; // OcrGraphicRegionType

    enum class OcrRegionType {
        IOcrRegion = 0,
        OcrImageRegion = 1,
        OcrTextRegion = 2,
        OcrGraphicRegion = 3,
    }; // OcrRegionType

    class Image;
    class IOcrRegion;
    class Ocr;
    class OcrGraphicRegion;
    class OcrImageRegion;
    class OcrLine;
    class OcrResults;
    class OcrTextRegion;
    class OcrWord;

    typedef std::shared_ptr <Image> ImagePtr;
    typedef std::shared_ptr <IOcrRegion> IOcrRegionPtr;
    typedef std::shared_ptr <Ocr> OcrPtr;
    typedef std::shared_ptr <OcrGraphicRegion> OcrGraphicRegionPtr;
    typedef std::shared_ptr <OcrImageRegion> OcrImageRegionPtr;
    typedef std::shared_ptr <OcrLine> OcrLinePtr;
    typedef std::shared_ptr <OcrResults> OcrResultsPtr;
    typedef std::shared_ptr <OcrTextRegion> OcrTextRegionPtr;
    typedef std::shared_ptr <OcrWord> OcrWordPtr;

}} // SolidFramework::Imaging

namespace SolidFramework { namespace Imaging { namespace Plumbing { 

    // Controls the sort order of the results from GetComponentImages. Also controls single versus multiple text component mode.
    enum class ImageComponentOrder {
        // Order of the result components from GetComponentImages is Images, Vectors then Text (order used by Solid Converter PDF to Word using MODI)
        ImagesVectorsText = 0,
        // Order of the result components from GetComponentImages is Images, Text then Vectors
        ImagesTextVectors = 1,
        // Order of the result components from GetComponentImages is Images, Vectors then asingle Text component (order used by Solid Converter PDF to Word using Solid OCR)
        ImagesVectorsSingleText = 2,
        // Order of the result components from GetComponentImages is Images, single Text component then Vectors
        ImagesSingleTextVectors = 3,
    }; // ImageComponentOrder

    // Represents types of Image components
    enum class ImageComponents {
        // GetComponentImages will process and return images, vectors and text components for full OCR
        All = 0,
        // GetComponentImages will process and return only text components (useful for searchable text)
        TextOnly = 1,
        // GetComponentImages will process and return only optimized image components (not for OCR)
        ImagesOnly = 2,
    }; // ImageComponents

    // GetComponentImages will process and return components optimized according to ImageCompression.
    enum class ImageCompression {
        // Unknown Image Compression
        Unknown = 0,
        // Image components will be returned as 24 bpp RGB, 8/4/2 bpp gray, 8/4 bpp indexed color or mono
        ColorOrBetter = 1,
        // Image components will be returned as 4/2 bpp gray or mono
        GrayOrBetter = 2,
        // Image components will be returned as 1 bpp mono
        Mono = 3,
        // Image components will be returned as 1 bpp mono
        FakeMono = 4,
        // Image components will be returned as 4/2 bpp gray or mono
        RealGrayOrBetter = 5,
        // Image components will be returned as a single lossless compressed image
        Lossless = 6,
        // The original images won't be changed
        PreserveOriginal = 7,
    }; // ImageCompression

    // Used by TextLanguage to return the detected CJK language for a text component.
    enum class TextLanguage {
        // Not set.
        Unknown = 0,
        // Text Component appears to be Japanese.
        Japanese = 1,
        // Text Component appears to be Korean.
        Korean = 2,
        // Text Component appears to be Simplified Chinese.
        SimplifiedChinese = 4,
        // Text Component appears to be Traditional Chinese.
        TraditionalChinese = 8,
        // Text Component appears to be Greek.
        Greek = 16,
        // Text Component appears to be Hebrew.
        Hebrew = 32,
        // Text Component appears to be Arabic.
        Arabic = 64,
    }; // TextLanguage



}}} // SolidFramework::Imaging::Plumbing

namespace SolidFramework { namespace Interop { 

    enum class ImageType {
        Unknown = 0,
        Format1bppIndexed = 1,
        Format4bppIndexed = 2,
        Format8bppIndexed = 3,
        Format24bppRgb = 4,
        Format32bppArgb = 5,
    }; // ImageType

    // Represents types of Native errors
    enum class NativeError {
        // Success
        Success = 0,
        // Failure
        Fail = 3,
        // Unavailable action
        UnavailableAction = 4,
        // Bad input data
        BadData = 5,
        // I/O Error
        IOError = 6,
        // Output file is locked
        IOFileLocked = 7,
        // Not enough RAM to perform the task
        NotEnoughMemory = 9,
        // Password is required
        FileHasCopyProtection = 10,
        // Document is already loaded
        AlreadyLoaded = 30,
        // Wrong open password
        WrongPassword = 31,
        // No User No Owner
        NoUserNoOwner = 32,
        // No User Owner
        NoUserOwner = 33,
        // User No Owner
        UserNoOwner = 34,
        // User Owner
        UserOwner = 35,
        // Invalid License
        InvalidLicense = 36,
    }; // NativeError

    // Represents types of Solid error codes
    enum class SolidErrorCodes {
        // Success
        SE_SUCCESS = 0,
        // Canceled
        SE_CANCEL = 1,
        // Was Internal Error
        SE_INTERNALERROR = 2,
        // Failure
        SE_FAILURE = 3,
        // Unavailable action
        SE_UNAVAILABLEACTION = 4,
        // Bad data
        SE_BADDATA = 5,
        // Was I/O Error
        SE_IOERROR = 6,
        // Input file is locked
        SE_IOFILELOCKED = 7,
        // Invalid page range
        SE_INVALIDPAGERANGE = 8,
        // Not enough RAM to perform the task
        SE_NOTENOUGHMEMORY = 9,
        // Password is required
        SE_PDF_HAS_COPYPROTECTION = 10,
        // Unsupported encryption handler
        SE_PDF_UNSUPPORTED_ENCRYPTION_HANDLER = 11,
        // Missing security certificate
        SE_MISSING_CERTIFICATE = 12,
        // OCR was canceled
        SE_PDF_OCR_CANCELED = 13,
        // Canceled, caused output file is already exist
        SE_CANCELED_FILE_EXIST = 14,
        // No tables to extract
        SE_NO_TABLES_TO_EXTRACT = 15,
        // No images to extract
        SE_NO_IMAGES_TO_EXTRACT = 16,
        // Pdf/A problems is fixed
        SE_PDFA_PROBLEMS_ADDRESSED = 20,
        // Pdf/A problems is not fixed
        SE_PDFA_PROBLEMS_NOT_ADDRESSED = 21,
        // Document is already loaded
        SDKAlreadyLoaded = 30,
        // Wrong password for the document
        SE_SSDK_WRONG_PASSWORD = 31,
        // No user, No owner
        SE_SSDK_NOUSER_NOOWNER = 32,
        // No user, Owner
        SE_SSDK_NOUSER_OWNER = 33,
        // User, No owner
        SE_SSDK_USER_NOOWNER = 34,
        // User, owner
        SE_SSDK_USER_OWNER = 35,
        // Invalid license
        SE_SSDK_INVALID_LICENCE = 36,
        // Nothing to print
        SE_NOTING_TO_PRINT = 40,
        // Canceled Pdf/A not compliant
        SE_CANCELED_PDFA_NOT_COMPLIANT = 41,
        // Resulted Pdf/A not compliant
        SE_RESULTED_PDFA_NOT_COMPLIANT = 42,
        // Pdf is encrypted
        SE_PDF_ENCRYPTED = 43,
        // Reserved first value in printing range
        SE_PRINTING_RESERVED_FIRST = 50,
        // Reserved last value in printing range
        SE_PRINTING_RESERVED_LAST = 60,
        // No BPP conversion
        SE_NOBPPCONVERSION = 150,
        // No Gray scale
        SE_NOGRAYSCALE = 151,
        // Unsupported PSD mode
        SE_PSDUNSUPPORTEDMODE = 152,
        SE_UNKNOWN = 200,
    }; // SolidErrorCodes

    class Bitmap;
    class Color;
    class DateTime;
    class Matrix;
    class Point;
    class PointF;
    class Rectangle;
    class RectangleF;
    class Size;
    class SizeF;
    class TimeSpan;
    class Uri;

    typedef std::shared_ptr <Bitmap> BitmapPtr;
    typedef std::shared_ptr <Color> ColorPtr;
    typedef std::shared_ptr <DateTime> DateTimePtr;
    typedef std::shared_ptr <Matrix> MatrixPtr;
    typedef std::shared_ptr <Point> PointPtr;
    typedef std::shared_ptr <PointF> PointFPtr;
    typedef std::shared_ptr <Rectangle> RectanglePtr;
    typedef std::shared_ptr <RectangleF> RectangleFPtr;
    typedef std::shared_ptr <Size> SizePtr;
    typedef std::shared_ptr <SizeF> SizeFPtr;
    typedef std::shared_ptr <TimeSpan> TimeSpanPtr;
    typedef std::shared_ptr <Uri> UriPtr;

}} // SolidFramework::Interop

namespace SolidFramework { namespace Language { 

    class Language;

    typedef std::shared_ptr <Language> LanguagePtr;

}} // SolidFramework::Language

namespace SolidFramework { namespace Model { 

    class BookmarksCollection;
    class CoreModel;
    class FontsCollection;
    class HyperlinksCollection;
    class ListsCollection;
    class PdfOptions;
    class StyleTemplatesCollection;
    class Topic;

    typedef std::shared_ptr <BookmarksCollection> BookmarksCollectionPtr;
    typedef std::shared_ptr <CoreModel> CoreModelPtr;
    typedef std::shared_ptr <FontsCollection> FontsCollectionPtr;
    typedef std::shared_ptr <HyperlinksCollection> HyperlinksCollectionPtr;
    typedef std::shared_ptr <ListsCollection> ListsCollectionPtr;
    typedef std::shared_ptr <PdfOptions> PdfOptionsPtr;
    typedef std::shared_ptr <StyleTemplatesCollection> StyleTemplatesCollectionPtr;
    typedef std::shared_ptr <Topic> TopicPtr;

}} // SolidFramework::Model

namespace SolidFramework { namespace Model { namespace Export { 

    class ExcelOptions;
    class ExportOptions;
    class HtmlOptions;
    class PowerPointOptions;
    class TxtOptions;
    class WordOptions;

    typedef std::shared_ptr <ExcelOptions> ExcelOptionsPtr;
    typedef std::shared_ptr <ExportOptions> ExportOptionsPtr;
    typedef std::shared_ptr <HtmlOptions> HtmlOptionsPtr;
    typedef std::shared_ptr <PowerPointOptions> PowerPointOptionsPtr;
    typedef std::shared_ptr <TxtOptions> TxtOptionsPtr;
    typedef std::shared_ptr <WordOptions> WordOptionsPtr;

}}} // SolidFramework::Model::Export

namespace SolidFramework { namespace Model { namespace Layout { 

    enum class LayoutObjectType {
        Page = 0,
        Image = 1,
        Table = 2,
        Group = 3,
        Graphic = 4,
        TextBox = 5,
        Paragraph = 6,
        ParagraphLine = 7,
        ParagraphWord = 8,
        ParagraphListItem = 9,
        Object = 100,
    }; // LayoutObjectType

    class LayoutChunk;
    class LayoutChunkLine;
    class LayoutChunkParagraph;
    class LayoutDocument;
    class ILayoutObjectContainer;
    class LayoutGraphic;
    class LayoutGroup;
    class LayoutImage;
    class LayoutObject;
    class LayoutPage;
    class LayoutParagraph;
    class LayoutParagraphLine;
    class LayoutParagraphListItem;
    class LayoutParagraphWord;
    class LayoutTable;
    class LayoutTextBox;

    typedef std::shared_ptr <LayoutChunk> LayoutChunkPtr;
    typedef std::shared_ptr <LayoutChunkLine> LayoutChunkLinePtr;
    typedef std::shared_ptr <LayoutChunkParagraph> LayoutChunkParagraphPtr;
    typedef std::shared_ptr <LayoutDocument> LayoutDocumentPtr;
    typedef std::shared_ptr <ILayoutObjectContainer> ILayoutObjectContainerPtr;
    typedef std::shared_ptr <LayoutGraphic> LayoutGraphicPtr;
    typedef std::shared_ptr <LayoutGroup> LayoutGroupPtr;
    typedef std::shared_ptr <LayoutImage> LayoutImagePtr;
    typedef std::shared_ptr <LayoutObject> LayoutObjectPtr;
    typedef std::shared_ptr <LayoutPage> LayoutPagePtr;
    typedef std::shared_ptr <LayoutParagraph> LayoutParagraphPtr;
    typedef std::shared_ptr <LayoutParagraphLine> LayoutParagraphLinePtr;
    typedef std::shared_ptr <LayoutParagraphListItem> LayoutParagraphListItemPtr;
    typedef std::shared_ptr <LayoutParagraphWord> LayoutParagraphWordPtr;
    typedef std::shared_ptr <LayoutTable> LayoutTablePtr;
    typedef std::shared_ptr <LayoutTextBox> LayoutTextBoxPtr;

}}} // SolidFramework::Model::Layout

namespace SolidFramework { namespace Model { namespace Pdf { namespace Pages { 

    // Represents types of Model Changed Event.
    enum class ModelChangedEvent {
        // Type of Model Changed Event is Clear.
        Clear = 0,
        // Type of Model Changed Event is Rotation.
        Rotation = 1,
        // Type of Model Changed Event is Start Import.
        StartImport = 2,
        // Type of Model Changed Event is End Import.
        EndImport = 3,
        // Type of Model Changed Event is Reset.
        Reset = 4,
        // Type of Model Changed Event is GenericChange.
        GenericChange = 5,
    }; // ModelChangedEvent

    // Represents types of Model Operation.
    enum class ModelOperation {
        // Type of Model Operation is Undefined.
        Undefined = 0,
        // Type of Model Operation is Insert.
        Insert = 1,
        // Type of Model Operation is Move.
        Move = 2,
        // Type of Model Operation is Copy.
        Copy = 3,
        // Type of Model Operation is Rotate.
        Rotate = 4,
        // Type of Model Operation is Delete.
        Delete = 5,
        // Type of Model Operation is Undo.
        Undo = 6,
        // Type of Model Operation is Redo.
        Redo = 7,
        AddComment = 8,
        DeleteComment = 9,
        ModifyComment = 10,
    }; // ModelOperation

    enum class PageHolderType {
        IPageHolder = 0,
        PdfPageHolder = 1,
        ImagePageHolder = 2,
    }; // PageHolderType

    enum class PageSourceType {
        IPageSource = 0,
        PdfPageSource = 1,
        ImagePageSource = 2,
    }; // PageSourceType

    enum class ParameterType {
        Undefined = 0,
        AddComment = 1,
        DeleteComment = 2,
        ModifyComment = 3,
        Insert = 4,
        Int = 5,
    }; // ParameterType

    // Represents types of Progress Phase.
    enum class ProgressPhase {
        // Type of Progress Phase is None.
        None = 0,
        // Type of Progress Phase is Pages.
        Pages = 1,
        // Type of Progress Phase is Segments.
        Segments = 2,
        // Type of Progress Phase is Complete.
        Complete = 3,
    }; // ProgressPhase

    class ImagePageHolder;
    class ImagePageSource;
    class IntParameter;
    class IPageHolder;
    class IPageSource;
    class ModelChangedEventArgs;
    class OfferPasswordEventArgs;
    class Operation;
    class PagesChangedEventArgs;
    class PageSelection;
    class PagesModel;
    class Parameter;
    class PasswordEnteredEventArgs;
    class PdfPageHolder;
    class PdfPageSource;
    class RepairFailedEventArgs;
    class SaveProgressEventArgs;
    class ThumbnailChangedEventArgs;
    class Transaction;

    typedef std::shared_ptr <ImagePageHolder> ImagePageHolderPtr;
    typedef std::shared_ptr <ImagePageSource> ImagePageSourcePtr;
    typedef std::shared_ptr <IntParameter> IntParameterPtr;
    typedef std::shared_ptr <IPageHolder> IPageHolderPtr;
    typedef std::shared_ptr <IPageSource> IPageSourcePtr;
    typedef std::shared_ptr <ModelChangedEventArgs> ModelChangedEventArgsPtr;
    typedef std::shared_ptr <OfferPasswordEventArgs> OfferPasswordEventArgsPtr;
    typedef std::shared_ptr <Operation> OperationPtr;
    typedef std::shared_ptr <PagesChangedEventArgs> PagesChangedEventArgsPtr;
    typedef std::shared_ptr <PageSelection> PageSelectionPtr;
    typedef std::shared_ptr <PagesModel> PagesModelPtr;
    typedef std::shared_ptr <Parameter> ParameterPtr;
    typedef std::shared_ptr <PasswordEnteredEventArgs> PasswordEnteredEventArgsPtr;
    typedef std::shared_ptr <PdfPageHolder> PdfPageHolderPtr;
    typedef std::shared_ptr <PdfPageSource> PdfPageSourcePtr;
    typedef std::shared_ptr <RepairFailedEventArgs> RepairFailedEventArgsPtr;
    typedef std::shared_ptr <SaveProgressEventArgs> SaveProgressEventArgsPtr;
    typedef std::shared_ptr <ThumbnailChangedEventArgs> ThumbnailChangedEventArgsPtr;
    typedef std::shared_ptr <Transaction> TransactionPtr;

}}}} // SolidFramework::Model::Pdf::Pages

namespace SolidFramework { namespace Model { namespace Pdf { namespace Pages { namespace Operations { 

    class AddComment;
    class AddCommentParameter;
    class Copy;
    class Delete;
    class DeleteComment;
    class DeleteCommentParameter;
    class Insert;
    class InsertParameter;
    class ModifyComment;
    class ModifyCommentParameter;
    class Move;
    class Redo;
    class Rotate;
    class Undo;

    typedef std::shared_ptr <AddComment> AddCommentPtr;
    typedef std::shared_ptr <AddCommentParameter> AddCommentParameterPtr;
    typedef std::shared_ptr <Copy> CopyPtr;
    typedef std::shared_ptr <Delete> DeletePtr;
    typedef std::shared_ptr <DeleteComment> DeleteCommentPtr;
    typedef std::shared_ptr <DeleteCommentParameter> DeleteCommentParameterPtr;
    typedef std::shared_ptr <Insert> InsertPtr;
    typedef std::shared_ptr <InsertParameter> InsertParameterPtr;
    typedef std::shared_ptr <ModifyComment> ModifyCommentPtr;
    typedef std::shared_ptr <ModifyCommentParameter> ModifyCommentParameterPtr;
    typedef std::shared_ptr <Move> MovePtr;
    typedef std::shared_ptr <Redo> RedoPtr;
    typedef std::shared_ptr <Rotate> RotatePtr;
    typedef std::shared_ptr <Undo> UndoPtr;

}}}}} // SolidFramework::Model::Pdf::Pages::Operations

namespace SolidFramework { namespace Model { namespace Pdf { namespace Pages { namespace Plumbing { 

    // This enum provides internal support for the PageView
    enum class AnnotationType {
        Annotation = 0,
        Link = 1,
        URILink = 2,
        GoToLink = 3,
        Comment = 4,
        HighlightComment = 5,
        StrikeOutComment = 6,
        UnderlineComment = 7,
        TextComment = 8,
    }; // AnnotationType

    enum class BorderStyle {
        None = 0,
        Box = 1,
        Underline = 2,
    }; // BorderStyle

    // This class primarily provides internal support for the DocumentPropertiesForm.
    enum class NavigationTab {
        // Represents navigation tab is page only.
        PageOnly = 0,
        // Represents navigation tab are bookmarks and pape.
        BookmarksAndPage = 1,
        // Represents navigation tab are thumbnails and page.
        ThumbnailsAndPage = 2,
        // Represents navigation tab are attachments and page.
        AttachmentsAndPage = 3,
        // Represents navigation tab are layers and page.
        LayersAndPage = 4,
    }; // NavigationTab

    // This class primarily provides internal support for the PageView and PdfViewer forms.
    enum class PageLayout {
        // Represents initial page layout is single page.
        SinglePage = 0,
        // Represents initial page layout is page continuous.
        SinglePageContinuous = 1,
        // Represents initial page layout is two up facing.
        TwoUpFacing = 2,
        // Represents initial page layout is two facing continuous.
        TwoUpFacingContinuous = 3,
        // Represents initial page layout is two up cover page.
        TwoUpCoverPage = 4,
        // Represents initial page layout is tw up cover page continuous.
        TwoUpCoverPageContinuous = 5,
    }; // PageLayout

    // This class primarily provides internal support for the PageView and PdfViewer forms.
    enum class PageMagnification {
        // Represents magnification is default..
        Default = 0,
        // Represents magnification is scaled.
        Scaled = 1,
        // Represents magnification is fit page.
        FitPage = 2,
        // Represents magnification is fit width.
        FitWidth = 3,
        // Represents magnification is height.
        FitHeight = 4,
        // Represents magnification is visible.
        FitVisible = 5,
    }; // PageMagnification

    class Annotation;
    class Comment;
    class Description;
    class GoToLink;
    class HighlightComment;
    class InitialView;
    class Link;
    class ReadOnlyInfo;
    class StrikeOutComment;
    class TextComment;
    class UnderlineComment;
    class URILink;

    typedef std::shared_ptr <Annotation> AnnotationPtr;
    typedef std::shared_ptr <Comment> CommentPtr;
    typedef std::shared_ptr <Description> DescriptionPtr;
    typedef std::shared_ptr <GoToLink> GoToLinkPtr;
    typedef std::shared_ptr <HighlightComment> HighlightCommentPtr;
    typedef std::shared_ptr <InitialView> InitialViewPtr;
    typedef std::shared_ptr <Link> LinkPtr;
    typedef std::shared_ptr <ReadOnlyInfo> ReadOnlyInfoPtr;
    typedef std::shared_ptr <StrikeOutComment> StrikeOutCommentPtr;
    typedef std::shared_ptr <TextComment> TextCommentPtr;
    typedef std::shared_ptr <UnderlineComment> UnderlineCommentPtr;
    typedef std::shared_ptr <URILink> URILinkPtr;

}}}}} // SolidFramework::Model::Pdf::Pages::Plumbing

namespace SolidFramework { namespace Model { namespace Plumbing { 

    // Represents alignment to be used for a paragraph.
    enum class Alignment {
        // Represents alignment is left.
        Left = 0,
        // Represents alignment is center.
        Center = 1,
        // Represents alignment is right.
        Right = 2,
        // Represents alignment is justified.
        Justified = 3,
    }; // Alignment

    // Represents BookmarkType.
    enum class BookmarkType {
        Nothing = 0,
        Bookmark = 1,
        CommentReference = 2,
        BookmarkAndComment = 3,
    }; // BookmarkType

    // Represents line type for borders and underlines.
    enum class BorderLineType {
        // Regular line.
        Default = 1,
        // Double regular line.
        Double = 2,
        // Thick line.
        Bold = 3,
        // Dashed line with large dashes.
        BigDashed = 4,
        // Dashed line with small dashes.
        SmallDashed = 5,
        // None.
        None = -1,
    }; // BorderLineType

    // Represents cell vertical alignment.
    enum class CellVerticalAlignment {
        // Represents vertical alignment is top.
        Top = 0,
        // Represents vertical alignment is centered.
        Centered = 1,
        // Represents vertical alignment is bottom.
        Bottom = 2,
    }; // CellVerticalAlignment

    // Represents how characters should be positioned (superscript/subscript/normal).
    enum class CharacterPosition {
        // Represents normal alignment.
        Normal = 0,
        // Represents superscript.
        Superscript = 1,
        // Represents subscript.
        Subscript = 2,
    }; // CharacterPosition

    // Specifies the Type of Fillstyle.
    // NOTE: Currently only "Solid" is supported within the SDK.
    enum class FillStyleType {
        Solid = 0,
        Gradient = 1,
        Image = 2,
    }; // FillStyleType

    // Gets the type of GraphicSegment, at its simplest "straight" or "curved".
    enum class GraphicSegmentType {
        GraphicSegment = 0,
        BezierGraphicSegment = 1,
    }; // GraphicSegmentType

    // Represents horizontal alignment.
    enum class HorizontalAlignment {
        // Represents horizontal alignment is left.
        Left = 0,
        // Represents horizontal alignment is centered.
        Centered = 1,
        // Represents horizontal alignment is right.
        Right = 2,
        // Represents horizontal alignment is offset.
        Offset = 3,
        // Represents horizontal alignment is justified.
        Justified = 4,
    }; // HorizontalAlignment

    // Represents horizontal anchoring.
    enum class HorizontalAnchoring {
        // Represents horizontal anchoring is margin.
        Margin = 0,
        // Represents horizontal anchoring is page.
        Page = 1,
        // Represents horizontal anchoring is column.
        Column = 2,
        // Represents horizontal anchoring is character.
        Character = 3,
    }; // HorizontalAnchoring

    // Represents how a line should be drawn, e.g. single, double or triple.
    enum class LineCompoundType {
        None = 0,
        Single = 1,
        Double = 2,
        ThickThin = 3,
        ThinThick = 4,
        Triple = 5,
        ThinThickThin = 6,
    }; // LineCompoundType

    // Represents the type of dash that should be used to draw a line
    enum class LineDashType {
        Solid = 0,
        ShortDash = 1,
        ShortDot = 2,
        ShortDashDot = 3,
        ShortDashDotDot = 4,
        Dot = 5,
        Dash = 6,
        LongDash = 7,
        DashDot = 8,
        LongDashDot = 9,
        LongDashDotDot = 10,
        Custom = 11,
        None = -1,
    }; // LineDashType

    // Represents line spacing type.
    enum class LineSpacingType {
        // Line spacing determined by the number of lines indicated.
        Multiply = 0,
        // Line spacing is only the exact maximum amount of space required. This setting commonly uses less space than single spacing.
        Exact = 1,
        // Line spacing is always at least a specified amount. The amount is specified separately.
        AtLeast = 2,
    }; // LineSpacingType

    // Represents formats for list styles, e.g. bullet, numeric, or Roman.
    enum class ListFormat {
        Bullet = 0,
        // 1, 2, ..., 9
        Decimal = 1,
        // i, ii, ..., ix
        LowerRoman = 2,
        // I, II, ..., IX
        UpperRoman = 3,
        // a, b, ..., aa
        LowerLatin = 4,
        // A, B, ..., AA
        UpperLatin = 5,
        // 01, 02, ..., 09
        DecimalZero = 6,
    }; // ListFormat

    enum class OptionsType {
        Options = 0,
        PdfOptions = 1,
        ExportOptions = 2,
        TxtOptions = 3,
        HtmlOptions = 4,
        WordOptions = 5,
        ExcelOptions = 6,
        PowerPointOptions = 7,
    }; // OptionsType

    // Represents page number formats for page number fields.
    enum class PageNumberFormat {
        None = 0,
        // 1, 2, ..., 9
        Decimal = 1,
        // i, ii, ..., ix
        LowerRoman = 2,
        // I, II, ..., IX
        UpperRoman = 3,
        // a, b, ..., aa
        LowerLatin = 4,
        // A, B, ..., AA
        UpperLatin = 5,
    }; // PageNumberFormat

    // Represents how a row.
    enum class RowHeightRule {
        // The row height is a suggestion and the row should grow or shrink to fit its content.
        Auto = 0,
        // The row height is the minimum size that should be used and the row should grow to fit its content.
        AtLeast = 1,
        // The row height is the exact size that should be used and the row should not grow or shrink.
        Exact = 2,
    }; // RowHeightRule

    // Represents section break.
    enum class SectionBreak {
        // Starts the new section on the same page. One of the most common reasons for using
        Continuous = 0,
        // Starts the new section on the following page.
        NextPage = 1,
        // Starts the new section on the next even-numbered page.
        EvenPage = 2,
        // Starts the new section on the next odd-numbered page.
        OddPage = 3,
    }; // SectionBreak

    // Represents PowerPoint slide layout type.
    enum class SlideLayout {
        Undefined = 0,
        TitleSlide = 1,
        TitleAndContent = 2,
        TitleAndTwoContent = 3,
        TitleOnly = 4,
        Blank = 5,
    }; // SlideLayout

    enum class SolidObjectType {
        SOT_SolidObject = 1,
        SOT_TextStyle = 3,
        SOT_ParagraphStyle = 4,
        SOT_TableStyle = 5,
        SOT_Font = 7,
        SOT_Paragraph = 8,
        SOT_Section = 9,
        SOT_Topic = 10,
        SOT_Shape = 12,
        SOT_Group = 13,
        SOT_Image = 14,
        SOT_Graphic = 15,
        SOT_TextBox = 16,
        SOT_Table = 17,
        SOT_Cell = 18,
        SOT_Header = 19,
        SOT_Footer = 20,
        SOT_HeaderFooterBlock = 21,
        SOT_Bookmark = 36,
        SOT_PageBookmark = 37,
        SOT_ListStyle = 38,
        SOT_ListStylesCollection = 39,
        SOT_List = 40,
        SOT_InternalHyperlink = 41,
        SOT_ExternalHyperlink = 42,
        SOT_TableOfContents = 43,
        SOT_ParagraphStylesCollection = 44,
        SOT_FontsCollection = 45,
        SOT_BookmarksCollection = 46,
        SOT_HyperlinksCollection = 47,
        SOT_ListsCollection = 48,
        SOT_Hyperlink = 101,
        SOT_Style = 103,
        SOT_SolidCollection = 104,
    }; // SolidObjectType

    enum class StyleType {
        Style = 0,
        TextStyle = 1,
        ParagraphStyle = 2,
        ListStyle = 3,
    }; // StyleType

    // Represents tab alignment.
    enum class TabAlignment {
        // Represents alignment is left.
        Left = 0,
        // Represents alignment is center.
        Center = 1,
        // Represents alignment is right.
        Right = 2,
    }; // TabAlignment

    // Represents how the space leading up to the tab should be filled.
    enum class TabLeader {
        // No tab stop leader.
        None = 0,
        // Dotted leader line.
        Dot = 1,
        // Dashed tab stop leader line.
        Hyphen = 2,
        // Solid leader line.
        Underscore = 3,
        // Heavy solid leader line.
        Heavy = 4,
        // Dotted leader line.
        MiddleDot = 5,
    }; // TabLeader

    // Represents text direction.
    enum class TextDirection {
        // Represents text direction is horizontal.
        Horizontal = 0,
        // Represents text direction is vertical.
        Vertical = 1,
        // Represents text direction is rotate 90.
        Rotate90 = 2,
        // Represents text direction is rotate 270.
        Rotate270 = 3,
        // Represents text direction rotate asian 270.
        RotateAsian270 = 4,
    }; // TextDirection

    // Represents the type of object that formatted text refers to to.
    enum class TextType {
        Undetermined = 0,
        Empty = 1,
        General = 2,
        Numeric = 4,
        Currency = 8,
        Percentage = 16,
        NumberLike = 28,
        Date = 32,
        Time = 64,
        DateTime = 128,
        DateLike = 224,
        Unknown = 225,
        Text = 256,
        AlphaNumeric = 512,
        TextLike = 770,
        EmailAddress = 1024,
    }; // TextType
    __SOLIDFRAMEWORK_ENUM_FLAGS__(TextType);

    enum class VerticalAlignment {
        Top = 0,
        Centered = 1,
        Bottom = 2,
        Inside = 3,
        Outside = 4,
        Offset = 5,
    }; // VerticalAlignment

    // Represents vertical anchording.
    enum class VerticalAnchoring {
        // Represents vertical anchording is margin.
        Margin = 0,
        // Represents vertical anchording is page.
        Page = 1,
        // Represents vertical anchording is paragraph.
        Paragraph = 2,
        // Represents vertical anchording is line.
        Line = 3,
    }; // VerticalAnchoring

    // Represents WrappingType.
    enum class WrappingType {
        SquareBoth = 0,
        SquareLeft = 1,
        SquareRight = 2,
        SquareLargest = 3,
        TightBoth = 4,
        TightLeft = 5,
        TightRight = 6,
        TightLargest = 7,
        ThroughBoth = 8,
        ThroughLeft = 9,
        ThroughRight = 10,
        ThroughLargest = 11,
        TopBottom = 12,
        Behind = 13,
        Infront = 14,
        Inline = 15,
    }; // WrappingType

    class BezierGraphicSegment;
    class Bookmark;
    class Borders;
    class Cell;
    class Column;
    class Columns;
    class ExternalHyperlink;
    class FillStyle;
    class Font;
    class GradientFill;
    class Graphic;
    class GraphicPath;
    class GraphicSegment;
    class Group;
    class HeaderFooter;
    class HeaderFooterBlockCollection;
    class Hyperlink;
    class ImageFill;
    class ImageShape;
    class InternalHyperlink;
    class LineStyle;
    class List;
    class ListLevels;
    class ListStyle;
    class Margins;
    class MatrixSO;
    class Options;
    class Paragraph;
    class ParagraphStyle;
    class ParagraphStyleTemplate;
    class Row;
    class Run;
    class Section;
    class Shape;
    class SolidCollection;
    class SolidFill;
    class SolidObject;
    class ISolidObjectContainer;
    class Style;
    class StyleTemplate;
    class Table;
    class TabStop;
    class TextBox;
    class TextStyle;
    class TextStyleTemplate;
    class TOC;

    typedef std::shared_ptr <BezierGraphicSegment> BezierGraphicSegmentPtr;
    typedef std::shared_ptr <Bookmark> BookmarkPtr;
    typedef std::shared_ptr <Borders> BordersPtr;
    typedef std::shared_ptr <Cell> CellPtr;
    typedef std::shared_ptr <Column> ColumnPtr;
    typedef std::shared_ptr <Columns> ColumnsPtr;
    typedef std::shared_ptr <ExternalHyperlink> ExternalHyperlinkPtr;
    typedef std::shared_ptr <FillStyle> FillStylePtr;
    typedef std::shared_ptr <Font> FontPtr;
    typedef std::shared_ptr <GradientFill> GradientFillPtr;
    typedef std::shared_ptr <Graphic> GraphicPtr;
    typedef std::shared_ptr <GraphicPath> GraphicPathPtr;
    typedef std::shared_ptr <GraphicSegment> GraphicSegmentPtr;
    typedef std::shared_ptr <Group> GroupPtr;
    typedef std::shared_ptr <HeaderFooter> HeaderFooterPtr;
    typedef std::shared_ptr <HeaderFooterBlockCollection> HeaderFooterBlockCollectionPtr;
    typedef std::shared_ptr <Hyperlink> HyperlinkPtr;
    typedef std::shared_ptr <ImageFill> ImageFillPtr;
    typedef std::shared_ptr <ImageShape> ImageShapePtr;
    typedef std::shared_ptr <InternalHyperlink> InternalHyperlinkPtr;
    typedef std::shared_ptr <LineStyle> LineStylePtr;
    typedef std::shared_ptr <List> ListPtr;
    typedef std::shared_ptr <ListLevels> ListLevelsPtr;
    typedef std::shared_ptr <ListStyle> ListStylePtr;
    typedef std::shared_ptr <Margins> MarginsPtr;
    typedef std::shared_ptr <MatrixSO> MatrixSOPtr;
    typedef std::shared_ptr <Options> OptionsPtr;
    typedef std::shared_ptr <Paragraph> ParagraphPtr;
    typedef std::shared_ptr <ParagraphStyle> ParagraphStylePtr;
    typedef std::shared_ptr <ParagraphStyleTemplate> ParagraphStyleTemplatePtr;
    typedef std::shared_ptr <Row> RowPtr;
    typedef std::shared_ptr <Run> RunPtr;
    typedef std::shared_ptr <Section> SectionPtr;
    typedef std::shared_ptr <Shape> ShapePtr;
    typedef std::shared_ptr <SolidCollection> SolidCollectionPtr;
    typedef std::shared_ptr <SolidFill> SolidFillPtr;
    typedef std::shared_ptr <SolidObject> SolidObjectPtr;
    typedef std::shared_ptr <ISolidObjectContainer> ISolidObjectContainerPtr;
    typedef std::shared_ptr <Style> StylePtr;
    typedef std::shared_ptr <StyleTemplate> StyleTemplatePtr;
    typedef std::shared_ptr <Table> TablePtr;
    typedef std::shared_ptr <TabStop> TabStopPtr;
    typedef std::shared_ptr <TextBox> TextBoxPtr;
    typedef std::shared_ptr <TextStyle> TextStylePtr;
    typedef std::shared_ptr <TextStyleTemplate> TextStyleTemplatePtr;
    typedef std::shared_ptr <TOC> TOCPtr;

}}} // SolidFramework::Model::Plumbing

namespace SolidFramework { namespace Pdf { 

    enum class AccessPermissions {
        // No restrictions
        None = 0,
        // Allows limited printing
        LimitedPrinting = 4,
        // Allows content editing
        ContentEditing = 8,
        // Allows extracting. This must be set (or AccessPermissions must be None) for document reconstruction to be possible
        Extracting = 16,
        // Allows form field editing
        FormFieldEditing = 32,
        // Allows existing form field editing
        ExistingFormFieldFilling = 256,
        // Allows access for disabilities
        AccessForDisabilities = 512,
        // Allows document assembling
        DocumentAssemblying = 1024,
        // Allows high quality printing
        HighQualityPrinting = 2048,
        // Allows printing
        Printing = 2052,
        // Allows all
        All = 3900,
        // Owner mode.
        Owner = 65535,
    }; // AccessPermissions
    __SOLIDFRAMEWORK_ENUM_FLAGS__(AccessPermissions);

    // Represents types of Authentication mode
    enum class AuthenticationModeType {
        // None - no security has been applied to the document
        None = 0,
        // User authentication
        User = 1,
        // Owner authentication
        Owner = 2,
    }; // AuthenticationModeType

    // Represents types of Duplex
    enum class Duplex {
        // Simplex duplex
        Simplex = 0,
        // Flip Short Edge duplex
        DuplexFlipShortEdge = 1,
        // Flip Long Edge duplex
        DuplexFlipLongEdge = 2,
    }; // Duplex

    // Represents types of Encryption algorithm
    enum class EncryptionAlgorithm {
        // Undefined Encryption Algorithm
        Undefined = 0,
        // Acrobat 3 Encryption Algorithm
        Acrobat3 = 2,
        // Encryption Algorithm is RC4 40 bits (Acrobat 3)
        RC440Bit = 2,
        // Acrobat 5 Encryption Algorithm
        Acrobat5 = 3,
        // Encryption Algorithm is RC4 128 bits (Acrobat 5)
        RC4128Bit = 3,
        // Acrobat 7 Encryption Algorithm
        Acrobat7 = 4,
        // Encryption Algorithm is AES 128 bits (Acrobat 7)
        Aes128Bits = 4,
        // Acrobat 9 Encryption Algorithm
        Acrobat9 = 5,
        // Encryption Algorithm is AES 256 bits (Acrobat 9)
        Aes256Bits = 5,
        // Acrobat X Encryption Algorithm
        AcrobatX = 6,
        // Encryption Algorithm is AES 256 bits (Acrobat X)
        Aes256BitsX = 6,
    }; // EncryptionAlgorithm

    // Represents types of Non full screen page mode
    enum class NonFullScreenPageMode {
        // None
        UseNone = 0,
        // Use outlines
        UseOutlines = 1,
        // Use Thumbs
        UseThumbs = 2,
        // Use OC
        UseOC = 3,
        // Use Attachments
        UseAttachments = 4,
    }; // NonFullScreenPageMode

    // Represents types of Page boundaries
    enum class PageBoundary {
        // Page boundaries is CropBox
        CropBox = 0,
        // Page boundaries MediaBox
        MediaBox = 1,
        // Page boundaries BleedBox
        BleedBox = 2,
        // Page boundaries TrimBox
        TrimBox = 3,
        // Page boundaries ArtBox
        ArtBox = 4,
    }; // PageBoundary

    enum class PageLayout {
        SinglePage = 0,
        OneColumn = 1,
        TwoColumnLeft = 2,
        TwoColumnRight = 3,
        TwoPageLeft = 4,
        TwoPageRight = 5,
    }; // PageLayout

    enum class PageMode {
        UseNone = 0,
        UseOutlines = 1,
        UseThumbs = 2,
        FullScreen = 3,
        UseOC = 4,
        UseAttachments = 5,
    }; // PageMode

    enum class PdfSecurityHandlerType {
        PdfSecurityHandler = 0,
        PdfPasswordSecurityHandler = 1,
        PdfCertificateSecurityHandler = 2,
    }; // PdfSecurityHandlerType

    // Represents types of Print scaling
    enum class PrintScaling {
        // Application default
        AppDefault = 0,
        // None
        None = 1,
    }; // PrintScaling

    // Represents types of Reading direction
    enum class ReadingDirection {
        // Left to right reading direction
        L2R = 0,
        // Right to left reading direction
        R2L = 1,
    }; // ReadingDirection

    class Catalog;
    class Info;
    class OptionalContentProperties;
    class PdfCertificateSecurityHandler;
    class PdfDocument;
    class PdfPasswordSecurityHandler;
    class PdfSecurityHandler;
    class ViewerPreferences;

    typedef std::shared_ptr <Catalog> CatalogPtr;
    typedef std::shared_ptr <Info> InfoPtr;
    typedef std::shared_ptr <OptionalContentProperties> OptionalContentPropertiesPtr;
    typedef std::shared_ptr <PdfCertificateSecurityHandler> PdfCertificateSecurityHandlerPtr;
    typedef std::shared_ptr <PdfDocument> PdfDocumentPtr;
    typedef std::shared_ptr <PdfPasswordSecurityHandler> PdfPasswordSecurityHandlerPtr;
    typedef std::shared_ptr <PdfSecurityHandler> PdfSecurityHandlerPtr;
    typedef std::shared_ptr <ViewerPreferences> ViewerPreferencesPtr;

}} // SolidFramework::Pdf

namespace SolidFramework { namespace Pdf { namespace Interop { 

    // Represents types of page drawing with cache
    enum class CacheDrawingType {
        // Page drawing with cache. Cache is resetted before page drawing
        UseResettedCache = 0,
        // Page drawing with cache. Cache isn't resetted before page drawing
        UseCache = 1,
    }; // CacheDrawingType

    // Represents types of Drawing core
    enum class DrawingCore {
        // Drawing core is GDI
        Gdi = 0,
        // Drawing core is GDI Plus
        GdiPlus = 1,
        // Drawing core is AGG
        Agg = 2,
    }; // DrawingCore



}}} // SolidFramework::Pdf::Interop

namespace SolidFramework { namespace Pdf { namespace Plumbing { 

    enum class FontStatus {
        // Font status is unknown
        Unknown = 0,
        // Font is built-in
        BuiltIn = 1,
        // Font has subset font program
        Subset = 2,
        // Font has embedded font program
        Embedded = 3,
        // Font has embedded font program
        NonEmbedded = 4,
    }; // FontStatus

    // Represents types of Font. For a description of Font Types see https://en.wikipedia.org/wiki/Computer_font
    enum class FontType {
        // Font is unknown
        Unknown = 0,
        // Font type is Type1
        Type1 = 1,
        // Font type is Type3
        Type3 = 2,
        // Font type is Type1
        MMType1 = 3,
        // Font type is TrueType
        TrueType = 4,
        // Font type is Type0
        Type0 = 5,
    }; // FontType

    // Used by GetScannedImage to return details of the content analysis for the page.
    enum class PageStatistics {
        // Not set.
        None = 0,
        // Page contains at least one image.
        Image = 1,
        // Page contains visible text.
        VisibleText = 2,
        // Page contains invisible text.
        InvisibleText = 4,
        // Page contains vector graphics.
        VectorGraphics = 8,
        // Page contains at least one image.
        LowResImage = 16,
        // Page contains more than 10,000 objects
        TooComplex = 32,
        // Page has no content.
        Empty = 64,
        // Page contains vector text
        VectorText = 128,
    }; // PageStatistics

    // Represents types of PDF object
    enum class PdfObjectType {
        Unknown = 0,
        Object = 1,
        Reference = 2,
        Dictionary = 3,
        Array = 4,
        Stream = 5,
        Boolean = 6,
        Number = 7,
        String = 8,
        Name = 9,
        Null = 11,
        Rectangle = 100,
        Kids = 101,
        Font = 102,
        OutlineItem = 103,
        Outlines = 104,
        Page = 105,
        Pages = 106,
        Action = 120,
        GoTo = 121,
        Uri = 122,
        Annot = 140,
        Link = 141,
        PopupAnnot = 142,
        MarkupAnnot = 150,
        TextAnnot = 151,
        FreeTextAnnot = 152,
        LineAnnot = 153,
        SquareAnnot = 154,
        CircleAnnot = 155,
        PolygonAnnot = 156,
        PolyLineAnnot = 157,
        HighlightAnnot = 158,
        UnderlineAnnot = 159,
        SquigglyAnnot = 160,
        StrikeOutAnnot = 161,
        StampAnnot = 162,
        CaretAnnot = 163,
        InkAnnot = 164,
        FileAttachmentAnnot = 165,
        SoundAnnot = 166,
    }; // PdfObjectType

    class PdfAction;
    class PdfAnnot;
    class PdfArray;
    class PdfBoolean;
    class PdfCaretAnnot;
    class PdfCircleAnnot;
    class PdfDate;
    class PdfDictionary;
    class PdfDictionaryItemsCollection;
    class PdfFileAttachmentAnnot;
    class PdfFont;
    class PdfFreeTextAnnot;
    class PdfGoTo;
    class PdfHighlightAnnot;
    class PdfInkAnnot;
    class PdfItem;
    class PdfKids;
    class PdfLineAnnot;
    class PdfLink;
    class PdfMarkupAnnot;
    class PdfName;
    class PdfNamedDestinationsCollection;
    class PdfNull;
    class PdfNumber;
    class PdfObject;
    class PdfOutlineItem;
    class PdfOutlines;
    class PdfPage;
    class PdfPages;
    class PdfPolygonAnnot;
    class PdfPolyLineAnnot;
    class PdfPopupAnnot;
    class PdfRectangle;
    class PdfReference;
    class PdfSoundAnnot;
    class PdfSquareAnnot;
    class PdfSquigglyAnnot;
    class PdfStampAnnot;
    class PdfStreamObject;
    class PdfStrikeOutAnnot;
    class PdfString;
    class PdfTextAnnot;
    class PdfUnderlineAnnot;
    class PdfUriDictionary;

    typedef std::shared_ptr <PdfAction> PdfActionPtr;
    typedef std::shared_ptr <PdfAnnot> PdfAnnotPtr;
    typedef std::shared_ptr <PdfArray> PdfArrayPtr;
    typedef std::shared_ptr <PdfBoolean> PdfBooleanPtr;
    typedef std::shared_ptr <PdfCaretAnnot> PdfCaretAnnotPtr;
    typedef std::shared_ptr <PdfCircleAnnot> PdfCircleAnnotPtr;
    typedef std::shared_ptr <PdfDate> PdfDatePtr;
    typedef std::shared_ptr <PdfDictionary> PdfDictionaryPtr;
    typedef std::shared_ptr <PdfDictionaryItemsCollection> PdfDictionaryItemsCollectionPtr;
    typedef std::shared_ptr <PdfFileAttachmentAnnot> PdfFileAttachmentAnnotPtr;
    typedef std::shared_ptr <PdfFont> PdfFontPtr;
    typedef std::shared_ptr <PdfFreeTextAnnot> PdfFreeTextAnnotPtr;
    typedef std::shared_ptr <PdfGoTo> PdfGoToPtr;
    typedef std::shared_ptr <PdfHighlightAnnot> PdfHighlightAnnotPtr;
    typedef std::shared_ptr <PdfInkAnnot> PdfInkAnnotPtr;
    typedef std::shared_ptr <PdfItem> PdfItemPtr;
    typedef std::shared_ptr <PdfKids> PdfKidsPtr;
    typedef std::shared_ptr <PdfLineAnnot> PdfLineAnnotPtr;
    typedef std::shared_ptr <PdfLink> PdfLinkPtr;
    typedef std::shared_ptr <PdfMarkupAnnot> PdfMarkupAnnotPtr;
    typedef std::shared_ptr <PdfName> PdfNamePtr;
    typedef std::shared_ptr <PdfNamedDestinationsCollection> PdfNamedDestinationsCollectionPtr;
    typedef std::shared_ptr <PdfNull> PdfNullPtr;
    typedef std::shared_ptr <PdfNumber> PdfNumberPtr;
    typedef std::shared_ptr <PdfObject> PdfObjectPtr;
    typedef std::shared_ptr <PdfOutlineItem> PdfOutlineItemPtr;
    typedef std::shared_ptr <PdfOutlines> PdfOutlinesPtr;
    typedef std::shared_ptr <PdfPage> PdfPagePtr;
    typedef std::shared_ptr <PdfPages> PdfPagesPtr;
    typedef std::shared_ptr <PdfPolygonAnnot> PdfPolygonAnnotPtr;
    typedef std::shared_ptr <PdfPolyLineAnnot> PdfPolyLineAnnotPtr;
    typedef std::shared_ptr <PdfPopupAnnot> PdfPopupAnnotPtr;
    typedef std::shared_ptr <PdfRectangle> PdfRectanglePtr;
    typedef std::shared_ptr <PdfReference> PdfReferencePtr;
    typedef std::shared_ptr <PdfSoundAnnot> PdfSoundAnnotPtr;
    typedef std::shared_ptr <PdfSquareAnnot> PdfSquareAnnotPtr;
    typedef std::shared_ptr <PdfSquigglyAnnot> PdfSquigglyAnnotPtr;
    typedef std::shared_ptr <PdfStampAnnot> PdfStampAnnotPtr;
    typedef std::shared_ptr <PdfStreamObject> PdfStreamObjectPtr;
    typedef std::shared_ptr <PdfStrikeOutAnnot> PdfStrikeOutAnnotPtr;
    typedef std::shared_ptr <PdfString> PdfStringPtr;
    typedef std::shared_ptr <PdfTextAnnot> PdfTextAnnotPtr;
    typedef std::shared_ptr <PdfUnderlineAnnot> PdfUnderlineAnnotPtr;
    typedef std::shared_ptr <PdfUriDictionary> PdfUriDictionaryPtr;

}}} // SolidFramework::Pdf::Plumbing

namespace SolidFramework { namespace Pdf { namespace Repair { 

    // Builds the results of the repair, for example xref table rebuilt, duplicate object resolution, stream Length correction, etc
    enum class RepairResult {
        // No repairs were made
        None = 0,
        // Items repaired like xref table rebuilt, duplicate object resolution, stream Length correction
        Repaired = 1,
        // Checks if there are no pages in the file
        NoPages = 2,
        // Unable to repair as no document dictionary catelogue was found in the file.
        NoCatalog = 3,
        // content of file is not PDF
        NotPDF = 4,
        // cannot repair PDF containing XRefStm or ObjStm yet
        NotXRef = 5,
        // unresolvable duplicate object (same generation or file revision and not same /Type)
        DuplicateObject = 6,
        // Items repaired like xref table rebuilt, duplicate object resolution, stream Length correction
        NotRepaired = 7,
    }; // RepairResult

    class RepairXRef;

    typedef std::shared_ptr <RepairXRef> RepairXRefPtr;

}}} // SolidFramework::Pdf::Repair

namespace SolidFramework { namespace Pdf { namespace Reports { 

    class PdfAProblem;
    class PdfAProblemClass;
    class PdfAProblemClassesCollection;
    class PdfAReport;
    class PdfAResult;
    class PdfAResultsCollection;

    typedef std::shared_ptr <PdfAProblem> PdfAProblemPtr;
    typedef std::shared_ptr <PdfAProblemClass> PdfAProblemClassPtr;
    typedef std::shared_ptr <PdfAProblemClassesCollection> PdfAProblemClassesCollectionPtr;
    typedef std::shared_ptr <PdfAReport> PdfAReportPtr;
    typedef std::shared_ptr <PdfAResult> PdfAResultPtr;
    typedef std::shared_ptr <PdfAResultsCollection> PdfAResultsCollectionPtr;

}}} // SolidFramework::Pdf::Reports

namespace SolidFramework { namespace Pdf { namespace Transformers { 

    enum class PdfTransformerType {
        PdfTransformer = 0,
        IRecognize = 1,
        ImageWatermarkTransformer = 2,
        TextWatermarkTransformer = 3,
        PasswordSecurityTransformer = 4,
        CertificateSecurityTransformer = 5,
        OcrTransformer = 6,
    }; // PdfTransformerType

    enum class TransformationResultType {
        ITransformationResult = 0,
        TransformationResult = 1,
        OCRTransformationResult = 2,
    }; // TransformationResultType

    class ImageWatermarkTransformer;
    class IRecognize;
    class ITransformationResult;
    class OCRTransformationResult;
    class OcrTransformer;
    class PasswordSecurityTransformer;
    class PdfTransformer;
    class TextWatermarkTransformer;
    class TransformationResult;

    typedef std::shared_ptr <ImageWatermarkTransformer> ImageWatermarkTransformerPtr;
    typedef std::shared_ptr <IRecognize> IRecognizePtr;
    typedef std::shared_ptr <ITransformationResult> ITransformationResultPtr;
    typedef std::shared_ptr <OCRTransformationResult> OCRTransformationResultPtr;
    typedef std::shared_ptr <OcrTransformer> OcrTransformerPtr;
    typedef std::shared_ptr <PasswordSecurityTransformer> PasswordSecurityTransformerPtr;
    typedef std::shared_ptr <PdfTransformer> PdfTransformerPtr;
    typedef std::shared_ptr <TextWatermarkTransformer> TextWatermarkTransformerPtr;
    typedef std::shared_ptr <TransformationResult> TransformationResultPtr;

}}} // SolidFramework::Pdf::Transformers

namespace SolidFramework { namespace Platform { 

    class Directory;
    class File;
    class Path;
    class Platform;

    typedef std::shared_ptr <Directory> DirectoryPtr;
    typedef std::shared_ptr <File> FilePtr;
    typedef std::shared_ptr <Path> PathPtr;
    typedef std::shared_ptr <Platform> PlatformPtr;

}} // SolidFramework::Platform

namespace SolidFramework { namespace Plumbing { 

    enum class CatalogType {
        ICatalog = 0,
        Catalog = 1,
    }; // CatalogType

    enum class DocumentType {
        Document = 0,
        PdfDocument = 1,
    }; // DocumentType

    // License permissions
    enum class LicensePermissions {
        // No permissions
        None = 0,
        // Permission to use 'conversion' features
        PdfToWord = 1,
        // Permission to use 'tools' features
        PdfTools = 2,
        // Permission to use 'free' features
        PdfFree = 4,
        // Permission to use OCR
        Ocr = 8,
    }; // LicensePermissions
    __SOLIDFRAMEWORK_ENUM_FLAGS__(LicensePermissions);

    // Used as return value by License.Validate
    enum class LicenseState {
        // License passed in is ok
        Valid = 0,
        // License passed in was not valid so new license was generated and returned
        Generated = 1,
        // License passed in was not valid (not valid and not found in developer account)
        Invalid = 2,
        // License passed in was not valid and account is in need of renewal
        Expired = 3,
        // There was a failure attempting to call the Solid Documents license server (http call)
        WebCallFailure = 4,
        // Validate isn't supported for your account, please contact support@soliddocuments.com to discuss options.
        Denied = 5,
    }; // LicenseState

    // License type
    enum class LicenseType {
        // Invalid license
        Invalid = 0,
        // Developer license
        Developer = 1,
        // Professional license
        Professional = 2,
        // Tools license
        Tools = 3,
        // Free license
        Free = 4,
        // Internal Tools license
        ToolsInternal = 5,
        // Internal Professional license
        ProfessionalInternal = 6,
        // Republisher Tools license
        ToolsRepublisher = 7,
        // Republisher Professional license
        ProfessionalRepublisher = 8,
        // Internal Professional licence with OCR
        ProfessionalOcrInternal = 9,
        // Republisher Professional licence with OCR
        ProfessionalOcrRepublisher = 10,
        // Professional licence with OCR
        ProfessionalOcr = 11,
        // Trial license
        ClientTrial = 12,
        // Universal Professional licence
        ProfessionalUniversal = 13,
    }; // LicenseType

    // Defines whether to overwrite or fail the conversion if the output file exists.
    enum class OverwriteMode {
        // If output file exists, fail conversion.
        FailIfExists = 0,
        // If output file exists, overwrite file.
        ForceOverwrite = 1,
    }; // OverwriteMode

    // Represents types of Page ranges
    enum class PageRanges {
        // None
        None = 0,
        // Odd pages
        Odd = 1,
        // Even pages
        Even = 2,
        // All pages
        All = 3,
        // Portrait pages
        Portrait = 4,
        // Landscape pages
        Landscape = 8,
        // Selected pages
        Selected = 16,
        // Current
        Current = 32,
    }; // PageRanges
    __SOLIDFRAMEWORK_ENUM_FLAGS__(PageRanges);

    // Variant of PDFa formats
    enum class ValidationMode {
        // Detect mode to validate
        Detect = 0,
        // Not PDF/A. Attempting to convert to PDF with ValidationMode set to PDF will throw an exception.
        Pdf = 1,
        // Pdf/A 1B
        PdfA1B = 2,
        // Pdf/A 1A
        PdfA1A = 3,
        // Pdf/A 2B
        PdfA2B = 4,
        // Pdf/A 2A
        PdfA2A = 5,
        // Pdf/A 2U
        PdfA2U = 6,
        // Pdf/A 3B
        PdfA3B = 7,
        // Pdf/A 3A
        PdfA3A = 8,
        // Pdf/A 3U
        PdfA3U = 9,
    }; // ValidationMode

    enum class WarningLevelType {
        none = 0,
        force = 1,
        errors = 2,
        verbose = 3,
        engineer_l1 = 4,
        engineer_l2 = 5,
        details = 6,
    }; // WarningLevelType

    class Document;
    class ICatalog;
    class IPagesCollection;

    typedef std::shared_ptr <Document> DocumentPtr;
    typedef std::shared_ptr <ICatalog> ICatalogPtr;
    typedef std::shared_ptr <IPagesCollection> IPagesCollectionPtr;

}} // SolidFramework::Plumbing

namespace SolidFramework { namespace Converters { 

    // This class is used internally by SolidFramework. It should not be used directly.
    // Base class for all converters.
    class Converter: public FrameworkObject
    {
    protected:
        Converter();
    public:
        Converter(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~Converter() { Dispose(); }

        virtual void Dispose();

        // Add a PDF file to be converted to the source collection.
        virtual void AddSourceFile(std::wstring fullpath);

        // Add a PDF file to be converted to the source collection from a loaded Document object.
        virtual void AddSourceFile(SolidFramework::Plumbing::DocumentPtr document);

        // Add a collection of PDF file paths to be converted to the source collection.
        void AddSourceFiles(const std::vector<std::wstring> & paths);

        // Cancels the conversion.
        void Cancel();

        bool IsCanceled();

        // Clear the current source files collection.
        void ClearSourceFiles();

        // Converts files
        virtual void Convert();

        // Converts single file to specified path
        Plumbing::ConversionStatus ConvertTo(std::wstring fullpath);

        // Converts single file to specified path
        virtual Plumbing::ConversionStatus ConvertTo(std::wstring fullpath, bool overwrite);

        // Gets the directory where converted files will be placed when created using Convert.
        std::wstring GetOutputDirectory();

        // Sets the directory where converted files will be placed when created using Convert.
        void SetOutputDirectory(std::wstring value);

        // Get whether to overwrite an existing output file.
        SolidFramework::Plumbing::OverwriteMode GetOverwriteMode();

        // Get whether to overwrite an existing output file.
        void SetOverwriteMode(SolidFramework::Plumbing::OverwriteMode value);

        // Gets the results of conversion
        std::vector<Plumbing::IConversionResultPtr> GetResults();

        // Gets the source directory.
        std::wstring GetSourceDirectory();

        // Gets the source directory.
        void SetSourceDirectory(std::wstring value);
        
        // Event callbacks
        std::function<void(SolidFramework::ProgressEventArgsPtr)> OnProgress;
        std::function<void(SolidFramework::WarningEventArgsPtr)> OnWarning;
        
        // Overridable event handlers
        virtual void FireProgress(SolidFramework::ProgressEventArgsPtr args);
        virtual void FireWarning(SolidFramework::WarningEventArgsPtr args);
    }; // Converter

}} // SolidFramework::Converters

namespace SolidFramework { namespace Converters { 

    // Converts PDF to PDF/A 1b or 2b compliant document.  
    // This class is used to convert from PDF to PDF/A. If you wish to convert to a PDF that is non-PDF/A then use the PdfToPdfConverter instead.
    class PdfToPdfAConverter: public Converter
    {
    public:
        PdfToPdfAConverter();
        PdfToPdfAConverter(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfToPdfAConverter() { Dispose(); }

        virtual void Dispose();

        virtual void AddSourceFile(std::wstring fullpath);

        virtual void AddSourceFile(SolidFramework::Plumbing::DocumentPtr document);

        virtual void Convert();

        virtual Plumbing::ConversionStatus ConvertTo(std::wstring fullpath, bool overwrite);

        // Gets the directory path to validation log.
        std::wstring GetExportLogPath();

        // Sets the directory path to validation log.
        void SetExportLogPath(std::wstring value);

        // Gets the path where the error log is located.
        std::wstring GetLogPath();

        // Sets the path where the error log is located.
        void SetLogPath(std::wstring value);

        // Gets whether OCR should be performed on every single character. Setting this to true can result in slow conversions.
        bool GetOcrAlways();

        // Sets whether OCR should be performed on every single character. Setting this to true can result in slow conversions.
        void SetOcrAlways(bool value);

        // Gets whether to automatically detect and rotate pages that require rotatation on conversion.
        bool GetOcrAutoRotate();

        // Sets whether to automatically detect and rotate pages that require rotatation on conversion.
        void SetOcrAutoRotate(bool value);

        // Gets the TextRecoveryEngine that should be used for OCR.
        Plumbing::TextRecoveryEngine GetOcrEngine();

        // Sets the TextRecoveryEngine that should be used for OCR.
        void SetOcrEngine(Plumbing::TextRecoveryEngine value);

        // Gets the compression method to be used for images. This will affect file size and image quality.
        SolidFramework::Imaging::Plumbing::ImageCompression GetOcrImageCompression();

        // Sets the compression method to be used for images. This will affect file size and image quality.
        void SetOcrImageCompression(SolidFramework::Imaging::Plumbing::ImageCompression value);

        // Gets the OCR language to to be used.
        std::wstring GetOcrLanguage();

        // Sets the OCR language to to be used.
        void SetOcrLanguage(std::wstring value);

        // Gets the type of OCR to be performed.
        Plumbing::OcrType GetOcrType();

        // Sets the type of OCR to be performed.
        void SetOcrType(Plumbing::OcrType value);

        // Gets the Report that shows the validation steps, errors, passes etc. of the conversion.
        SolidFramework::Pdf::Reports::PdfAReportPtr GetReport();

        // Gets the language for validation log.
        std::wstring GetReportLanguage();

        // Sets the language for validation log.
        void SetReportLanguage(std::wstring value);

        // Gets whether warnings should be shown.
        bool GetShowWarnings();

        // Sets whether warnings should be shown.
        void SetShowWarnings(bool value);

        // Gets the Text Watermark to be used in the converted file.
        Plumbing::TextWatermarkPtr GetTextWatermark();

        // Sets the Text Watermark to be used in the converted file.
        void SetTextWatermark(Plumbing::TextWatermarkPtr value);

        // Check whether the PDF file passes PDF/A validation for the current validation mode.
        void Validate();

        // Check whether the PDF file passes PDF/A validation for the current validation mode.
        void Validate(SolidFramework::Plumbing::ValidationMode mode);

        // Sets the PDF/A Validation Mode. Setting this to PDF will cause an exception to be thrown if an attempt is made to convert the file.
        SolidFramework::Plumbing::ValidationMode GetValidationMode();

        // Sets the PDF/A Validation Mode. Setting this to PDF will cause an exception to be thrown if an attempt is made to convert the file.
        void SetValidationMode(SolidFramework::Plumbing::ValidationMode value);

        // Verify that the PDF file passes PDF/A validation with the ValidationMode that it claims to be.
        void Verify();

        static PdfToPdfAConverterPtr DynamicCast(ConverterPtr value);
    }; // PdfToPdfAConverter

}} // SolidFramework::Converters

namespace SolidFramework { namespace Converters { 

    // Converts a PDF file into another PDF file
    class PdfToPdfConverter: public Converter
    {
    public:
        PdfToPdfConverter();
        PdfToPdfConverter(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfToPdfConverter() { Dispose(); }

        virtual void Dispose();

        virtual void AddSourceFile(std::wstring fullpath);

        virtual void AddSourceFile(SolidFramework::Plumbing::DocumentPtr document);

        virtual void Convert();

        static Plumbing::ConversionStatus Convert(std::wstring sourceFileName, std::wstring destinationFileName);

        static Plumbing::ConversionStatus Convert(std::wstring sourceFileName, std::wstring destinationFileName, bool overwrite);

        static Plumbing::ConversionStatus Convert(std::wstring sourceFileName, std::wstring destinationFileName, bool overwrite, std::wstring ocrLanguage);

        virtual Plumbing::ConversionStatus ConvertTo(std::wstring fullpath, bool overwrite);

        // Gets a Create Tags flag
        bool GetCreateTags();

        // Gets a Create Tags flag
        void SetCreateTags(bool value);

        // Gets an OCR flag
        bool GetOcrAlways();

        // Gets an OCR flag
        void SetOcrAlways(bool value);

        // Set to true of false to automatically detect if pages require rotatation on conversion.
        bool GetOcrAutoRotate();

        // Set to true of false to automatically detect if pages require rotatation on conversion.
        void SetOcrAutoRotate(bool value);

        // Gets a value indicating whether [OCR always].
        Plumbing::TextRecoveryEngine GetOcrEngine();

        // Gets a value indicating whether [OCR always].
        void SetOcrEngine(Plumbing::TextRecoveryEngine value);

        // Set this to compress or change image properties to manage the size of the converted file.
        SolidFramework::Imaging::Plumbing::ImageCompression GetOcrImageCompression();

        // Set this to compress or change image properties to manage the size of the converted file.
        void SetOcrImageCompression(SolidFramework::Imaging::Plumbing::ImageCompression value);

        // Set the OCR language to that of the original document.
        std::wstring GetOcrLanguage();

        // Set the OCR language to that of the original document.
        void SetOcrLanguage(std::wstring value);

        // Choose the type of OCR you want to apply
        Plumbing::OcrType GetOcrType();

        // Choose the type of OCR you want to apply
        void SetOcrType(Plumbing::OcrType value);

        std::wstring GetPassword();

        void SetPassword(std::wstring value);

        Plumbing::TextWatermarkPtr GetTextWatermark();

        void SetTextWatermark(Plumbing::TextWatermarkPtr value);

        static PdfToPdfConverterPtr DynamicCast(ConverterPtr value);
    }; // PdfToPdfConverter

}} // SolidFramework::Converters

namespace SolidFramework { namespace Converters { 

    // This class is used internally by SolidFramework. It should not be used directly.
    class SolidConverterPdf: public Converter
    {
    protected:
        SolidConverterPdf();
    public:
        SolidConverterPdf(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~SolidConverterPdf() { Dispose(); }

        virtual void Dispose();

        virtual void AddSourceFile(std::wstring fullpath);

        virtual void AddSourceFile(SolidFramework::Plumbing::DocumentPtr document);

        virtual void Convert();

        virtual Plumbing::ConversionStatus ConvertTo(std::wstring fullpath, bool overwrite);

        // Gets a value indicating whether soft-hyphens should be detected. The default is false.
        bool GetDetectSoftHyphens();

        // Sets a value indicating whether soft-hyphens should be detected. The default is false.
        void SetDetectSoftHyphens(bool value);

        // Gets whether vector images should be converted to bitmap images. Default is false.
        bool GetGraphicsAsImages();

        // Sets whether vector images should be converted to bitmap images. Default is false.
        void SetGraphicsAsImages(bool value);

        // Gets an image as a watermark in the converted file
        Plumbing::ImageWatermarkPtr GetImageWatermark();

        // Sets an image as a watermark in the converted file
        void SetImageWatermark(Plumbing::ImageWatermarkPtr value);

        // Gets whether text that is invisible because it is the same colour as the background should be recovered. Default is false.
        bool GetKeepBackgroundColorText();

        // Sets whether text that is invisible because it is the same colour as the background should be recovered. Default is false.
        void SetKeepBackgroundColorText(bool value);

        // Gets whether text that is invisible because it has no stroke or fill (PDF rendering mode 3 is typically used for a searchable layer in scanned pages) should be recovered. Default is false.
        bool GetKeepInvisibleText();

        // Sets whether text that is invisible because it has no stroke or fill (PDF rendering mode 3 is typically used for a searchable layer in scanned pages) should be recovered. Default is false.
        void SetKeepInvisibleText(bool value);

        // In the event of a damaged PDF (InternalError) do not attempt to repair the PDF before conversion.
        bool GetNoRepairing();

        // In the event of a damaged PDF (InternalError) do not attempt to repair the PDF before conversion.
        void SetNoRepairing(bool value);

        // Gets the page range.
        SolidFramework::PageRangePtr GetPageRange();

        // Gets the page range.
        void SetPageRange(SolidFramework::PageRangePtr value);

        // Gets the password.
        std::wstring GetPassword();

        // Gets the password.
        void SetPassword(std::wstring value);

        static SolidConverterPdfPtr DynamicCast(ConverterPtr value);
    }; // SolidConverterPdf

}} // SolidFramework::Converters

namespace SolidFramework { namespace Converters { 

    // Extracts images from PDF files or renders the pages of PDFs as images.
    class PdfToImageConverter: public SolidConverterPdf
    {
    public:
        PdfToImageConverter();
        PdfToImageConverter(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfToImageConverter() { Dispose(); }

        virtual void Dispose();

        virtual void AddSourceFile(std::wstring fullpath);

        virtual void AddSourceFile(SolidFramework::Plumbing::DocumentPtr document);

        // Gets the type of the conversion.
        Plumbing::ImageConversionType GetConversionType();

        // Gets the type of the conversion.
        void SetConversionType(Plumbing::ImageConversionType value);

        virtual void Convert();

        virtual Plumbing::ConversionStatus ConvertTo(std::wstring fullpath, bool overwrite);

        // Gets the drawing core to use when rendering PDF pages.
        SolidFramework::Pdf::Interop::DrawingCore GetDrawingCore();

        // Gets the drawing core to use when rendering PDF pages.
        void SetDrawingCore(SolidFramework::Pdf::Interop::DrawingCore value);

        // Gets the format of output image files.
        Plumbing::ImageDocumentType GetOutputType();

        // Gets the format of output image files.
        void SetOutputType(Plumbing::ImageDocumentType value);

        // Gets the page DPI.
        int GetPageDPI();

        // Gets the page DPI.
        void SetPageDPI(int value);

        static PdfToImageConverterPtr DynamicCast(ConverterPtr value);
    }; // PdfToImageConverter

}} // SolidFramework::Converters

namespace SolidFramework { namespace Converters { 

    // This class is used internally by SolidFramework. It should not be used directly.
    // Abstract class for converting PDF to MS Office application files.
    class PdfToOfficeDocumentConverter: public SolidConverterPdf
    {
    protected:
        PdfToOfficeDocumentConverter();
    public:
        PdfToOfficeDocumentConverter(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfToOfficeDocumentConverter() { Dispose(); }

        virtual void Dispose();

        virtual void AddSourceFile(std::wstring fullpath);

        virtual void AddSourceFile(SolidFramework::Plumbing::DocumentPtr document);

        // Gets whether to automatically rotate pages based on the orientation of the text on the pages. Default is true.
        bool GetAutoRotate();

        // Sets whether to automatically rotate pages based on the orientation of the text on the pages. Default is true.
        void SetAutoRotate(bool value);

        virtual void Convert();

        virtual Plumbing::ConversionStatus ConvertTo(std::wstring fullpath, bool overwrite);

        // Gets the selected areas. The default is null.
        Plumbing::SelectedAreasPtr GetSelectedAreas();

        // Sets the selected areas. The default is null.
        void SetSelectedAreas(Plumbing::SelectedAreasPtr value);

        // Gets the automatic GNSE options.
        Plumbing::TextRecoveryAutomaticGNse GetTextRecoveryAutomaticGNse();

        // Gets the automatic GNSE options.
        void SetTextRecoveryAutomaticGNse(Plumbing::TextRecoveryAutomaticGNse value);

        // Gets  the text recovery engine.  The default value is automatic.
        Plumbing::TextRecoveryEngine GetTextRecoveryEngine();

        // Sets  the text recovery engine.  The default value is automatic.
        void SetTextRecoveryEngine(Plumbing::TextRecoveryEngine value);

        // Gets the non-standard encoding recovery engine.  The default value is automatic.
        Plumbing::TextRecoveryEngineNse GetTextRecoveryEngineNse();

        // Sets the non-standard encoding recovery engine.  The default value is automatic.
        void SetTextRecoveryEngineNse(Plumbing::TextRecoveryEngineNse value);

        // Gets the text recovery language. Default is an empty string.
        std::wstring GetTextRecoveryLanguage();

        // Sets the text recovery language. Default is an empty string.
        void SetTextRecoveryLanguage(std::wstring value);

        // Gets os sets the text recovery NSE type.  The default value is automatic.
        Plumbing::TextRecoveryNSE GetTextRecoveryNseType();

        // Gets os sets the text recovery NSE type.  The default value is automatic.
        void SetTextRecoveryNseType(Plumbing::TextRecoveryNSE value);

        // Gets a value indicating whether to highlight low confidence words (any word with Confidence less than 100) in the OCR output in yellow.
        // This property has no effect if OCR has not occurred. Default is false.
        bool GetTextRecoverySuspects();

        // Sets a value indicating whether to highlight low confidence words (any word with Confidence less than 100) in the OCR output in yellow.
        // This property has no effect if OCR has not occurred. Default is false.
        void SetTextRecoverySuspects(bool value);

        // Gets the text recovery type. The default value is automatic.
        Plumbing::TextRecovery GetTextRecoveryType();

        // Sets the text recovery type. The default value is automatic.
        void SetTextRecoveryType(Plumbing::TextRecovery value);

        // Gets the user properties. The default is an empty string.
        std::wstring GetUserProperties();

        // Sets the user properties. The default is an empty string.
        void SetUserProperties(std::wstring value);

        static PdfToOfficeDocumentConverterPtr DynamicCast(ConverterPtr value);
    }; // PdfToOfficeDocumentConverter

}} // SolidFramework::Converters

namespace SolidFramework { namespace Converters { 

    // Convert PDF to CSV, SQL or unformatted Excel.
    class PdfToDataConverter: public PdfToOfficeDocumentConverter
    {
    public:
        PdfToDataConverter();
        PdfToDataConverter(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfToDataConverter() { Dispose(); }

        virtual void Dispose();

        virtual void AddSourceFile(std::wstring fullpath);

        virtual void AddSourceFile(SolidFramework::Plumbing::DocumentPtr document);

        // Gets whether to attempt to automatically detect decimal and thousands separators, based on the content of the PDF being converted. Default is true.
        bool GetAutoDetectSeparators();

        // Sets whether to attempt to automatically detect decimal and thousands separators, based on the content of the PDF being converted. Default is true.
        void SetAutoDetectSeparators(bool value);

        virtual void Convert();

        virtual Plumbing::ConversionStatus ConvertTo(std::wstring fullpath, bool overwrite);

        // Gets the decimal separator.
        Plumbing::DecimalSeparator GetDecimalSeparator();

        // Gets the decimal separator.
        void SetDecimalSeparator(Plumbing::DecimalSeparator value);

        // Gets the character that will be used to separate each field in the data file. ie. comma, tab or semicolon
        Plumbing::DelimiterOptions GetDelimiterOption();

        // Gets the character that will be used to separate each field in the data file. ie. comma, tab or semicolon
        void SetDelimiterOption(Plumbing::DelimiterOptions value);

        // Attempt to automatically detect tables tiled across multiple pages.
        bool GetDetectTiledPages();

        // Attempt to automatically detect tables tiled across multiple pages.
        void SetDetectTiledPages(bool value);

        int GetEncoding();

        void SetEncoding(int value);

        // Gets the export format to use.
        Plumbing::DataExportFormat GetExportFormat();

        // Gets the export format to use.
        void SetExportFormat(Plumbing::DataExportFormat value);

        // Gets what characters terminate the line, ie Platform, Windows or Mac
        Plumbing::LineTerminator GetLineTerminator();

        // Gets what characters terminate the line, ie Platform, Windows or Mac
        void SetLineTerminator(Plumbing::LineTerminator value);

        // Gets the data document format.
        Plumbing::DataDocumentType GetOutputType();

        // Gets the data document format.
        void SetOutputType(Plumbing::DataDocumentType value);

        // Get whether to join all output information into a single file.
        Plumbing::ExcelTablesOnSheet GetSingleTable();

        // Get whether to join all output information into a single file.
        void SetSingleTable(Plumbing::ExcelTablesOnSheet value);

        // Gets the thousands separator.
        Plumbing::ThousandsSeparator GetThousandsSeparator();

        // Gets the thousands separator.
        void SetThousandsSeparator(Plumbing::ThousandsSeparator value);

        static PdfToDataConverterPtr DynamicCast(ConverterPtr value);
    }; // PdfToDataConverter

}} // SolidFramework::Converters

namespace SolidFramework { namespace Converters { 

    // Extract tables from PDF Pages to Microsoft Excel Spreadsheet.
    class PdfToExcelConverter: public PdfToOfficeDocumentConverter
    {
    public:
        PdfToExcelConverter();
        PdfToExcelConverter(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfToExcelConverter() { Dispose(); }

        virtual void Dispose();

        virtual void AddSourceFile(std::wstring fullpath);

        virtual void AddSourceFile(SolidFramework::Plumbing::DocumentPtr document);

        // Gets whether to attempt to automatically detect decimal and thousands separators, based on the content of the PDF being converted. Default is true.
        bool GetAutoDetectSeparators();

        // Sets whether to attempt to automatically detect decimal and thousands separators, based on the content of the PDF being converted. Default is true.
        void SetAutoDetectSeparators(bool value);

        virtual void Convert();

        // Convert the specified source file into the specified Excel file.
        static Plumbing::ConversionStatus Convert(std::wstring sourceFileName, std::wstring destinationFileName);

        // Convert the specified source file into the specified text file.
        static Plumbing::ConversionStatus Convert(std::wstring sourceFileName, std::wstring destinationFileName, bool overwrite);

        virtual Plumbing::ConversionStatus ConvertTo(std::wstring fullpath, bool overwrite);

        // Gets the decimal separator.
        Plumbing::DecimalSeparator GetDecimalSeparator();

        // Gets the decimal separator.
        void SetDecimalSeparator(Plumbing::DecimalSeparator value);

        // Gets whether to attempt to automatically reconstruct tables that were so wide that they were tiled across multiple pages. Default is true.
        bool GetDetectTiledPages();

        // Sets whether to attempt to automatically reconstruct tables that were so wide that they were tiled across multiple pages. Default is true.
        void SetDetectTiledPages(bool value);

        // Get or sets a FootnotesMode that specifies the footnotes mode. Default is FootnotesMode.Ignore.
        Plumbing::FootnotesMode GetFootnotesMode();

        // Get or sets a FootnotesMode that specifies the footnotes mode. Default is FootnotesMode.Ignore.
        void SetFootnotesMode(Plumbing::FootnotesMode value);

        // Get or sets a HeaderAndFooterMode that specifies the header and footer mode. Default is HeaderAndFooterMode.Remove.
        Plumbing::HeaderAndFooterMode GetHeaderAndFooterMode();

        // Get or sets a HeaderAndFooterMode that specifies the header and footer mode. Default is HeaderAndFooterMode.Remove.
        void SetHeaderAndFooterMode(Plumbing::HeaderAndFooterMode value);

        // Gets whether to include non-table content such as images or text in the reconstructed Excel document.
        bool GetKeepNonTableContent();

        // Sets whether to include non-table content such as images or text in the reconstructed Excel document.
        void SetKeepNonTableContent(bool value);

        // This method is deprecated. Use the property TextAnnotationsAsContent instead.
        Plumbing::MarkupAnnotConversionType GetMarkupAnnotConversionType();

        // This method is deprecated. Use the property TextAnnotationsAsContent instead.
        void SetMarkupAnnotConversionType(Plumbing::MarkupAnnotConversionType value);

        // Gets the data document format.
        Plumbing::ExcelDocumentType GetOutputType();

        // Gets the data document format.
        void SetOutputType(Plumbing::ExcelDocumentType value);

        // Gets whether to place non-table content that is detected within columns in the PDF into separate columns in the reconstructed Excel file. 
        // If true then non-table content that is detected will be placed into separate columns. If false then such content will all be placed into the first column.
        // If KeepNonTableContent is false then no non-table content will be included in the reconstructed file and this option will have no meaning.
        bool GetPreserveColumnsInNonTableContent();

        // Sets whether to place non-table content that is detected within columns in the PDF into separate columns in the reconstructed Excel file. 
        // If true then non-table content that is detected will be placed into separate columns. If false then such content will all be placed into the first column.
        // If KeepNonTableContent is false then no non-table content will be included in the reconstructed file and this option will have no meaning.
        void SetPreserveColumnsInNonTableContent(bool value);

        // Gets whether to join all output information into a single sheet. Default is PlaceEachTableOnOwnSheet.
        Plumbing::ExcelTablesOnSheet GetSingleTable();

        // Sets whether to join all output information into a single sheet. Default is PlaceEachTableOnOwnSheet.
        void SetSingleTable(Plumbing::ExcelTablesOnSheet value);

        // Deprecated alias for KeepNonTableContent.
        bool GetTablesFromContent();

        // Deprecated alias for KeepNonTableContent.
        void SetTablesFromContent(bool value);

        // Gets whether text annotations should be converted into content.
        // This property will be ignored if KeepNonTableContent is false.
        bool GetTextAnnotationsAsContent();

        // Sets whether text annotations should be converted into content.
        // This property will be ignored if KeepNonTableContent is false.
        void SetTextAnnotationsAsContent(bool value);

        // Gets the thousands separator.
        Plumbing::ThousandsSeparator GetThousandsSeparator();

        // Gets the thousands separator.
        void SetThousandsSeparator(Plumbing::ThousandsSeparator value);

        static PdfToExcelConverterPtr DynamicCast(ConverterPtr value);
    }; // PdfToExcelConverter

}} // SolidFramework::Converters

namespace SolidFramework { namespace Converters { 

    // Convert PDF to HTML document.
    class PdfToHtmlConverter: public PdfToOfficeDocumentConverter
    {
    public:
        PdfToHtmlConverter();
        PdfToHtmlConverter(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfToHtmlConverter() { Dispose(); }

        virtual void Dispose();

        virtual void AddSourceFile(std::wstring fullpath);

        virtual void AddSourceFile(SolidFramework::Plumbing::DocumentPtr document);

        virtual void Convert();

        // Convert the specified source file into the specified HTML file.
        static Plumbing::ConversionStatus Convert(std::wstring sourceFileName, std::wstring destinationFileName);

        // Convert the specified source file into the specified HTML file.
        static Plumbing::ConversionStatus Convert(std::wstring sourceFileName, std::wstring destinationFileName, bool overwrite);

        virtual Plumbing::ConversionStatus ConvertTo(std::wstring fullpath, bool overwrite);

        // Gets a value indicating whether the document language should be detected from the document content. The default is true.
        bool GetDetectLanguage();

        // Sets a value indicating whether the document language should be detected from the document content. The default is true.
        void SetDetectLanguage(bool value);

        // Get whether to detect lists.
        bool GetDetectLists();

        // Get whether to detect lists.
        void SetDetectLists(bool value);

        // Attempt to automatically detect tables tiled across multiple pages.
        bool GetDetectTiledPages();

        // Attempt to automatically detect tables tiled across multiple pages.
        void SetDetectTiledPages(bool value);

        // Gets a value indicating whether the reconstruction should attempt to represent the original pdf.
        // The default is false.
        bool GetExactMode();

        // Sets a value indicating whether the reconstruction should attempt to represent the original pdf.
        // The default is false.
        void SetExactMode(bool value);

        // Get or sets a HeaderAndFooterMode that specifies the header and footer mode. Default is HeaderAndFooterMode.Remove.
        Plumbing::HeaderAndFooterMode GetHeaderAndFooterMode();

        // Get or sets a HeaderAndFooterMode that specifies the header and footer mode. Default is HeaderAndFooterMode.Remove.
        void SetHeaderAndFooterMode(Plumbing::HeaderAndFooterMode value);

        // Gets the HTML navigation.
        HtmlNavigation GetHtmlNavigation();

        // Gets the HTML navigation.
        void SetHtmlNavigation(HtmlNavigation value);

        // Gets the HTML splitting using.
        HtmlSplittingUsing GetHtmlSplittingUsing();

        // Gets the HTML splitting using.
        void SetHtmlSplittingUsing(HtmlSplittingUsing value);

        // Get or set the how images are handled during conversion.
        Plumbing::HtmlImages GetImages();

        // Get or set the how images are handled during conversion.
        void SetImages(Plumbing::HtmlImages value);

        // Get the format of the image you are converting i.e bmp for Bitmap etc.
        Plumbing::ImageDocumentType GetImageType();

        // Get the format of the image you are converting i.e bmp for Bitmap etc.
        void SetImageType(Plumbing::ImageDocumentType value);

        // Gets whether line breaks should be preserved in the content. The default is false.
        bool GetKeepLineBreaks();

        // Sets whether line breaks should be preserved in the content. The default is false.
        void SetKeepLineBreaks(bool value);

        // Rebases the HTML.
        static void RebaseHtml(std::wstring path, std::wstring oldFolder, std::wstring newFolder);

        // Get maximum width for HTML file images.
        int GetWidthLimit();

        // Get maximum width for HTML file images.
        void SetWidthLimit(int value);

        static PdfToHtmlConverterPtr DynamicCast(ConverterPtr value);
    }; // PdfToHtmlConverter

}} // SolidFramework::Converters

namespace SolidFramework { namespace Converters { 

    // Converts and reconstructs PDF files to editable Microsoft Word documents
    class PdfToJsonConverter: public PdfToOfficeDocumentConverter
    {
    public:
        PdfToJsonConverter();
        PdfToJsonConverter(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfToJsonConverter() { Dispose(); }

        virtual void Dispose();

        virtual void AddSourceFile(std::wstring fullpath);

        virtual void AddSourceFile(SolidFramework::Plumbing::DocumentPtr document);

        // Get or sets whether the spacing between characters should be set as to their average size. 
        // This is required because fonts in PDF and docx are different and therefore the same Unicode characters 
        // have different character widths in PDF and docx. This value must be set to true if the generated docx file 
        // is to look the same as the PDF file. Setting the value to true will also result in the creation of fewer, but 
        // larger "Run" objects.
        // The default is true.
        bool GetAverageCharacterScaling();

        // Get or sets whether the spacing between characters should be set as to their average size. 
        // This is required because fonts in PDF and docx are different and therefore the same Unicode characters 
        // have different character widths in PDF and docx. This value must be set to true if the generated docx file 
        // is to look the same as the PDF file. Setting the value to true will also result in the creation of fewer, but 
        // larger "Run" objects.
        // The default is true.
        void SetAverageCharacterScaling(bool value);

        virtual void Convert();

        // Converts the specified source file. If the destination file exists then the conversion will fail with ConversionStatus.IOFileLocked.
        // Always check the value of the returned ConversionResult to verify that conversion was successful.
        static Plumbing::ConversionStatus Convert(std::wstring sourceFileName, std::wstring destinationFileName);

        // Converts the selected or specified source file.
        // Always check the value of the returned ConversionResult to verify that conversion was successful.
        static Plumbing::ConversionStatus Convert(std::wstring sourceFileName, std::wstring destinationFileName, bool overwrite);

        virtual Plumbing::ConversionStatus ConvertTo(std::wstring fullpath, bool overwrite);

        // Gets whether the document language should be detected from the document content. The default is true.
        bool GetDetectLanguage();

        // Sets whether the document language should be detected from the document content. The default is true.
        void SetDetectLanguage(bool value);

        // Gets whether lists should be detected from the document content. The default is true.
        bool GetDetectLists();

        // Sets whether lists should be detected from the document content. The default is true.
        void SetDetectLists(bool value);

        // Gets whether styles should be detected from the document content. The default is true.
        bool GetDetectStyles();

        // Sets whether styles should be detected from the document content. The default is true.
        void SetDetectStyles(bool value);

        // Gets whether tables should be detected as tables, rather than being treated as plain text. The default is true.
        bool GetDetectTables();

        // Sets whether tables should be detected as tables, rather than being treated as plain text. The default is true.
        void SetDetectTables(bool value);

        // Gets whether text that is tagged as being a table in the PDF should be always be considered to be a table, rather than being considered as a table only if it "looks" like one. 
        // The default is true.
        bool GetDetectTaggedTables();

        // Sets whether text that is tagged as being a table in the PDF should be always be considered to be a table, rather than being considered as a table only if it "looks" like one. 
        // The default is true.
        void SetDetectTaggedTables(bool value);

        // Attempt to automatically detect tables tiled across multiple pages.
        bool GetDetectTiledPages();

        // Attempt to automatically detect tables tiled across multiple pages.
        void SetDetectTiledPages(bool value);

        // Gets whether Table of Contents should be detected from the document content. The default is true.
        bool GetDetectToc();

        // Sets whether Table of Contents should be detected from the document content. The default is true.
        void SetDetectToc(bool value);

        // Get or sets a FootnotesMode that specifies the header and footer mode. Default is FootnotesMode.Ignore.
        Plumbing::FootnotesMode GetFootnotesMode();

        // Get or sets a FootnotesMode that specifies the header and footer mode. Default is FootnotesMode.Ignore.
        void SetFootnotesMode(Plumbing::FootnotesMode value);

        // Get or sets a HeaderAndFooterMode that specifies the header and footer mode. Default is HeaderAndFooterMode.Detect.
        Plumbing::HeaderAndFooterMode GetHeaderAndFooterMode();

        // Get or sets a HeaderAndFooterMode that specifies the header and footer mode. Default is HeaderAndFooterMode.Detect.
        void SetHeaderAndFooterMode(Plumbing::HeaderAndFooterMode value);

        // Get or sets a ImageAnchoringMode that specifies the header and footer mode. Default is ImageAnchoringMode.Automatic.
        Plumbing::ImageAnchoringMode GetImageAnchoringMode();

        // Get or sets a ImageAnchoringMode that specifies the header and footer mode. Default is ImageAnchoringMode.Automatic.
        void SetImageAnchoringMode(Plumbing::ImageAnchoringMode value);

        // Get or sets whether to keep character spacing. Default is true.
        bool GetKeepCharacterSpacing();

        // Get or sets whether to keep character spacing. Default is true.
        void SetKeepCharacterSpacing(bool value);

        // Get or sets a MarkupAnnotConversionType that specifies how Markup Annotations should be converted. Default is MarkupAnnotConversionType.TextBox.
        Plumbing::MarkupAnnotConversionType GetMarkupAnnotConversionType();

        // Get or sets a MarkupAnnotConversionType that specifies how Markup Annotations should be converted. Default is MarkupAnnotConversionType.TextBox.
        void SetMarkupAnnotConversionType(Plumbing::MarkupAnnotConversionType value);

        bool GetMergeParagraphIndents();

        void SetMergeParagraphIndents(bool value);

        // Gets the ReconstructionMode reconstruction mode. Default is ReconstructionMode.Flowing.
        Plumbing::ReconstructionMode GetReconstructionMode();

        // Sets the ReconstructionMode reconstruction mode. Default is ReconstructionMode.Flowing.
        void SetReconstructionMode(Plumbing::ReconstructionMode value);

        // Get whether to support right-to-left writing direction. Default is true.
        bool GetSupportRightToLeftWritingDirection();

        // Get whether to support right-to-left writing direction. Default is true.
        void SetSupportRightToLeftWritingDirection(bool value);

        // Gets a value indicating whether [word installed].
        bool GetWordInstalled();

        static PdfToJsonConverterPtr DynamicCast(ConverterPtr value);
    }; // PdfToJsonConverter

}} // SolidFramework::Converters

namespace SolidFramework { namespace Converters { 

    // Converts a PDF file to a PowerPoint Presentation.
    class PdfToPowerPointConverter: public PdfToOfficeDocumentConverter
    {
    public:
        PdfToPowerPointConverter();
        PdfToPowerPointConverter(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfToPowerPointConverter() { Dispose(); }

        virtual void Dispose();

        virtual void AddSourceFile(std::wstring fullpath);

        virtual void AddSourceFile(SolidFramework::Plumbing::DocumentPtr document);

        virtual void Convert();

        // Convert the specified source file into the specified PowerPoint document.
        static Plumbing::ConversionStatus Convert(std::wstring sourceFileName, std::wstring destinationFileName);

        // Convert the specified source file into the specified PowerPoint document.
        static Plumbing::ConversionStatus Convert(std::wstring sourceFileName, std::wstring destinationFileName, bool overwrite);

        virtual Plumbing::ConversionStatus ConvertTo(std::wstring fullpath, bool overwrite);

        // Get or sets whether lists within the PDF file should be reconstructed as lists within the PowerPoint Document.
        bool GetDetectLists();

        // Get or sets whether lists within the PDF file should be reconstructed as lists within the PowerPoint Document.
        void SetDetectLists(bool value);

        // Gets a EmbedFontsMode that specifies embed ot not font programs to output document. Default is EmbedFontsMode.NoFontEmbedding.
        Plumbing::EmbedFontsMode GetEmbedFontsMode();

        // Gets a EmbedFontsMode that specifies embed ot not font programs to output document. Default is EmbedFontsMode.NoFontEmbedding.
        void SetEmbedFontsMode(Plumbing::EmbedFontsMode value);

        // This method is deprecated. Use the property TextAnnotationsAsSpeakerNotes instead.
        Plumbing::MarkupAnnotConversionType GetMarkupAnnotConversionType();

        // This method is deprecated. Use the property TextAnnotationsAsSpeakerNotes instead.
        void SetMarkupAnnotConversionType(Plumbing::MarkupAnnotConversionType value);

        // Gets whether text annotations should be converted into speaker notes.
        bool GetTextAnnotationsAsSpeakerNotes();

        // Sets whether text annotations should be converted into speaker notes.
        void SetTextAnnotationsAsSpeakerNotes(bool value);

        static PdfToPowerPointConverterPtr DynamicCast(ConverterPtr value);
    }; // PdfToPowerPointConverter

}} // SolidFramework::Converters

namespace SolidFramework { namespace Converters { 

    // Convert a PDF file into plain text document.
    class PdfToTextConverter: public PdfToOfficeDocumentConverter
    {
    public:
        PdfToTextConverter();
        PdfToTextConverter(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfToTextConverter() { Dispose(); }

        virtual void Dispose();

        virtual void AddSourceFile(std::wstring fullpath);

        virtual void AddSourceFile(SolidFramework::Plumbing::DocumentPtr document);

        virtual void Convert();

        // Convert the specified source file into the specified text file.
        static Plumbing::ConversionStatus Convert(std::wstring sourceFileName, std::wstring destinationFileName);

        // Convert the specified source file into the specified text file.
        static Plumbing::ConversionStatus Convert(std::wstring sourceFileName, std::wstring destinationFileName, bool overwrite);

        virtual Plumbing::ConversionStatus ConvertTo(std::wstring fullpath, bool overwrite);

        // Gets whether footnotes should be removed if detected.
        // If true then footnotes will be removed.
        bool GetDetectAndRemoveFootnotes();

        // Sets whether footnotes should be removed if detected.
        // If true then footnotes will be removed.
        void SetDetectAndRemoveFootnotes(bool value);

        // Gets whether headers and footers should be removed if detected.
        // If true then headers and footers will be removed.
        bool GetDetectAndRemoveHeadersAndFooters();

        // Sets whether headers and footers should be removed if detected.
        // If true then headers and footers will be removed.
        void SetDetectAndRemoveHeadersAndFooters(bool value);

        int GetEncoding();

        void SetEncoding(int value);

        // Gets whether line breaks should be preserved in the content. The default is false.
        bool GetKeepLineBreaks();

        // Sets whether line breaks should be preserved in the content. The default is false.
        void SetKeepLineBreaks(bool value);

        // Get the maximum number of characters to display in each line of your text document.  Example: textConverter.LineLength = 80;.
        int GetLineLength();

        // Get the maximum number of characters to display in each line of your text document.  Example: textConverter.LineLength = 80;.
        void SetLineLength(int value);

        // Get the line terminator.
        Plumbing::LineTerminator GetLineTerminator();

        // Get the line terminator.
        void SetLineTerminator(Plumbing::LineTerminator value);

        bool GetPreserveRareUnicode();

        void SetPreserveRareUnicode(bool value);

        // Gets whether text annotations should be converted into content.
        bool GetTextAnnotationsAsContent();

        // Sets whether text annotations should be converted into content.
        void SetTextAnnotationsAsContent(bool value);

        static PdfToTextConverterPtr DynamicCast(ConverterPtr value);
    }; // PdfToTextConverter

}} // SolidFramework::Converters

namespace SolidFramework { namespace Converters { 

    // Converts and reconstructs PDF files to editable Microsoft Word documents
    class PdfToWordConverter: public PdfToOfficeDocumentConverter
    {
    public:
        PdfToWordConverter();
        PdfToWordConverter(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfToWordConverter() { Dispose(); }

        virtual void Dispose();

        virtual void AddSourceFile(std::wstring fullpath);

        virtual void AddSourceFile(SolidFramework::Plumbing::DocumentPtr document);

        // Get or sets whether the spacing between characters should be set as to their average size. 
        // This is required because fonts in PDF and docx are different and therefore the same Unicode characters 
        // have different character widths in PDF and docx. This value must be set to true if the generated docx file 
        // is to look the same as the PDF file. Setting the value to true will also result in the creation of fewer, but 
        // larger "Run" objects.
        // The default is true.
        bool GetAverageCharacterScaling();

        // Get or sets whether the spacing between characters should be set as to their average size. 
        // This is required because fonts in PDF and docx are different and therefore the same Unicode characters 
        // have different character widths in PDF and docx. This value must be set to true if the generated docx file 
        // is to look the same as the PDF file. Setting the value to true will also result in the creation of fewer, but 
        // larger "Run" objects.
        // The default is true.
        void SetAverageCharacterScaling(bool value);

        virtual void Convert();

        // Converts the specified source file. If the destination file exists then the conversion will fail with ConversionStatus.IOFileLocked.
        // Always check the value of the returned ConversionResult to verify that conversion was successful.
        static Plumbing::ConversionStatus Convert(std::wstring sourceFileName, std::wstring destinationFileName);

        // Converts the selected or specified source file.
        // Always check the value of the returned ConversionResult to verify that conversion was successful.
        static Plumbing::ConversionStatus Convert(std::wstring sourceFileName, std::wstring destinationFileName, bool overwrite);

        virtual Plumbing::ConversionStatus ConvertTo(std::wstring fullpath, bool overwrite);

        // Gets whether the document language should be detected from the document content. The default is true.
        bool GetDetectLanguage();

        // Sets whether the document language should be detected from the document content. The default is true.
        void SetDetectLanguage(bool value);

        // Gets whether lists should be detected from the document content. The default is true.
        bool GetDetectLists();

        // Sets whether lists should be detected from the document content. The default is true.
        void SetDetectLists(bool value);

        // Gets whether styles should be detected from the document content. The default is true.
        bool GetDetectStyles();

        // Sets whether styles should be detected from the document content. The default is true.
        void SetDetectStyles(bool value);

        // Gets whether tables should be detected as tables, rather than being treated as plain text. The default is true.
        bool GetDetectTables();

        // Sets whether tables should be detected as tables, rather than being treated as plain text. The default is true.
        void SetDetectTables(bool value);

        // Gets whether text that is tagged as being a table in the PDF should be always be considered to be a table, rather than being considered as a table only if it "looks" like one. 
        // The default is true.
        bool GetDetectTaggedTables();

        // Sets whether text that is tagged as being a table in the PDF should be always be considered to be a table, rather than being considered as a table only if it "looks" like one. 
        // The default is true.
        void SetDetectTaggedTables(bool value);

        // Attempt to automatically detect tables tiled across multiple pages.
        bool GetDetectTiledPages();

        // Attempt to automatically detect tables tiled across multiple pages.
        void SetDetectTiledPages(bool value);

        // Gets whether Table of Contents should be detected from the document content. The default is true.
        bool GetDetectToc();

        // Sets whether Table of Contents should be detected from the document content. The default is true.
        void SetDetectToc(bool value);

        // Get or sets a FootnotesMode that specifies the header and footer mode. Default is FootnotesMode.Ignore.
        Plumbing::FootnotesMode GetFootnotesMode();

        // Get or sets a FootnotesMode that specifies the header and footer mode. Default is FootnotesMode.Ignore.
        void SetFootnotesMode(Plumbing::FootnotesMode value);

        // Get or sets a HeaderAndFooterMode that specifies the header and footer mode. Default is HeaderAndFooterMode.Detect.
        Plumbing::HeaderAndFooterMode GetHeaderAndFooterMode();

        // Get or sets a HeaderAndFooterMode that specifies the header and footer mode. Default is HeaderAndFooterMode.Detect.
        void SetHeaderAndFooterMode(Plumbing::HeaderAndFooterMode value);

        // Get or sets a ImageAnchoringMode that specifies the header and footer mode. Default is ImageAnchoringMode.Automatic.
        Plumbing::ImageAnchoringMode GetImageAnchoringMode();

        // Get or sets a ImageAnchoringMode that specifies the header and footer mode. Default is ImageAnchoringMode.Automatic.
        void SetImageAnchoringMode(Plumbing::ImageAnchoringMode value);

        // Get or sets whether to keep character spacing. Default is true.
        bool GetKeepCharacterSpacing();

        // Get or sets whether to keep character spacing. Default is true.
        void SetKeepCharacterSpacing(bool value);

        // Get or sets a MarkupAnnotConversionType that specifies how Markup Annotations should be converted. Default is MarkupAnnotConversionType.TextBox.
        Plumbing::MarkupAnnotConversionType GetMarkupAnnotConversionType();

        // Get or sets a MarkupAnnotConversionType that specifies how Markup Annotations should be converted. Default is MarkupAnnotConversionType.TextBox.
        void SetMarkupAnnotConversionType(Plumbing::MarkupAnnotConversionType value);

        bool GetMergeParagraphIndents();

        void SetMergeParagraphIndents(bool value);

        // Gets the type of the output to be created by the converter.  Default is WordDocumentType.DocX
        Plumbing::WordDocumentType GetOutputType();

        // Sets the type of the output to be created by the converter.  Default is WordDocumentType.DocX
        void SetOutputType(Plumbing::WordDocumentType value);

        // Gets the ReconstructionMode reconstruction mode. Default is ReconstructionMode.Flowing.
        Plumbing::ReconstructionMode GetReconstructionMode();

        // Sets the ReconstructionMode reconstruction mode. Default is ReconstructionMode.Flowing.
        void SetReconstructionMode(Plumbing::ReconstructionMode value);

        // Get whether to support right-to-left writing direction. Default is true.
        bool GetSupportRightToLeftWritingDirection();

        // Get whether to support right-to-left writing direction. Default is true.
        void SetSupportRightToLeftWritingDirection(bool value);

        // Get a TargetWordFormat that specifies Target MS Word File Format Version. Default is TargetWordFormat.Automatic.
        Plumbing::TargetWordFormat GetTargetWordFormat();

        // Get a TargetWordFormat that specifies Target MS Word File Format Version. Default is TargetWordFormat.Automatic.
        void SetTargetWordFormat(Plumbing::TargetWordFormat value);

        // Gets a value indicating whether [word installed].
        bool GetWordInstalled();

        static PdfToWordConverterPtr DynamicCast(ConverterPtr value);
    }; // PdfToWordConverter

}} // SolidFramework::Converters

namespace SolidFramework { namespace Converters { namespace Plumbing { 

    // Interface for conversion results
    class IConversionResult: public FrameworkObject
    {
    public:
        IConversionResult();
        IConversionResult(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~IConversionResult() { Dispose(); }

        virtual void Dispose();

        // Adds the result
        virtual void AddResult(std::wstring path, bool temporary);

        // Adds the result
        virtual void AddResultFolder(std::wstring path, bool temporary);

        // Adds the temporary file
        virtual void AddTemporaryFile(std::wstring path);

        // Adds the temporary folder
        virtual void AddTemporaryFolder(std::wstring path);

        virtual std::vector<std::wstring> GetPaths();

        virtual void SetPaths(const std::vector<std::wstring> & value);

        // Removes the temporary.
        virtual bool RemoveTemporary(std::wstring path);

        // Gets the source file
        virtual std::wstring GetSource();

        // Gets the source file
        virtual void SetSource(std::wstring value);

        // Gets the conversion status
        virtual ConversionStatus GetStatus();

        // Gets the conversion status
        virtual void SetStatus(ConversionStatus value);
    }; // IConversionResult

}}} // SolidFramework::Converters::Plumbing

namespace SolidFramework { namespace Converters { namespace Plumbing { 

    // Implementation of the IConversionResult
    class ConversionResult: public IConversionResult
    {
    protected:
        ConversionResult();
    public:
        ConversionResult(std::wstring source, ConversionStatus status);
        ConversionResult(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~ConversionResult() { Dispose(); }

        virtual void Dispose();

        // Adds the result
        virtual void AddResult(std::wstring path, bool temporary);

        // Adds the result
        virtual void AddResultFolder(std::wstring path, bool temporary);

        // Adds the temporary file
        virtual void AddTemporaryFile(std::wstring path);

        // Adds the temporary folder
        virtual void AddTemporaryFolder(std::wstring path);

        SolidFramework::Pdf::Transformers::OCRTransformationResultPtr GetOcrStepResults();

        void SetOcrStepResults(SolidFramework::Pdf::Transformers::OCRTransformationResultPtr OCRStepResults);

        virtual std::vector<std::wstring> GetPaths();

        virtual void SetPaths(const std::vector<std::wstring> & value);

        // Removes the temporary.
        virtual bool RemoveTemporary(std::wstring path);

        // Gets the source file
        virtual std::wstring GetSource();

        // Gets the source file
        virtual void SetSource(std::wstring value);

        // Gets the conversion status
        virtual ConversionStatus GetStatus();

        // Gets the conversion status
        virtual void SetStatus(ConversionStatus value);

        static ConversionResultPtr DynamicCast(IConversionResultPtr value);
    }; // ConversionResult

}}} // SolidFramework::Converters::Plumbing

namespace SolidFramework { namespace Converters { namespace Plumbing { 

    // Represents PDF/A conversion result
    class PdfAConversionResult: public ConversionResult
    {
    public:
        PdfAConversionResult(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfAConversionResult() { Dispose(); }

        virtual void Dispose();

        // Gets the PDF/A status.
        PdfAConversionStatus GetPdfAStatus();

        // Gets the type of the validation mode to PDF file.
        SolidFramework::Plumbing::ValidationMode GetPdfType();

        static PdfAConversionResultPtr DynamicCast(IConversionResultPtr value);
    }; // PdfAConversionResult

}}} // SolidFramework::Converters::Plumbing

namespace SolidFramework { namespace Converters { namespace Plumbing { 

    // Represents Image watermark
    class ImageWatermark: public FrameworkObject
    {
    public:
        ImageWatermark();
        ImageWatermark(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~ImageWatermark() { Dispose(); }

        virtual void Dispose();

        // Gets the path to image
        std::wstring GetPath();

        // Sets the path to image
        void SetPath(std::wstring value);
    }; // ImageWatermark

}}} // SolidFramework::Converters::Plumbing

namespace SolidFramework { namespace Converters { namespace Plumbing { 

    // Represents Selected area
    class SelectedArea: public FrameworkObject
    {
    public:
        SelectedArea(int page, int x, int y, int width, int height);
        SelectedArea(int page, SolidFramework::Interop::PointPtr location, int width, int height);
        SelectedArea(int page, SolidFramework::Interop::RectanglePtr rect);
        SelectedArea(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~SelectedArea() { Dispose(); }

        virtual void Dispose();

        // Gets the area.
        SolidFramework::Interop::RectanglePtr GetArea();

        // Sets the area.
        void SetArea(SolidFramework::Interop::RectanglePtr value);

        // Gets the height.
        int GetHeight();

        // Gets the location.
        SolidFramework::Interop::PointPtr GetLocation();

        // Gets the page number
        int GetPage();

        // Gets the page number
        void SetPage(int value);

        // Parses the specified content.
        static SelectedAreaPtr Parse(std::wstring content);

        virtual std::wstring ToString();

        // Gets the width.
        int GetWidth();
    }; // SelectedArea

}}} // SolidFramework::Converters::Plumbing

namespace SolidFramework { namespace Converters { namespace Plumbing { 

    // Represents Selected area
    class SelectedAreas: public FrameworkObject
    {
    public:
        SelectedAreas();
        SelectedAreas(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~SelectedAreas() { Dispose(); }

        virtual void Dispose();

        void Add(SelectedAreaPtr value);

        int GetCount();

        // Parses the specified content.
        static SelectedAreasPtr Parse(std::wstring content);

        std::vector<SelectedAreaPtr> GetSelectedAreas();

        virtual std::wstring ToString();
    }; // SelectedAreas

}}} // SolidFramework::Converters::Plumbing

namespace SolidFramework { namespace Converters { namespace Plumbing { 

    // Represents Text watermark
    class TextWatermark: public FrameworkObject
    {
    public:
        TextWatermark();
        TextWatermark(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~TextWatermark() { Dispose(); }

        virtual void Dispose();

        // Gets the angle.
        int GetAngle();

        // Gets the angle.
        void SetAngle(int value);

        // Gets a value indicating whether this TextWatermark is background.
        bool GetBackground();

        // Sets a value indicating whether this TextWatermark is background.
        void SetBackground(bool value);

        // Gets the color.
        SolidFramework::Interop::ColorPtr GetColor();

        // Gets the color.
        void SetColor(SolidFramework::Interop::ColorPtr value);

        // Gets a value indicating whether this TextWatermark is filled.
        bool GetFill();

        // Gets a value indicating whether this TextWatermark is filled.
        void SetFill(bool value);

        // Gets the line weight.
        float GetLineWeight();

        // Gets the line weight.
        void SetLineWeight(float value);

        // Parses the specified content.
        static TextWatermarkPtr Parse(std::wstring content);

        // Gets a value indicating whether [serif font].
        bool GetSerifFont();

        // Gets a value indicating whether [serif font].
        void SetSerifFont(bool value);

        // Gets a value indicating whether this TextWatermark is strokde.
        bool GetStroke();

        // Gets a value indicating whether this TextWatermark is strokde.
        void SetStroke(bool value);

        // Gets the text to be displayed on the watermark i.e. DRAFT
        std::wstring GetText();

        // Gets the text to be displayed on the watermark i.e. DRAFT
        void SetText(std::wstring value);

        virtual std::wstring ToString();

        // Gets the URL you want to target
        std::wstring GetUrl();

        // Gets the URL you want to target
        void SetUrl(std::wstring value);
    }; // TextWatermark

}}} // SolidFramework::Converters::Plumbing

namespace SolidFramework { namespace Imaging { 

    // Represents Image
    class Image: public FrameworkObject
    {
    public:
        Image(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~Image() { Dispose(); }

        virtual void Dispose();

        // Gets the autorotate and deskew angles for the image.
        void GetAngles();

        // Gets the bits per component.
        int GetBitsPerComponent();

        // Gets the component images.
        std::vector<ImagePtr> GetComponentImages();

        // Gets the component images.
        std::vector<ImagePtr> GetComponentImages(Plumbing::ImageCompression compression);

        // Gets the component images.
        std::vector<ImagePtr> GetComponentImages(Plumbing::ImageCompression compression, Plumbing::ImageComponents components);

        // Gets the component images.
        std::vector<ImagePtr> GetComponentImages(Plumbing::ImageCompression compression, Plumbing::ImageComponents components, Plumbing::ImageComponentOrder imageOrder);

        // Creates from the specified path.
        static ImagePtr Create(HANDLE native_image, SolidFramework::Pdf::Plumbing::PageStatistics statistics);

        static ImagePtr Create(std::wstring path);

        // Gets a value indicating the quality of lossy compression to use.
        BYTE GetDCTCompressionQuality();

        // Gets the dpi processed.
        int GetDpiProcessed();

        // Gets the dpi X co-ordinates.
        int GetDpiX();

        // Gets the dpi Y co-ordinates.
        int GetDpiY();

        // Gets the delta x co-ordinates.
        int GetDx();

        // Gets the delta y co-ordinates.
        int GetDy();

        // Gets the height.
        int GetHeight();

        // Gets the height processed.
        int GetHeightProcessed();

        // Gets the image encoder.
        ImageEncoder GetImageEncoder();

        // Gets the type of the image.
        ImageType GetImageType();

        // Gets a value indicating whether this instance is alpha channel used.
        bool IsAlphaUsed();

        // Gets a value indicating whether this instance is image component.
        bool IsImageComponent();

        // Gets a value indicating whether this instance is inverse component.
        bool IsInverseComponent();

        // Gets a value indicating whether this instance is needing OCR.
        bool IsNeedingOCR();

        // Gets a value indicating whether this instance is palette used.
        bool IsPaletteUsed();

        // Gets a value indicating whether this instance is rectangle component.
        bool IsRectangleComponent();

        // Gets a value indicating whether this instance is text background.
        bool IsTextBackground();

        // Gets a value indicating whether this instance is text component.
        bool IsTextComponent();

        // Gets a value indicating whether this instance is underline component.
        bool IsUnderlineComponent();

        // Gets a value indicating whether this instance is vector component.
        bool IsVectorComponent();

        std::wstring GetName();

        void SetName(std::wstring value);

        SolidFramework::Pdf::Plumbing::PageStatistics GetPageStatistics();

        // Gets the color of the palette.
        SolidFramework::Interop::ColorPtr GetPaletteColor(int i);

        // Gets the number of entries in the current palette.
        int GetPaletteSize();

        // Gets the color of the pixel.
        SolidFramework::Interop::ColorPtr GetPixelColor(int x, int y);

        // Gets a value indicating whether this instance ortho rotated text
        int GetTextAngle();

        // Gets a value indicating whether this instance was detected as CJK text.
        Plumbing::TextLanguage GetTextLanguage();

        // Converts to System.Drawing.Bitmap
        SolidFramework::Interop::BitmapPtr ToBitmap();

        std::wstring ToDataURI(SolidFramework::Converters::Plumbing::ImageDocumentType format);

        std::wstring ToDataURI();

        // index in a palette image of transparent color (-1 means no transparent color)
        int GetTransparentIndex();

        // Gets a value indicating whether this instance is lossy.
        bool GetUsesDCTCompression();

        // Gets the auto rotate angle (non-zero if the image needed to be rotated)
        double WasAutoRotated();

        // Gets the deskew angle (non-zero if the image needed to be deskewed)
        double WasDeskewed();

        // Gets the width.
        int GetWidth();

        // Gets the width processed.
        int GetWidthProcessed();
    }; // Image

}} // SolidFramework::Imaging

namespace SolidFramework { namespace Imaging { 

    // Interface for OCR region
    class IOcrRegion: public FrameworkObject
    {
    public:
        IOcrRegion();
        IOcrRegion(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~IOcrRegion() { Dispose(); }

        virtual void Dispose();

        // Gets the delta x co-ordinates.
        int GetDx();

        // Gets the delta y co-ordinate.
        int GetDy();

        // Gets the height.
        int GetHeight();

        OcrRegionType GetObjectType();

        // Gets the width.
        int GetWidth();
    }; // IOcrRegion

}} // SolidFramework::Imaging

namespace SolidFramework { namespace Imaging { 

    // Represents OCR graphic region
    class OcrGraphicRegion: public IOcrRegion
    {
    public:
        OcrGraphicRegion(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~OcrGraphicRegion() { Dispose(); }

        virtual void Dispose();

        // Gets the color.
        SolidFramework::Interop::ColorPtr GetColor();

        // Gets the type.
        OcrGraphicRegionType GetGraphicType();

        // Gets the width of the line.
        int GetLineWidth();

        static OcrGraphicRegionPtr DynamicCast(IOcrRegionPtr value);
    }; // OcrGraphicRegion

}} // SolidFramework::Imaging

namespace SolidFramework { namespace Imaging { 

    // Represents image region
    class OcrImageRegion: public IOcrRegion
    {
    public:
        OcrImageRegion(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~OcrImageRegion() { Dispose(); }

        virtual void Dispose();

        static OcrImageRegionPtr DynamicCast(IOcrRegionPtr value);
    }; // OcrImageRegion

}} // SolidFramework::Imaging

namespace SolidFramework { namespace Imaging { 

    // Represents OCR text region
    class OcrTextRegion: public IOcrRegion
    {
    public:
        OcrTextRegion(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~OcrTextRegion() { Dispose(); }

        virtual void Dispose();

        // Gets count of supsect words OCRd in this region.
        int GetOcrConfidentWords();

        int GetOcrLineCount();

        std::vector<OcrLinePtr> GetOcrLines();

        // Gets count of words OCRd in this region.
        int GetOcrWordCount();

        // Gets two letter language code this text region.
        std::wstring GetTextLanguage();

        virtual std::wstring ToString();

        static OcrTextRegionPtr DynamicCast(IOcrRegionPtr value);
    }; // OcrTextRegion

}} // SolidFramework::Imaging

namespace SolidFramework { namespace Imaging { 

    class Ocr: public FrameworkObject
    {
    public:
        Ocr();
        Ocr(std::wstring languageCode);
        Ocr(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~Ocr() { Dispose(); }

        virtual void Dispose();

        // Adds a word to the custom dictionary for the lifetime of this Ocr session. Useful for domain specific names or terminology.
        void AddWord(std::wstring customWord);

        int GetCount();

        static std::vector<std::wstring> GetLanguages();

        static std::wstring NormalizeOcrLanguage(std::wstring language);

        HANDLE GetOcrEngine();

        // Processes the specified image.
        OcrResultsPtr Process(ImagePtr image);

        static std::wstring GetTesseractDataDirectoryLocation();

        static void SetTesseractDataDirectoryLocation(std::wstring value);

        // Gets a value indicating whether to highlight low confidence words (any word with Confidence less than 100) in the OCR output in yellow.
        bool GetTextRecoverySuspects();

        // Sets a value indicating whether to highlight low confidence words (any word with Confidence less than 100) in the OCR output in yellow.
        void SetTextRecoverySuspects(bool value);
    }; // Ocr

}} // SolidFramework::Imaging

namespace SolidFramework { namespace Imaging { 

    // Represents line of OCR words
    class OcrLine: public FrameworkObject
    {
    public:
        OcrLine(const std::vector<SolidFramework::Imaging::OcrWordPtr> & words);
        OcrLine(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~OcrLine() { Dispose(); }

        virtual void Dispose();

        int GetCount();

        std::vector<OcrWordPtr> GetOcrWords();

        virtual std::wstring ToString();
    }; // OcrLine

}} // SolidFramework::Imaging

namespace SolidFramework { namespace Imaging { 

    // Represents OCR results
    class OcrResults: public FrameworkObject
    {
    public:
        OcrResults(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~OcrResults() { Dispose(); }

        virtual void Dispose();

        int GetCount();

        std::vector<IOcrRegionPtr> GetOcrResults();
    }; // OcrResults

}} // SolidFramework::Imaging

namespace SolidFramework { namespace Imaging { 

    // Represents OCR word
    class OcrWord: public FrameworkObject
    {
    public:
        OcrWord(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~OcrWord() { Dispose(); }

        virtual void Dispose();

        // Gets the background color.
        SolidFramework::Interop::ColorPtr GetBackground();

        // Gets the base line.
        int GetBaseLine();

        // Gets a value indicating whether this OcrWord is bold.
        bool GetBold();

        // Gets the color.
        SolidFramework::Interop::ColorPtr GetColor();

        // Gets a value indicating whether this OcrWord is of high recognition confidence.
        int GetConfidence();

        // Gets the delta x co-ordinates.
        int GetDx();

        // Gets the delta y co-ordinates.
        int GetDy();

        // Gets the font.
        std::wstring GetFont();

        // Gets the height.
        int GetHeight();

        // Gets a value indicating whether this OcrWord is italic.
        bool GetItalic();

        // Gets the PTS.
        double GetPts();

        virtual std::wstring ToString();

        // Gets the width.
        int GetWidth();
    }; // OcrWord

}} // SolidFramework::Imaging

namespace SolidFramework { namespace Interop { 

    class Bitmap: public FrameworkObject
    {
    public:
        Bitmap();
        Bitmap(int width, int height, ImageType type);
        Bitmap(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~Bitmap() { Dispose(); }

        virtual void Dispose();

        HANDLE GetDIB();

        int GetHeight();

        bool Load(std::wstring path);

        bool LoadFromHandle(HANDLE handle);

        std::vector<ColorPtr> GetPalette();

        void SetPalette(const std::vector<SolidFramework::Interop::ColorPtr> & value);

        bool ReadBits(HANDLE buffer, int stride);

        bool SaveAs(std::wstring path);

        bool SaveAs(std::wstring path, SolidFramework::Converters::Plumbing::ImageDocumentType format);

        std::wstring ToDataURI(SolidFramework::Converters::Plumbing::ImageDocumentType format);

        std::wstring ToDataURI();

        int GetWidth();

        bool WriteBits(HANDLE buffer, int stride);

        int GetXDpi();

        void SetXDpi(int value);

        int GetYDpi();

        void SetYDpi(int value);
    }; // Bitmap

}} // SolidFramework::Interop

namespace SolidFramework { namespace Interop { 

    class Color: public FrameworkObject
    {
    public:
        Color();
        Color(BYTE r, BYTE g, BYTE b, BYTE a);
        Color(unsigned int color);
        Color(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~Color() { Dispose(); }

        virtual void Dispose();

        static int Compare(ColorPtr color1, ColorPtr color2);

        int CompareTo(ColorPtr color);

        BYTE GetAlpha();

        BYTE GetBlue();

        BYTE GetGreen();

        BYTE GetRed();

        void SetAlpha(BYTE a);

        void SetBlue(BYTE b);

        void SetGreen(BYTE g);

        void SetRed(BYTE r);
    }; // Color

}} // SolidFramework::Interop

namespace SolidFramework { namespace Interop { 

    class DateTime: public FrameworkObject
    {
    public:
        DateTime();
        DateTime(int year, int month, int day, int hour, int minute, int second, bool isLocal);
        DateTime(int year, int month, int day, int hour, int minute, int second);
        DateTime(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~DateTime() { Dispose(); }

        virtual void Dispose();

        int GetDay();

        int GetHour();

        int GetMinute();

        int GetMonth();

        int GetSecond();

        int GetYear();

        bool IsLocalTime();

        // Creates a UTC DateTime based on the current system time.
        static DateTimePtr Now();

        // Parses a DateTime from the specified string or throws an exception if it can't be parsed.
        static DateTimePtr Parse(std::wstring date);

        void SetIsLocalTime(bool isLocal);

        // Returns an ISO 8601 representation of the DateTime
        std::wstring ToISOString();

        // Returns a string representation of the DateTime that will be chronologically sortable for positive years.
        std::wstring ToSortableString();

        static bool TryParse(std::wstring date, SolidFramework::Interop::DateTimePtr result);
    }; // DateTime

}} // SolidFramework::Interop

namespace SolidFramework { namespace Interop { 

    class Matrix: public FrameworkObject
    {
    public:
        Matrix();
        Matrix(double A, double B, double C, double D, double Tx, double Ty);
        Matrix(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~Matrix() { Dispose(); }

        virtual void Dispose();

        float A();

        float B();

        float C();

        void Concat(SolidFramework::Interop::MatrixPtr mtx);

        void ConcatRight(SolidFramework::Interop::MatrixPtr mtx);

        float D();

        bool IsFlipped();

        bool IsFlippedHor();

        bool IsFlippedVert();

        bool IsIdentity();

        bool Invert();

        void Multi(double scale);

        void Multi(double scaleA, double scaleB, double scaleC, double scaleD);

        bool IsOrthoBasis();

        double GetSkew();

        SizePtr Transform(SolidFramework::Interop::SizePtr pt);

        SizeFPtr Transform(SolidFramework::Interop::SizeFPtr pt);

        PointPtr Transform(SolidFramework::Interop::PointPtr pt);

        PointFPtr Transform(SolidFramework::Interop::PointFPtr pt);

        RectanglePtr Transform(SolidFramework::Interop::RectanglePtr rc);

        RectangleFPtr Transform(SolidFramework::Interop::RectangleFPtr rc);

        // Specifies the offset to be applied in the X direction.
        float Tx();

        // Specifies the offset to be applied in the Y direction.
        float Ty();

        std::vector<float> GetValues();

        double GetXRotation();

        double GetXScale();

        double GetYRotation();

        double GetYScale();
    }; // Matrix

}} // SolidFramework::Interop

namespace SolidFramework { namespace Interop { 

    class Point: public FrameworkObject
    {
    public:
        Point();
        Point(SolidFramework::Interop::PointPtr value);
        Point(int x, int y);
        Point(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~Point() { Dispose(); }

        virtual void Dispose();

        int GetX();

        int GetY();

        void SetX(int value);

        void SetY(int value);
    }; // Point

}} // SolidFramework::Interop

namespace SolidFramework { namespace Interop { 

    class PointF: public FrameworkObject
    {
    public:
        PointF();
        PointF(float x, float y);
        PointF(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PointF() { Dispose(); }

        virtual void Dispose();

        int CompareTo(PointFPtr other);

        float GetX();

        float GetY();

        void SetX(float value);

        void SetY(float value);
    }; // PointF

}} // SolidFramework::Interop

namespace SolidFramework { namespace Interop { 

    class Rectangle: public FrameworkObject
    {
    public:
        Rectangle();
        Rectangle(int left, int top, int right, int bottom);
        Rectangle(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~Rectangle() { Dispose(); }

        virtual void Dispose();

        int GetBottom();

        int GetHeight();

        int GetLeft();

        int GetRight();

        int GetTop();

        int GetWidth();

        void SetBottom(int value);

        void SetLeft(int value);

        void SetRight(int value);

        void SetTop(int value);
    }; // Rectangle

}} // SolidFramework::Interop

namespace SolidFramework { namespace Interop { 

    class RectangleF: public FrameworkObject
    {
    public:
        RectangleF();
        RectangleF(double left, double top, double right, double bottom);
        RectangleF(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~RectangleF() { Dispose(); }

        virtual void Dispose();

        double GetBottom();

        double GetHeight();

        double GetLeft();

        double GetRight();

        double GetTop();

        double GetWidth();

        void SetBottom(double value);

        void SetLeft(double value);

        void SetRight(double value);

        void SetTop(double value);
    }; // RectangleF

}} // SolidFramework::Interop

namespace SolidFramework { namespace Interop { 

    class Size: public FrameworkObject
    {
    public:
        Size();
        Size(SolidFramework::Interop::SizePtr value);
        Size(int x, int y);
        Size(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~Size() { Dispose(); }

        virtual void Dispose();

        int GetX();

        int GetY();

        void SetX(int value);

        void SetY(int value);
    }; // Size

}} // SolidFramework::Interop

namespace SolidFramework { namespace Interop { 

    class SizeF: public FrameworkObject
    {
    public:
        SizeF();
        SizeF(float x, float y);
        SizeF(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~SizeF() { Dispose(); }

        virtual void Dispose();

        float GetX();

        float GetY();

        void SetX(float value);

        void SetY(float value);
    }; // SizeF

}} // SolidFramework::Interop

namespace SolidFramework { namespace Interop { 

    class TimeSpan: public FrameworkObject
    {
    public:
        TimeSpan();
        TimeSpan(int hour, int minute, int second);
        TimeSpan(int seconds);
        TimeSpan(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~TimeSpan() { Dispose(); }

        virtual void Dispose();

        int GetHour();

        int GetMinute();

        int GetSecond();

        int GetTotalMinutes();

        int GetTotalSeconds();
    }; // TimeSpan

}} // SolidFramework::Interop

namespace SolidFramework { namespace Interop { 

    class Uri: public FrameworkObject
    {
    public:
        Uri();
        Uri(std::wstring uri);
        Uri(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~Uri() { Dispose(); }

        virtual void Dispose();

        std::wstring GetUri();

        void SetUri(std::wstring uri);
    }; // Uri

}} // SolidFramework::Interop

namespace SolidFramework { namespace Language { 

    class Language: public FrameworkObject
    {
    public:
        Language();
        Language(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~Language() { Dispose(); }

        virtual void Dispose();

        std::wstring Detect(std::wstring text, bool useEntireText);

        std::wstring Detect(std::wstring text);

        void DetectLanguages(std::wstring text, bool useEntireText, std::vector<std::wstring> & languages, std::vector<float> & scores);

        bool HarvestWords(std::wstring text, std::wstring wlanguageCode);

        bool IsSoftHyphen(std::wstring firstPart, std::wstring secondPart, std::wstring wlanguageCode);

        bool IsWord(std::wstring text, std::wstring wlanguageCode);
    }; // Language

}} // SolidFramework::Language

namespace SolidFramework { 

    class License: public FrameworkObject
    {
        License() = delete;
    public:

        // Checks if the current license has the required LicensePermissions.
        static bool Allows(Plumbing::LicensePermissions permissions);

        // Clears the current license and loads the default free license (if available)
        static void Clear();

        // Loads a seed license from an XML file and uses the web service to import a license that will work in cloud environments
        static void CloudImport(std::wstring xml);

        // Uses the web service to import a license that will work in cloud environments
        static void CloudImport(std::wstring name, std::wstring email, std::wstring organization, std::wstring code);

        // Uses the web service to import a license that will work in cloud environments
        static void CloudImport(std::wstring name, std::wstring email, std::wstring organization, std::wstring code, std::wstring applicationCode);

        // Checks whether the current license has permission to be distributed.
        static bool DistributionOk();

        // Checks whether the current license has permission to use 'free' features.
        static bool FreeOk();

        // Imports license
        static void Import(std::wstring name, std::wstring email, std::wstring organization, std::wstring code);

        // Imports license
        static void Import(std::wstring name, std::wstring email, std::wstring organization, std::wstring code, std::wstring applicationCode);

        // Imports license from the specified XML
        static void Import(std::wstring xml);

        // Loads a temporary machine specific license that was created by GetTemporaryLicense.
        static void LoadTemporaryLicense(std::wstring license);

        static Plumbing::LicensePermissions GetPermissions();

        // Checks whether the current license has permission to use 'conversion' and OCR features.
        static bool ProfessionalOcrOk();

        // Checks whether the current license has permission to use 'conversion' features.
        static bool ProfessionalOk();

        // Gets whether this machine needs to use CloudImport.
        static bool RequiresCloudImport();

        // Gets a temporary machine specific copy of the current license.
        static std::wstring GetTemporaryLicense();

        // Checks whether the current license has permission to use 'tools' features.
        static bool ToolsOk();

        static Plumbing::LicenseType GetType();

        // Gets the unique machine ID for this machine.
        static std::wstring GetUniqueMachineID();

        // Validates the license and uses the web service to generate a new license if required.
        static std::wstring WebValidate(std::wstring xml);

        // Validates the license and uses the web service to generate a new license if required.
        static std::wstring WebValidate(std::wstring name, std::wstring email, std::wstring organization, std::wstring code);
        static Plumbing::LicenseState Validate(const std::wstring& xml, std::wstring& newXml);
    }; // License

} // SolidFramework

namespace SolidFramework { namespace Model { 

    class CoreModel: public FrameworkObject
    {
    public:
        CoreModel(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~CoreModel() { Dispose(); }

        virtual void Dispose();

        // Closes the model. Use the method to disposing native data, when application is about to be closed, for example.
        virtual void Close();

        // Gets the bookmarks present in the PDF document.
        BookmarksCollectionPtr GetBookmarks();

        // Gets the conversion status.
        SolidFramework::Converters::Plumbing::ConversionStatus GetConversionStatus();

        // Creates a CoreModel object based on the file that is located at path.
        static CoreModelPtr Create(std::wstring path);

        // Creates a CoreModel object based on the file that is located at path.
        static CoreModelPtr Create(std::wstring path, bool noRepairing);

        // Creates a CoreModel object based on the file that is located at path.
        static CoreModelPtr Create(std::wstring path, PdfOptionsPtr options);

        // Creates the specified path.
        static CoreModelPtr Create(std::wstring path, PdfOptionsPtr options, bool readOnly);

        // Creates the specified path.
        static CoreModelPtr Create(std::wstring path, PdfOptionsPtr options, bool readOnly, bool noRepairing);

        // Get the default section.
        Plumbing::SectionPtr GetDefaultSection();

        // Get the default paragraph style template.
        Plumbing::ParagraphStyleTemplatePtr GetDefaultStyleTemplate();

        // Exports the contents of the Model as an Excel, PowerPoint or Word Document at the location specified in path. Fails if the destination file already exists.
        void Export(std::wstring path);

        // Exports the contents of the Model as an Excel, PowerPoint or Word Document at the location specified in path.
        void Export(std::wstring path, bool overwrite);

        // Exports the contents of the Model to the location specified in path.
        void Export(std::wstring path, Export::ExportOptionsPtr options);

        // Get the fonts.
        FontsCollectionPtr GetFonts();

        // Gets the hyperlinks present in the PDF document.
        HyperlinksCollectionPtr GetHyperlinks();

        std::wstring GetLanguage();

        // Gets the LayoutDocument for the model. This will be null unless the CoreModel was created with PdfOptions.ExposeTargetDocumentPagination set to true.
        Layout::LayoutDocumentPtr GetLayout();

        // Get the lists.
        ListsCollectionPtr GetLists();

        // Gets the number of pages.
        int GetPageCount();

        // Indicates if the page was auto rotated.
        int GetPageWasAutoRotated(int idx);

        // True if for the page OCR is performed.
        bool GetPageWasOCRd(int idx);

        Plumbing::ParagraphPtr GetParagraph(HANDLE nSolidRef);

        // Get the paragraph style templates.
        StyleTemplatesCollectionPtr GetStyleTemplates();

        // Get the topic of the PDF document.
        TopicPtr GetTopic();

        // True if at least one document page was auto rotated.
        bool WasAutoRotated();

        // True if at least for one document page OCR is performed.
        bool WasOCRd();

        // Creates a CoreModel object from a native model handle.
        static CoreModelPtr WrapNativeModel(HANDLE nativeModel);
    }; // CoreModel

}} // SolidFramework::Model

namespace SolidFramework { namespace Model { namespace Layout { 

    // Represents a chunk of text as loaded from the PDF
    class LayoutChunk: public FrameworkObject
    {
    public:
        LayoutChunk(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~LayoutChunk() { Dispose(); }

        virtual void Dispose();

        // Gets the bounds of the chunk.
        SolidFramework::Interop::RectangleFPtr GetBounds();

        bool IsEqual(LayoutChunkPtr value);

        // Gets the text.
        std::wstring GetText();
    }; // LayoutChunk

}}} // SolidFramework::Model::Layout

namespace SolidFramework { namespace Model { namespace Layout { 

    // Represents a line of text chunks that were loaded from the PDF
    class LayoutChunkLine: public FrameworkObject
    {
    public:
        LayoutChunkLine(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~LayoutChunkLine() { Dispose(); }

        virtual void Dispose();

        // Gets the bounds of the line.
        SolidFramework::Interop::RectangleFPtr GetBounds();

        // Gets the collection of LayoutChunk objects that are in the LayoutChunkLine.
        std::vector<LayoutChunkPtr> GetChunks();

        // Gets the number of LayoutChunk objects that are in the LayoutChunkLine.
        int GetCount();
    }; // LayoutChunkLine

}}} // SolidFramework::Model::Layout

namespace SolidFramework { namespace Model { namespace Layout { 

    // Represents a paragraph of text chunks that were loaded from the PDF
    class LayoutChunkParagraph: public FrameworkObject
    {
    public:
        LayoutChunkParagraph(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~LayoutChunkParagraph() { Dispose(); }

        virtual void Dispose();

        // Gets the bounds of the paragraph.
        SolidFramework::Interop::RectangleFPtr GetBounds();

        // Gets the LayoutChunkParagraph for a specific paragraph. This will return null if the CoreModel was not created with PdfOptions.ExposeSourceDocumentPagination set to true.
        static LayoutChunkParagraphPtr GetChunkParagraph(SolidFramework::Model::Plumbing::ParagraphPtr paragraph);

        // Gets the number of LayoutChunkLine objects that are in the LayoutChunkParagraph.
        int GetCount();

        // Gets the collection of LayoutChunkLine objects that are in the LayoutChunkParagraph.
        std::vector<LayoutChunkLinePtr> GetLines();

        // Gets the page number this LayoutChunkParagraph started on in the PDF.
        int GetPageNumber();

        // Gets the rotation of the paragraph and its chunks.
        double GetRotation();
    }; // LayoutChunkParagraph

}}} // SolidFramework::Model::Layout

namespace SolidFramework { namespace Model { namespace Layout { 
    class ILayoutObjectContainer
    {
    public:
        virtual std::vector<LayoutObjectPtr> GetLayoutObjects() = 0;
    };

}}} // SolidFramework::Model::Layout

namespace SolidFramework { namespace Model { namespace Layout { 

    // Contains information about the location of objects within the document.
    class LayoutDocument: public FrameworkObject, public ILayoutObjectContainer
    {
    public:
        LayoutDocument();
        LayoutDocument(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~LayoutDocument() { Dispose(); }

        virtual void Dispose();

        // Gets the number of LayoutObject within the document.
        int GetCount();

        // Finds the LayoutObject that represents the SolidObject which has the specified ID.
        LayoutObjectPtr FindLayoutObject(int val);

        // Gets the collection of LayoutObject within the document.
        virtual std::vector<LayoutObjectPtr> GetLayoutObjects();
    }; // LayoutDocument

}}} // SolidFramework::Model::Layout

namespace SolidFramework { namespace Model { namespace Layout { 

    // Abstract base class for all objects that contain layout information for items in a PDF document
    class LayoutObject: public FrameworkObject
    {
    protected:
        LayoutObject();
    public:
        LayoutObject(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~LayoutObject() { Dispose(); }

        virtual void Dispose();

        // Gets the transformed bounds of the object.
        virtual SolidFramework::Interop::RectangleFPtr GetBounds();

        // This function always returns zero.
        virtual int GetID();

        LayoutObjectType GetObjectType();

        // Gets the page number this object appears on (starting at 1) or -1 if it couldn't find the page
        virtual int GetPageNumber();

        virtual HANDLE GetRef();

        // Gets the Size of the LayoutPage.
        virtual SolidFramework::Interop::SizeFPtr GetSize();

        // Gets the transformation matrix.
        virtual SolidFramework::Interop::MatrixPtr GetTransform();
    }; // LayoutObject

}}} // SolidFramework::Model::Layout

namespace SolidFramework { namespace Model { namespace Layout { 

    // Provides access to the layout information for a graphic in a PDF document.
    class LayoutGraphic: public LayoutObject
    {
    public:
        LayoutGraphic(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~LayoutGraphic() { Dispose(); }

        virtual void Dispose();

        // Gets the ID of the Graphic object that this object refers to.
        virtual int GetID();

        virtual HANDLE GetRef();

        // Gets the Size of the LayoutGraphic.
        virtual SolidFramework::Interop::SizeFPtr GetSize();

        // Gets the transformation matrix.
        virtual SolidFramework::Interop::MatrixPtr GetTransform();

        static LayoutGraphicPtr DynamicCast(LayoutObjectPtr value);
    }; // LayoutGraphic

}}} // SolidFramework::Model::Layout

namespace SolidFramework { namespace Model { namespace Layout { 

    class LayoutGroup: public LayoutObject, public ILayoutObjectContainer
    {
    public:
        LayoutGroup(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~LayoutGroup() { Dispose(); }

        virtual void Dispose();

        // Gets the number of LayoutObject objects that are contained within the group.
        int GetCount();

        // Gets the ID of the Group object that this object refers to.
        virtual int GetID();

        // Gets the collection of LayoutObject objects that are contained within the group.
        virtual std::vector<LayoutObjectPtr> GetLayoutObjects();

        virtual HANDLE GetRef();

        // Gets the Size of the LayoutImage.
        virtual SolidFramework::Interop::SizeFPtr GetSize();

        // Gets the transformation matrix.
        virtual SolidFramework::Interop::MatrixPtr GetTransform();

        static LayoutGroupPtr DynamicCast(LayoutObjectPtr value);
    }; // LayoutGroup

}}} // SolidFramework::Model::Layout

namespace SolidFramework { namespace Model { namespace Layout { 

    // Provides access to the layout information for an image in a PDF document.
    class LayoutImage: public LayoutObject
    {
    public:
        LayoutImage(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~LayoutImage() { Dispose(); }

        virtual void Dispose();

        // Gets the ID of the ImageShape object that this object refers to.
        virtual int GetID();

        virtual HANDLE GetRef();

        // Gets the Size of the LayoutImage.
        virtual SolidFramework::Interop::SizeFPtr GetSize();

        // Gets the transformation matrix.
        virtual SolidFramework::Interop::MatrixPtr GetTransform();

        static LayoutImagePtr DynamicCast(LayoutObjectPtr value);
    }; // LayoutImage

}}} // SolidFramework::Model::Layout

namespace SolidFramework { namespace Model { namespace Layout { 

    // Provides access to the layout information for a page of a PDF document.
    class LayoutPage: public LayoutObject, public ILayoutObjectContainer
    {
    public:
        LayoutPage(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~LayoutPage() { Dispose(); }

        virtual void Dispose();

        // Gets the number of LayoutObject objects that are contained within the page.
        int GetCount();

        // This function always returns zero.
        virtual int GetID();

        // Gets the collection of LayoutObject objects that are contained within the page.
        virtual std::vector<LayoutObjectPtr> GetLayoutObjects();

        virtual int GetPageNumber();

        virtual HANDLE GetRef();

        // Gets the Size of the LayoutPage.
        virtual SolidFramework::Interop::SizeFPtr GetSize();

        // Gets the transformation matrix.
        virtual SolidFramework::Interop::MatrixPtr GetTransform();

        static LayoutPagePtr DynamicCast(LayoutObjectPtr value);
    }; // LayoutPage

}}} // SolidFramework::Model::Layout

namespace SolidFramework { namespace Model { namespace Layout { 

    class LayoutParagraph: public LayoutObject, public ILayoutObjectContainer
    {
    public:
        LayoutParagraph(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~LayoutParagraph() { Dispose(); }

        virtual void Dispose();

        // Gets all of the text associated with the paragraph that this object contains layout information for.
        std::wstring GetAllText();

        // Gets the PDF chunk locations for this paragraph grouped together into lines. This will be empty unless the CoreModel was created with PdfOptions.ExposeSourceDocumentPagination set to true.
        std::vector<LayoutChunkLinePtr> GetChunkLines();

        // Gets the number of LayoutParagraphLine objects that are contained within the paragraph.
        int GetCount();

        // Gets the ID of the Paragraph object that this object refers to.
        virtual int GetID();

        // Gets the collection of LayoutParagraphLine objects that are contained within the paragraph.
        virtual std::vector<LayoutObjectPtr> GetLayoutObjects();

        virtual HANDLE GetRef();

        // Gets the rotation of the paragraph.
        double GetRotation();

        // Gets the Size of the LayoutParagraph.
        virtual SolidFramework::Interop::SizeFPtr GetSize();

        // Gets the transformation matrix.
        virtual SolidFramework::Interop::MatrixPtr GetTransform();

        static LayoutParagraphPtr DynamicCast(LayoutObjectPtr value);
    }; // LayoutParagraph

}}} // SolidFramework::Model::Layout

namespace SolidFramework { namespace Model { namespace Layout { 

    // Represents a single line within the document, and provides access to the layout information for that line.
    class LayoutParagraphLine: public LayoutObject, public ILayoutObjectContainer
    {
    public:
        LayoutParagraphLine(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~LayoutParagraphLine() { Dispose(); }

        virtual void Dispose();

        // Gets all of the text associated with the paragraph that this object contains layout information for.
        std::wstring GetAllText();

        // Gets the number of LayoutParagraphWord objects that are contained within the line.
        int GetCount();

        int GetFirstRunCharIndex();

        int GetFirstRunIndex();

        // This function always returns zero.
        virtual int GetID();

        int GetLastRunCharIndex();

        int GetLastRunIndex();

        // Gets the collection of LayoutParagraphWord objects that are contained within the line.
        virtual std::vector<LayoutObjectPtr> GetLayoutObjects();

        HANDLE GetParagraphRef();

        virtual HANDLE GetRef();

        // Gets the Size of the LayoutParagraphLine.
        virtual SolidFramework::Interop::SizeFPtr GetSize();

        // Gets the transformation matrix.
        virtual SolidFramework::Interop::MatrixPtr GetTransform();

        static LayoutParagraphLinePtr DynamicCast(LayoutObjectPtr value);
    }; // LayoutParagraphLine

}}} // SolidFramework::Model::Layout

namespace SolidFramework { namespace Model { namespace Layout { 

    // Represents a single word within the document, and provides access to the layout information for that word.
    // NOTE: a LayoutParagraphWord does not map directly to a Run.
    class LayoutParagraphWord: public LayoutObject
    {
    protected:
        LayoutParagraphWord();
    public:
        LayoutParagraphWord(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~LayoutParagraphWord() { Dispose(); }

        virtual void Dispose();

        // Gets the bounds on page.
        virtual SolidFramework::Interop::RectangleFPtr GetBounds();

        int GetFirstRunCharIndex();

        int GetFirstRunIndex();

        // This function always returns zero.
        virtual int GetID();

        int GetLastRunCharIndex();

        int GetLastRunIndex();

        HANDLE GetParagraphRef();

        virtual HANDLE GetRef();

        // Gets the Size of the LayoutParagraphWord.
        virtual SolidFramework::Interop::SizeFPtr GetSize();

        // Gets the text that this layout object represents.
        virtual std::wstring GetText();

        // Gets the transformation matrix.
        virtual SolidFramework::Interop::MatrixPtr GetTransform();

        static LayoutParagraphWordPtr DynamicCast(LayoutObjectPtr value);
    }; // LayoutParagraphWord

}}} // SolidFramework::Model::Layout

namespace SolidFramework { namespace Model { namespace Layout { 

    // Represents a single word within the document, and provides access to the layout information for that word.
    // NOTE: a LayoutParagraphWord does not map directly to a Run.
    class LayoutParagraphListItem: public LayoutParagraphWord
    {
    public:
        LayoutParagraphListItem(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~LayoutParagraphListItem() { Dispose(); }

        virtual void Dispose();

        // Gets the bounds on page.
        virtual SolidFramework::Interop::RectangleFPtr GetBounds();

        // Gets the Size of the LayoutParagraphWord.
        virtual SolidFramework::Interop::SizeFPtr GetSize();

        // Gets the text that this layout object represents.
        virtual std::wstring GetText();

        // Gets the transformation matrix.
        virtual SolidFramework::Interop::MatrixPtr GetTransform();

        static LayoutParagraphListItemPtr DynamicCast(LayoutObjectPtr value);
    }; // LayoutParagraphListItem

}}} // SolidFramework::Model::Layout

namespace SolidFramework { namespace Model { namespace Layout { 

    // Represents a table within the document, and provides access to the layout information for that table.
    class LayoutTable: public LayoutObject, public ILayoutObjectContainer
    {
    public:
        LayoutTable(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~LayoutTable() { Dispose(); }

        virtual void Dispose();

        // Gets the number of LayoutObject objects that are contained within the table.
        int GetCount();

        // Gets the ID of the Table object that this object refers to.
        virtual int GetID();

        // Gets the collection of LayoutObject objects that are contained within the table.
        virtual std::vector<LayoutObjectPtr> GetLayoutObjects();

        virtual HANDLE GetRef();

        // Gets the Size of the LayoutTable.
        virtual SolidFramework::Interop::SizeFPtr GetSize();

        // Gets the transformation matrix.
        virtual SolidFramework::Interop::MatrixPtr GetTransform();

        static LayoutTablePtr DynamicCast(LayoutObjectPtr value);
    }; // LayoutTable

}}} // SolidFramework::Model::Layout

namespace SolidFramework { namespace Model { namespace Layout { 

    // Represents a textbox within the document, and provides access to the layout information for that textbox.
    class LayoutTextBox: public LayoutObject, public ILayoutObjectContainer
    {
    public:
        LayoutTextBox(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~LayoutTextBox() { Dispose(); }

        virtual void Dispose();

        // Gets the number of LayoutObject objects that are contained within the table.
        int GetCount();

        // Gets the ID of the TextBox object that this object refers to.
        virtual int GetID();

        // Gets the collection of LayoutObject objects that are contained within the table.
        virtual std::vector<LayoutObjectPtr> GetLayoutObjects();

        virtual HANDLE GetRef();

        // Gets the Size of the LayoutTextBox.
        virtual SolidFramework::Interop::SizeFPtr GetSize();

        // Gets the transformation matrix.
        virtual SolidFramework::Interop::MatrixPtr GetTransform();

        static LayoutTextBoxPtr DynamicCast(LayoutObjectPtr value);
    }; // LayoutTextBox

}}} // SolidFramework::Model::Layout

namespace SolidFramework { namespace Model { namespace Pdf { namespace Pages { 

    // Abstract interface that represents a single page within the PDF document. Concrete implementations are PdfPageHolder and ImagePageHolder.
    class IPageHolder: public FrameworkObject
    {
    protected:
        IPageHolder();
    public:
        IPageHolder(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~IPageHolder() { Dispose(); }

        virtual void Dispose();

        // Clones this instance. This is only implemented in derived classes.
        virtual IPageHolderPtr Clone();

        // Gets the index of the current object.
        int GetCurrentIndex();

        // Generates the thumbnail.
        virtual void GenerateThumbnail();

        // Gets the rotated Height.
        float GetHeight();

        PagesModelPtr GetModel();

        // Gets the angle by which the IPageHolder is rotated.
        virtual int GetRotation();

        // Sets the angle by which the IPageHolder is rotated.
        virtual void SetRotation(int value);

        // Gets the size of the object in points.
        SolidFramework::Interop::SizeFPtr GetSize();

        // Gets the target ID.
        int GetTargetID();

        // Sets the target ID.
        void SetTargetID(int value);

        // Gets the thumbnail.
        SolidFramework::Interop::BitmapPtr GetThumbnail();

        // Gets a value indicating whether thumbnail generated.
        bool GetThumbnailGenerated();

        // Gets a value indicating whether thumbnail generated.
        void SetThumbnailGenerated(bool value);

        virtual std::wstring ToString();

        // Gets whether this IPageHolder is visible.
        bool GetVisible();

        // Sets whether this IPageHolder is visible.
        void SetVisible(bool value);

        // Gets the rotated Width.
        float GetWidth();
    }; // IPageHolder

}}}} // SolidFramework::Model::Pdf::Pages

namespace SolidFramework { namespace Model { namespace Pdf { namespace Pages { 

    // Represents image page holder.
    class ImagePageHolder: public IPageHolder
    {
    public:
        ImagePageHolder(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~ImagePageHolder() { Dispose(); }

        virtual void Dispose();

        virtual IPageHolderPtr Clone();

        // Generates the thumbnail.
        virtual void GenerateThumbnail();

        // Gets the path to the image.
        std::wstring GetPath();

        // Gets the angle by which the IPageHolder is rotated.
        virtual int GetRotation();

        // Sets the angle by which the IPageHolder is rotated.
        virtual void SetRotation(int value);

        static ImagePageHolderPtr DynamicCast(IPageHolderPtr value);
    }; // ImagePageHolder

}}}} // SolidFramework::Model::Pdf::Pages

namespace SolidFramework { namespace Model { namespace Pdf { namespace Pages { 

    // Represents a page in a PDF file. Supports page level operations.
    class PdfPageHolder: public IPageHolder
    {
    public:
        PdfPageHolder(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfPageHolder() { Dispose(); }

        virtual void Dispose();

        // Adds a highlight comment to the PDF page.
        // location: The location of the comment. This consists of a PointFsCollection containing exactly 4 points representing the location of the  bottom-left, bottom-right, top-left and top-right of the comment. 
        // color: The color of the comment.
        // title: The title of the comment. By convention this should be the name of the person who made the change.
        // content: The text that is to be associated with the comment.
        Plumbing::CommentPtr AddHighlightComment(const std::vector<SolidFramework::Interop::PointFPtr> & location, SolidFramework::Interop::ColorPtr color, std::wstring title, std::wstring content);

        // Adds a strikeout comment to the PDF page.
        // location: The location of the comment. This consists of a PointFsCollection containing exactly 4 points representing the location of the  bottom-left, bottom-right, top-left and top-right of the comment. 
        // color: The color of the comment.
        // title: The title of the comment. By convention this should be the name of the person who made the change.
        // content: The text that is to be associated with the comment.
        Plumbing::CommentPtr AddStrikeOutComment(const std::vector<SolidFramework::Interop::PointFPtr> & location, SolidFramework::Interop::ColorPtr color, std::wstring title, std::wstring content);

        // Adds a text comment to the PDF page.
        // location: The location of the comment. This consists of a PointFsCollection containing exactly 4 points representing the location of the  bottom-left, bottom-right, top-left and top-right of the comment. 
        // color: The color of the comment.
        // title: The title of the comment. By convention this should be the name of the person who made the change.
        // content: The text that is to be associated with the comment.
        Plumbing::CommentPtr AddTextComment(const std::vector<SolidFramework::Interop::PointFPtr> & location, SolidFramework::Interop::ColorPtr color, std::wstring title, std::wstring content);

        // Adds a text comment to an existing comment.
        // parent: The existing comment to which the new comment should be attached. 
        // title: The title of the comment. By convention this should be the name of the person who made the change.
        // content: The text that is to be associated with the comment.
        Plumbing::CommentPtr AddTextComment(Plumbing::CommentPtr parent, std::wstring title, std::wstring content);

        // Adds an underline comment to the PDF page.
        // location: The location of the comment. This consists of a PointFsCollection containing exactly 4 points representing the location of the  bottom-left, bottom-right, top-left and top-right of the comment. 
        // color: The color of the comment.
        // title: The title of the comment. By convention this should be the name of the person who made the change.
        // content: The text that is to be associated with the comment.
        Plumbing::CommentPtr AddUnderlineComment(const std::vector<SolidFramework::Interop::PointFPtr> & location, SolidFramework::Interop::ColorPtr color, std::wstring title, std::wstring content);

        virtual IPageHolderPtr Clone();

        Plumbing::CommentPtr GetComment(int index);

        // Gets the number of comment annotations on the PDF page.
        int GetCommentCount();

        // Deletes an existing comment annotation on the PDF page.
        bool DeleteComment(Plumbing::CommentPtr comment);

        // Generates a thumbnail for the page.
        virtual void GenerateThumbnail();

        Plumbing::LinkPtr GetLink(int index);

        // Gets the number of link annotations on the PDF page.
        int GetLinkCount();

        // Modifies an existing comment annotation on the PDF page.
        bool ModifyComment(Plumbing::CommentPtr comment, const std::vector<SolidFramework::Interop::PointFPtr> & newLocation, SolidFramework::Interop::ColorPtr newColor, std::wstring newTitle, std::wstring newContent);

        // Gets a rectangle defining the size and location of the PdfVisibleBox.
        SolidFramework::Interop::RectangleFPtr GetPdfVisibleBox();

        // Gets the original rotation of the page in degrees.
        virtual int GetRotation();

        // Gets the original rotation of the page in degrees.
        virtual void SetRotation(int value);

        static PdfPageHolderPtr DynamicCast(IPageHolderPtr value);
    }; // PdfPageHolder

}}}} // SolidFramework::Model::Pdf::Pages

namespace SolidFramework { namespace Model { namespace Pdf { namespace Pages { 

    // Represents page holder interface.
    class IPageSource: public FrameworkObject
    {
    protected:
        IPageSource();
    public:
        IPageSource(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~IPageSource() { Dispose(); }

        virtual void Dispose();
    }; // IPageSource

}}}} // SolidFramework::Model::Pdf::Pages

namespace SolidFramework { namespace Model { namespace Pdf { namespace Pages { 

    // Represents image page source.
    class ImagePageSource: public IPageSource
    {
    public:
        ImagePageSource(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~ImagePageSource() { Dispose(); }

        virtual void Dispose();

        std::vector<ImagePageHolderPtr> GetImagePages();

        SolidFramework::Pdf::Repair::RepairResult GetRepairIssue();

        void SetRepairIssue(SolidFramework::Pdf::Repair::RepairResult value);

        static ImagePageSourcePtr DynamicCast(IPageSourcePtr value);
    }; // ImagePageSource

}}}} // SolidFramework::Model::Pdf::Pages

namespace SolidFramework { namespace Model { namespace Pdf { namespace Pages { 

    // Represents pdf page source.
    class PdfPageSource: public IPageSource
    {
    public:
        PdfPageSource(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfPageSource() { Dispose(); }

        virtual void Dispose();

        static PdfPageSourcePtr CreateSource(std::wstring originalPath, std::wstring tempCopyPath, PagesModelPtr model, std::wstring password, SolidFramework::Pdf::AccessPermissions permissionsRequired);

        // Gets the password.
        std::wstring GetPassword();

        SolidFramework::Pdf::Repair::RepairResult GetRepairIssue();

        void SetRepairIssue(SolidFramework::Pdf::Repair::RepairResult value);

        bool WasRepaired();

        void SetWasRepaired(bool value);

        static PdfPageSourcePtr DynamicCast(IPageSourcePtr value);
    }; // PdfPageSource

}}}} // SolidFramework::Model::Pdf::Pages

namespace SolidFramework { namespace Model { namespace Pdf { namespace Pages { 

    class ModelChangedEventArgs: public FrameworkObject
    {
    public:
        ModelChangedEventArgs();
        ModelChangedEventArgs(ModelChangedEvent theEvent);
        ModelChangedEventArgs(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~ModelChangedEventArgs() { Dispose(); }

        virtual void Dispose();

        // Gets a value indicating whether this ModelChangedEventArgs is clear.
        bool GetClear();

        // Gets the event.
        ModelChangedEvent GetEvent();

        // Gets a value indicating whether this ModelChangedEventArgs is rotation.
        bool GetRotation();
    }; // ModelChangedEventArgs

}}}} // SolidFramework::Model::Pdf::Pages

namespace SolidFramework { namespace Model { namespace Pdf { namespace Pages { 

    class OfferPasswordEventArgs: public FrameworkObject
    {
    public:
        OfferPasswordEventArgs();
        OfferPasswordEventArgs(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~OfferPasswordEventArgs() { Dispose(); }

        virtual void Dispose();

        std::wstring GetPassword();

        void SetPassword(std::wstring value);

        // Gets the path.
        std::wstring GetPath();

        // Gets the path.
        void SetPath(std::wstring value);

        std::wstring GetPrompt();

        void SetPrompt(std::wstring value);
        bool GetPasswordWasEntered();
        void SetPasswordWasEntered(bool value);
    }; // OfferPasswordEventArgs

}}}} // SolidFramework::Model::Pdf::Pages

namespace SolidFramework { namespace Model { namespace Pdf { namespace Pages { 

    // Abstract class that represents an operation which can be performed to a page of a PdfDocument.
    class Operation: public FrameworkObject
    {
    protected:
        Operation();
    public:
        Operation(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~Operation() { Dispose(); }

        virtual void Dispose();

        // Gets a value indicating whether this Operation is enabled.
        virtual bool GetEnabled();

        // This method is not available for use in this class.
        virtual bool Execute();

        // This method is not available for use in this class.
        virtual bool Execute(ParameterPtr parameter);

        PagesModelPtr GetModel();

        // Gets the model operation.
        virtual ModelOperation GetModelOperation();

        // Gets the name.
        virtual std::wstring GetName();
    }; // Operation

}}}} // SolidFramework::Model::Pdf::Pages

namespace SolidFramework { namespace Model { namespace Pdf { namespace Pages { namespace Operations { 

    // Operation that allows addition of an annotation within the PagesModel.
    class AddComment: public SolidFramework::Model::Pdf::Pages::Operation
    {
    public:
        AddComment(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~AddComment() { Dispose(); }

        virtual void Dispose();

        // Gets a value indicating whether this Operation is enabled.
        virtual bool GetEnabled();

        virtual bool Execute(SolidFramework::Model::Pdf::Pages::ParameterPtr parameter);

        // Gets the type model operation.
        virtual SolidFramework::Model::Pdf::Pages::ModelOperation GetModelOperation();

        // Gets the name of the current operation.
        virtual std::wstring GetName();

        static AddCommentPtr DynamicCast(SolidFramework::Model::Pdf::Pages::OperationPtr value);
    }; // AddComment

}}}}} // SolidFramework::Model::Pdf::Pages::Operations

namespace SolidFramework { namespace Model { namespace Pdf { namespace Pages { namespace Operations { 

    // Represents copy.
    class Copy: public SolidFramework::Model::Pdf::Pages::Operation
    {
    public:
        Copy(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~Copy() { Dispose(); }

        virtual void Dispose();

        // Gets a value indicating whether this Operation is enabled.
        virtual bool GetEnabled();

        virtual bool Execute(SolidFramework::Model::Pdf::Pages::ParameterPtr parameter);

        // Gets the type model operation.
        virtual SolidFramework::Model::Pdf::Pages::ModelOperation GetModelOperation();

        // Gets the name of the operation.
        virtual std::wstring GetName();

        static CopyPtr DynamicCast(SolidFramework::Model::Pdf::Pages::OperationPtr value);
    }; // Copy

}}}}} // SolidFramework::Model::Pdf::Pages::Operations

namespace SolidFramework { namespace Model { namespace Pdf { namespace Pages { namespace Operations { 

    // Operation that allows deletion of pages within the PagesModel.
    class Delete: public SolidFramework::Model::Pdf::Pages::Operation
    {
    public:
        Delete(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~Delete() { Dispose(); }

        virtual void Dispose();

        // Gets a value indicating whether this Operation is enabled.
        virtual bool GetEnabled();

        virtual bool Execute();

        virtual bool Execute(SolidFramework::Model::Pdf::Pages::ParameterPtr parameter);

        // Gets the type model operation.
        virtual SolidFramework::Model::Pdf::Pages::ModelOperation GetModelOperation();

        // Gets the name of the operation.
        virtual std::wstring GetName();

        static DeletePtr DynamicCast(SolidFramework::Model::Pdf::Pages::OperationPtr value);
    }; // Delete

}}}}} // SolidFramework::Model::Pdf::Pages::Operations

namespace SolidFramework { namespace Model { namespace Pdf { namespace Pages { namespace Operations { 

    // Operation that allows deletion of an annotation within the PagesModel.
    class DeleteComment: public SolidFramework::Model::Pdf::Pages::Operation
    {
    public:
        DeleteComment(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~DeleteComment() { Dispose(); }

        virtual void Dispose();

        // Gets a value indicating whether this Operation is enabled.
        virtual bool GetEnabled();

        virtual bool Execute(SolidFramework::Model::Pdf::Pages::ParameterPtr parameter);

        // Gets the type model operation.
        virtual SolidFramework::Model::Pdf::Pages::ModelOperation GetModelOperation();

        // Gets the name of the operation.
        virtual std::wstring GetName();

        static DeleteCommentPtr DynamicCast(SolidFramework::Model::Pdf::Pages::OperationPtr value);
    }; // DeleteComment

}}}}} // SolidFramework::Model::Pdf::Pages::Operations

namespace SolidFramework { namespace Model { namespace Pdf { namespace Pages { namespace Operations { 

    // Represents insert.
    class Insert: public SolidFramework::Model::Pdf::Pages::Operation
    {
    public:
        Insert(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~Insert() { Dispose(); }

        virtual void Dispose();

        // Gets a value indicating whether this Operation is enabled.
        virtual bool GetEnabled();

        virtual bool Execute(SolidFramework::Model::Pdf::Pages::ParameterPtr parameter);

        // Gets the type model operation.
        virtual SolidFramework::Model::Pdf::Pages::ModelOperation GetModelOperation();

        // Gets the name of the operation.
        virtual std::wstring GetName();

        static InsertPtr DynamicCast(SolidFramework::Model::Pdf::Pages::OperationPtr value);
    }; // Insert

}}}}} // SolidFramework::Model::Pdf::Pages::Operations

namespace SolidFramework { namespace Model { namespace Pdf { namespace Pages { namespace Operations { 

    // Operation that allows modification of an annotation within the PagesModel.
    class ModifyComment: public SolidFramework::Model::Pdf::Pages::Operation
    {
    public:
        ModifyComment(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~ModifyComment() { Dispose(); }

        virtual void Dispose();

        // Gets a value indicating whether this Operation is enabled.
        virtual bool GetEnabled();

        virtual bool Execute(SolidFramework::Model::Pdf::Pages::ParameterPtr parameter);

        // Gets the type model operation.
        virtual SolidFramework::Model::Pdf::Pages::ModelOperation GetModelOperation();

        // Gets the name of the operation.
        virtual std::wstring GetName();

        static ModifyCommentPtr DynamicCast(SolidFramework::Model::Pdf::Pages::OperationPtr value);
    }; // ModifyComment

}}}}} // SolidFramework::Model::Pdf::Pages::Operations

namespace SolidFramework { namespace Model { namespace Pdf { namespace Pages { namespace Operations { 

    // Operation that allows moving of pages within the PagesModel.
    class Move: public SolidFramework::Model::Pdf::Pages::Operation
    {
    public:
        Move(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~Move() { Dispose(); }

        virtual void Dispose();

        // Gets a value indicating whether this Operation is enabled.
        virtual bool GetEnabled();

        virtual bool Execute(SolidFramework::Model::Pdf::Pages::ParameterPtr parameter);

        // Gets the type model operation.
        virtual SolidFramework::Model::Pdf::Pages::ModelOperation GetModelOperation();

        // Gets the name of the operation.
        virtual std::wstring GetName();

        static MovePtr DynamicCast(SolidFramework::Model::Pdf::Pages::OperationPtr value);
    }; // Move

}}}}} // SolidFramework::Model::Pdf::Pages::Operations

namespace SolidFramework { namespace Model { namespace Pdf { namespace Pages { namespace Operations { 

    // Operation that allows redoing of an operation that was undone within the PagesModel.
    class Redo: public SolidFramework::Model::Pdf::Pages::Operation
    {
    public:
        Redo(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~Redo() { Dispose(); }

        virtual void Dispose();

        // Gets a value indicating whether this Operation is enabled.
        virtual bool GetEnabled();

        virtual bool Execute();

        // Gets the type model operation.
        virtual SolidFramework::Model::Pdf::Pages::ModelOperation GetModelOperation();

        // Gets the name of the operation.
        virtual std::wstring GetName();

        virtual std::wstring ToString();

        static RedoPtr DynamicCast(SolidFramework::Model::Pdf::Pages::OperationPtr value);
    }; // Redo

}}}}} // SolidFramework::Model::Pdf::Pages::Operations

namespace SolidFramework { namespace Model { namespace Pdf { namespace Pages { namespace Operations { 

    // Operation that allows rotation of pages within the PagesModel.
    class Rotate: public SolidFramework::Model::Pdf::Pages::Operation
    {
    public:
        Rotate(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~Rotate() { Dispose(); }

        virtual void Dispose();

        // Gets a value indicating whether this Operation is enabled.
        virtual bool GetEnabled();

        virtual bool Execute(SolidFramework::Model::Pdf::Pages::ParameterPtr parameter);

        // Gets the type model operation.
        virtual SolidFramework::Model::Pdf::Pages::ModelOperation GetModelOperation();

        // Gets the name of the operation.
        virtual std::wstring GetName();

        static RotatePtr DynamicCast(SolidFramework::Model::Pdf::Pages::OperationPtr value);
    }; // Rotate

}}}}} // SolidFramework::Model::Pdf::Pages::Operations

namespace SolidFramework { namespace Model { namespace Pdf { namespace Pages { namespace Operations { 

    // Operation that allows undoing an operation that has been executed on the PagesModel.
    class Undo: public SolidFramework::Model::Pdf::Pages::Operation
    {
    public:
        Undo(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~Undo() { Dispose(); }

        virtual void Dispose();

        // Gets a value indicating whether this Operation is enabled.
        virtual bool GetEnabled();

        virtual bool Execute();

        // Gets the type model operation.
        virtual SolidFramework::Model::Pdf::Pages::ModelOperation GetModelOperation();

        // Gets the name of the operation.
        virtual std::wstring GetName();

        virtual std::wstring ToString();

        static UndoPtr DynamicCast(SolidFramework::Model::Pdf::Pages::OperationPtr value);
    }; // Undo

}}}}} // SolidFramework::Model::Pdf::Pages::Operations

namespace SolidFramework { namespace Model { namespace Pdf { namespace Pages { 

    // Represents types of Pages Changed Event Args.
    class PagesChangedEventArgs: public FrameworkObject
    {
    public:
        PagesChangedEventArgs();
        PagesChangedEventArgs(OperationPtr operation);
        PagesChangedEventArgs(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PagesChangedEventArgs() { Dispose(); }

        virtual void Dispose();

        // Gets the operation.
        OperationPtr GetOperation();

        // Gets the operation.
        void SetOperation(OperationPtr value);
    }; // PagesChangedEventArgs

}}}} // SolidFramework::Model::Pdf::Pages

namespace SolidFramework { namespace Model { namespace Pdf { namespace Pages { 

    // Represents a selection that is part or all of a page.
    class PageSelection: public FrameworkObject
    {
    public:
        PageSelection(IPageHolderPtr page, SolidFramework::Interop::RectangleFPtr area);
        PageSelection(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PageSelection() { Dispose(); }

        virtual void Dispose();

        // Gets the area within the page that has been selected.
        SolidFramework::Interop::RectangleFPtr GetArea();

        // Gets the page that the selection is located on.
        IPageHolderPtr GetPage();
    }; // PageSelection

}}}} // SolidFramework::Model::Pdf::Pages

namespace SolidFramework { namespace Model { namespace Pdf { namespace Pages { 

    class PagesModel: public FrameworkObject
    {
    public:
        PagesModel();
        PagesModel(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PagesModel() { Dispose(); }

        virtual void Dispose();

        // Closes this instance.
        virtual void Close();

        std::vector<IPageHolderPtr> GetAllPages();

        bool CanRedo();

        bool CanUndo();

        SolidFramework::Plumbing::ValidationMode GetClaimedValidationMode();

        void SetClaimedValidationMode(SolidFramework::Plumbing::ValidationMode value);

        // Clears this instance.
        void Clear();

        IPageSourcePtr CreateSource(std::wstring originalPath);

        IPageSourcePtr CreateSource(std::wstring originalPath, SolidFramework::Pdf::AccessPermissions permissionsRequired, std::wstring attemptPassword);

        IPageSourcePtr CreateSource(std::wstring tempCopyPath, std::wstring originalPath, SolidFramework::Pdf::AccessPermissions permissionsRequired, std::wstring attemptPassword);

        // Gets the current page.
        int GetCurrentPage();

        // Gets the current page.
        void SetCurrentPage(int value);

        // Gets the description.
        Plumbing::DescriptionPtr GetDescription();

        // Gets the description.
        void SetDescription(Plumbing::DescriptionPtr value);

        // Gets a value indicating whether empty model allowed.
        bool GetEmptyModelAllowed();

        // Gets a value indicating whether empty model allowed.
        void SetEmptyModelAllowed(bool value);

        bool EnterOwnerAuthenticationMode();

        bool IsEqual(PagesModelPtr value);

        int FindPageIndex(int objectID);

        IPageHolderPtr FindVisiblePage(int pageIndex);

        // Gets the fonts used in this model (all PDF sources). Fonts from deleted pages will still be listed.
        std::vector<SolidFramework::Pdf::Plumbing::PdfFontPtr> GetFonts();

        int IndexToTargetID(int index);

        // Gets the initial view.
        Plumbing::InitialViewPtr GetInitialView();

        // Gets the initial view.
        void SetInitialView(Plumbing::InitialViewPtr value);

        virtual void InsertRange(int position, const std::vector<SolidFramework::Model::Pdf::Pages::IPageSourcePtr> & collection);

        virtual void InsertRange(int insert, const std::vector<SolidFramework::Model::Pdf::Pages::IPageHolderPtr> & collection);

        virtual void InsertRange(int position, const std::vector<std::wstring> & pageFiles, bool preferPdfAWhenCreating);

        // Gets the instance.
        static int GetInstance();

        // Sets the instance.
        static void SetInstance(int value);

        bool IsSinglePDFSource();

        // Gets a value indicating whether this instance is untitled.
        bool IsUntitled();

        // Gets a value indicating whether this instance is untitled.
        void SetIsUntitled(bool value);

        // Gets a value indicating whether this PagesModel is modified.
        bool GetModified();

        void ModifySecurity(SolidFramework::Pdf::EncryptionAlgorithm algorithm, SolidFramework::Pdf::AccessPermissions permissions, std::wstring ownerPassword, std::wstring userPassword, std::wstring originalOwnerPassword);

        SolidFramework::Pdf::Plumbing::PdfNamedDestinationsCollectionPtr GetNamedDestinations();

        int GetNextTargetID();

        static int NormalizeRotation(int rotation);

        virtual void OpenPDF(std::wstring pdfPath);

        virtual void OpenPDF(std::wstring pdfPath, std::wstring attemptPassword);

        virtual void OpenPDF(std::wstring pdfPath, SolidFramework::Pdf::AccessPermissions permissionsRequired, SolidFramework::Pdf::AuthenticationModeType authenticationRequired, std::wstring attemptPassword);

        // Gets the operation.
        OperationPtr GetOperation(ModelOperation modelOperation);

        // Gets the original path.
        std::wstring GetOriginalPath();

        // Gets the original path.
        void SetOriginalPath(std::wstring value);

        HANDLE GetOwnerHandle();

        void SetOwnerHandle(HANDLE value);

        // Gets pages count.
        int GetPageCount();

        IPageHolderPtr GetPageHolder(int index);

        SolidFramework::Interop::RectangleFPtr GetPageRectangle(Plumbing::PageLayout pageLayout, int visiblePageIndex, double gutter);

        // Gets the page selections.
        std::vector<PageSelectionPtr> GetPageSelections();

        // Gets the page selections.
        void SetPageSelections(const std::vector<SolidFramework::Model::Pdf::Pages::PageSelectionPtr> & value);

        // Gets the popup title.
        std::wstring GetPopupTitle();

        // Gets the popup title.
        void SetPopupTitle(std::wstring value);

        // Gets the producer.
        std::wstring GetProducer();

        // Gets the producer.
        void SetProducer(std::wstring value);

        // Gets the progress fraction.
        int GetProgressFraction();

        // Gets the progress fraction.
        void SetProgressFraction(int value);

        // Gets a value indicating whether read only.
        bool GetReadOnly();

        // Gets a value indicating whether read only.
        void SetReadOnly(bool value);

        // Gets the read only info.
        Plumbing::ReadOnlyInfoPtr GetReadOnlyInfo();

        void Redo();

        // Gets the redo stack.
        std::vector<TransactionPtr> GetRedoStack();

        // The type of repair that took place in the event of repair of the PDF duing loading.
        SolidFramework::Pdf::Repair::RepairResult GetRepairIssue();

        // The type of repair that took place in the event of repair of the PDF duing loading.
        void SetRepairIssue(SolidFramework::Pdf::Repair::RepairResult value);

        virtual std::wstring GetResourceString(std::wstring id);

        void Save(std::wstring pdfPath);

        bool Save(std::wstring pdfPath, bool selectionOnly);

        bool Save(std::wstring pdfPath, bool selectionOnly, bool copyOnly);

        bool Save(std::wstring pdfPath, bool selectionOnly, bool copyOnly, bool noOptimize);

        bool IsSecurityEditable();

        // Gets the selected page range.
        SolidFramework::PageRangePtr GetSelectedPageRange();

        bool HasSelectedScannedPages();

        // Gets the selection.
        std::vector<int> GetSelection();

        // Gets the selection.
        void SetSelection(const std::vector<int> & value);

        SolidFramework::Pdf::PdfDocumentPtr GetSinglePDFSource();

        std::vector<IPageSourcePtr> GetSources();

        int TargetIDtoIndex(int targetID);

        // Gets the thumbnail dpi.
        virtual int GetThumbnailDpi();

        // Sets the thumbnail dpi.
        virtual void SetThumbnailDpi(int value);

        void Undo();

        // Gets the undo disabled.
        bool GetUndoDisabled();

        // Gets the undo disabled.
        void SetUndoDisabled(bool value);

        // Gets the undo stack.
        std::vector<TransactionPtr> GetUndoStack();

        void ValidateIndices(const std::vector<int> & indices);

        bool HasVisibleNewScannedPages();

        std::vector<IPageHolderPtr> GetVisiblePages();

        bool HasVisibleScannedPages();
        
        // Event callbacks
        std::function<void(ModelChangedEventArgsPtr)> OnModelChanged;
        std::function<void(PagesChangedEventArgsPtr)> OnPagesChanged;
        std::function<void(SaveProgressEventArgsPtr)> OnSaveProgress;
        std::function<void(PasswordEnteredEventArgsPtr)> OnPasswordEntered;
        std::function<void(OfferPasswordEventArgsPtr)> OnOfferPasswordPrompt;
        std::function<void(ThumbnailChangedEventArgsPtr)> OnThumbnailChanged;
        std::function<void(RepairFailedEventArgsPtr)> OnRepairFailed;
        
        // Overridable event handlers
        virtual void FireModelChanged(const ModelChangedEventArgsPtr args) const;
        virtual void FirePagesChanged(const PagesChangedEventArgsPtr args) const;
        virtual void FireSaveProgress(const SaveProgressEventArgsPtr args) const;
        virtual void FirePasswordEntered(const PasswordEnteredEventArgsPtr args) const;
        virtual void FireOfferPasswordPrompt(const OfferPasswordEventArgsPtr args) const;
        virtual void FireThumbnailChanged(const ThumbnailChangedEventArgsPtr args) const;
        virtual void FireRepairFailed(const RepairFailedEventArgsPtr args) const;
    }; // PagesModel

}}}} // SolidFramework::Model::Pdf::Pages

namespace SolidFramework { namespace Model { namespace Pdf { namespace Pages { 

    class Parameter: public FrameworkObject
    {
    public:
        Parameter();
        Parameter(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~Parameter() { Dispose(); }

        virtual void Dispose();

        bool IsEqual(ParameterPtr value);
    }; // Parameter

}}}} // SolidFramework::Model::Pdf::Pages

namespace SolidFramework { namespace Model { namespace Pdf { namespace Pages { 

    // Operation that allows deletion of an annotation within the PagesModel.
    class IntParameter: public Parameter
    {
    public:
        IntParameter(int value);
        IntParameter(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~IntParameter() { Dispose(); }

        virtual void Dispose();

        int GetValue();

        static IntParameterPtr DynamicCast(ParameterPtr value);
    }; // IntParameter

}}}} // SolidFramework::Model::Pdf::Pages

namespace SolidFramework { namespace Model { namespace Pdf { namespace Pages { namespace Operations { 

    class AddCommentParameter: public SolidFramework::Model::Pdf::Pages::Parameter
    {
    public:
        AddCommentParameter(SolidFramework::Model::Pdf::Pages::IPageHolderPtr page, SolidFramework::Model::Pdf::Pages::Plumbing::AnnotationType type, const std::vector<SolidFramework::Interop::PointFPtr> & position, SolidFramework::Interop::ColorPtr color, std::wstring title, std::wstring content);
        AddCommentParameter(SolidFramework::Model::Pdf::Pages::Plumbing::CommentPtr parentComment, std::wstring title, std::wstring content);
        AddCommentParameter(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~AddCommentParameter() { Dispose(); }

        virtual void Dispose();

        SolidFramework::Interop::ColorPtr GetColor();

        std::wstring GetContent();

        SolidFramework::Model::Pdf::Pages::IPageHolderPtr GetIPageHolder();

        std::vector<SolidFramework::Interop::PointFPtr> GetPosition();

        std::wstring GetTitle();

        SolidFramework::Model::Pdf::Pages::Plumbing::AnnotationType GetType();

        static AddCommentParameterPtr DynamicCast(SolidFramework::Model::Pdf::Pages::ParameterPtr value);
    }; // AddCommentParameter

}}}}} // SolidFramework::Model::Pdf::Pages::Operations

namespace SolidFramework { namespace Model { namespace Pdf { namespace Pages { namespace Operations { 

    // Operation that allows deletion of an annotation within the PagesModel.
    class DeleteCommentParameter: public SolidFramework::Model::Pdf::Pages::Parameter
    {
    public:
        DeleteCommentParameter(SolidFramework::Model::Pdf::Pages::Plumbing::CommentPtr comment);
        DeleteCommentParameter(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~DeleteCommentParameter() { Dispose(); }

        virtual void Dispose();

        SolidFramework::Model::Pdf::Pages::Plumbing::CommentPtr GetDeletedComment();

        static DeleteCommentParameterPtr DynamicCast(SolidFramework::Model::Pdf::Pages::ParameterPtr value);
    }; // DeleteCommentParameter

}}}}} // SolidFramework::Model::Pdf::Pages::Operations

namespace SolidFramework { namespace Model { namespace Pdf { namespace Pages { namespace Operations { 

    // Operation that allows deletion of an annotation within the PagesModel.
    class InsertParameter: public SolidFramework::Model::Pdf::Pages::Parameter
    {
    public:
        InsertParameter(int position, SolidFramework::Model::Pdf::Pages::IPageSourcePtr source);
        InsertParameter(int position, const std::vector<SolidFramework::Model::Pdf::Pages::IPageSourcePtr> & sources);
        InsertParameter(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~InsertParameter() { Dispose(); }

        virtual void Dispose();

        int GetPosition();

        static InsertParameterPtr DynamicCast(SolidFramework::Model::Pdf::Pages::ParameterPtr value);
    }; // InsertParameter

}}}}} // SolidFramework::Model::Pdf::Pages::Operations

namespace SolidFramework { namespace Model { namespace Pdf { namespace Pages { namespace Operations { 

    // Operation that allows deletion of an annotation within the PagesModel.
    class ModifyCommentParameter: public SolidFramework::Model::Pdf::Pages::Parameter
    {
    public:
        ModifyCommentParameter(SolidFramework::Model::Pdf::Pages::Plumbing::CommentPtr modifiedComment, const std::vector<SolidFramework::Interop::PointFPtr> & position, SolidFramework::Interop::ColorPtr color, std::wstring title, std::wstring content);
        ModifyCommentParameter(SolidFramework::Model::Pdf::Pages::Plumbing::CommentPtr modifiedComment, const std::vector<SolidFramework::Interop::PointFPtr> & position);
        ModifyCommentParameter(SolidFramework::Model::Pdf::Pages::Plumbing::CommentPtr modifiedComment, SolidFramework::Interop::ColorPtr color);
        ModifyCommentParameter(SolidFramework::Model::Pdf::Pages::Plumbing::CommentPtr modifiedComment, std::wstring title, std::wstring content);
        ModifyCommentParameter(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~ModifyCommentParameter() { Dispose(); }

        virtual void Dispose();

        SolidFramework::Interop::ColorPtr GetColor();

        std::wstring GetContent();

        SolidFramework::Model::Pdf::Pages::Plumbing::CommentPtr GetModifiedComment();

        std::vector<SolidFramework::Interop::PointFPtr> GetPosition();

        std::wstring GetTitle();

        static ModifyCommentParameterPtr DynamicCast(SolidFramework::Model::Pdf::Pages::ParameterPtr value);
    }; // ModifyCommentParameter

}}}}} // SolidFramework::Model::Pdf::Pages::Operations

namespace SolidFramework { namespace Model { namespace Pdf { namespace Pages { 

    // Represents types of Password Entered Event Args.
    class PasswordEnteredEventArgs: public FrameworkObject
    {
    public:
        PasswordEnteredEventArgs();
        PasswordEnteredEventArgs(std::wstring password);
        PasswordEnteredEventArgs(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PasswordEnteredEventArgs() { Dispose(); }

        virtual void Dispose();

        // Gets the password.
        std::wstring GetPassword();

        // Gets the password.
        void SetPassword(std::wstring value);
    }; // PasswordEnteredEventArgs

}}}} // SolidFramework::Model::Pdf::Pages

namespace SolidFramework { namespace Model { namespace Pdf { namespace Pages { namespace Plumbing { 

    // This class provides internal support for the PageView
    class Annotation: public FrameworkObject
    {
    protected:
        Annotation();
    public:
        Annotation(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~Annotation() { Dispose(); }

        virtual void Dispose();

        bool IsActive();

        BorderStyle GetBorderStyle();

        SolidFramework::Interop::RectangleFPtr GetCanvasRectangle(SolidFramework::Interop::RectangleFPtr pageCanvasRectangle);

        SolidFramework::Interop::ColorPtr GetColor();

        void Draw(HANDLE hdc, SolidFramework::Interop::RectangleFPtr screenRectangle);

        SolidFramework::Model::Pdf::Pages::IPageHolderPtr GetIPageHolder();

        double GetLineWidth();

        // Gets the date and time when the annotation was most recently modified.
        SolidFramework::Interop::DateTimePtr GetModificationDate();

        std::wstring GetText();

        virtual std::wstring ToString();
    }; // Annotation

}}}}} // SolidFramework::Model::Pdf::Pages::Plumbing

namespace SolidFramework { namespace Model { namespace Pdf { namespace Pages { namespace Plumbing { 

    // This class provides internal support for the PageView
    class Comment: public Annotation
    {
    protected:
        Comment();
    public:
        Comment(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~Comment() { Dispose(); }

        virtual void Dispose();

        std::vector<AnnotationPtr> GetChildren();

        int GetCount();

        // Gets the date and time when the annotation was created.
        SolidFramework::Interop::DateTimePtr GetCreationDate();

        // Gets the text label to be displayed in the title bar of the annotation's pop-up window when open and active.
        // By convention, this entry identifies the user who added the annotation.
        std::wstring GetLabel();

        std::vector<SolidFramework::Interop::PointFPtr> GetLocation();

        static CommentPtr DynamicCast(AnnotationPtr value);
    }; // Comment

}}}}} // SolidFramework::Model::Pdf::Pages::Plumbing

namespace SolidFramework { namespace Model { namespace Pdf { namespace Pages { namespace Plumbing { 

    // This class provides internal support for the PageView
    class HighlightComment: public Comment
    {
    public:
        HighlightComment();
        HighlightComment(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~HighlightComment() { Dispose(); }

        virtual void Dispose();

        virtual std::wstring ToString();

        static HighlightCommentPtr DynamicCast(AnnotationPtr value);
    }; // HighlightComment

}}}}} // SolidFramework::Model::Pdf::Pages::Plumbing

namespace SolidFramework { namespace Model { namespace Pdf { namespace Pages { namespace Plumbing { 

    // This class provides internal support for the PageView
    class StrikeOutComment: public Comment
    {
    public:
        StrikeOutComment();
        StrikeOutComment(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~StrikeOutComment() { Dispose(); }

        virtual void Dispose();

        virtual std::wstring ToString();

        static StrikeOutCommentPtr DynamicCast(AnnotationPtr value);
    }; // StrikeOutComment

}}}}} // SolidFramework::Model::Pdf::Pages::Plumbing

namespace SolidFramework { namespace Model { namespace Pdf { namespace Pages { namespace Plumbing { 

    // This class provides internal support for the PageView
    class TextComment: public Comment
    {
    public:
        TextComment();
        TextComment(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~TextComment() { Dispose(); }

        virtual void Dispose();

        virtual std::wstring ToString();

        static TextCommentPtr DynamicCast(AnnotationPtr value);
    }; // TextComment

}}}}} // SolidFramework::Model::Pdf::Pages::Plumbing

namespace SolidFramework { namespace Model { namespace Pdf { namespace Pages { namespace Plumbing { 

    // This class provides internal support for the PageView
    class UnderlineComment: public Comment
    {
    public:
        UnderlineComment();
        UnderlineComment(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~UnderlineComment() { Dispose(); }

        virtual void Dispose();

        virtual std::wstring ToString();

        static UnderlineCommentPtr DynamicCast(AnnotationPtr value);
    }; // UnderlineComment

}}}}} // SolidFramework::Model::Pdf::Pages::Plumbing

namespace SolidFramework { namespace Model { namespace Pdf { namespace Pages { namespace Plumbing { 

    // This class provides internal support for the PageView
    class Link: public Annotation
    {
    protected:
        Link();
    public:
        Link(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~Link() { Dispose(); }

        virtual void Dispose();

        static LinkPtr DynamicCast(AnnotationPtr value);
    }; // Link

}}}}} // SolidFramework::Model::Pdf::Pages::Plumbing

namespace SolidFramework { namespace Model { namespace Pdf { namespace Pages { namespace Plumbing { 

    // This class provides internal support for the PageView
    class GoToLink: public Link
    {
    public:
        GoToLink();
        GoToLink(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~GoToLink() { Dispose(); }

        virtual void Dispose();

        int GetIndex();

        static GoToLinkPtr DynamicCast(AnnotationPtr value);
    }; // GoToLink

}}}}} // SolidFramework::Model::Pdf::Pages::Plumbing

namespace SolidFramework { namespace Model { namespace Pdf { namespace Pages { namespace Plumbing { 

    // This class primarily provides internal support for the PageView form.
    class URILink: public Link
    {
    public:
        URILink();
        URILink(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~URILink() { Dispose(); }

        virtual void Dispose();

        bool IsSafe();

        std::wstring GetURI();

        void SetURI(std::wstring value);

        static URILinkPtr DynamicCast(AnnotationPtr value);
    }; // URILink

}}}}} // SolidFramework::Model::Pdf::Pages::Plumbing

namespace SolidFramework { namespace Model { namespace Pdf { namespace Pages { namespace Plumbing { 

    // Provides access to author specified metadata for the document.
    // This class provides internal support for the DocumentPropertiesForm.
    class Description: public FrameworkObject
    {
    public:
        Description();
        Description(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~Description() { Dispose(); }

        virtual void Dispose();

        // Gets of sets the author for the document.
        std::wstring GetAuthor();

        // Gets of sets the author for the document.
        void SetAuthor(std::wstring value);

        // Gets the keywords for the document.
        std::wstring GetKeywords();

        // Sets the keywords for the document.
        void SetKeywords(std::wstring value);

        // Gets a value indicating whether this Description has been modified.
        bool GetModified();

        // Sets a value indicating whether this Description has been modified.
        void SetModified(bool value);

        // Gets the subject of the document.
        std::wstring GetSubject();

        // Sets the subject of the document.
        void SetSubject(std::wstring value);

        // Gets the title of the document.
        std::wstring GetTitle();

        // Sets the title of the document.
        void SetTitle(std::wstring value);
    }; // Description

}}}}} // SolidFramework::Model::Pdf::Pages::Plumbing

namespace SolidFramework { namespace Model { namespace Pdf { namespace Pages { namespace Plumbing { 

    // Represents initial view to be used when displaying a PDF document.
    // This class primarily provides internal support for the DocumentPropertiesForm.
    class InitialView: public FrameworkObject
    {
    public:
        InitialView(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~InitialView() { Dispose(); }

        virtual void Dispose();

        // Gets a value indicating whether [center window on screen].
        bool GetCenterWindowOnScreen();

        // Gets a value indicating whether [center window on screen].
        void SetCenterWindowOnScreen(bool value);

        // Gets a value indicating whether [display document title].
        bool GetDisplayDocumentTitle();

        // Gets a value indicating whether [display document title].
        void SetDisplayDocumentTitle(bool value);

        // Gets a value indicating whether [hide menubar].
        bool GetHideMenuBar();

        // Gets a value indicating whether [hide menubar].
        void SetHideMenuBar(bool value);

        // Gets a value indicating whether [hide toolbars].
        bool GetHideToolBars();

        // Gets a value indicating whether [hide toolbars].
        void SetHideToolBars(bool value);

        // Gets a value indicating whether [hide window controls].
        bool GetHideWindowControls();

        // Gets a value indicating whether [hide window controls].
        void SetHideWindowControls(bool value);

        // Gets the initial page layout.
        PageLayout GetInitialPageLayout();

        // Gets the initial page layout.
        void SetInitialPageLayout(PageLayout value);

        bool IsDefault(SolidFramework::Model::Pdf::Pages::PagesModelPtr model);

        // Gets the magnification.
        PageMagnification GetMagnification();

        // Gets the magnification.
        void SetMagnification(PageMagnification value);

        // Gets a value indicating whether this InitialView is modified.
        bool GetModified();

        // Gets a value indicating whether this InitialView is modified.
        void SetModified(bool value);

        // Gets the navigation tab.
        NavigationTab GetNavigationTab();

        // Gets the navigation tab.
        void SetNavigationTab(NavigationTab value);

        // Gets a value indicating whether [open in full screen mode].
        bool GetOpenInFullScreenMode();

        // Gets a value indicating whether [open in full screen mode].
        void SetOpenInFullScreenMode(bool value);

        // Gets the open to page target ID.
        int GetOpenToPageTargetID();

        // Gets the open to page target ID.
        void SetOpenToPageTargetID(int value);

        // Gets the percent.
        double GetPercent();

        // Gets the percent.
        void SetPercent(double value);

        // Gets a value indicating whether [resize window to initial page].
        bool GetResizeWindowToInitialPage();

        // Gets a value indicating whether [resize window to initial page].
        void SetResizeWindowToInitialPage(bool value);
    }; // InitialView

}}}}} // SolidFramework::Model::Pdf::Pages::Plumbing

namespace SolidFramework { namespace Model { namespace Pdf { namespace Pages { namespace Plumbing { 

    // This class primarily provides internal support for the DocumentPropertiesForm and PdfViewer forms.
    // However it can also be accessed from the PagesModel object.
    class ReadOnlyInfo: public FrameworkObject
    {
    public:
        ReadOnlyInfo();
        ReadOnlyInfo(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~ReadOnlyInfo() { Dispose(); }

        virtual void Dispose();

        // Gets the access permissions.
        SolidFramework::Pdf::AccessPermissions GetAccessPermissions();

        // Gets the authentication mode.
        SolidFramework::Pdf::AuthenticationModeType GetAuthenticationMode();

        // Gets the creation date.
        SolidFramework::Interop::DateTimePtr GetCreationDate();

        // Gets the creator.
        std::wstring GetCreator();

        // Gets the encryption algorithm.
        SolidFramework::Pdf::EncryptionAlgorithm GetEncryptionAlgorithm();

        // Gets the first height of the page.
        double GetFirstPageHeight();

        // Gets the first width of the page.
        double GetFirstPageWidth();

        // Gets a value indicating whether this instance is linearized.
        bool IsLinearized();

        // Gets a value indicating whether this instance is tagged.
        bool IsTagged();

        // Gets the modification date.
        SolidFramework::Interop::DateTimePtr GetModificationDate();

        // Gets the password used to open the PDF file.
        std::wstring GetOpenPassword();

        // Gets the producer.
        std::wstring GetProducer();

        // Gets the size in bytes.
        int GetSizeInBytes();

        // Gets the version.
        std::wstring GetVersion();

        // Gets a value indicating whether this PDF was repaired as part of being loaded.
        bool WasRepaired();
    }; // ReadOnlyInfo

}}}}} // SolidFramework::Model::Pdf::Pages::Plumbing

namespace SolidFramework { namespace Model { namespace Pdf { namespace Pages { 

    class RepairFailedEventArgs: public FrameworkObject
    {
    public:
        RepairFailedEventArgs();
        RepairFailedEventArgs(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~RepairFailedEventArgs() { Dispose(); }

        virtual void Dispose();

        std::wstring GetPath();

        void SetPath(std::wstring value);

        std::wstring GetReason();

        void SetReason(std::wstring value);
    }; // RepairFailedEventArgs

}}}} // SolidFramework::Model::Pdf::Pages

namespace SolidFramework { namespace Model { namespace Pdf { namespace Pages { 

    class SaveProgressEventArgs: public FrameworkObject
    {
    public:
        SaveProgressEventArgs();
        SaveProgressEventArgs(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~SaveProgressEventArgs() { Dispose(); }

        virtual void Dispose();

        // Gets the maximum.
        int GetMaximum();

        // Gets the maximum.
        void SetMaximum(int value);

        // Gets the phase.
        ProgressPhase GetPhase();

        // Gets the phase.
        void SetPhase(ProgressPhase value);

        // Gets the tick.
        int GetTick();

        // Gets the tick.
        void SetTick(int value);
    }; // SaveProgressEventArgs

}}}} // SolidFramework::Model::Pdf::Pages

namespace SolidFramework { namespace Model { namespace Pdf { namespace Pages { 

    class ThumbnailChangedEventArgs: public FrameworkObject
    {
    public:
        ThumbnailChangedEventArgs();
        ThumbnailChangedEventArgs(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~ThumbnailChangedEventArgs() { Dispose(); }

        virtual void Dispose();

        // Gets a value indicating whether invoke required.
        bool GetInvokeRequired();

        // Gets a value indicating whether invoke required.
        void SetInvokeRequired(bool value);

        // Gets the page holder.
        IPageHolderPtr GetPageHolder();

        // Gets the page holder.
        void SetPageHolder(IPageHolderPtr value);
    }; // ThumbnailChangedEventArgs

}}}} // SolidFramework::Model::Pdf::Pages

namespace SolidFramework { namespace Model { namespace Pdf { namespace Pages { 

    // Represents page holder interface.
    class Transaction: public FrameworkObject
    {
    public:
        Transaction(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~Transaction() { Dispose(); }

        virtual void Dispose();

        // Gets the operation.
        OperationPtr GetOperation();

        // Gets the operation.
        void SetOperation(OperationPtr value);

        // Gets the parameter.
        ParameterPtr GetParameter();

        // Gets the parameter.
        void SetParameter(ParameterPtr value);
    }; // Transaction

}}}} // SolidFramework::Model::Pdf::Pages

namespace SolidFramework { namespace Model { namespace Plumbing { 

    // Specifies the line style for the borders of a cell or section. The line style can be set independently for each border.
    class Borders: public FrameworkObject
    {
    public:
        Borders(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~Borders() { Dispose(); }

        virtual void Dispose();

        // Gets the line style for the Left border or null if one is not defined.
        LineStylePtr GetBottom();

        bool IsEqual(BordersPtr value);

        // Gets the line style for the Left border or null if one is not defined.
        LineStylePtr GetLeft();

        // Gets the line style for the Left borderor null if one is not defined.
        LineStylePtr GetRight();

        // Gets the line style for the Left border or null if one is not defined.
        LineStylePtr GetTop();
    }; // Borders

}}} // SolidFramework::Model::Plumbing

namespace SolidFramework { namespace Model { namespace Plumbing { 

    // Represents a text column within a section. This class does not represent a column within a table.
    class Column: public FrameworkObject
    {
    public:
        Column(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~Column() { Dispose(); }

        virtual void Dispose();

        // Gets the spacing of the column in points.
        double GetSpacing();

        // Sets the spacing of the column in points.
        void SetSpacing(double value);

        // Gets the width of the column in points.
        double GetWidth();

        // Sets the width of the column in points.
        void SetWidth(double value);
    }; // Column

}}} // SolidFramework::Model::Plumbing

namespace SolidFramework { namespace Model { namespace Plumbing { 

    // Represents the collection of columns
    class Columns: public FrameworkObject
    {
    public:
        Columns(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~Columns() { Dispose(); }

        virtual void Dispose();

        // Gets the collection of columns.
        std::vector<ColumnPtr> GetColumns();

        // Gets the number of columns.
        int GetCount();

        virtual std::wstring ToString();
    }; // Columns

}}} // SolidFramework::Model::Plumbing

namespace SolidFramework { namespace Model { namespace Plumbing { 

    // Specifies the formatting to be used when filling an object.
    class FillStyle: public FrameworkObject
    {
    protected:
        FillStyle();
    public:
        FillStyle(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~FillStyle() { Dispose(); }

        virtual void Dispose();

        bool IsEqual(FillStylePtr value);

        FillStyleType GetFillType();
    }; // FillStyle

}}} // SolidFramework::Model::Plumbing

namespace SolidFramework { namespace Model { namespace Plumbing { 

    // Represents a gradient fill. This is a fill style that changes from one color to another.
    // NOTE: This is not currently exposed in the SDK.
    class GradientFill: public FillStyle
    {
    public:
        GradientFill(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~GradientFill() { Dispose(); }

        virtual void Dispose();

        // Gets the angle for the gradient in degrees.
        double GetAngle();

        double GetFocus();

        // Gets the "From" color.
        SolidFramework::Interop::ColorPtr GetFrom();

        // Gets the "To" color.
        SolidFramework::Interop::ColorPtr GetTo();

        static GradientFillPtr DynamicCast(FillStylePtr value);
    }; // GradientFill

}}} // SolidFramework::Model::Plumbing

namespace SolidFramework { namespace Model { namespace Plumbing { 

    // Represents a fill containing and image. 
    // NOTE: This is not currently exposed in the SDK.
    class ImageFill: public FillStyle
    {
    public:
        ImageFill(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~ImageFill() { Dispose(); }

        virtual void Dispose();

        // Gets the image.
        SolidFramework::Imaging::ImagePtr GetImage();

        static ImageFillPtr DynamicCast(FillStylePtr value);
    }; // ImageFill

}}} // SolidFramework::Model::Plumbing

namespace SolidFramework { namespace Model { namespace Plumbing { 

    class SolidFill: public FillStyle
    {
    public:
        SolidFill(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~SolidFill() { Dispose(); }

        virtual void Dispose();

        SolidFramework::Interop::ColorPtr GetColor();

        static SolidFillPtr DynamicCast(FillStylePtr value);
    }; // SolidFill

}}} // SolidFramework::Model::Plumbing

namespace SolidFramework { namespace Model { namespace Plumbing { 

    class GraphicPath: public FrameworkObject
    {
    public:
        GraphicPath(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~GraphicPath() { Dispose(); }

        virtual void Dispose();

        // Gets the number of GraphicSegment elements that are used to define the path.
        int GetCount();

        // Gets whether the path defines a closed shape, is so then the endpoint of every line segment is joined to another line segment.
        bool IsClosed();

        // Gets the collection of GraphicSegment elements that are used to define the path.
        std::vector<GraphicSegmentPtr> GetSegments();
    }; // GraphicPath

}}} // SolidFramework::Model::Plumbing

namespace SolidFramework { namespace Model { namespace Plumbing { 

    // Represents a staright or curved line thatmakes up part of a GraphicPath.
    class GraphicSegment: public FrameworkObject
    {
    protected:
        GraphicSegment();
    public:
        GraphicSegment(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~GraphicSegment() { Dispose(); }

        virtual void Dispose();

        // Gets the end location of the line (measured in points).
        SolidFramework::Interop::PointFPtr GetEnd();

        bool IsEqual(GraphicSegmentPtr value);

        // Gets the start location of the line (measured in points).
        SolidFramework::Interop::PointFPtr GetStart();
    }; // GraphicSegment

}}} // SolidFramework::Model::Plumbing

namespace SolidFramework { namespace Model { namespace Plumbing { 

    // A smooth curve that is defined by its start and end points and two control points or handles. A GraphicPath is a combination of linked Bezier or line segments.
    class BezierGraphicSegment: public GraphicSegment
    {
    public:
        BezierGraphicSegment(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~BezierGraphicSegment() { Dispose(); }

        virtual void Dispose();

        // Gets the location of the control point nearest to the start point.
        SolidFramework::Interop::PointFPtr GetEndHandle();

        // Gets the location of the control point nearest to the start point.
        SolidFramework::Interop::PointFPtr GetStartHandle();

        static BezierGraphicSegmentPtr DynamicCast(GraphicSegmentPtr value);
    }; // BezierGraphicSegment

}}} // SolidFramework::Model::Plumbing

namespace SolidFramework { namespace Model { namespace Plumbing { 

    // Represents the formatting for a line.
    class LineStyle: public FrameworkObject
    {
    public:
        LineStyle(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~LineStyle() { Dispose(); }

        virtual void Dispose();

        // Gets the color of the line.
        SolidFramework::Interop::ColorPtr GetColor();

        // Gets the LineCompoundType of the line.
        LineCompoundType GetCompoundType();

        // Gets the DashType of the line.
        LineDashType GetDashType();

        bool IsEqual(LineStylePtr value);

        // Gets the weight (width) of the line in points.
        double GetWeight();
    }; // LineStyle

}}} // SolidFramework::Model::Plumbing

namespace SolidFramework { namespace Model { namespace Plumbing { 

    class Margins: public FrameworkObject
    {
    public:
        Margins(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~Margins() { Dispose(); }

        virtual void Dispose();

        // Gets the bottom margin in points.
        double GetBottom();

        // Gets the left margin in points.
        double GetLeft();

        // Gets the right margin in points.
        double GetRight();

        // Gets the top margin in points.
        double GetTop();

        virtual std::wstring ToString();
    }; // Margins

}}} // SolidFramework::Model::Plumbing

namespace SolidFramework { namespace Model { namespace Plumbing { 

    // Describes the transformation that is to applied to SolidObjects when rendering them.
    class MatrixSO: public FrameworkObject
    {
    public:
        MatrixSO(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~MatrixSO() { Dispose(); }

        virtual void Dispose();

        // Gets the rotation.
        double GetRotation();

        // Gets the skew.
        double GetSkew();

        // Gets the X offset.
        double GetXOffset();

        // Gets the X scale.
        double GetXScale();

        // Gets the Y offset.
        double GetYOffset();

        // Gets the Y scale.
        double GetYScale();
    }; // MatrixSO

}}} // SolidFramework::Model::Plumbing

namespace SolidFramework { namespace Model { namespace Plumbing { 

    // Specifies options for the document.
    class Options: public FrameworkObject
    {
    public:
        Options();
        Options(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~Options() { Dispose(); }

        virtual void Dispose();
    }; // Options

}}} // SolidFramework::Model::Plumbing

namespace SolidFramework { namespace Model { namespace Export { 

    // Specifies options for exporting the document.
    class ExportOptions: public SolidFramework::Model::Plumbing::Options
    {
    public:
        ExportOptions();
        ExportOptions(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~ExportOptions() { Dispose(); }

        virtual void Dispose();

        // Checks the license.
        virtual void CheckLicense();

        // Get or sets whether to overwrite an existing output file.
        SolidFramework::Plumbing::OverwriteMode GetOverwriteMode();

        // Get or sets whether to overwrite an existing output file.
        void SetOverwriteMode(SolidFramework::Plumbing::OverwriteMode value);

        static ExportOptionsPtr DynamicCast(SolidFramework::Model::Plumbing::OptionsPtr value);
    }; // ExportOptions

}}} // SolidFramework::Model::Export

namespace SolidFramework { namespace Model { namespace Export { 

    class ExcelOptions: public ExportOptions
    {
    public:
        ExcelOptions();
        ExcelOptions(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~ExcelOptions() { Dispose(); }

        virtual void Dispose();

        static ExcelOptionsPtr DynamicCast(SolidFramework::Model::Plumbing::OptionsPtr value);
    }; // ExcelOptions

}}} // SolidFramework::Model::Export

namespace SolidFramework { namespace Model { namespace Export { 

    class HtmlOptions: public ExportOptions
    {
    public:
        HtmlOptions();
        HtmlOptions(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~HtmlOptions() { Dispose(); }

        virtual void Dispose();

        virtual void CheckLicense();

        // Get whether to detect lists.
        bool GetDetectLists();

        // Get whether to detect lists.
        void SetDetectLists(bool value);

        // Gets the type of the word document.
        SolidFramework::Converters::Plumbing::HtmlNavigationType GetHtmlNavigationType();

        // Gets the type of the word document.
        void SetHtmlNavigationType(SolidFramework::Converters::Plumbing::HtmlNavigationType value);

        // Get or set the how images are handled during conversion.
        SolidFramework::Converters::Plumbing::HtmlImages GetImages();

        // Get or set the how images are handled during conversion.
        void SetImages(SolidFramework::Converters::Plumbing::HtmlImages value);

        // Get the format of the image you are converting i.e bmp for Bitmap etc.
        SolidFramework::Converters::Plumbing::ImageDocumentType GetImageType();

        // Get the format of the image you are converting i.e bmp for Bitmap etc.
        void SetImageType(SolidFramework::Converters::Plumbing::ImageDocumentType value);

        // Get maximum width for HTML file images.
        int GetWidthLimit();

        // Get maximum width for HTML file images.
        void SetWidthLimit(int value);

        static HtmlOptionsPtr DynamicCast(SolidFramework::Model::Plumbing::OptionsPtr value);
    }; // HtmlOptions

}}} // SolidFramework::Model::Export

namespace SolidFramework { namespace Model { namespace Export { 

    class PowerPointOptions: public ExportOptions
    {
    public:
        PowerPointOptions();
        PowerPointOptions(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PowerPointOptions() { Dispose(); }

        virtual void Dispose();

        static PowerPointOptionsPtr DynamicCast(SolidFramework::Model::Plumbing::OptionsPtr value);
    }; // PowerPointOptions

}}} // SolidFramework::Model::Export

namespace SolidFramework { namespace Model { namespace Export { 

    class TxtOptions: public ExportOptions
    {
    public:
        TxtOptions();
        TxtOptions(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~TxtOptions() { Dispose(); }

        virtual void Dispose();

        virtual void CheckLicense();

        // Get the character set encoding used for the output text document.
        int GetEncoding();

        // Get the character set encoding used for the output text document.
        void SetEncoding(int value);

        // Get the maximum number of characters to display in each line of your text document.  Example: textConverter.LineLength = 80;.
        int GetLineLength();

        // Get the maximum number of characters to display in each line of your text document.  Example: textConverter.LineLength = 80;.
        void SetLineLength(int value);

        // Get the line terminator.
        SolidFramework::Converters::Plumbing::LineTerminator GetLineTerminator();

        // Get the line terminator.
        void SetLineTerminator(SolidFramework::Converters::Plumbing::LineTerminator value);

        static TxtOptionsPtr DynamicCast(SolidFramework::Model::Plumbing::OptionsPtr value);
    }; // TxtOptions

}}} // SolidFramework::Model::Export

namespace SolidFramework { namespace Model { namespace Export { 

    class WordOptions: public ExportOptions
    {
    public:
        WordOptions();
        WordOptions(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~WordOptions() { Dispose(); }

        virtual void Dispose();

        // Get a TargetWordFormat that specifies Target MS Word File Format Version. Default is TargetWordFormat.Automatic.
        SolidFramework::Converters::Plumbing::TargetWordFormat GetTargetWordFormat();

        // Get a TargetWordFormat that specifies Target MS Word File Format Version. Default is TargetWordFormat.Automatic.
        void SetTargetWordFormat(SolidFramework::Converters::Plumbing::TargetWordFormat value);

        // Gets the type of the word document.
        SolidFramework::Converters::Plumbing::WordDocumentType GetWordDocumentType();

        // Gets the type of the word document.
        void SetWordDocumentType(SolidFramework::Converters::Plumbing::WordDocumentType value);

        static WordOptionsPtr DynamicCast(SolidFramework::Model::Plumbing::OptionsPtr value);
    }; // WordOptions

}}} // SolidFramework::Model::Export

namespace SolidFramework { namespace Model { 

    // Specifies values to be used when importing a Pdf file.
    class PdfOptions: public Plumbing::Options
    {
    public:
        PdfOptions();
        PdfOptions(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfOptions() { Dispose(); }

        virtual void Dispose();

        // Gets whether to attempt to automatically detect decimal and thousands separators, based on the content of the PDF being converted. Default is true.
        bool GetAutoDetectSeparators();

        // Sets whether to attempt to automatically detect decimal and thousands separators, based on the content of the PDF being converted. Default is true.
        void SetAutoDetectSeparators(bool value);

        // Gets whether to automatically rotate pages based on the orientation of the text on the pages. Default is true.
        bool GetAutoRotate();

        // Sets whether to automatically rotate pages based on the orientation of the text on the pages. Default is true.
        void SetAutoRotate(bool value);

        // Get or sets whether the spacing between characters should be set as to their average size. 
        // This is required because fonts in PDF and docx are different and therefore the same Unicode characters 
        // have different character widths in PDF and docx. This value must be set to true if the generated docx file 
        // is to look the same as the PDF file. Setting the value to true will also result in the creation of fewer, but 
        // larger "Run" objects.
        // The default is true.
        bool GetAverageCharacterScaling();

        // Get or sets whether the spacing between characters should be set as to their average size. 
        // This is required because fonts in PDF and docx are different and therefore the same Unicode characters 
        // have different character widths in PDF and docx. This value must be set to true if the generated docx file 
        // is to look the same as the PDF file. Setting the value to true will also result in the creation of fewer, but 
        // larger "Run" objects.
        // The default is true.
        void SetAverageCharacterScaling(bool value);

        // Get or sets a ConvertMode that specifies whether the document, or just images, or just tables should be converted . Default is ConvertMode.Document.
        SolidFramework::Converters::Plumbing::ConvertMode GetConvertMode();

        // Get or sets a ConvertMode that specifies whether the document, or just images, or just tables should be converted . Default is ConvertMode.Document.
        void SetConvertMode(SolidFramework::Converters::Plumbing::ConvertMode value);

        // Gets the decimal separator.
        SolidFramework::Converters::Plumbing::DecimalSeparator GetDecimalSeparator();

        // Gets the decimal separator.
        void SetDecimalSeparator(SolidFramework::Converters::Plumbing::DecimalSeparator value);

        // Gets whether the document language should be detected from the document content. The default is true.
        bool GetDetectLanguage();

        // Sets whether the document language should be detected from the document content. The default is true.
        void SetDetectLanguage(bool value);

        // Gets whether lists should be detected from the document content. The default is true.
        bool GetDetectLists();

        // Sets whether lists should be detected from the document content. The default is true.
        void SetDetectLists(bool value);

        // Gets whether soft-hyphens should be detected. The default is false.
        bool GetDetectSoftHyphens();

        // Sets whether soft-hyphens should be detected. The default is false.
        void SetDetectSoftHyphens(bool value);

        bool GetDetectTables();

        void SetDetectTables(bool value);

        bool GetDetectTaggedTables();

        void SetDetectTaggedTables(bool value);

        // Attempt to automatically detect tables tiled across multiple pages.
        bool GetDetectTiledPages();

        // Attempt to automatically detect tables tiled across multiple pages.
        void SetDetectTiledPages(bool value);

        // Gets whether Table of Contents should be detected from the document content. The default is true.
        bool GetDetectToc();

        // Sets whether Table of Contents should be detected from the document content. The default is true.
        void SetDetectToc(bool value);

        // Get or sets a ExcelTablesOnSheet that specifies how tables should be handled when exporting to Excel. Default is ExcelTablesOnSheet.PlaceEachTableOnOwnSheet.
        SolidFramework::Converters::Plumbing::ExcelTablesOnSheet GetExcelTablesOnSheet();

        // Get or sets a ExcelTablesOnSheet that specifies how tables should be handled when exporting to Excel. Default is ExcelTablesOnSheet.PlaceEachTableOnOwnSheet.
        void SetExcelTablesOnSheet(SolidFramework::Converters::Plumbing::ExcelTablesOnSheet value);

        // Gets a value indicating whether source document pagination should be exposed. The default is false.
        bool GetExposeSourceDocumentPagination();

        // Sets a value indicating whether source document pagination should be exposed. The default is false.
        void SetExposeSourceDocumentPagination(bool value);

        // Gets a value indicating whether document pagination should be exposed. The default is false.
        bool GetExposeTargetDocumentPagination();

        // Sets a value indicating whether document pagination should be exposed. The default is false.
        void SetExposeTargetDocumentPagination(bool value);

        // Get or sets a FootnotesMode that specifies the footnotes mode. Default is FootnotesMode.Ignore.
        SolidFramework::Converters::Plumbing::FootnotesMode GetFootnotesMode();

        // Get or sets a FootnotesMode that specifies the footnotes mode. Default is FootnotesMode.Ignore.
        void SetFootnotesMode(SolidFramework::Converters::Plumbing::FootnotesMode value);

        // Gets whether vector images should be converted to bitmap images. Default is false.
        bool GetGraphicsAsImages();

        // Sets whether vector images should be converted to bitmap images. Default is false.
        void SetGraphicsAsImages(bool value);

        // Get or sets a HeaderAndFooterMode that specifies the header and footer mode. Default is HeaderAndFooterMode.Detect.
        SolidFramework::Converters::Plumbing::HeaderAndFooterMode GetHeaderAndFooterMode();

        // Get or sets a HeaderAndFooterMode that specifies the header and footer mode. Default is HeaderAndFooterMode.Detect.
        void SetHeaderAndFooterMode(SolidFramework::Converters::Plumbing::HeaderAndFooterMode value);

        // Gets whether the background color of text should be retained. The default is false.
        bool GetKeepBackgroundColorText();

        // Sets whether the background color of text should be retained. The default is false.
        void SetKeepBackgroundColorText(bool value);

        // Gets whether invisible text should be kept. The default is false.
        bool GetKeepInvisibleText();

        // Sets whether invisible text should be kept. The default is false.
        void SetKeepInvisibleText(bool value);

        // Gets whether line breaks should be preserved in the content. The default is false.
        bool GetKeepLineBreaks();

        // Sets whether line breaks should be preserved in the content. The default is false.
        void SetKeepLineBreaks(bool value);

        // Gets whether to include non-table content such as images or text when using ConvertMode.Tables.
        bool GetKeepNonTableContent();

        // Sets whether to include non-table content such as images or text when using ConvertMode.Tables.
        void SetKeepNonTableContent(bool value);

        // Gets a MarkupAnnotConversionType that specifies the markup annotation mode. Default is MarkupAnnotConversionType.TextBox.
        SolidFramework::Converters::Plumbing::MarkupAnnotConversionType GetMarkupAnnotConversionType();

        // Sets a MarkupAnnotConversionType that specifies the markup annotation mode. Default is MarkupAnnotConversionType.TextBox.
        void SetMarkupAnnotConversionType(SolidFramework::Converters::Plumbing::MarkupAnnotConversionType value);

        bool GetMergeParagraphIndents();

        void SetMergeParagraphIndents(bool value);

        // Gets a PageRange that defines which pages are to be converted. Null means convert all pages.
        SolidFramework::PageRangePtr GetPageRange();

        // Sets a PageRange that defines which pages are to be converted. Null means convert all pages.
        void SetPageRange(SolidFramework::PageRangePtr value);

        // Gets the password for the Pdf file.
        std::wstring GetPassword();

        // Sets the password for the Pdf file.
        void SetPassword(std::wstring value);

        // Gets whether to place non-table content that is detected within columns in the PDF into separate columns when using ConvertMode.Tables. 
        // If true then non-table content that is detected will be placed into separate columns. If false then such content will all be placed into the first column.
        // If KeepNonTableContent is false then no non-table content will be included in the reconstructed file and this option will have no meaning.
        bool GetPreserveColumnsInNonTableContent();

        // Sets whether to place non-table content that is detected within columns in the PDF into separate columns when using ConvertMode.Tables. 
        // If true then non-table content that is detected will be placed into separate columns. If false then such content will all be placed into the first column.
        // If KeepNonTableContent is false then no non-table content will be included in the reconstructed file and this option will have no meaning.
        void SetPreserveColumnsInNonTableContent(bool value);

        // Get or sets a ReconstructionMode that specifies the file reconstruction mode. The default is ReconstructionMode.Flowing.
        SolidFramework::Converters::Plumbing::ReconstructionMode GetReconstructionMode();

        // Get or sets a ReconstructionMode that specifies the file reconstruction mode. The default is ReconstructionMode.Flowing.
        void SetReconstructionMode(SolidFramework::Converters::Plumbing::ReconstructionMode value);

        // Gets the selected areas. The default is null.
        SolidFramework::Converters::Plumbing::SelectedAreasPtr GetSelectedAreas();

        // Sets the selected areas. The default is null.
        void SetSelectedAreas(SolidFramework::Converters::Plumbing::SelectedAreasPtr value);

        // Deprecated alias for KeepNonTableContent.
        bool GetTablesFromContent();

        // Deprecated alias for KeepNonTableContent.
        void SetTablesFromContent(bool value);

        // Get or sets a TextRecovery that specifies when OCR is used. Default is TextRecovery.Automatic.
        // Warning: setting this value to TextRecovery.Always will casue OCR to occur even on non-scanned Pdf files.
        SolidFramework::Converters::Plumbing::TextRecovery GetTextRecovery();

        // Get or sets a TextRecovery that specifies when OCR is used. Default is TextRecovery.Automatic.
        // Warning: setting this value to TextRecovery.Always will casue OCR to occur even on non-scanned Pdf files.
        void SetTextRecovery(SolidFramework::Converters::Plumbing::TextRecovery value);

        // Get or sets a TextRecoveryEngine that specifies the text recovery engine to be used for OCR. Default is TextRecoveryEngine.Automatic.
        SolidFramework::Converters::Plumbing::TextRecoveryEngine GetTextRecoveryEngine();

        // Get or sets a TextRecoveryEngine that specifies the text recovery engine to be used for OCR. Default is TextRecoveryEngine.Automatic.
        void SetTextRecoveryEngine(SolidFramework::Converters::Plumbing::TextRecoveryEngine value);

        // Get or sets a TextRecoveryEngineNse that specifies the text recovery engine to be used for OCR when dealing with non-standard encoding. 
        // Default is TextRecoveryEngineNse.Automatic.
        SolidFramework::Converters::Plumbing::TextRecoveryEngineNse GetTextRecoveryEngineNse();

        // Get or sets a TextRecoveryEngineNse that specifies the text recovery engine to be used for OCR when dealing with non-standard encoding. 
        // Default is TextRecoveryEngineNse.Automatic.
        void SetTextRecoveryEngineNse(SolidFramework::Converters::Plumbing::TextRecoveryEngineNse value);

        // Gets the text recovery language. Default is an empty string.
        std::wstring GetTextRecoveryLanguage();

        // Sets the text recovery language. Default is an empty string.
        void SetTextRecoveryLanguage(std::wstring value);

        // Get or sets a TextRecoveryNSE that specifies whether OCR is used for characters with non-standard encoding. Default is TextRecoveryNSE.Automatic.
        SolidFramework::Converters::Plumbing::TextRecoveryNSE GetTextRecoveryNSE();

        // Get or sets a TextRecoveryNSE that specifies whether OCR is used for characters with non-standard encoding. Default is TextRecoveryNSE.Automatic.
        void SetTextRecoveryNSE(SolidFramework::Converters::Plumbing::TextRecoveryNSE value);

        // Gets the thousands separator.
        SolidFramework::Converters::Plumbing::ThousandsSeparator GetThousandsSeparator();

        // Gets the thousands separator.
        void SetThousandsSeparator(SolidFramework::Converters::Plumbing::ThousandsSeparator value);

        // Gets a value indicating whether to transform characters in Unicode Private Use area to true Unicode equivalents.
        // Default is false.
        bool GetTransformPrivateUnicode();

        // Sets a value indicating whether to transform characters in Unicode Private Use area to true Unicode equivalents.
        // Default is false.
        void SetTransformPrivateUnicode(bool value);

        static PdfOptionsPtr DynamicCast(Plumbing::OptionsPtr value);
        
        // Event callbacks
        std::function<void(SolidFramework::ProgressEventArgsPtr)> OnProgress;
        std::function<void(SolidFramework::WarningEventArgsPtr)> OnWarning;
        
        // Overridable event handlers
        virtual void FireProgress(SolidFramework::ProgressEventArgsPtr args);
        virtual void FireWarning(SolidFramework::WarningEventArgsPtr args);
    }; // PdfOptions

}} // SolidFramework::Model

namespace SolidFramework { namespace Model { namespace Plumbing { 

    // Represents a row within a table.
    class Row: public FrameworkObject
    {
    public:
        Row(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~Row() { Dispose(); }

        virtual void Dispose();

        // Calculates a row height suitable for exporting to Excel.
        double CalculateExcelHeight();

        // Calculates a row height suitable for exporting to Word.
        double CalculateWordHeight();

        // Gets the collection of cells in the row.
        std::vector<CellPtr> GetCells();

        // Gets the number of cells in the row.
        int GetCount();

        // Gets the height of the row in points.
        double GetHeight();

        // Gets the height rule for this row.
        RowHeightRule GetHeightRule();
    }; // Row

}}} // SolidFramework::Model::Plumbing

namespace SolidFramework { namespace Model { namespace Plumbing { 

    // Represents a block of text that has the same formatting
    class Run: public FrameworkObject
    {
    public:
        Run(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~Run() { Dispose(); }

        virtual void Dispose();

        // Gets the Bookmark that ends on this run.
        BookmarkPtr GetBookmarkEnd();

        // Gets the Bookmark that starts on this run.
        BookmarkPtr GetBookmarkStart();

        // Gets the hyperlink attached to the Run. This may be null.
        HyperlinkPtr GetHyperlink();

        // Gets the inline shape. This may be null.
        ShapePtr GetInlineShape();

        // Gets whether the run represents a page number field.
        bool IsPageNumberField();

        // Gets the TextStyle for this Run.
        TextStylePtr GetStyle();

        // Gets the text.
        std::wstring GetText();
    }; // Run

}}} // SolidFramework::Model::Plumbing

namespace SolidFramework { namespace Model { namespace Plumbing { 

    class SolidObject: public FrameworkObject
    {
    protected:
        SolidObject();
    public:
        SolidObject(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~SolidObject() { Dispose(); }

        virtual void Dispose();

        int GetID();

        SolidObjectType GetObjectType();

        virtual std::wstring ToString();
    }; // SolidObject

}}} // SolidFramework::Model::Plumbing

namespace SolidFramework { namespace Model { namespace Plumbing { 
    class ISolidObjectContainer
    {
    public:
        virtual std::vector<SolidObjectPtr> GetSolidObjects() = 0;
    };

}}} // SolidFramework::Model::Plumbing

namespace SolidFramework { namespace Model { namespace Plumbing { 

    // A bookmark represents a named contiguous area in a document, with both a starting position and an ending position. 
    // You can use bookmarks to mark a location in a document, or as a container for text in a document. 
    // It can be as small as just the insertion point, or it can be as large as the entire document.
    // NOTE: A comment is considered to be a types of bookmark.
    class Bookmark: public SolidObject
    {
    public:
        Bookmark(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~Bookmark() { Dispose(); }

        virtual void Dispose();

        BookmarkPtr GetChild(int index);

        // Gets number of child bookmarks.
        int GetCount();

        // Gets whether the bookmark is hidden.
        bool IsHidden();

        // Gets the name of the bookmark.
        std::wstring GetName();

        // Gets the page number where the bookmark is located.
        int GetPageNumber();

        // Gets the type of bookmark.
        BookmarkType GetType();

        static BookmarkPtr DynamicCast(SolidObjectPtr value);
    }; // Bookmark

}}} // SolidFramework::Model::Plumbing

namespace SolidFramework { namespace Model { namespace Plumbing { 

    // Describes a Font object used within a document. Includes the name, and whether the font is bold and/or italic
    class Font: public SolidObject
    {
    public:
        Font(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~Font() { Dispose(); }

        virtual void Dispose();

        // Gets whether the font is bold.
        bool GetBold();

        // Gets whether the font is an East Asian font
        bool GetEastAsiaFont();

        double GetGlyphWidth(int cid);

        // Gets whether the font is italicised.
        bool GetItalic();

        // Gets the name of the font.
        std::wstring GetName();

        std::wstring TransformPrivateToUnicode(std::wstring text);

        static FontPtr DynamicCast(SolidObjectPtr value);
    }; // Font

}}} // SolidFramework::Model::Plumbing

namespace SolidFramework { namespace Model { namespace Plumbing { 

    class Hyperlink: public SolidObject
    {
    protected:
        Hyperlink();
    public:
        Hyperlink(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~Hyperlink() { Dispose(); }

        virtual void Dispose();

        static int Compare(HyperlinkPtr first, HyperlinkPtr second);

        static HyperlinkPtr DynamicCast(SolidObjectPtr value);
    }; // Hyperlink

}}} // SolidFramework::Model::Plumbing

namespace SolidFramework { namespace Model { namespace Plumbing { 

    // Represents a hyperlink to an external location.
    class ExternalHyperlink: public Hyperlink
    {
    public:
        ExternalHyperlink(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~ExternalHyperlink() { Dispose(); }

        virtual void Dispose();

        // Gets the destination of the hyperlink.
        std::wstring GetDestination();

        // Gets the URL for the hyperlink.
        SolidFramework::Interop::UriPtr GetUrl();

        static ExternalHyperlinkPtr DynamicCast(SolidObjectPtr value);
    }; // ExternalHyperlink

}}} // SolidFramework::Model::Plumbing

namespace SolidFramework { namespace Model { namespace Plumbing { 

    // Represents a hyperlink within the document, such as bookmarks, comments and references.
    class InternalHyperlink: public Hyperlink
    {
    public:
        InternalHyperlink(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~InternalHyperlink() { Dispose(); }

        virtual void Dispose();

        // Gets the bookmark associated with the hyperlink.
        BookmarkPtr GetBookmark();

        static InternalHyperlinkPtr DynamicCast(SolidObjectPtr value);
    }; // InternalHyperlink

}}} // SolidFramework::Model::Plumbing

namespace SolidFramework { namespace Model { namespace Plumbing { 

    // Represents a numbered or bullet-point list within a document.
    class List: public SolidObject
    {
    public:
        List(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~List() { Dispose(); }

        virtual void Dispose();

        // Gets the collection of paragraphs within the list.
        std::vector<ParagraphPtr> GetAttachedParagraphs();

        // Gets the numeric value associated with a specific paragraph within a list. This function supports lists starting at arbitrary values.
        int CalculateNumericValue(ParagraphPtr paragraph);

        // Gets the number of paragraphs within the list.
        int GetCount();

        // Gets the text associated with a specific paragraph within a list. This function supports formatting stle, including nested list levels, e.g. 1.3.a.
        std::wstring GenerateLevelText(ParagraphPtr paragraph);

        ListLevelsPtr GetListLevels();

        // Gets the topic associated with the list.
        SolidFramework::Model::TopicPtr GetTopic();

        static ListPtr DynamicCast(SolidObjectPtr value);
    }; // List

}}} // SolidFramework::Model::Plumbing

namespace SolidFramework { namespace Model { namespace Plumbing { 

    class Paragraph: public SolidObject
    {
    public:
        Paragraph(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~Paragraph() { Dispose(); }

        virtual void Dispose();

        // Gets the List that is associated with the paragraph. This will be null if the paragraph is not part of a list.
        ListPtr GetAttachedList();

        int GetAttachedListLevel();

        // Gets the number of Run objects that are in the Paragraph.
        int GetCount();

        // Gets the collection of Run objects that are in the Paragraph.
        std::vector<RunPtr> GetRuns();

        // Gets the ParagraphStyle for this Paragraph.
        ParagraphStylePtr GetStyle();

        std::vector<TabStopPtr> GetTabStops();

        static ParagraphPtr DynamicCast(SolidObjectPtr value);
    }; // Paragraph

}}} // SolidFramework::Model::Plumbing

namespace SolidFramework { namespace Model { namespace Plumbing { 

    // Represents an object in the drawing layer, such as an AutoShape, 
    // freeform, OLE object, ActiveX control, or picture.
    class Shape: public SolidObject
    {
    protected:
        Shape();
    public:
        Shape(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~Shape() { Dispose(); }

        virtual void Dispose();

        std::wstring GetAltText();

        // Gets the FillStyle for this Shape.
        FillStylePtr GetFill();

        // Gets the height of the shape in points.
        double GetHeight();

        // Gets the horizontal alignment of the shape. Default is Left.
        HorizontalAlignment GetHorizontalAlignment();

        // Gets the horizontal anchoring for the shape. The default is Top.
        HorizontalAnchoring GetHorizontalAnchoring();

        // Gets the LineStyle for this Shape.
        LineStylePtr GetLine();

        // Gets the transformation matrix that is to be appleid to the shape.
        MatrixSOPtr GetTransform();

        // Gets the vertical alignment of the shape. Default is Top.
        VerticalAlignment GetVerticalAlignment();

        // Gets the vertical anchoring for the shape. The default is Margin.
        VerticalAnchoring GetVerticalAnchoring();

        // Gets the width of the shape in points.
        double GetWidth();

        // Gets the properties for wrapping text around the specified shape. Default is SquareBoth.
        WrappingType GetWrappingType();

        static ShapePtr DynamicCast(SolidObjectPtr value);
    }; // Shape

}}} // SolidFramework::Model::Plumbing

namespace SolidFramework { namespace Model { namespace Plumbing { 

    // Represents a Graphic in the document. A Graphic consists of a collection of GraphicPaths, each of which is made up of GraphicSegment objects.
    class Graphic: public Shape
    {
    public:
        Graphic(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~Graphic() { Dispose(); }

        virtual void Dispose();

        // Gets the number of GraphicPath elements that are used to define the shape.
        int GetCount();

        // Gets the collection of GraphicPath elements that are used to define the shape.
        std::vector<GraphicPathPtr> GetPaths();

        static GraphicPtr DynamicCast(SolidObjectPtr value);
    }; // Graphic

}}} // SolidFramework::Model::Plumbing

namespace SolidFramework { namespace Model { namespace Plumbing { 

    // Represents a group of shapes within the document
    class Group: public Shape, public ISolidObjectContainer
    {
    public:
        Group(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~Group() { Dispose(); }

        virtual void Dispose();

        // Gets the number of SolidObject within the group.
        int GetCount();

        // Gets the collection of SolidObject within the group.
        virtual std::vector<SolidObjectPtr> GetSolidObjects();

        static GroupPtr DynamicCast(SolidObjectPtr value);
    }; // Group

}}} // SolidFramework::Model::Plumbing

namespace SolidFramework { namespace Model { namespace Plumbing { 

    // Represents an image within a document.
    class ImageShape: public Shape
    {
    public:
        ImageShape(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~ImageShape() { Dispose(); }

        virtual void Dispose();

        // Gets the image from the ImageShape.
        SolidFramework::Imaging::ImagePtr GetImage();

        static ImageShapePtr DynamicCast(SolidObjectPtr value);
    }; // ImageShape

}}} // SolidFramework::Model::Plumbing

namespace SolidFramework { namespace Model { namespace Plumbing { 

    class Table: public Shape
    {
    public:
        Table(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~Table() { Dispose(); }

        virtual void Dispose();

        // Gets the cell at the specified row index and column index . Note that rows and columns are zero indexed, so that the "first" cell is at "0,0".
        CellPtr GetCell(int ri, int ci);

        // Gets the space between cells.
        double GetCellSpacing();

        int GetRowCount();

        std::vector<RowPtr> GetRows();

        static TablePtr DynamicCast(SolidObjectPtr value);
    }; // Table

}}} // SolidFramework::Model::Plumbing

namespace SolidFramework { namespace Model { namespace Plumbing { 

    class TextBox: public Shape, public ISolidObjectContainer
    {
    public:
        TextBox(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~TextBox() { Dispose(); }

        virtual void Dispose();

        // Gets the number of SolidObject within the textbox.
        int GetCount();

        // Gets a value indicating whether this instance is annotation.
        bool IsAnnotation();

        // Gets the margins.
        MarginsPtr GetMargins();

        // Gets the rotation in degrees.
        int GetRotation();

        // Gets the collection of SolidObject within the textbox.
        virtual std::vector<SolidObjectPtr> GetSolidObjects();

        // Gets the text direction.
        TextDirection GetTextDirection();

        static TextBoxPtr DynamicCast(SolidObjectPtr value);
    }; // TextBox

}}} // SolidFramework::Model::Plumbing

namespace SolidFramework { namespace Model { namespace Plumbing { 

    class SolidCollection: public SolidObject, public ISolidObjectContainer
    {
    protected:
        SolidCollection();
    public:
        SolidCollection(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~SolidCollection() { Dispose(); }

        virtual void Dispose();

        int GetCount();

        virtual std::vector<SolidObjectPtr> GetSolidObjects();

        static SolidCollectionPtr DynamicCast(SolidObjectPtr value);
    }; // SolidCollection

}}} // SolidFramework::Model::Plumbing

namespace SolidFramework { namespace Model { 

    class BookmarksCollection: public Plumbing::SolidCollection
    {
    public:
        BookmarksCollection(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~BookmarksCollection() { Dispose(); }

        virtual void Dispose();

        static BookmarksCollectionPtr DynamicCast(Plumbing::SolidObjectPtr value);
    }; // BookmarksCollection

}} // SolidFramework::Model

namespace SolidFramework { namespace Model { 

    class FontsCollection: public Plumbing::SolidCollection
    {
    public:
        FontsCollection(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~FontsCollection() { Dispose(); }

        virtual void Dispose();

        static FontsCollectionPtr DynamicCast(Plumbing::SolidObjectPtr value);
    }; // FontsCollection

}} // SolidFramework::Model

namespace SolidFramework { namespace Model { 

    class HyperlinksCollection: public Plumbing::SolidCollection
    {
    public:
        HyperlinksCollection(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~HyperlinksCollection() { Dispose(); }

        virtual void Dispose();

        static HyperlinksCollectionPtr DynamicCast(Plumbing::SolidObjectPtr value);
    }; // HyperlinksCollection

}} // SolidFramework::Model

namespace SolidFramework { namespace Model { 

    // Provides access to the individual items that are available in the document.
    class ListsCollection: public Plumbing::SolidCollection
    {
    public:
        ListsCollection(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~ListsCollection() { Dispose(); }

        virtual void Dispose();

        static ListsCollectionPtr DynamicCast(Plumbing::SolidObjectPtr value);
    }; // ListsCollection

}} // SolidFramework::Model

namespace SolidFramework { namespace Model { namespace Plumbing { 

    // Represents a cell within a table.
    class Cell: public SolidCollection
    {
    public:
        Cell(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~Cell() { Dispose(); }

        virtual void Dispose();

        // Gets the borders for the cell.
        BordersPtr GetBorders();

        CellVerticalAlignment GetCellVerticalAlignment();

        void SetCellVerticalAlignment(CellVerticalAlignment value);

        // Gets the number of columns that the cell spans. If the value is greater than 1, then the cell has been merged.
        int GetColSpan();

        // Gets the fill color for the cell.
        SolidFramework::Interop::ColorPtr GetFillColor();

        // Gets the format string to be used for the cell.
        std::wstring GetFormat();

        bool HasFillColor();

        // Gets the first non-null hyperlink in the cell
        HyperlinkPtr GetHyperlink();

        // Gets the number of rows that the cell spans. If the value is greater than 1, then the cell has been merged.
        int GetRowSpan();

        // Gets a formatted string representing the value stored in the cell.
        std::wstring GetText();

        // Gets the text direction.
        TextDirection GetTextDirection();

        // Gets the type of text (e.g. Decimal, Currency, Text) used within the cell. Default is Undetermined.
        TextType GetTextType();

        // Sets the type of text (e.g. Decimal, Currency, Text) used within the cell. Default is Undetermined.
        void SetTextType(TextType value);

        // Gets the value stored in the cell.
        std::wstring GetValue();

        // Gets the width of the cell in points.
        double GetWidth();

        static CellPtr DynamicCast(SolidObjectPtr value);
    }; // Cell

}}} // SolidFramework::Model::Plumbing

namespace SolidFramework { namespace Model { namespace Plumbing { 

    // Represents a header or footer within the document.
    class HeaderFooter: public SolidCollection
    {
    public:
        HeaderFooter(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~HeaderFooter() { Dispose(); }

        virtual void Dispose();

        // Gets whether the object is a header.
        bool IsHeader();

        double GetOffset();

        static HeaderFooterPtr DynamicCast(SolidObjectPtr value);
    }; // HeaderFooter

}}} // SolidFramework::Model::Plumbing

namespace SolidFramework { namespace Model { namespace Plumbing { 

    class HeaderFooterBlockCollection: public SolidCollection
    {
    public:
        HeaderFooterBlockCollection(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~HeaderFooterBlockCollection() { Dispose(); }

        virtual void Dispose();

        PageNumberFormat GetPageNumberFormat();

        int GetPageNumberStartValue();

        static HeaderFooterBlockCollectionPtr DynamicCast(SolidObjectPtr value);
    }; // HeaderFooterBlockCollection

}}} // SolidFramework::Model::Plumbing

namespace SolidFramework { namespace Model { namespace Plumbing { 

    // A section is a part of a document that contains its own page formatting. 
    // It can be a single page or a range of pages, or a section can comprise the entire document.
    // Sections affect only page formatting. They are entirely unrelated to text and paragraph formatting.
    // Sections are also used to define an area of the document that contains text columns. 
    // If you wish to change the number of columns then a new section is required.
    class Section: public SolidCollection
    {
    public:
        Section(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~Section() { Dispose(); }

        virtual void Dispose();

        // Gets the columns used in the section.
        ColumnsPtr GetColumns();

        // Gets whether the document has separate headers and footers for odd and even pages or not
        bool HasDifferentHeaderFooterForEvenPages();

        // Gets whether or not a color has been specified for the background in this section.
        bool HasPageBackgroundColor();

        // Gets whether this section should connect with the section on the previous page
        bool IsSoftPageBreak();

        // Gets the margins.
        MarginsPtr GetMargins();

        // Gets the page number that this section originally came from.
        int GetOriginalPageNumber();

        // Gets the color used for the page background in this section.
        SolidFramework::Interop::ColorPtr GetPageBackgroundColor();

        // Gets the page border or null if it doesn't have one.
        LineStylePtr GetPageBorder();

        // Gets the margins between the edge of the page and the page border.
        MarginsPtr GetPageBorderMargins();

        // Gets the height of the page in points.
        double GetPageHeight();

        // Gets the width of the page in points.
        double GetPageWidth();

        // Gets the type of section break that terminates this section.
        SectionBreak GetSectionBreak();

        // Gets the type of slide layout for a PowerPoint presentation.
        SlideLayout GetSlideLayout();

        // Gets the text direction.
        TextDirection GetTextDirection();

        static SectionPtr DynamicCast(SolidObjectPtr value);
    }; // Section

}}} // SolidFramework::Model::Plumbing

namespace SolidFramework { namespace Model { namespace Plumbing { 

    // Represents a Table of Contents within a document.
    class TOC: public SolidCollection
    {
    public:
        TOC(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~TOC() { Dispose(); }

        virtual void Dispose();

        static TOCPtr DynamicCast(SolidObjectPtr value);
    }; // TOC

}}} // SolidFramework::Model::Plumbing

namespace SolidFramework { namespace Model { 

    class StyleTemplatesCollection: public Plumbing::SolidCollection
    {
    public:
        StyleTemplatesCollection(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~StyleTemplatesCollection() { Dispose(); }

        virtual void Dispose();

        static StyleTemplatesCollectionPtr DynamicCast(Plumbing::SolidObjectPtr value);
    }; // StyleTemplatesCollection

}} // SolidFramework::Model

namespace SolidFramework { namespace Model { 

    class Topic: public Plumbing::SolidCollection
    {
    public:
        Topic(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~Topic() { Dispose(); }

        virtual void Dispose();

        static TopicPtr DynamicCast(Plumbing::SolidObjectPtr value);
    }; // Topic

}} // SolidFramework::Model

namespace SolidFramework { namespace Model { namespace Plumbing { 

    class StyleTemplate: public SolidObject
    {
    protected:
        StyleTemplate();
    public:
        StyleTemplate(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~StyleTemplate() { Dispose(); }

        virtual void Dispose();

        std::wstring GetName();

        StyleTemplatePtr GetStyleBasedOn();

        static StyleTemplatePtr DynamicCast(SolidObjectPtr value);
    }; // StyleTemplate

}}} // SolidFramework::Model::Plumbing

namespace SolidFramework { namespace Model { namespace Plumbing { 

    // Represents style information for a numbered or bullet-point list.
    class ListLevels: public StyleTemplate
    {
    public:
        ListLevels(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~ListLevels() { Dispose(); }

        virtual void Dispose();

        int GetCount();

        // Gets the ListStyle for the list.
        ListStylePtr GetListStyle(int level);

        std::vector<ListStylePtr> GetListStyles();

        static ListLevelsPtr DynamicCast(SolidObjectPtr value);
    }; // ListLevels

}}} // SolidFramework::Model::Plumbing

namespace SolidFramework { namespace Model { namespace Plumbing { 

    class TextStyleTemplate: public StyleTemplate
    {
    protected:
        TextStyleTemplate();
    public:
        TextStyleTemplate(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~TextStyleTemplate() { Dispose(); }

        virtual void Dispose();

        // Gets the text style.
        TextStylePtr GetStyle();

        static TextStyleTemplatePtr DynamicCast(SolidObjectPtr value);
    }; // TextStyleTemplate

}}} // SolidFramework::Model::Plumbing

namespace SolidFramework { namespace Model { namespace Plumbing { 

    class ParagraphStyleTemplate: public TextStyleTemplate
    {
    public:
        ParagraphStyleTemplate(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~ParagraphStyleTemplate() { Dispose(); }

        virtual void Dispose();

        double GetAfterSpacing();

        Alignment GetAlignment();

        double GetBeforeSpacing();

        double GetFirstLineIndent();

        double GetLeftIndent();

        double GetLineSpacing();

        LineSpacingType GetLineSpacingType();

        int GetOutlineLevel();

        double GetRightIndent();

        bool GetRightToLeftWriting();

        // Gets the paragraph style.
        ParagraphStylePtr GetStyle();

        static ParagraphStyleTemplatePtr DynamicCast(SolidObjectPtr value);
    }; // ParagraphStyleTemplate

}}} // SolidFramework::Model::Plumbing

namespace SolidFramework { namespace Model { namespace Plumbing { 

    class Style: public FrameworkObject
    {
    protected:
        Style();
    public:
        Style(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~Style() { Dispose(); }

        virtual void Dispose();
    }; // Style

}}} // SolidFramework::Model::Plumbing

namespace SolidFramework { namespace Model { namespace Plumbing { 

    class TextStyle: public Style
    {
    protected:
        TextStyle();
    public:
        TextStyle(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~TextStyle() { Dispose(); }

        virtual void Dispose();

        // Gets the style template that this style is based on.
        StyleTemplatePtr GetBasedOn();

        // Gets a CharacterPosition that specifies the position of the characters (superscript/subscript/normal).
        CharacterPosition GetCharacterPosition();

        // Gets the char spacing.
        double GetCharSpacing();

        // Gets the font. This may be null
        FontPtr GetFont();

        // Gets the size of the font in points.
        double GetFontSize();

        // Gets the font weight in points.
        int GetFontWeight();

        // Gets whether a HighlightColor has been explicitly assigned.
        bool HasHighlightColor();

        // Gets whether a TextColor has been explicitly assigned.
        bool HasTextColor();

        // Gets whether an UnderlineColor has been explicitly assigned.
        bool HasUnderlineColor();

        // Gets the color of the highlight.
        SolidFramework::Interop::ColorPtr GetHighlightColor();

        // Gets a value indicating whether this text is italic.
        bool GetItalic();

        // Gets a value indicating whether [right to left].
        bool GetRightToLeft();

        // Gets a value indicating whether this text is smallcaps.
        bool GetSmallCaps();

        // Gets the color of the text.
        SolidFramework::Interop::ColorPtr GetTextColor();

        // Gets the text rise.
        double GetTextRise();

        // Gets the text scale. This is a ratio.
        double GetTextScale();

        // Gets the color of the underline.
        SolidFramework::Interop::ColorPtr GetUnderlineColor();

        // Gets the underline effect.
        BorderLineType GetUnderlineType();

        // Gets the word spacing.
        double GetWordSpacing();

        static TextStylePtr DynamicCast(StylePtr value);
    }; // TextStyle

}}} // SolidFramework::Model::Plumbing

namespace SolidFramework { namespace Model { namespace Plumbing { 

    class ParagraphStyle: public TextStyle
    {
    protected:
        ParagraphStyle();
    public:
        ParagraphStyle(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~ParagraphStyle() { Dispose(); }

        virtual void Dispose();

        // Gets the amount of spacing (in points) after the specified paragraph or text column.
        double GetAfterSpacing();

        // Gets the alignment for the specified paragraphs.
        Alignment GetAlignment();

        // Gets the amount of spacing (in points) before the specified paragraph or text column.
        double GetBeforeSpacing();

        // Gets the value (in points) for a first line or hanging indent.
        double GetFirstLineIndent();

        // Gets the left indent value (in points) for the specified paragraphs, table rows.
        double GetLeftIndent();

        // Gets the line spacing (in points) for the specified paragraphs.
        double GetLineSpacing();

        // Gets the type of the line spacing.
        LineSpacingType GetLineSpacingType();

        int GetOutlineLevel();

        // Gets the right indent value (in points) for the specified paragraphs.
        double GetRightIndent();

        static ParagraphStylePtr DynamicCast(StylePtr value);
    }; // ParagraphStyle

}}} // SolidFramework::Model::Plumbing

namespace SolidFramework { namespace Model { namespace Plumbing { 

    // Represents style information for a numbered or bullet-point list.
    class ListStyle: public ParagraphStyle
    {
    public:
        ListStyle(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~ListStyle() { Dispose(); }

        virtual void Dispose();

        ListFormat GetFormat();

        int GetStartValue();

        std::wstring GetTemplate();

        static ListStylePtr DynamicCast(StylePtr value);
    }; // ListStyle

}}} // SolidFramework::Model::Plumbing

namespace SolidFramework { namespace Model { namespace Plumbing { 

    class TabStop: public FrameworkObject
    {
    public:
        TabStop(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~TabStop() { Dispose(); }

        virtual void Dispose();

        TabAlignment GetAlignment();

        bool IsEqual(TabStopPtr value);

        TabLeader GetLeader();

        int GetNativeHashCode();

        double GetTabPosition();
    }; // TabStop

}}} // SolidFramework::Model::Plumbing

namespace SolidFramework { 

    // Represents a page range
    class PageRange: public FrameworkObject
    {
    public:
        PageRange(const std::vector<int> & range);
        PageRange(Plumbing::DocumentPtr document);
        PageRange(Plumbing::DocumentPtr document, Plumbing::PageRanges pageRangeSet);
        PageRange(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PageRange() { Dispose(); }

        virtual void Dispose();

        bool ContainsPage(int page, int maxPagesInDocument);

        bool ContainsPage(int page);

        // Gets the page count
        int GetCount();

        // Gets the first number of page
        int GetFirst();

        // Gets the last number of page
        int GetLast();

        // Parses a PageRange from the specified string or throws an exception if it can't be parsed.
        static PageRangePtr Parse(std::wstring range);

        // Return the complete array of integer page numbers after Parse is called.
        // 
        // NOTE: This is a stop gap to allow multiple page ranges like 1-3,4,6-10 used by a couple sample apps like PDFtoImage.
        std::vector<int> ToArray(int maxPagesInDocument);

        virtual std::wstring ToString();

        static bool TryParse(std::wstring range, PageRangePtr& result);
    }; // PageRange

} // SolidFramework

namespace SolidFramework { namespace Pdf { 

    // Represents PDF Info
    class Info: public FrameworkObject
    {
    public:
        Info(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~Info() { Dispose(); }

        virtual void Dispose();

        // Gets the author of the document
        std::wstring GetAuthor();

        // Gets the author of the document
        void SetAuthor(std::wstring value);

        // Creates the specified document.
        static InfoPtr Create(PdfDocumentPtr document);

        // Gets the creation date of the document
        Plumbing::PdfDatePtr GetCreationDate();

        // Gets the creation date of the document
        void SetCreationDate(Plumbing::PdfDatePtr value);

        // Gets the creator of the document
        std::wstring GetCreator();

        // Gets the creator of the document
        void SetCreator(std::wstring value);

        // Gets the keywords of the document
        std::wstring GetKeywords();

        // Gets the keywords of the document
        void SetKeywords(std::wstring value);

        // Gets the modification date of the document
        Plumbing::PdfDatePtr GetModificationDate();

        // Gets the modification date of the document
        void SetModificationDate(Plumbing::PdfDatePtr value);

        // Gets the producer of the document
        std::wstring GetProducer();

        // Gets the producer of the document
        void SetProducer(std::wstring value);

        // Gets the subject of the document
        std::wstring GetSubject();

        // Gets the subject of the document
        void SetSubject(std::wstring value);

        // Gets the title of the document
        std::wstring GetTitle();

        // Gets the title of the document
        void SetTitle(std::wstring value);
    }; // Info

}} // SolidFramework::Pdf

namespace SolidFramework { namespace Pdf { 

    // Represents Optional content properties
    class OptionalContentProperties: public FrameworkObject
    {
    public:
        OptionalContentProperties(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~OptionalContentProperties() { Dispose(); }

        virtual void Dispose();

        // Gets the alternate configurations.
        Plumbing::PdfArrayPtr GetAlternateConfigurations();

        // Creates Optional Content Properties from the specified catalog.
        static OptionalContentPropertiesPtr Create(CatalogPtr catalog);

        // Gets the default dictionary
        Plumbing::PdfDictionaryPtr GetDefault();

        // Gets the optional content groups.
        Plumbing::PdfArrayPtr GetOptionalContentGroups();
    }; // OptionalContentProperties

}} // SolidFramework::Pdf

namespace SolidFramework { namespace Pdf { 

    // Represents types of Pdf Security Handler.
    class PdfSecurityHandler: public FrameworkObject
    {
    protected:
        PdfSecurityHandler();
    public:
        PdfSecurityHandler(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfSecurityHandler() { Dispose(); }

        virtual void Dispose();

        // Gets the encryption algorithm.
        EncryptionAlgorithm GetEncryptionAlgorithm();

        // Gets a value indicating whether this instance is owner.
        bool IsOwner();

        // Gets the permissions.
        AccessPermissions GetPermissions();
    }; // PdfSecurityHandler

}} // SolidFramework::Pdf

namespace SolidFramework { namespace Pdf { 

    // Represents types of Pdf Certificate Security Handler.
    class PdfCertificateSecurityHandler: public PdfSecurityHandler
    {
    public:
        PdfCertificateSecurityHandler();
        PdfCertificateSecurityHandler(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfCertificateSecurityHandler() { Dispose(); }

        virtual void Dispose();

        static PdfCertificateSecurityHandlerPtr DynamicCast(PdfSecurityHandlerPtr value);
    }; // PdfCertificateSecurityHandler

}} // SolidFramework::Pdf

namespace SolidFramework { namespace Pdf { 

    // Represents types of Pdf Password Security Handler.
    class PdfPasswordSecurityHandler: public PdfSecurityHandler
    {
    public:
        PdfPasswordSecurityHandler();
        PdfPasswordSecurityHandler(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfPasswordSecurityHandler() { Dispose(); }

        virtual void Dispose();

        EncryptionAlgorithm GetEncryptionAlgorithm();

        void SetEncryptionAlgorithm(EncryptionAlgorithm value);

        // Gets the open password.
        std::wstring GetOpenPassword();

        // Gets the open password.
        void SetOpenPassword(std::wstring value);

        // Gets the owner password.
        std::wstring GetOwnerPassword();

        // Gets the owner password.
        void SetOwnerPassword(std::wstring value);

        AccessPermissions GetPermissions();

        void SetPermissions(AccessPermissions value);

        static PdfPasswordSecurityHandlerPtr DynamicCast(PdfSecurityHandlerPtr value);
    }; // PdfPasswordSecurityHandler

}} // SolidFramework::Pdf

namespace SolidFramework { namespace Pdf { namespace Plumbing { 

    // Represents PDF date
    // ISO/IEC 8824 - see PDF Reference
    class PdfDate: public FrameworkObject
    {
    public:
        PdfDate();
        PdfDate(SolidFramework::Interop::DateTimePtr date);
        PdfDate(std::wstring date);
        PdfDate(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfDate() { Dispose(); }

        virtual void Dispose();

        // Gets value.
        SolidFramework::Interop::DateTimePtr GetDateTime();

        // Gets value.
        void SetDateTime(SolidFramework::Interop::DateTimePtr value);

        // Gets a value indicating whether this instance is valid.
        bool IsValid();

        // Parses a PdfDate from the specified string or throws an exception if it can't be parsed.
        static PdfDatePtr Parse(std::wstring date);

        virtual std::wstring ToString();

        // Converts to the string XMP.
        std::wstring ToStringXmp();

        static bool TryParse(std::wstring date, PdfDatePtr& result);

        SolidFramework::Interop::TimeSpanPtr GetUtcOffset();

        void SetUtcOffset(SolidFramework::Interop::TimeSpanPtr value);

        // Gets a value indicating whether [UTC offset defined].
        bool GetUtcOffsetDefined();

        // Sets a value indicating whether [UTC offset defined].
        void SetUtcOffsetDefined(bool value);
    }; // PdfDate

}}} // SolidFramework::Pdf::Plumbing

namespace SolidFramework { namespace Pdf { namespace Plumbing { 

    class PdfDictionaryItemsCollection: public FrameworkObject
    {
    public:
        PdfDictionaryItemsCollection();
        PdfDictionaryItemsCollection(PdfDictionaryItemsCollectionPtr other);
        PdfDictionaryItemsCollection(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfDictionaryItemsCollection() { Dispose(); }

        virtual void Dispose();

        virtual void Add(std::wstring key, PdfItemPtr val);

        virtual void Clear();

        virtual bool ContainsKey(std::wstring key);

        virtual int GetCount();

        bool Empty();

        virtual PdfItemPtr GetItem(std::wstring key);

        virtual bool Remove(std::wstring key);
    }; // PdfDictionaryItemsCollection

}}} // SolidFramework::Pdf::Plumbing

namespace SolidFramework { namespace Pdf { namespace Plumbing { 

    // Represents PDF item
    class PdfItem: public FrameworkObject
    {
    protected:
        PdfItem();
    public:
        PdfItem(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfItem() { Dispose(); }

        virtual void Dispose();

        HANDLE GetHandle();

        static PdfItemPtr GetIndirectionItem(PdfItemPtr item);

        static PdfItemPtr GetIndirectionItem(PdfItemPtr item, std::wstring forceType);

        static PdfObjectPtr GetIndirectionObject(PdfItemPtr item);

        // Gets the type of the PDF object.
        PdfObjectType GetPdfObjectType();

        virtual std::wstring ToString();
    }; // PdfItem

}}} // SolidFramework::Pdf::Plumbing

namespace SolidFramework { namespace Pdf { namespace Plumbing { 

    // Represents PDF array
    class PdfArray: public PdfItem
    {
    protected:
        PdfArray();
    public:
        PdfArray(SolidFramework::Pdf::PdfDocumentPtr pdfDocument);
        PdfArray(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfArray() { Dispose(); }

        virtual void Dispose();

        int Add(PdfItemPtr value);

        void Add(PdfItemPtr value, int index);

        // Removes all items from the array.
        void Clear();

        // Gets the items count.
        int GetCount();

        // Gets a value indicating whether this instance has fixed size.
        bool IsFixedSize();

        PdfItemPtr GetItem(int index);

        // Returns reference to PdfArrayItemsCollection object.
        // PdfArrayItemsCollection implements System.Collections.IEnumerable
        // and System.Collections.Generic.IList{PdfItem} interfaces in C#
        std::vector<PdfItemPtr> GetPdfArrayItemsCollection();

        // Gets a value indicating whether the array is read-only.
        bool IsReadOnly();

        // Gets a value indicating whether this instance is synchronized.
        bool IsSynchronized();

        // Returns a std::wstring that represents this instance.
        virtual std::wstring ToString();

        static PdfArrayPtr DynamicCast(PdfItemPtr value);
    }; // PdfArray

}}} // SolidFramework::Pdf::Plumbing

namespace SolidFramework { namespace Pdf { namespace Plumbing { 

    // Represents PDF kids
    class PdfKids: public PdfArray
    {
    public:
        PdfKids(SolidFramework::Pdf::PdfDocumentPtr pdfDocument);
        PdfKids(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfKids() { Dispose(); }

        virtual void Dispose();

        static PdfKidsPtr DynamicCast(PdfItemPtr value);
    }; // PdfKids

}}} // SolidFramework::Pdf::Plumbing

namespace SolidFramework { namespace Pdf { namespace Plumbing { 

    // Represents PDF rectangle
    class PdfRectangle: public PdfArray
    {
    public:
        PdfRectangle(SolidFramework::Pdf::PdfDocumentPtr pdfDocument);
        PdfRectangle(SolidFramework::Pdf::PdfDocumentPtr pdfDocument, double x, double y, double width, double height);
        PdfRectangle(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfRectangle() { Dispose(); }

        virtual void Dispose();

        // Gets the bottom.
        double GetBottom();

        // Gets the left.
        double GetLeft();

        // Gets the right.
        double GetRight();

        // Gets the top.
        double GetTop();

        static PdfRectanglePtr DynamicCast(PdfItemPtr value);
    }; // PdfRectangle

}}} // SolidFramework::Pdf::Plumbing

namespace SolidFramework { namespace Pdf { namespace Plumbing { 

    // Represents PDF boolean
    class PdfBoolean: public PdfItem
    {
    public:
        PdfBoolean(SolidFramework::Pdf::PdfDocumentPtr pdfDocument);
        PdfBoolean(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfBoolean() { Dispose(); }

        virtual void Dispose();

        // Returns a std::wstring that represents this instance.
        virtual std::wstring ToString();

        // Gets the value.
        bool GetValue();

        // Gets the value.
        void SetValue(bool value);

        static PdfBooleanPtr DynamicCast(PdfItemPtr value);
    }; // PdfBoolean

}}} // SolidFramework::Pdf::Plumbing

namespace SolidFramework { namespace Pdf { namespace Plumbing { 

    // Represents PDF dictionary
    class PdfDictionary: public PdfItem
    {
    protected:
        PdfDictionary();
    public:
        PdfDictionary(SolidFramework::Pdf::PdfDocumentPtr pdfDocument);
        PdfDictionary(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfDictionary() { Dispose(); }

        virtual void Dispose();

        // Adds an element with the provided key and value to the dictionary.
        void Add(std::wstring key, PdfItemPtr value);

        // Removes all items from the dictionary.
        void Clear();

        // Clears the metadata.
        bool ClearMetadata();

        // Determines whether the dictionary contains an element with the specified key.
        bool ContainsKey(std::wstring key);

        // Gets the number of elements contained in the dictionary.
        int GetCount();

        PdfItemPtr GetDictionaryValueByIndex(int i);

        PdfItemPtr GetItem(std::wstring key);

        PdfDictionaryItemsCollectionPtr GetPdfDictionaryItemsCollection();

        // Returns a std::wstring that represents this instance.
        virtual std::wstring ToString();

        // Gets the type.
        std::wstring GetType();

        static PdfDictionaryPtr DynamicCast(PdfItemPtr value);
    }; // PdfDictionary

}}} // SolidFramework::Pdf::Plumbing

namespace SolidFramework { namespace Pdf { namespace Plumbing { 

    // Represents PDF action
    class PdfAction: public PdfDictionary
    {
    protected:
        PdfAction();
    public:
        PdfAction(SolidFramework::Pdf::PdfDocumentPtr pdfDocument);
        PdfAction(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfAction() { Dispose(); }

        virtual void Dispose();

        // Gets the D.
        PdfItemPtr GetD();

        // Gets the S.
        std::wstring GetS();

        static PdfActionPtr DynamicCast(PdfItemPtr value);
    }; // PdfAction

}}} // SolidFramework::Pdf::Plumbing

namespace SolidFramework { namespace Pdf { namespace Plumbing { 

    // Represents PDF GoTo Action
    class PdfGoTo: public PdfAction
    {
    public:
        PdfGoTo(SolidFramework::Pdf::PdfDocumentPtr pdfDocument);
        PdfGoTo(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfGoTo() { Dispose(); }

        virtual void Dispose();

        static PdfGoToPtr DynamicCast(PdfItemPtr value);
    }; // PdfGoTo

}}} // SolidFramework::Pdf::Plumbing

namespace SolidFramework { namespace Pdf { namespace Plumbing { 

    // Represents PDF URI dictionary
    class PdfUriDictionary: public PdfAction
    {
    public:
        PdfUriDictionary(SolidFramework::Pdf::PdfDocumentPtr pdfDocument);
        PdfUriDictionary(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfUriDictionary() { Dispose(); }

        virtual void Dispose();

        // Gets the URI.
        std::wstring GetUri();

        static PdfUriDictionaryPtr DynamicCast(PdfItemPtr value);
    }; // PdfUriDictionary

}}} // SolidFramework::Pdf::Plumbing

namespace SolidFramework { namespace Pdf { namespace Plumbing { 

    // Represents PDF annotation
    class PdfAnnot: public PdfDictionary
    {
    protected:
        PdfAnnot();
    public:
        PdfAnnot(SolidFramework::Pdf::PdfDocumentPtr pdfDocument);
        PdfAnnot(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfAnnot() { Dispose(); }

        virtual void Dispose();

        // Gets the contents.
        std::wstring GetContents();

        // Gets the date and time when the annotation was most recently modified.
        std::wstring GetModified();

        virtual PdfObjectPtr GetParentAnnotationObj();

        // Gets the annotation rectangle, defining the location of the annotation on the page in default user space units.
        SolidFramework::Interop::RectangleFPtr GetRect();

        // Gets the type of annotation that this dictionary describes.
        std::wstring GetSubtype();

        static PdfAnnotPtr DynamicCast(PdfItemPtr value);
    }; // PdfAnnot

}}} // SolidFramework::Pdf::Plumbing

namespace SolidFramework { namespace Pdf { namespace Plumbing { 

    // Represents PDF link
    class PdfLink: public PdfAnnot
    {
    public:
        PdfLink(SolidFramework::Pdf::PdfDocumentPtr pdfDocument);
        PdfLink(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfLink() { Dispose(); }

        virtual void Dispose();

        // Gets the A.
        PdfActionPtr GetA();

        // Gets the dest.
        PdfItemPtr GetDest();

        static PdfLinkPtr DynamicCast(PdfItemPtr value);
    }; // PdfLink

}}} // SolidFramework::Pdf::Plumbing

namespace SolidFramework { namespace Pdf { namespace Plumbing { 

    // Represents PDF markup annotation
    class PdfMarkupAnnot: public PdfAnnot
    {
    protected:
        PdfMarkupAnnot();
    public:
        PdfMarkupAnnot(SolidFramework::Pdf::PdfDocumentPtr pdfDocument);
        PdfMarkupAnnot(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfMarkupAnnot() { Dispose(); }

        virtual void Dispose();

        // Gets the date and time when the annotation was created.
        std::wstring GetCreationDate();

        virtual PdfObjectPtr GetParentAnnotationObj();

        // Gets a rich text string to be displayed in the pop-up window when the annotation is opened.
        std::wstring GetRC();

        // Gets text representing a short description of the subject being addressed by the annotation.
        std::wstring GetSubj();

        // Gets the text label to be displayed in the title bar of the annotation's pop-up window when open and active.
        // By convention, this entry identifies the user who added the annotation.
        std::wstring GetT();

        static PdfMarkupAnnotPtr DynamicCast(PdfItemPtr value);
    }; // PdfMarkupAnnot

}}} // SolidFramework::Pdf::Plumbing

namespace SolidFramework { namespace Pdf { namespace Plumbing { 

    // Represents PDF caret annotation
    class PdfCaretAnnot: public PdfMarkupAnnot
    {
    public:
        PdfCaretAnnot(SolidFramework::Pdf::PdfDocumentPtr pdfDocument);
        PdfCaretAnnot(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfCaretAnnot() { Dispose(); }

        virtual void Dispose();

        static PdfCaretAnnotPtr DynamicCast(PdfItemPtr value);
    }; // PdfCaretAnnot

}}} // SolidFramework::Pdf::Plumbing

namespace SolidFramework { namespace Pdf { namespace Plumbing { 

    // Represents PDF circle annotation
    class PdfCircleAnnot: public PdfMarkupAnnot
    {
    public:
        PdfCircleAnnot(SolidFramework::Pdf::PdfDocumentPtr pdfDocument);
        PdfCircleAnnot(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfCircleAnnot() { Dispose(); }

        virtual void Dispose();

        static PdfCircleAnnotPtr DynamicCast(PdfItemPtr value);
    }; // PdfCircleAnnot

}}} // SolidFramework::Pdf::Plumbing

namespace SolidFramework { namespace Pdf { namespace Plumbing { 

    // Represents PDF file attachment annotation
    class PdfFileAttachmentAnnot: public PdfMarkupAnnot
    {
    public:
        PdfFileAttachmentAnnot(SolidFramework::Pdf::PdfDocumentPtr pdfDocument);
        PdfFileAttachmentAnnot(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfFileAttachmentAnnot() { Dispose(); }

        virtual void Dispose();

        static PdfFileAttachmentAnnotPtr DynamicCast(PdfItemPtr value);
    }; // PdfFileAttachmentAnnot

}}} // SolidFramework::Pdf::Plumbing

namespace SolidFramework { namespace Pdf { namespace Plumbing { 

    // Represents PDF free text annotation
    class PdfFreeTextAnnot: public PdfMarkupAnnot
    {
    public:
        PdfFreeTextAnnot(SolidFramework::Pdf::PdfDocumentPtr pdfDocument);
        PdfFreeTextAnnot(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfFreeTextAnnot() { Dispose(); }

        virtual void Dispose();

        static PdfFreeTextAnnotPtr DynamicCast(PdfItemPtr value);
    }; // PdfFreeTextAnnot

}}} // SolidFramework::Pdf::Plumbing

namespace SolidFramework { namespace Pdf { namespace Plumbing { 

    // Represents PDF highlight annotation
    class PdfHighlightAnnot: public PdfMarkupAnnot
    {
    public:
        PdfHighlightAnnot(SolidFramework::Pdf::PdfDocumentPtr pdfDocument);
        PdfHighlightAnnot(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfHighlightAnnot() { Dispose(); }

        virtual void Dispose();

        static PdfHighlightAnnotPtr DynamicCast(PdfItemPtr value);
    }; // PdfHighlightAnnot

}}} // SolidFramework::Pdf::Plumbing

namespace SolidFramework { namespace Pdf { namespace Plumbing { 

    // Represents PDF ink annotation
    class PdfInkAnnot: public PdfMarkupAnnot
    {
    public:
        PdfInkAnnot(SolidFramework::Pdf::PdfDocumentPtr pdfDocument);
        PdfInkAnnot(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfInkAnnot() { Dispose(); }

        virtual void Dispose();

        static PdfInkAnnotPtr DynamicCast(PdfItemPtr value);
    }; // PdfInkAnnot

}}} // SolidFramework::Pdf::Plumbing

namespace SolidFramework { namespace Pdf { namespace Plumbing { 

    // Represents PDF line annotation
    class PdfLineAnnot: public PdfMarkupAnnot
    {
    public:
        PdfLineAnnot(SolidFramework::Pdf::PdfDocumentPtr pdfDocument);
        PdfLineAnnot(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfLineAnnot() { Dispose(); }

        virtual void Dispose();

        static PdfLineAnnotPtr DynamicCast(PdfItemPtr value);
    }; // PdfLineAnnot

}}} // SolidFramework::Pdf::Plumbing

namespace SolidFramework { namespace Pdf { namespace Plumbing { 

    // Represents PDF polygon annotation
    class PdfPolygonAnnot: public PdfMarkupAnnot
    {
    public:
        PdfPolygonAnnot(SolidFramework::Pdf::PdfDocumentPtr pdfDocument);
        PdfPolygonAnnot(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfPolygonAnnot() { Dispose(); }

        virtual void Dispose();

        static PdfPolygonAnnotPtr DynamicCast(PdfItemPtr value);
    }; // PdfPolygonAnnot

}}} // SolidFramework::Pdf::Plumbing

namespace SolidFramework { namespace Pdf { namespace Plumbing { 

    // Represents PDF polyline annotation
    class PdfPolyLineAnnot: public PdfMarkupAnnot
    {
    public:
        PdfPolyLineAnnot(SolidFramework::Pdf::PdfDocumentPtr pdfDocument);
        PdfPolyLineAnnot(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfPolyLineAnnot() { Dispose(); }

        virtual void Dispose();

        static PdfPolyLineAnnotPtr DynamicCast(PdfItemPtr value);
    }; // PdfPolyLineAnnot

}}} // SolidFramework::Pdf::Plumbing

namespace SolidFramework { namespace Pdf { namespace Plumbing { 

    // Represents PDF sound annotation
    class PdfSoundAnnot: public PdfMarkupAnnot
    {
    public:
        PdfSoundAnnot(SolidFramework::Pdf::PdfDocumentPtr pdfDocument);
        PdfSoundAnnot(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfSoundAnnot() { Dispose(); }

        virtual void Dispose();

        static PdfSoundAnnotPtr DynamicCast(PdfItemPtr value);
    }; // PdfSoundAnnot

}}} // SolidFramework::Pdf::Plumbing

namespace SolidFramework { namespace Pdf { namespace Plumbing { 

    // Represents PDF square annotation
    class PdfSquareAnnot: public PdfMarkupAnnot
    {
    public:
        PdfSquareAnnot(SolidFramework::Pdf::PdfDocumentPtr pdfDocument);
        PdfSquareAnnot(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfSquareAnnot() { Dispose(); }

        virtual void Dispose();

        static PdfSquareAnnotPtr DynamicCast(PdfItemPtr value);
    }; // PdfSquareAnnot

}}} // SolidFramework::Pdf::Plumbing

namespace SolidFramework { namespace Pdf { namespace Plumbing { 

    // Represents PDF squiggly annotation
    class PdfSquigglyAnnot: public PdfMarkupAnnot
    {
    public:
        PdfSquigglyAnnot(SolidFramework::Pdf::PdfDocumentPtr pdfDocument);
        PdfSquigglyAnnot(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfSquigglyAnnot() { Dispose(); }

        virtual void Dispose();

        static PdfSquigglyAnnotPtr DynamicCast(PdfItemPtr value);
    }; // PdfSquigglyAnnot

}}} // SolidFramework::Pdf::Plumbing

namespace SolidFramework { namespace Pdf { namespace Plumbing { 

    // Represents PDF stamp annotation
    class PdfStampAnnot: public PdfMarkupAnnot
    {
    public:
        PdfStampAnnot(SolidFramework::Pdf::PdfDocumentPtr pdfDocument);
        PdfStampAnnot(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfStampAnnot() { Dispose(); }

        virtual void Dispose();

        static PdfStampAnnotPtr DynamicCast(PdfItemPtr value);
    }; // PdfStampAnnot

}}} // SolidFramework::Pdf::Plumbing

namespace SolidFramework { namespace Pdf { namespace Plumbing { 

    // Represents PDF strike out annotation
    class PdfStrikeOutAnnot: public PdfMarkupAnnot
    {
    public:
        PdfStrikeOutAnnot(SolidFramework::Pdf::PdfDocumentPtr pdfDocument);
        PdfStrikeOutAnnot(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfStrikeOutAnnot() { Dispose(); }

        virtual void Dispose();

        static PdfStrikeOutAnnotPtr DynamicCast(PdfItemPtr value);
    }; // PdfStrikeOutAnnot

}}} // SolidFramework::Pdf::Plumbing

namespace SolidFramework { namespace Pdf { namespace Plumbing { 

    // Represents PDF text annotation
    class PdfTextAnnot: public PdfMarkupAnnot
    {
    public:
        PdfTextAnnot(SolidFramework::Pdf::PdfDocumentPtr pdfDocument);
        PdfTextAnnot(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfTextAnnot() { Dispose(); }

        virtual void Dispose();

        static PdfTextAnnotPtr DynamicCast(PdfItemPtr value);
    }; // PdfTextAnnot

}}} // SolidFramework::Pdf::Plumbing

namespace SolidFramework { namespace Pdf { namespace Plumbing { 

    // Represents PDF underline annotation
    class PdfUnderlineAnnot: public PdfMarkupAnnot
    {
    public:
        PdfUnderlineAnnot(SolidFramework::Pdf::PdfDocumentPtr pdfDocument);
        PdfUnderlineAnnot(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfUnderlineAnnot() { Dispose(); }

        virtual void Dispose();

        static PdfUnderlineAnnotPtr DynamicCast(PdfItemPtr value);
    }; // PdfUnderlineAnnot

}}} // SolidFramework::Pdf::Plumbing

namespace SolidFramework { namespace Pdf { namespace Plumbing { 

    // Represents PDF highlight
    class PdfPopupAnnot: public PdfAnnot
    {
    public:
        PdfPopupAnnot(SolidFramework::Pdf::PdfDocumentPtr pdfDocument);
        PdfPopupAnnot(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfPopupAnnot() { Dispose(); }

        virtual void Dispose();

        virtual PdfObjectPtr GetParentAnnotationObj();

        static PdfPopupAnnotPtr DynamicCast(PdfItemPtr value);
    }; // PdfPopupAnnot

}}} // SolidFramework::Pdf::Plumbing

namespace SolidFramework { namespace Pdf { namespace Plumbing { 

    // Represents PDF font.
    class PdfFont: public PdfDictionary
    {
    public:
        PdfFont(SolidFramework::Pdf::PdfDocumentPtr pdfDocument);
        PdfFont(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfFont() { Dispose(); }

        virtual void Dispose();

        // Gets a base font name.
        std::wstring GetBaseFontName();

        // Gets a value indicating whether this instance is built in.
        bool IsBuiltIn();

        // Gets a value indicating whether this instance is embedded.
        bool IsEmbedded();

        FontStatus GetPdfFontStatus();

        // Gets the font type.
        FontType GetPdfFontType();

        // Gets a value indicating whether this instance has the predefined encoding.
        bool IsPredefinedEncoding();

        // Gets a value indicating whether this instance is subset.
        bool IsSubset();

        // Gets a value indicating whether this instance is symbolic.
        bool IsSymbolic();

        // Gets a value indicating whether this instance has to unicode.
        bool HasToUnicode();

        static PdfFontPtr DynamicCast(PdfItemPtr value);
    }; // PdfFont

}}} // SolidFramework::Pdf::Plumbing

namespace SolidFramework { namespace Pdf { namespace Plumbing { 

    // Represents PDF outline item
    class PdfOutlineItem: public PdfDictionary
    {
    public:
        PdfOutlineItem(SolidFramework::Pdf::PdfDocumentPtr pdfDocument);
        PdfOutlineItem(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfOutlineItem() { Dispose(); }

        virtual void Dispose();

        // Gets the A.
        PdfActionPtr GetA();

        // Gets the destination.
        PdfItemPtr GetDest();

        // Gets the first item
        PdfOutlineItemPtr GetFirst();

        // Gets the last item
        PdfOutlineItemPtr GetLast();

        // Gets the next item
        PdfOutlineItemPtr GetNext();

        // Gets the open items.
        int GetOpenItems();

        // Gets the parent.
        PdfItemPtr GetParent();

        // Gets the previous item
        PdfOutlineItemPtr GetPrev();

        // Gets the title.
        std::wstring GetTitle();

        static PdfOutlineItemPtr DynamicCast(PdfItemPtr value);
    }; // PdfOutlineItem

}}} // SolidFramework::Pdf::Plumbing

namespace SolidFramework { namespace Pdf { namespace Plumbing { 

    // Represents PDF outlines
    class PdfOutlines: public PdfDictionary
    {
    public:
        PdfOutlines(SolidFramework::Pdf::PdfDocumentPtr pdfDocument);
        PdfOutlines(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfOutlines() { Dispose(); }

        virtual void Dispose();

        // Gets the first item
        PdfOutlineItemPtr GetFirst();

        // Gets the last item
        PdfOutlineItemPtr GetLast();

        // Gets the open items.
        int GetOpenItems();

        static PdfOutlinesPtr DynamicCast(PdfItemPtr value);
    }; // PdfOutlines

}}} // SolidFramework::Pdf::Plumbing

namespace SolidFramework { namespace Pdf { namespace Plumbing { 

    // Represents a single page within a PDF document
    class PdfPage: public PdfDictionary
    {
    public:
        PdfPage(SolidFramework::Pdf::PdfDocumentPtr pdfDocument);
        PdfPage(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfPage() { Dispose(); }

        virtual void Dispose();

        // Gets the annotations.
        PdfArrayPtr GetAnnots();

        // Gets the annotations.
        void SetAnnots(PdfArrayPtr value);

        // Gets the art box. This is a rectangle, expressed in points with the origin at the bottom left corner, defining the extent of the page 
        // meaningful content (including potential white space). 
        // This property may be null, in which case use the CropBox property.
        PdfRectanglePtr GetArtBox();

        // Auto rotates the document, and returns the angle by which the document was rotated.
        int AutoRotate();

        // Gets the bleed box. This is a rectangle, expressed in points with the origin at the bottom left corner, defining the size of 
        // the page to which the contents need tobe clipped in a "production" environment.
        // When the page is printed, its contents are to be clipped (cropped) to this rectangle. 
        // This property may be null, in which case use the CropBox property.
        PdfRectanglePtr GetBleedBox();

        int BuildLayout();

        bool CanSelectTextWithRect(SolidFramework::Interop::RectangleFPtr selectionRect);

        // Gets the contents.
        PdfItemPtr GetContents();

        // Gets the crop box. This is a rectangle, expressed in points with the origin at the bottom left corner, defining the visible region of 
        // the page. When the page is displayed or printed, its contents are to be clipped (cropped) to this rectangle. 
        // This property may be null, in which case use the MediaBox property.
        PdfRectanglePtr GetCropBox();

        // Gets the crop box. This is a rectangle, expressed in points with the origin at the bottom left corner, defining the visible region of 
        // the page. When the page is displayed or printed, its contents are to be clipped (cropped) to this rectangle. 
        // This property may be null, in which case use the MediaBox property.
        void SetCropBox(PdfRectanglePtr value);

        // Deskew.
        double Deskew();

        // Draws the bitmap using GDIplus and without resetting the cache before drawing.
        SolidFramework::Interop::BitmapPtr DrawBitmap(int nDpi);

        // Draws the bitmap.
        SolidFramework::Interop::BitmapPtr DrawBitmap(int nDpi, SolidFramework::Pdf::Interop::DrawingCore drawingCore, SolidFramework::Pdf::Interop::CacheDrawingType cache);

        // Determines if the page contains hidden text (text render mode 3 used for searchable text in scanned documents).
        bool HasInvisibleText();

        // Determines if the page is scanned (needing OCR). This will also return true if the page contains vector text.
        bool IsScanned();

        // Determines if the page contains vector text (typically drawn by CAD) which would need OCR.
        bool IsVectorText();

        // Gets the time when the page was last modified. this may be null.
        PdfDatePtr GetLastModified();

        // Loads the page into the document.
        int LoadPage();

        // Gets the media box. This is a rectangle, expressed in points with the origin at the bottom left corner, defining the boundaries of the physical 
        // medium on which the page is intented to be displayed or printed.
        PdfRectanglePtr GetMediaBox();

        // Gets the media box. This is a rectangle, expressed in points with the origin at the bottom left corner, defining the boundaries of the physical 
        // medium on which the page is intented to be displayed or printed.
        void SetMediaBox(PdfRectanglePtr value);

        // Gets the original reference.
        PdfReferencePtr GetOriginalReference();

        // Gets information about the items that the page contains. e.g. Images, visible text, invisible text, etc.
        PageStatistics GetPageStatistics();

        // Gets the parent.
        PdfPagesPtr GetParent();

        int ResetLayout();

        // Gets the resources.
        PdfDictionaryPtr GetResources();

        // Gets the angle by which the page has been rotated. The only values that should be used are -270,-180,-90,90,180 and 270.
        // However other values can be entered and the rendering engine will show the page rotated by whichever orientation is closest to the specified value.
        int GetRotation();

        // Gets the angle by which the page has been rotated. The only values that should be used are -270,-180,-90,90,180 and 270.
        // However other values can be entered and the rendering engine will show the page rotated by whichever orientation is closest to the specified value.
        void SetRotation(int value);

        // Gets the scanned image if the page was scanned, otherwise returns null.
        SolidFramework::Imaging::ImagePtr GetScannedImage();

        // Searches the visible text on the page for the searchText.
        int SearchText(std::wstring searchText, bool matchCase, bool matchWholeWord);

        std::wstring GetSelectedText();

        bool SelectText(SolidFramework::Interop::RectangleFPtr selectionRect, bool resetPreviousSelection);

        bool SelectTextWithRect(SolidFramework::Interop::RectangleFPtr selectionRect, bool resetPreviousSelection);

        SolidFramework::Interop::RectangleFPtr SetTextSelection(int index);

        // Gets the trim box. This is a rectangle, expressed in points with the origin at the bottom left corner, defining the intended size of 
        // the finished page.
        // This property may be null, in which case use the CropBox property.
        PdfRectanglePtr GetTrimBox();

        // Gets the UserUnit which is 1.0 by default and is multiples of 1/72 inch (points).
        double GetUserUnit();

        // Sets the UserUnit which is 1.0 by default and is multiples of 1/72 inch (points).
        void SetUserUnit(double value);

        // Gets the visible box. This is the intersection of the MediaBox and CropBox.
        SolidFramework::Interop::RectangleFPtr GetVisibleBox();

        static PdfPagePtr DynamicCast(PdfItemPtr value);
    }; // PdfPage

}}} // SolidFramework::Pdf::Plumbing

namespace SolidFramework { namespace Pdf { namespace Plumbing { 

    // Represents PDF page collection
    class PdfPages: public PdfDictionary
    {
    public:
        PdfPages(SolidFramework::Pdf::PdfDocumentPtr pdfDocument);
        PdfPages(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfPages() { Dispose(); }

        virtual void Dispose();

        // Gets the art box.
        PdfRectanglePtr GetArtBox();

        // Gets the bleed box.
        PdfRectanglePtr GetBleedBox();

        // Gets the crop box.
        PdfRectanglePtr GetCropBox();

        // Gets the kids.
        PdfKidsPtr GetKids();

        // Gets the media box.
        PdfRectanglePtr GetMediaBox();

        // Gets the page count.
        int GetPageCount();

        // Gets the parent.
        PdfPagesPtr GetParent();

        // Gets the resources.
        PdfDictionaryPtr GetResources();

        // Gets the angle
        int GetRotation();

        // Gets the trim box.
        PdfRectanglePtr GetTrimBox();

        static PdfPagesPtr DynamicCast(PdfItemPtr value);

        static PdfPagesPtr DynamicCast(SolidFramework::Plumbing::IPagesCollectionPtr value);
    }; // PdfPages

}}} // SolidFramework::Pdf::Plumbing

namespace SolidFramework { namespace Pdf { namespace Plumbing { 

    // Represents PDF stream object
    class PdfStreamObject: public PdfDictionary
    {
    public:
        PdfStreamObject(SolidFramework::Pdf::PdfDocumentPtr pdfDocument);
        PdfStreamObject(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfStreamObject() { Dispose(); }

        virtual void Dispose();

        // Gets the data.
        HANDLE GetData();

        // Gets the data as string.
        std::wstring GetDataAsString();

        // Gets the data length.
        int GetDataLength();

        // Gets the length.
        int GetLength();

        SolidFramework::Interop::BitmapPtr ToBitmap(SolidFramework::Pdf::PdfDocumentPtr document);

        static PdfStreamObjectPtr DynamicCast(PdfItemPtr value);
    }; // PdfStreamObject

}}} // SolidFramework::Pdf::Plumbing

namespace SolidFramework { namespace Pdf { namespace Plumbing { 

    // Represents PDF named object
    class PdfName: public PdfItem
    {
    public:
        PdfName(SolidFramework::Pdf::PdfDocumentPtr pdfDocument);
        PdfName(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfName() { Dispose(); }

        virtual void Dispose();

        // Returns a std::wstring that represents this instance.
        virtual std::wstring ToString();

        // Gets the value.
        std::wstring GetValue();

        // Gets the value.
        void SetValue(std::wstring value);

        static PdfNamePtr DynamicCast(PdfItemPtr value);
    }; // PdfName

}}} // SolidFramework::Pdf::Plumbing

namespace SolidFramework { namespace Pdf { namespace Plumbing { 

    // Represents PDF null
    class PdfNull: public PdfItem
    {
    public:
        PdfNull(SolidFramework::Pdf::PdfDocumentPtr pdfDocument);
        PdfNull(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfNull() { Dispose(); }

        virtual void Dispose();

        // Returns a std::wstring that represents this instance.
        virtual std::wstring ToString();

        static PdfNullPtr DynamicCast(PdfItemPtr value);
    }; // PdfNull

}}} // SolidFramework::Pdf::Plumbing

namespace SolidFramework { namespace Pdf { namespace Plumbing { 

    // Represents PDF number
    class PdfNumber: public PdfItem
    {
    public:
        PdfNumber(SolidFramework::Pdf::PdfDocumentPtr pdfDocument);
        PdfNumber(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfNumber() { Dispose(); }

        virtual void Dispose();

        // Returns a std::wstring that represents this instance.
        virtual std::wstring ToString();

        // Gets the value.
        double GetValue();

        // Gets the value.
        void SetValue(double value);

        static PdfNumberPtr DynamicCast(PdfItemPtr value);
    }; // PdfNumber

}}} // SolidFramework::Pdf::Plumbing

namespace SolidFramework { namespace Pdf { namespace Plumbing { 

    // Represents PDF object
    class PdfObject: public PdfItem
    {
    public:
        PdfObject(SolidFramework::Pdf::PdfDocumentPtr pdfDocument);
        PdfObject(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfObject() { Dispose(); }

        virtual void Dispose();

        // Gets the generation ID.
        int GetGenerationId();

        // Gets the item.
        PdfItemPtr GetItem();

        // Gets the object ID.
        int GetObjectId();

        // Returns a std::wstring that represents this instance.
        virtual std::wstring ToString();

        static PdfObjectPtr DynamicCast(PdfItemPtr value);
    }; // PdfObject

}}} // SolidFramework::Pdf::Plumbing

namespace SolidFramework { namespace Pdf { namespace Plumbing { 

    // Represents PDF reference
    class PdfReference: public PdfItem
    {
    public:
        PdfReference(SolidFramework::Pdf::PdfDocumentPtr pdfDocument);
        PdfReference(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfReference() { Dispose(); }

        virtual void Dispose();

        // Gets the generation ID.
        int GetGenerationId();

        // Gets the generation ID.
        void SetGenerationId(int value);

        // Gets the pdf object.
        PdfObjectPtr GetObject();

        // Gets the object ID.
        int GetObjectId();

        // Gets the object ID.
        void SetObjectId(int value);

        // Returns a std::wstring that represents this instance.
        virtual std::wstring ToString();

        static PdfReferencePtr DynamicCast(PdfItemPtr value);
    }; // PdfReference

}}} // SolidFramework::Pdf::Plumbing

namespace SolidFramework { namespace Pdf { namespace Plumbing { 

    // Represents PDF string
    class PdfString: public PdfItem
    {
    public:
        PdfString(SolidFramework::Pdf::PdfDocumentPtr pdfDocument);
        PdfString(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfString() { Dispose(); }

        virtual void Dispose();

        // Gets the raw value.
        std::vector<BYTE> GetRawValue();

        // Gets the raw value.
        void SetRawValue(const std::vector<BYTE> & value);

        // Returns a std::wstring that represents this instance.
        virtual std::wstring ToString();

        // Gets the value.
        std::wstring GetValue();

        // Gets the value.
        void SetValue(std::wstring value);

        static PdfStringPtr DynamicCast(PdfItemPtr value);
    }; // PdfString

}}} // SolidFramework::Pdf::Plumbing

namespace SolidFramework { namespace Pdf { namespace Plumbing { 

    class PdfNamedDestinationsCollection: public FrameworkObject
    {
    public:
        PdfNamedDestinationsCollection();
        PdfNamedDestinationsCollection(PdfNamedDestinationsCollectionPtr other);
        PdfNamedDestinationsCollection(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfNamedDestinationsCollection() { Dispose(); }

        virtual void Dispose();

        virtual void Add(std::wstring key, PdfArrayPtr val);

        virtual void Clear();

        virtual bool ContainsKey(std::wstring key);

        virtual int GetCount();

        bool Empty();

        virtual PdfArrayPtr GetItem(std::wstring key);

        virtual bool Remove(std::wstring key);
    }; // PdfNamedDestinationsCollection

}}} // SolidFramework::Pdf::Plumbing

namespace SolidFramework { namespace Pdf { namespace Repair { 

    // Builds the results of the repair, for example xref table rebuilt, duplicate object resolution, stream Length correction, etc
    class RepairXRef: public FrameworkObject
    {
    public:
        RepairXRef(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~RepairXRef() { Dispose(); }

        virtual void Dispose();

        static bool IsPDF(std::wstring path);

        static RepairResult Rebuild(std::wstring originalPath, std::wstring repairedPath);

        static RepairResult Rebuild(std::wstring originalPath, std::wstring repairedPath, bool overwrite);

        int GetVersion();

        void SetVersion(int value);
    }; // RepairXRef

}}} // SolidFramework::Pdf::Repair

namespace SolidFramework { namespace Pdf { namespace Reports { 

    // Represents PDF/A problem
    class PdfAProblem: public FrameworkObject
    {
    public:
        PdfAProblem(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfAProblem() { Dispose(); }

        virtual void Dispose();

        std::wstring GetClause();

        // Gets the description.
        std::wstring GetDescription();

        // Gets the location.
        std::wstring GetLocation();

        // Gets a value indicating whether this PdfAProblem is matched.
        bool GetMatched();

        // Gets a value indicating whether this PdfAProblem is matched.
        void SetMatched(bool value);

        // Gets the section.
        std::wstring GetSection();

        // Gets the severity.
        std::wstring GetSeverity();

        std::wstring GetStandard();

        virtual std::wstring ToString();
    }; // PdfAProblem

}}} // SolidFramework::Pdf::Reports

namespace SolidFramework { namespace Pdf { namespace Reports { 

    // Represents PDF/A problem class
    class PdfAProblemClass: public FrameworkObject
    {
    public:
        PdfAProblemClass();
        PdfAProblemClass(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfAProblemClass() { Dispose(); }

        virtual void Dispose();

        // Compares the specified objects.
        std::wstring Compare(PdfAProblemClassPtr other, bool showWarnings);

        int GetCount();

        std::vector<PdfAProblemPtr> GetProblems();
    }; // PdfAProblemClass

}}} // SolidFramework::Pdf::Reports

namespace SolidFramework { namespace Pdf { namespace Reports { 

    class PdfAProblemClassesCollection: public FrameworkObject
    {
    public:
        PdfAProblemClassesCollection();
        PdfAProblemClassesCollection(PdfAProblemClassesCollectionPtr other);
        PdfAProblemClassesCollection(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfAProblemClassesCollection() { Dispose(); }

        virtual void Dispose();

        virtual void Add(std::wstring key, PdfAProblemClassPtr val);

        virtual void Clear();

        virtual bool ContainsKey(std::wstring key);

        virtual int GetCount();

        bool Empty();

        virtual PdfAProblemClassPtr GetItem(std::wstring key);

        virtual bool Remove(std::wstring key);
    }; // PdfAProblemClassesCollection

}}} // SolidFramework::Pdf::Reports

namespace SolidFramework { namespace Pdf { namespace Reports { 

    // Represents PDF/A report
    class PdfAReport: public FrameworkObject
    {
    public:
        PdfAReport(std::wstring path);
        PdfAReport(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfAReport() { Dispose(); }

        virtual void Dispose();

        // Gets the name of the application.
        std::wstring GetApplicationName();

        int GetCount();

        // At least one of the results was an encrypted PDF
        bool GetEncrypted();

        std::vector<std::wstring> GetKeys();

        // At least one of the results had missing 'StructTreeRoot' (Tags)
        bool GetMissingStructTreeRoot();

        SolidFramework::Plumbing::ValidationMode GetMode();

        PdfAResultsCollectionPtr GetResults();

        // Saves to the specified file.
        void Save(std::wstring path);
    }; // PdfAReport

}}} // SolidFramework::Pdf::Reports

namespace SolidFramework { namespace Pdf { namespace Reports { 

    // Represents PDF/A result
    class PdfAResult: public FrameworkObject
    {
    public:
        PdfAResult(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfAResult() { Dispose(); }

        virtual void Dispose();

        PdfAProblemClassesCollectionPtr GetClasses();

        // Compares the specified objects.
        std::wstring Compare(PdfAResultPtr other, bool showWarnings);

        int GetCount();

        bool GetEncrypted();

        std::vector<std::wstring> GetKeys();

        bool GetMissingStructTreeRoot();
    }; // PdfAResult

}}} // SolidFramework::Pdf::Reports

namespace SolidFramework { namespace Pdf { namespace Reports { 

    class PdfAResultsCollection: public FrameworkObject
    {
    public:
        PdfAResultsCollection();
        PdfAResultsCollection(PdfAResultsCollectionPtr other);
        PdfAResultsCollection(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfAResultsCollection() { Dispose(); }

        virtual void Dispose();

        virtual void Add(std::wstring key, PdfAResultPtr val);

        virtual void Clear();

        virtual bool ContainsKey(std::wstring key);

        virtual int GetCount();

        bool Empty();

        virtual PdfAResultPtr GetItem(std::wstring key);

        virtual bool Remove(std::wstring key);
    }; // PdfAResultsCollection

}}} // SolidFramework::Pdf::Reports

namespace SolidFramework { namespace Pdf { namespace Transformers { 

    // Represents results of transformation
    class ITransformationResult: public FrameworkObject
    {
    public:
        ITransformationResult();
        ITransformationResult(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~ITransformationResult() { Dispose(); }

        virtual void Dispose();

        // Transformed document
        virtual SolidFramework::Pdf::PdfDocumentPtr GetDocument();

        bool IsEqual(ITransformationResultPtr value);

        // Result of transformation
        virtual SolidFramework::Interop::NativeError GetStatus();
    }; // ITransformationResult

}}} // SolidFramework::Pdf::Transformers

namespace SolidFramework { namespace Pdf { namespace Transformers { 

    // Implementation of ITransformationResult
    class TransformationResult: public ITransformationResult
    {
    protected:
        TransformationResult();
    public:
        TransformationResult(SolidFramework::Pdf::PdfDocumentPtr doc, SolidFramework::Interop::NativeError stat);
        TransformationResult(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~TransformationResult() { Dispose(); }

        virtual void Dispose();

        // Implementation of ITransformationResult property
        virtual SolidFramework::Pdf::PdfDocumentPtr GetDocument();

        // Implementation of ITransformationResult property
        virtual SolidFramework::Interop::NativeError GetStatus();

        static TransformationResultPtr DynamicCast(ITransformationResultPtr value);
    }; // TransformationResult

}}} // SolidFramework::Pdf::Transformers

namespace SolidFramework { namespace Pdf { namespace Transformers { 

    // Represents results of transformation
    class OCRTransformationResult: public TransformationResult
    {
    public:
        OCRTransformationResult(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~OCRTransformationResult() { Dispose(); }

        virtual void Dispose();

        // Transformed document
        int GetDocumentConfidentWordCount();

        // Transformed document
        int GetDocumentGraphicCount();

        // Transformed document
        int GetDocumentImageCount();

        // Transformed document
        int GetDocumentWordCount();

        // Transformed document
        int GetOCRPageDetailsPageCount();

        // Transformed document
        int GetPageConfidentWordCount(int nPageIndex);

        // Transformed document
        int GetPageGraphicCount(int nPageIndex);

        // Transformed document
        int GetPageImageCount(int nPageIndex);

        // Transformed document
        int GetPageWordCount(int nPageIndex);

        static TransformationResultPtr DynamicCast(ITransformationResultPtr value);
    }; // OCRTransformationResult

}}} // SolidFramework::Pdf::Transformers

namespace SolidFramework { namespace Pdf { namespace Transformers { 

    // Represents abstract base class for all pdf transformers
    class PdfTransformer: public FrameworkObject
    {
    protected:
        PdfTransformer();
    public:
        PdfTransformer(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfTransformer() { Dispose(); }

        virtual void Dispose();

        // Adds PdfDocument to the source for transformation
        virtual void AddDocument(SolidFramework::Pdf::PdfDocumentPtr document);

        // Clears defined source for transformation and old results
        void Clear();

        // Adds PdfDocument to the source for transformation from the path
        void ImportFromFile(std::wstring path);

        // Adds PdfDocuments to the source for transformation from the path to folder
        void ImportFromFolder(std::wstring path);

        // Gets the folder where generated files should be placed. 
        // If set then all documents will be saved in the folder
        // and closed if they were not opened. This is a typical use case if we
        // work with file paths instead of PdfDocuments
        std::wstring GetOutputFolder();

        // Sets the folder where generated files should be placed. 
        // If set then all documents will be saved in the folder
        // and closed if they were not opened. This is a typical use case if we
        // work with file paths instead of PdfDocuments
        void SetOutputFolder(std::wstring value);

        // Gets whether files should be overwritten if they exist.
        // This is only required if OutputFolder is specified.
        SolidFramework::Plumbing::OverwriteMode GetOverwriteMode();

        // Sets whether files should be overwritten if they exist.
        // This is only required if OutputFolder is specified.
        void SetOverwriteMode(SolidFramework::Plumbing::OverwriteMode value);

        std::vector<ITransformationResultPtr> GetResults();

        // Starts transformation
        virtual void Transform();

        // Starts transformation
        static ITransformationResultPtr Transform(SolidFramework::Pdf::PdfDocumentPtr document, PdfTransformerPtr transformer);
    }; // PdfTransformer

}}} // SolidFramework::Pdf::Transformers

namespace SolidFramework { namespace Pdf { namespace Transformers { 

    // Represents image watermark pdf transformation
    class ImageWatermarkTransformer: public PdfTransformer
    {
    public:
        ImageWatermarkTransformer();
        ImageWatermarkTransformer(SolidFramework::Converters::Plumbing::ImageWatermarkPtr watermark);
        ImageWatermarkTransformer(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~ImageWatermarkTransformer() { Dispose(); }

        virtual void Dispose();

        // Image watermark for adding
        SolidFramework::Converters::Plumbing::ImageWatermarkPtr GetImageWatermark();

        // Image watermark for adding
        void SetImageWatermark(SolidFramework::Converters::Plumbing::ImageWatermarkPtr value);

        static ImageWatermarkTransformerPtr DynamicCast(PdfTransformerPtr value);
    }; // ImageWatermarkTransformer

}}} // SolidFramework::Pdf::Transformers

namespace SolidFramework { namespace Pdf { namespace Transformers { 

    // Represents interface for recognizing pdf document
    class IRecognize: public PdfTransformer
    {
    protected:
        IRecognize();
    public:
        IRecognize(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~IRecognize() { Dispose(); }

        virtual void Dispose();

        // Does OCR for each page in the document
        bool GetOcrAlways();

        // Does OCR for each page in the document
        void SetOcrAlways(bool value);

        // Does auto rotate pages or not
        bool GetOcrAutoRotate();

        // Does auto rotate pages or not
        void SetOcrAutoRotate(bool value);

        // Sets OCR engine
        SolidFramework::Converters::Plumbing::TextRecoveryEngine GetOcrEngine();

        // Sets OCR engine
        void SetOcrEngine(SolidFramework::Converters::Plumbing::TextRecoveryEngine value);

        // Sets OCR image compression
        SolidFramework::Imaging::Plumbing::ImageCompression GetOcrImageCompression();

        // Sets OCR image compression
        void SetOcrImageCompression(SolidFramework::Imaging::Plumbing::ImageCompression value);

        // Sets language for OCR
        std::wstring GetOcrLanguage();

        // Sets language for OCR
        void SetOcrLanguage(std::wstring value);

        // Type of recognizing
        SolidFramework::Converters::Plumbing::OcrType GetOcrType();

        // Type of recognizing
        void SetOcrType(SolidFramework::Converters::Plumbing::OcrType value);

        // Starts the recognizing
        virtual SolidFramework::Interop::NativeError Recognize();

        static IRecognizePtr DynamicCast(PdfTransformerPtr value);
    }; // IRecognize

}}} // SolidFramework::Pdf::Transformers

namespace SolidFramework { namespace Pdf { namespace Transformers { 

    // Represents OCR pdf transformation
    class OcrTransformer: public IRecognize
    {
    public:
        OcrTransformer();
        OcrTransformer(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~OcrTransformer() { Dispose(); }

        virtual void Dispose();

        // Starts the recognizing. Same as Transform
        virtual SolidFramework::Interop::NativeError Recognize();

        static OcrTransformerPtr DynamicCast(PdfTransformerPtr value);
    }; // OcrTransformer

}}} // SolidFramework::Pdf::Transformers

namespace SolidFramework { namespace Pdf { namespace Transformers { 

    // Represents password security pdf transformation
    class PasswordSecurityTransformer: public PdfTransformer
    {
    public:
        PasswordSecurityTransformer();
        PasswordSecurityTransformer(std::wstring ownerPassword, SolidFramework::Pdf::EncryptionAlgorithm algorithm);
        PasswordSecurityTransformer(std::wstring ownerPassword, std::wstring userPassword, SolidFramework::Pdf::EncryptionAlgorithm algorithm, SolidFramework::Pdf::AccessPermissions permission);
        PasswordSecurityTransformer(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PasswordSecurityTransformer() { Dispose(); }

        virtual void Dispose();

        // Encryption algorithm for documents
        SolidFramework::Pdf::EncryptionAlgorithm GetEncryptionAlgorithm();

        // Encryption algorithm for documents
        void SetEncryptionAlgorithm(SolidFramework::Pdf::EncryptionAlgorithm value);

        // Owner password for documents
        std::wstring GetOwnerPassword();

        // Owner password for documents
        void SetOwnerPassword(std::wstring value);

        // Assess permission for user security
        SolidFramework::Pdf::AccessPermissions GetPermission();

        // Assess permission for user security
        void SetPermission(SolidFramework::Pdf::AccessPermissions value);

        // User password for documents
        std::wstring GetUserPassword();

        // User password for documents
        void SetUserPassword(std::wstring value);

        static PasswordSecurityTransformerPtr DynamicCast(PdfTransformerPtr value);
    }; // PasswordSecurityTransformer

}}} // SolidFramework::Pdf::Transformers

namespace SolidFramework { namespace Pdf { namespace Transformers { 

    // Represents text watermark pdf transformation
    class TextWatermarkTransformer: public PdfTransformer
    {
    public:
        TextWatermarkTransformer();
        TextWatermarkTransformer(SolidFramework::Converters::Plumbing::TextWatermarkPtr watermark);
        TextWatermarkTransformer(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~TextWatermarkTransformer() { Dispose(); }

        virtual void Dispose();

        // Text watermark for adding
        SolidFramework::Converters::Plumbing::TextWatermarkPtr GetTextWatermark();

        // Text watermark for adding
        void SetTextWatermark(SolidFramework::Converters::Plumbing::TextWatermarkPtr value);

        static TextWatermarkTransformerPtr DynamicCast(PdfTransformerPtr value);
    }; // TextWatermarkTransformer

}}} // SolidFramework::Pdf::Transformers

namespace SolidFramework { namespace Pdf { 

    // Represents Viewer preferences functionality
    class ViewerPreferences: public FrameworkObject
    {
    public:
        ViewerPreferences(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~ViewerPreferences() { Dispose(); }

        virtual void Dispose();

        // Gets a value indicating whether [center window].
        bool GetCenterWindow();

        // Gets a value indicating whether [center window].
        void SetCenterWindow(bool value);

        // Creates from the specified catalog.
        static ViewerPreferencesPtr Create(CatalogPtr catalog);

        // Gets a value indicating whether [display document title].
        bool GetDisplayDocumentTitle();

        // Gets a value indicating whether [display document title].
        void SetDisplayDocumentTitle(bool value);

        // Gets the duplex.
        Duplex GetDuplex();

        // Gets the duplex.
        void SetDuplex(Duplex value);

        // Gets a value indicating whether [fit window].
        bool GetFitWindow();

        // Gets a value indicating whether [fit window].
        void SetFitWindow(bool value);

        // Gets a value indicating whether [hide menu bar].
        bool GetHideMenuBar();

        // Gets a value indicating whether [hide menu bar].
        void SetHideMenuBar(bool value);

        // Gets a value indicating whether [hide tool bar].
        bool GetHideToolBar();

        // Gets a value indicating whether [hide tool bar].
        void SetHideToolBar(bool value);

        // Gets a value indicating whether [hide window UI].
        bool GetHideWindowUI();

        // Gets a value indicating whether [hide window UI].
        void SetHideWindowUI(bool value);

        // Gets the non full screen page mode.
        NonFullScreenPageMode GetNonFullScreenPageMode();

        // Gets the non full screen page mode.
        void SetNonFullScreenPageMode(NonFullScreenPageMode value);

        // Gets the number of copies.
        int GetNumberOfCopies();

        // Gets the number of copies.
        void SetNumberOfCopies(int value);

        // Gets a value indicating whether [pick tray by PDF size].
        bool GetPickTrayByPdfSize();

        // Gets a value indicating whether [pick tray by PDF size].
        void SetPickTrayByPdfSize(bool value);

        // Gets the print area.
        PageBoundary GetPrintArea();

        // Gets the print area.
        void SetPrintArea(PageBoundary value);

        // Gets the print clip.
        PageBoundary GetPrintClip();

        // Gets the print clip.
        void SetPrintClip(PageBoundary value);

        // Gets the print page range.
        SolidFramework::PageRangePtr GetPrintPageRange();

        // Gets the print page range.
        void SetPrintPageRange(SolidFramework::PageRangePtr value);

        // Gets the print scaling.
        PrintScaling GetPrintScaling();

        // Gets the print scaling.
        void SetPrintScaling(PrintScaling value);

        // Gets the reading direction.
        ReadingDirection GetReadingDirection();

        // Gets the reading direction.
        void SetReadingDirection(ReadingDirection value);

        // Gets the view area.
        PageBoundary GetViewArea();

        // Gets the view area.
        void SetViewArea(PageBoundary value);

        // Gets the view clip.
        PageBoundary GetViewClip();

        // Gets the view clip.
        void SetViewClip(PageBoundary value);
    }; // ViewerPreferences

}} // SolidFramework::Pdf

namespace SolidFramework { namespace Platform { 

    class Directory: public FrameworkObject
    {
        Directory() = delete;
    public:

        static std::vector<std::wstring> GetContents(std::wstring path, bool allDirectories);

        static bool CreateDirectory(std::wstring path);

        static std::wstring GetCurrentDirectory();

        static bool Delete(std::wstring path, bool recursive);

        static bool Delete(std::wstring path);

        static bool Exists(std::wstring path);

        static std::vector<std::wstring> GetFiles(std::wstring path, bool allDirectories);

        static std::vector<std::wstring> GetFolders(std::wstring path, bool allDirectories);
    }; // Directory

}} // SolidFramework::Platform

namespace SolidFramework { namespace Platform { 

    class File: public FrameworkObject
    {
        File() = delete;
    public:

        static bool Copy(std::wstring source, std::wstring destination, bool overwrite);

        static bool Copy(std::wstring source, std::wstring destination);

        static bool Delete(std::wstring path);

        static bool Exists(std::wstring path);

        static bool Move(std::wstring source, std::wstring destination, bool overwrite);

        static bool Move(std::wstring source, std::wstring destination);

        // Returns the contents of the specified file (assumes it is encoded as UTF8)
        static std::wstring ReadAllText(std::wstring path);

        // Writes the text to the specified file using UTF8 encoding
        static bool WriteAllText(std::wstring path, std::wstring text);
    }; // File

}} // SolidFramework::Platform

namespace SolidFramework { namespace Platform { 

    class Path: public FrameworkObject
    {
        Path() = delete;
    public:

        static std::wstring ChangeExtension(std::wstring path, std::wstring newExtension);

        static std::wstring ChangeFileName(std::wstring path, std::wstring newFileName);

        static std::wstring Combine(std::wstring first, std::wstring second);

        static std::wstring GetDirectoryName(std::wstring path);

        static std::wstring GetExecutablePath();

        static std::wstring GetExtension(std::wstring path);

        static std::wstring GetFileName(std::wstring path);

        static std::wstring GetFileNameWithoutExtension(std::wstring path);

        static std::wstring GetFullPath(std::wstring path);

        static std::wstring RemoveBackslash(std::wstring path);

        static bool IsRooted(std::wstring path);
    }; // Path

}} // SolidFramework::Platform

namespace SolidFramework { namespace Platform { 

    class Platform: public FrameworkObject
    {
        Platform() = delete;
    public:
        typedef bool (SOLIDCALLBACK *TempFileIsAboutToBeCreatedCallback)(const wchar_t*);

        // Gets the directory where the application data is be stored.
        static std::wstring GetAppDataDirectory();

        // Sets the directory where the application data is be stored.
        static void SetAppDataDirectory(std::wstring path);

        // Gets the build number for this version of SolidFramework (e.g. "8680")
        static std::wstring GetBuild();

        // Gets the directory where the Character Mapping information is stored. It is unlikely that you will need to modify this. The default value is the folder "CMap" Within the folder "Resources" within the SupportDirectory.
        static std::wstring GetCMapDirectory();

        // Sets the directory where the Character Mapping information is stored. It is unlikely that you will need to modify this. The default value is the folder "CMap" Within the folder "Resources" within the SupportDirectory.
        static void SetCMapDirectory(std::wstring path);

        // Gets the directory where the Color Profile is stored. It is unlikely that you will need to modify this. The default value is the folder "Resources" within the SupportDirectory.
        static std::wstring GetColorProfileDirectory();

        // Sets the directory where the Color Profile is stored. It is unlikely that you will need to modify this. The default value is the folder "Resources" within the SupportDirectory.
        static void SetColorProfileDirectory(std::wstring path);

        // Linux only: Gets the directory which should be serached for fonts in addition to those in the system Fonts folder.
        // This is not implemented for Windows versions of the SDK.
        static std::wstring GetFontDirectory();

        // Linux only: Sets the directory which should be serached for fonts in addition to those in the system Fonts folder.
        // This is not implemented for Windows versions of the SDK.
        static void SetFontDirectory(std::wstring path);

        // Gets the location of a file (usually called fonts.pdf) that contains information about the fonts that should be considered 
        // as candidates when reconstructing files. 
        // If this is specified then other fonts available on the system will be ignored.
        // Note that the file must have a specific structure, and as such you cannot use just any PDF for this purpose.
        // Please contact support@soliddocuments.com for information about how to create a Fonts Database File.
        static std::wstring GetFontsDataBaseFile();

        // Gets the location of a file (usually called fonts.pdf) that contains information about the fonts that should be considered 
        // as candidates when reconstructing files. 
        // If this is specified then other fonts available on the system will be ignored.
        // Note that the file must have a specific structure, and as such you cannot use just any PDF for this purpose.
        // Please contact support@soliddocuments.com for information about how to create a Fonts Database File.
        static void SetFontsDataBaseFile(std::wstring path);

        // Gets the full version string for this version of SolidFramework (e.g. "9.2.8680.1")
        static std::wstring GetFullVersion();

        // Gets the name of the logfile that is being generated by Solid Framework. If this value is empty then logging is not
        // occurring.
        // This is an alternative method of getting SolidFramework.Plumbing.Logging.Instance.Path;
        static std::wstring GetLogFilename();

        // Sets the name of the logfile that Solid Framework should generate. If this value is set then logging is enabled.
        // This has the same effect as setting SolidFramework.Plumbing.Logging.Instance.Path = path;
        static void SetLogFilename(std::wstring path);

        // Sets whether SolidFramework should accept both '\' and '/' as directory separators
        // Defaults to false.
        static void SupportPlatformIndependentPaths(bool value);

        // Gets the path to the directory that this process will save its temporary files into
        static std::wstring GetTempDirectory();

        static void SetTempFileIsAboutToBeCreatedCallback(TempFileIsAboutToBeCreatedCallback callback);

        // Unloads the converters and cleans up the temporary files
        static void Terminate();

        // Gets the version string for this version of SolidFramework (e.g. "9.2")
        static std::wstring GetVersion();

        // Gets the level of messages that will be logged. It is unlikely that you will need to change this.
        static SolidFramework::Plumbing::WarningLevelType GetWarningLevel();

        // Sets the level of messages that will be logged. It is unlikely that you will need to change this.
        static bool SetWarningLevel(SolidFramework::Plumbing::WarningLevelType val);

        // Writes the specific text to the logfile that is generated by Solid Framework.
        static void WriteToLog(std::wstring content);
        // always call this API first (and only call once)
        static bool SetSupportDirectory(const std::wstring path);
    
        static void OutputDebugString(const std::wstring message);
    }; // Platform

}} // SolidFramework::Platform

namespace SolidFramework { namespace Plumbing { 

    // Represents wrapper other document
    class Document: public FrameworkObject
    {
    protected:
        Document();
    public:
        Document(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~Document() { Dispose(); }

        virtual void Dispose();

        virtual SolidFramework::Interop::NativeError Close();

        // Gets the catalog.
        virtual ICatalogPtr GetCatalog();

        // Creates this empty instance.
        virtual void Create();

        // Gets a value indicating whether this Document is dirty.
        bool GetDirty();

        // Gets a value indicating whether this Document is dirty.
        void SetDirty(bool value);

        // Gets the extension.
        virtual std::wstring GetExtension();

        // Gets a value indicating whether this document is loaded.
        bool IsLoaded();

        // Opens the document. This will throw an exception if the path has not already been specified in the constructor.
        void Open();

        // Opens the document. This will throw an exception if the path has not already been specified in the constructor.
        void Open(std::wstring password);

        // Gets the overwrite mode.
        OverwriteMode GetOverwriteMode();

        // Gets the overwrite mode.
        void SetOverwriteMode(OverwriteMode value);

        // Gets the path.
        std::wstring GetPath();

        // Gets the path.
        void SetPath(std::wstring value);

        bool GetRequiresOptimization();

        void SetRequiresOptimization(bool value);

        // Saves the document
        void Save();

        // Saves the document
        void Save(OverwriteMode mode);

        // Saves the document
        void Save(OverwriteMode mode, bool useOptimizer);

        // Saves the document to specified path
        void SaveAs(std::wstring path);

        // Saves to specified path
        void SaveAs(std::wstring path, OverwriteMode mode);

        // Saves to specified path
        void SaveAs(std::wstring path, OverwriteMode mode, bool useOptimizer);

        // Saves and apply optimization
        void SaveOptimized();

        // Saves and apply optimization
        void SaveOptimized(OverwriteMode mode);

        // Saves to specified path and apply optimization
        void SaveOptimizedAs(std::wstring path);

        // Saves to specified path and apply optimization
        void SaveOptimizedAs(std::wstring path, OverwriteMode mode);
    }; // Document

}} // SolidFramework::Plumbing

namespace SolidFramework { namespace Pdf { 

    class PdfDocument: public SolidFramework::Plumbing::Document
    {
    public:
        PdfDocument();
        PdfDocument(std::wstring path);
        PdfDocument(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfDocument() { Dispose(); }

        virtual void Dispose();

        virtual SolidFramework::Interop::NativeError Close();

        SolidFramework::Interop::NativeError AddTextWatermark(std::wstring strWatermarkText, std::wstring strURL, bool bSerifFont, float angle, bool bFill, int dwColor, bool bStroke, float lineWeight, int zorder, int vpos, int hpos);

        // Appends the specified PDF document to append.
        void Append(PdfDocumentPtr pdfDocumentToAppend);

        // Gets the authentication mode.
        AuthenticationModeType GetAuthenticationMode();

        // Gets the catalog.
        virtual SolidFramework::Plumbing::ICatalogPtr GetCatalog();

        // Gets the catalog dictionary.
        Plumbing::PdfDictionaryPtr GetCatalogDictionary();

        bool CheckOwnerPassword(std::wstring password);

        bool CheckUserPassword(std::wstring password);

        // Decodes all streams.
        void DecodeAllStreams();

        // Gets the encryption algorithm.
        EncryptionAlgorithm GetEncryptionAlgorithm();

        SolidFramework::Interop::NativeError EnterOwnerAuthenticationMode(std::wstring ownerPassword);

        // Gets the extension.
        virtual std::wstring GetExtension();

        // Gets the fonts used in this document.
        std::vector<Plumbing::PdfFontPtr> GetFonts();

        // Gets the ID.
        Plumbing::PdfArrayPtr GetID();

        // Gets the ID array.
        Plumbing::PdfArrayPtr GetIDArray();

        // Gets the info.
        InfoPtr GetInfo();

        // Gets the info dictionary.
        Plumbing::PdfDictionaryPtr GetInfoDictionary();

        // InquireUserPasswordByOwnerPassword returns user password by owner password for password security handlers. Works for revisions prior to Acrobat 9. 
        // In case of failure return null string
        std::wstring InquireUserPasswordByOwnerPassword(std::wstring ownerPassword);

        // Determines if the PDF is "fast web view" using the header
        bool IsLinearized();

        // Gets a value indicating whether this document is modified.
        bool IsModified();

        // Gets the native data base.
        HANDLE NativeDataBase();

        // Sets the ID to use when the document is saved.
        void SetOutputID(const std::vector<BYTE> & permanentIdentifier, const std::vector<BYTE> & changingIdentifier);

        int GetPageCount();

        // Gets the pages.
        std::vector<Plumbing::PdfPagePtr> GetPages();

        // Gets the pages.
        std::vector<Plumbing::PdfPagePtr> GetPages(int nFrom, int nTo);

        bool IsPdfLoaded();

        // Gets the permissions.
        AccessPermissions GetPermissions();

        SolidFramework::Interop::NativeError RecognizeDocument(std::wstring OCRLang, SolidFramework::Converters::Plumbing::TextRecovery textRecovery, SolidFramework::Converters::Plumbing::OcrType ocrType, bool pageAutoRotate, SolidFramework::Imaging::Plumbing::ImageCompression imageCompression);

        // Removes "tagging" from the PdfDocument. 
        // If the PdfDocument was not tagged then this has no effect.
        void RemoveStructTreeRoot();

        // Gets the root.
        Plumbing::PdfDictionaryPtr GetRoot();

        // Saves the specified mode.
        void Save(SolidFramework::Plumbing::OverwriteMode mode, SolidFramework::PageRangePtr pageRange);

        void SaveAsProtected(std::wstring path, PdfSecurityHandlerPtr securityHandler, SolidFramework::Plumbing::OverwriteMode overwriteMode, bool shouldOptimize);

        void SaveAsProtected(std::wstring path, PdfSecurityHandlerPtr securityHandler, SolidFramework::Plumbing::OverwriteMode overwriteMode);

        void SaveAsProtected(std::wstring path, PdfSecurityHandlerPtr securityHandler);

        void SaveProtected(PdfSecurityHandlerPtr securityHandler, SolidFramework::Plumbing::OverwriteMode overwriteMode, bool shouldOptimize);

        void SaveProtected(PdfSecurityHandlerPtr securityHandler, SolidFramework::Plumbing::OverwriteMode overwriteMode);

        void SaveProtected(PdfSecurityHandlerPtr securityHandler);

        // Gets the trailer.
        Plumbing::PdfDictionaryPtr GetTrailer();

        // Gets the version using either Catalog.Version or file header.
        std::wstring GetVersion();

        static PdfDocumentPtr DynamicCast(SolidFramework::Plumbing::DocumentPtr value);
    }; // PdfDocument

}} // SolidFramework::Pdf

namespace SolidFramework { namespace Plumbing { 

    // Interface for PDF catalog
    class ICatalog: public FrameworkObject
    {
    public:
        ICatalog();
        ICatalog(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~ICatalog() { Dispose(); }

        virtual void Dispose();

        // Gets the current page number
        virtual int GetCurrentPage();

        // Gets the pages collection
        virtual IPagesCollectionPtr GetPages();
    }; // ICatalog

}} // SolidFramework::Plumbing

namespace SolidFramework { namespace Pdf { 

    // Represents PDF catalog
    class Catalog: public SolidFramework::Plumbing::ICatalog
    {
    public:
        Catalog(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~Catalog() { Dispose(); }

        virtual void Dispose();

        // Creates the specified document.
        static SolidFramework::Plumbing::ICatalogPtr Create(PdfDocumentPtr doc);

        // Gets the current page number
        virtual int GetCurrentPage();

        // Gets the current page number
        virtual void SetCurrentPage(int value);

        Plumbing::PdfDictionaryPtr GetDests();

        // Is this a Tagged PDF? (Must have MarkInfo with Marked true).
        bool IsTagged();

        Plumbing::PdfDictionaryPtr GetNames();

        // Gets the OpenAction if present.
        Plumbing::PdfItemPtr GetOpenAction();

        // Gets the OpenAction if present.
        void SetOpenAction(Plumbing::PdfItemPtr value);

        // Gets the optional content properties.
        OptionalContentPropertiesPtr GetOptionalContentProperties();

        // Gets the outlines.
        Plumbing::PdfOutlinesPtr GetOutlines();

        // Gets the page layout.
        PageLayout GetPageLayout();

        // Gets the page layout.
        void SetPageLayout(PageLayout value);

        // Gets the page mode.
        PageMode GetPageMode();

        // Sets the page mode.
        void SetPageMode(PageMode value);

        // Gets the pages collection
        virtual SolidFramework::Plumbing::IPagesCollectionPtr GetPages();

        // Removes the viewer preferences.
        void RemoveViewerPreferences();

        // Gets the type.
        std::wstring GetType();

        // Gets the version.
        std::wstring GetVersion();

        // Gets the viewer preferences.
        ViewerPreferencesPtr GetViewerPreferences();

        // Gets the viewer preferences.
        ViewerPreferencesPtr GetViewerPreferences(bool create);

        static CatalogPtr DynamicCast(SolidFramework::Plumbing::ICatalogPtr value);
    }; // Catalog

}} // SolidFramework::Pdf

namespace SolidFramework { namespace Plumbing { 

    // Interface for PDF catalog
    class IPagesCollection: public FrameworkObject
    {
    public:
        IPagesCollection();
        IPagesCollection(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~IPagesCollection() { Dispose(); }

        virtual void Dispose();

        // Gets the page count.
        int GetPageCount();

        static IPagesCollectionPtr DynamicCast(SolidFramework::Pdf::Plumbing::PdfPagesPtr value);
    }; // IPagesCollection

}} // SolidFramework::Plumbing

namespace SolidFramework { 

    // Describes and manipulates progress event arguments.
    class ProgressEventArgs: public FrameworkObject
    {
    public:
        ProgressEventArgs();
        ProgressEventArgs(int progress, int maxProgress, Interop::SolidErrorCodes statusCode, std::wstring statusDescription);
        ProgressEventArgs(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~ProgressEventArgs() { Dispose(); }

        virtual void Dispose();

        // Cancels the current operation and causes it to terminate at the next appropriate time.
        void Cancel();

        // Indicates whether the current operation has been canceled and will terminate at an appropriate time.
        bool IsCanceled();

        // Gets the max progress.
        int GetMaxProgress();

        // Sets the max progress.
        void SetMaxProgress(int value);

        // Gets the progress.
        int GetProgress();

        // Sets the progress.
        void SetProgress(int value);

        // Gets the status code.
        Interop::SolidErrorCodes GetStatusCode();

        // Gets the status code.
        void SetStatusCode(Interop::SolidErrorCodes value);

        // Gets the status description.
        std::wstring GetStatusDescription();

        // Gets the status description.
        void SetStatusDescription(std::wstring value);
    }; // ProgressEventArgs

} // SolidFramework

namespace SolidFramework { 

    // Describes and manipulates warning event arguments.
    class WarningEventArgs: public FrameworkObject
    {
    protected:
        WarningEventArgs();
    public:
        WarningEventArgs(std::wstring message);
        WarningEventArgs(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~WarningEventArgs() { Dispose(); }

        virtual void Dispose();

        // Gets the message.
        std::wstring GetMessage();

        // Gets the message.
        void SetMessage(std::wstring value);

        // Gets the warning.
        WarningType GetWarning();
    }; // WarningEventArgs

} // SolidFramework

namespace SolidFramework { 

    // Represents types of PdfA Warning Event Args.
    class PdfAWarningEventArgs: public WarningEventArgs
    {
    public:
        PdfAWarningEventArgs(std::wstring message);
        PdfAWarningEventArgs(HANDLE cPtr, bool memoryOwn) { Wrap(cPtr, memoryOwn); }
        virtual ~PdfAWarningEventArgs() { Dispose(); }

        virtual void Dispose();

        static PdfAWarningEventArgsPtr DynamicCast(WarningEventArgsPtr value);
    }; // PdfAWarningEventArgs

} // SolidFramework


#endif // __SOLIDFRAMEWORK_H__
