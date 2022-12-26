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
            Loaded += SelectFormat_Loaded;
        }

        private void SelectFormat_Loaded(object sender, RoutedEventArgs e)
        {
            var m = GetModel();
            if(m != null)
            {

            }
        }

        private void RadioButton_Checked(object sender, RoutedEventArgs e)
        {
            var rButton = sender as RadioButton;
            var m = GetModel();
            if (m != null)
            {
                string format = rButton.Name.Replace("IDC_RadioButton_", "");
                FileFormat fileFormat = (FileFormat)Enum.Parse(typeof(FileFormat), format);

                if (fileFormat == FileFormat.IMAGE)
                {
                    fileFormat = GetModel().ImageFormat;
                }
                else
                {
                    GetModel().IsImage = false;
                }
                GetModel().SelectedFileFormat = fileFormat;
            }
        }

        public MainViewModel GetModel()
        {
            return DataContext as MainViewModel;
        }

        private void RadioButton_Image_Checked(object sender, RoutedEventArgs e)
        {
            var m = GetModel();
            if (m != null)
            {
                GetModel().IsImage = true;
                var rButton = sender as RadioButton;
                string format = rButton.Name.Replace("IDC_RadioButton_", "");
                FileFormat imageFormat = (FileFormat)Enum.Parse(typeof(FileFormat), format);

                GetModel().ImageFormat = imageFormat;
                GetModel().SelectedFileFormat = imageFormat;
            }
        }
    }
}
