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

            Loaded += DocListBox_Loaded;
        }

        private void DocListBox_Loaded(object sender, System.Windows.RoutedEventArgs e)
        {
            if (GetModel() == null)
            {
                return;
            }

            GetModel().SelectedItems = SelectedItems;
        }

        protected override void OnItemsChanged(NotifyCollectionChangedEventArgs e)
        {
            base.OnItemsChanged(e);

            if (GetModel() == null)
            {
                return;
            }

            if (GetModel().Docs.Count > 0)
            {
                GetModel().AppStatus = APP_STATUS.READY;
            }
            else
            {
                GetModel().AppStatus = APP_STATUS.INIT;
            }
        }

        public MainViewModel GetModel()
        {
            return DataContext as MainViewModel;
        }
    }
}
