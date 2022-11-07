using Microsoft.Win32;
using System;
using System.Collections.Generic;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

namespace GgomiLab
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        private IList<Doc> DocListData;

        public MainWindow()
        {
            InitializeComponent();
        }

        protected override void OnInitialized(EventArgs e)
        {
            base.OnInitialized(e);

            DocListData = FindResource("doclistbox-data") as IList<Doc>;

            AddCommandHandlers(ApplicationCommands.Open, OnOpen, CanOpen);
            AddCommandHandlers(Convert, OnConvert, CanConvert);
        }

        public static RoutedCommand Convert = new RoutedCommand("Convert", typeof(Button));

        private void OnOpen(object sender, RoutedEventArgs e)
        {
            var dialog = new OpenFileDialog();
            dialog.Multiselect = true;
            bool? result = dialog.ShowDialog();
            if (result != true)
            {
                return;
            }

            DocListBox.AddDoc.Execute(dialog.FileNames, IDC_DocList);
        }

        private void CanOpen(object sender, CanExecuteRoutedEventArgs e)
        {
            e.CanExecute = true;
        }

        private void OnConvert(object sender, RoutedEventArgs e)
        {
            var appModule = new PdfToOfficeAppModule.PdfToOfficeAppModule();

            int code = appModule.RunSample();
            if (code != 0)
            {
                MessageBox.Show(string.Format("Conversion Failed.\n[{0}] {1}", code, Utils.GetErrorMessage(code)));
                return;
            }

            MessageBox.Show("Conversion Success");
        }

        private void CanConvert(object sender, CanExecuteRoutedEventArgs e)
        {
            if (DocListData == null || DocListData.Count == 0)
            {
                e.CanExecute = false;
                return;
            }

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
