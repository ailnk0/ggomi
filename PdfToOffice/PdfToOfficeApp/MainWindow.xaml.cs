using System.Windows;

namespace PdfToOfficeApp
{
    /// <summary>
    /// MainWindow.xaml에 대한 상호 작용 논리
    /// </summary>
    public partial class MainWindow : Window
    {
        public MainWindow()
        {
            InitializeComponent();

            int resultCode = RunSample();
            if (resultCode == 0)
            {
                pdftooffice_result.Content = "Conversion is done.";
            }
            else
            {
                pdftooffice_result.Content = "Conversion is failed.";
            }
        }

        public int RunSample()
        {
            PdfToOffice pdfToOffice = new PdfToOffice();
            return pdfToOffice.RunSample();
        }
    }
}
