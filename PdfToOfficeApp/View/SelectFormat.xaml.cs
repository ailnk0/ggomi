using System.Windows;
using System.Windows.Controls;
using PdfToOfficeAppModule;

namespace PdfToOfficeApp
{
    /// <summary>
    /// SelectFormat.xaml에 대한 상호 작용 논리
    /// </summary>
    public partial class SelectFormat : UserControl
    {
        public SelectFormat()
        {
            InitializeComponent();
        }

        public MainViewModel GetModel()
        {
            return DataContext as MainViewModel;
        }

        private void IDC_SelXlsx_Click(object sender, RoutedEventArgs e)
        {
            GetModel().ConvFileType = FILE_TYPE.XLSX;
        }

        private void IDC_SelPptx_Click(object sender, RoutedEventArgs e)
        {
            GetModel().ConvFileType = FILE_TYPE.PPTX;
        }

        private void IDC_SelDocx_Click(object sender, RoutedEventArgs e)
        {
            GetModel().ConvFileType = FILE_TYPE.DOCX;
        }

        private void IDC_SelImg_Click(object sender, RoutedEventArgs e)
        {
            GetModel().ConvFileType = FILE_TYPE.IMAGE;
        }

        private void IDC_SelPng_Click(object sender, RoutedEventArgs e)
        {
            GetModel().ConvImgType = IMG_TYPE.PNG;
        }

        private void IDC_SelJpg_Click(object sender, RoutedEventArgs e)
        {
            GetModel().ConvImgType = IMG_TYPE.JPEG;
        }

        private void IDC_SelGif_Click(object sender, RoutedEventArgs e)
        {
            GetModel().ConvImgType = IMG_TYPE.GIF;
        }

        private void IDC_SelImgType_PreviewMouseDown(object sender, System.Windows.Input.MouseButtonEventArgs e)
        {
            GetModel().ConvFileType = FILE_TYPE.IMAGE;
        }
    }
}
