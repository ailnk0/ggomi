#pragma once

#include <string>

namespace HpdfToOffice
{

typedef std::wstring String;

enum class ConversionFormat
{
    hwp,
    hwpx,

    show,
    cell,

    docx,
    pptx,
    xlsx,
    txt,
    rtf,
    html,

    bmp,
    jpeg,
    png,
    tiff,
    gif,
};

enum class ErrorStatus
{
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
    // No tables were found to extract. This will occur when converting to Excel while ignoring non-table content, if
    // only non-table content exists.
    NoTablesToExtract = 15,
    // No images were found to extract.
    NoImagesToExtract = 16,
    // The source document has PDFA Errors, but it was posible to correct these in the reconstructed file. For further
    // details look at the PdfAConversionStatus.
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
};

} // namespace HpdfToOffice
