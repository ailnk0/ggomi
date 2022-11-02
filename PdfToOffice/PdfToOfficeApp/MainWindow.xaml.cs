using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

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
            if (resultCode == 0) {
                pdftooffice_result.Content = "Conversion is done.";
            }
            else {
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
