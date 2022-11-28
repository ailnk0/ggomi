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
            this.AllowDrop = true;
        }

        protected override void OnItemsChanged(NotifyCollectionChangedEventArgs e)
        {
            IList<Doc> itemsSource = ItemsSource as IList<Doc>;
            if (itemsSource == null)
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

        public MainViewModel GetModel()
        {
            return DataContext as MainViewModel;
        }
    }
}
