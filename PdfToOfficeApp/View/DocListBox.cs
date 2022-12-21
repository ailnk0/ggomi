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

            GetModel().SelectedItems = SelectedItems;
        }

        protected override void OnItemsChanged(NotifyCollectionChangedEventArgs e)
        {
            base.OnItemsChanged(e);

            if (GetModel().Docs.Count > 0)
            {
                GetModel().Status = AppStatus.Ready;
            }
            else
            {
                GetModel().Status = AppStatus.Init;
            }
        }

        public MainViewModel GetModel()
        {
            return DataContext as MainViewModel;
        }
    }
}
