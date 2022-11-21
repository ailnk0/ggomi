﻿using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

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
            if (Application.Current == null)
                return;

            var mainWindow = Application.Current.MainWindow as MainWindow;
            var itemsSource = ItemsSource as IList<Doc>;
            if (itemsSource.Count > 0)
            {
                mainWindow.Status = AppStatus.Ready;
            }
            else
            {
                mainWindow.Status = AppStatus.Init;
            }
        }
    }
}
