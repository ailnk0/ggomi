using System.Windows;
using System.Windows.Controls;

namespace PdfToOfficeApp
{
    /// <summary>
    /// SelectFormat.xaml에 대한 상호 작용 논리
    /// </summary>
    public partial class SelectFormat : UserControl
    {
        public static bool bRadioSelected = false;
        public SelectFormat()
        {
            InitializeComponent();
        }

        private void RadioButton_Checked(object sender, RoutedEventArgs e)
        {
            var rButton = sender as RadioButton;

            if (rButton.IsChecked == true)
                bRadioSelected = true;
            else
                bRadioSelected = false;
        }
    }
}
