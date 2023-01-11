using System;
using System.Collections.Specialized;
using System.IO;
using System.Windows.Controls;
using System.Windows.Input;
using PdfToOfficeAppModule;

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

        protected override void OnMouseDoubleClick(MouseButtonEventArgs e)
        {
            if (GetModel().AppStatus == APP_STATUS.COMPLETED)
            {
                var process = new System.Diagnostics.Process();
                Doc doc = (Doc)this.SelectedItem;

                try
                {
                    process.StartInfo.FileName = doc.OutPath;
                    process.Start();
                }
                catch (Exception ex)
                {
                    Console.WriteLine(ex.Message);
                }
            }
        }

        public MainViewModel GetModel()
        {
            return DataContext as MainViewModel;
        }
    }
}
