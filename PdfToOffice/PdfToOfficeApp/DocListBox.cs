using System;
using System.Collections.Generic;
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

            AddCommandHandlers(AddDoc, OnAddDoc, CanAddDoc);
        }

        protected override void OnPreviewDrop(DragEventArgs e)
        {
            e.Handled = true;
            var fileNames = e.Data.GetData(DataFormats.FileDrop, true) as string[];
            AddDoc.Execute(fileNames, this);
        }

        public static RoutedCommand AddDoc = new RoutedCommand("AddDoc", typeof(DocListBox));

        private void OnAddDoc(object sender, ExecutedRoutedEventArgs e)
        {
            var fileNames = e.Parameter as string[];
            if (fileNames == null)
            {
                return;
            }

            var itemsSource = ItemsSource as ICollection<Doc>;
            foreach (var file in fileNames)
            {
                itemsSource.Add(new Doc(file));
            }
        }

        private void CanAddDoc(object sender, CanExecuteRoutedEventArgs e)
        {
            e.CanExecute = true;
        }

        private void AddCommandHandlers(RoutedCommand command, ExecutedRoutedEventHandler execute, CanExecuteRoutedEventHandler canExecute)
        {
            CommandBindings.Add(
                new CommandBinding(command,
                    new ExecutedRoutedEventHandler(execute),
                    new CanExecuteRoutedEventHandler(canExecute)));
        }
    }
}

