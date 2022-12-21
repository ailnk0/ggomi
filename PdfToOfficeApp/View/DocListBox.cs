using System;
using System.Collections.Specialized;
using System.Windows.Controls;

namespace PdfToOfficeApp
{
    public partial class DocListBox : ListBox
    {
        protected override void OnInitialized(EventArgs e)
        {
            base.OnInitialized(e);

            if (GetModel() != null && GetModel().Docs != null)
            {
                GetModel().Docs.ListSelectedItems = SelectedItems;
            }
        }

        protected override void OnItemsChanged(NotifyCollectionChangedEventArgs e)
        {
            if (!(ItemsSource is DocList itemsSource))
                return;

            if (itemsSource.Count > 0)
            {
                GetModel().Status = AppStatus.Ready;
            }
            else
            {
                GetModel().Status = AppStatus.Init;
            }
        }

        protected override void OnSelectionChanged(SelectionChangedEventArgs e)
        {
        }

        public MainViewModel GetModel()
        {
            return DataContext as MainViewModel;
        }
    }
}
