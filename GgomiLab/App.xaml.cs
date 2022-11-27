using System.Windows;

namespace GgomiLab
{
    /// <summary>
    /// Interaction logic for App.xaml
    /// </summary>
    public partial class App : Application
    {
        public DocList DocListModel = new DocList();

        App()
        {
            DocListModel.Add(new Doc("hi.pdf"));
        }
    }
}
