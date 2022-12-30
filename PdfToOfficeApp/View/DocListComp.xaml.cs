using System.Windows.Controls;

namespace PdfToOfficeApp
{
    /// <summary>
    /// Interaction logic for DocListComp.xaml
    /// </summary>
    public partial class DocListComp : UserControl
    {
        public DocListComp()
        {
            InitializeComponent();
        }

        public MainViewModel GetModel()
        {
            return DataContext as MainViewModel;
        }
    }
}
