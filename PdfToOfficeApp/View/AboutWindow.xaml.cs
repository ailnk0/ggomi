using System;
using System.Windows;

namespace PdfToOfficeApp
{
    /// <summary>
    /// Interaction logic for ConfigWindow.xaml
    /// </summary>
    public partial class AboutWindow : Window
    {
        public AboutWindow()
        {
            InitializeComponent();
        }

        protected override void OnInitialized(EventArgs e)
        {
            base.OnInitialized(e);
        }

        public MainViewModel GetModel()
        {
            return DataContext as MainViewModel;
        }
    }
}
