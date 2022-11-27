using PdfToOfficeAppModule;

namespace PdfToOfficeApp
{
    public class PdfToOffice
    {
        public ErrorStatus DoWordConversion(string path, string pwd)
        {
            var module = new PdfToOfficeProxy();

            ErrorStatus status = module.InitializeSolidFramework();
            if (status != ErrorStatus.Success)
            {
                return status;
            }

            return module.DoWordConversion(path, pwd);
        }
    }
}
