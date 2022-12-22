using System;
using System.Windows;
using System.Windows.Controls;

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

        private void RadioButton_Checked(object sender, RoutedEventArgs e)
        {
            var rButton = sender as RadioButton;
            string format = rButton.Name.Replace("IDC_RadioButton_", "");
            FileFormat fileFormat = (FileFormat)Enum.Parse(typeof(FileFormat), format);

            if(fileFormat == FileFormat.IMAGE)
            {
                fileFormat = GetModel().Docs.ImageFormat;
            }
            GetModel().Docs.SelectedFileFormat = fileFormat;
        }

        public MainViewModel GetModel()
        {
            return DataContext as MainViewModel;
        }

        private void RadioButton_Image_Checked(object sender, RoutedEventArgs e)
        {
            var rButton = sender as RadioButton;
            string format = rButton.Name.Replace("IDC_RadioButton_", "");
            FileFormat imageFormat = (FileFormat)Enum.Parse(typeof(FileFormat), format);

            GetModel().Docs.ImageFormat = imageFormat;
            GetModel().Docs.SelectedFileFormat = imageFormat;
        }
    }
}
