using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Windows.Controls;

namespace PdfToOfficeApp
{
    public partial class DocListBox : ListBox
    {
        protected override void OnInitialized(EventArgs e)
        {
            base.OnInitialized(e);
        }

        protected override void OnItemsChanged(NotifyCollectionChangedEventArgs e)
        {
            //IList<Doc> itemsSource = ItemsSource as IList<Doc>;
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
            GetModel().Docs.ListSelectedItems = SelectedItems;
        }

        public MainViewModel GetModel()
        {
            return DataContext as MainViewModel;
        }
    }
}
