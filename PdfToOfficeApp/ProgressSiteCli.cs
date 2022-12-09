using PdfToOfficeAppModule;

namespace PdfToOfficeApp
{
    public class ProgressSiteCli : IProgressSiteCli
    {
        readonly Doc doc = null;

        public ProgressSiteCli(Doc _doc)
        {
            doc = _doc;
        }

        public void SetPercent(int percent)
        {
            doc.ProgressValue = percent;
        }
    }
}
