namespace PdfToOfficeApp
{
    public class PdfToOffice
    {
        public int RunSample(string path)
        {
            var module = new PdfToOfficeAppModule.PdfToOfficeAppModule();

            return module.RunSample(path);
        }
    }
}
