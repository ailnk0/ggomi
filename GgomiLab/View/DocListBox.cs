using System.Collections.Specialized;
using System.Windows.Controls;
using System.Windows.Input;

namespace GgomiLab
{
    public partial class DocListBox : ListBox
    {
        public static RoutedCommand AddDoc = new RoutedCommand("AddDoc", typeof(DocListBox));

        public DocListBox()
        {
            AddCommandHandlers();
        }

        protected void AddCommandHandlers()
        {
            CommandUtil.AddCommandHandler(this, AddDoc, OnAddDoc, CanAddDoc);
        }

        public MainViewModel GetModel()
        {
            return DataContext as MainViewModel;
        }

        protected override void OnItemsChanged(NotifyCollectionChangedEventArgs e)
        {
            MainViewModel model = GetModel();
            if (model == null)
            {
                return;
            }

            if (model.Docs.Count > 0)
            {
                model.Status = AppStatus.Ready;
            }
            else
            {
                model.Status = AppStatus.Init;
            }
        }

        private void OnAddDoc(object sender, ExecutedRoutedEventArgs e)
        {
            var fileNames = e.Parameter as string[];
            if (fileNames == null)
            {
                return;
            }

            foreach (var file in fileNames)
            {
                GetModel().Docs.Add(new Doc(file));
            }
        }

        private void CanAddDoc(object sender, CanExecuteRoutedEventArgs e)
        {
            e.CanExecute = true;
        }
    }
}
