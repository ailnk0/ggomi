using System.Windows;

namespace PdfToOfficeApp
{
    /// <summary>
    /// Interaction logic for ConfigWindow.xaml
    /// </summary>
    public partial class ConfigWindow : Window
    {
        public ConfigWindow()
        {
            InitializeComponent();
        }

        public MainViewModel GetModel()
        {
            return DataContext as MainViewModel;
        }
    }
}
