using System;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using Microsoft.Win32;
using PdfToOfficeAppModule;

namespace GgomiLab
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        public static RoutedCommand Convert = new RoutedCommand("Convert", typeof(Button));
        public static RoutedCommand ShowMsg = new RoutedCommand("ShowMsg", typeof(MainWindow));

        private PdfToOfficeProxy pdfToOffice;

        public MainWindow()
        {
            InitializeComponent();
        }

        protected override void OnInitialized(EventArgs e)
        {
            base.OnInitialized(e);

            AddCommandHandlers();

            pdfToOffice = new PdfToOfficeProxy();

            ErrorStatus status = pdfToOffice.InitializeSolidFramework();
            if (GetModel().ShowMsg)
            {
                if (status != ErrorStatus.Success)
                {
                    MessageBox.Show(Utils.GetErrorMessage(-100));
                }
            }
        }

        protected override void OnClosed(EventArgs e)
        {
            if (pdfToOffice != null)
            {
                pdfToOffice.Dispose();
                pdfToOffice = null;
            }

            base.OnClosed(e);
        }

        protected void AddCommandHandlers()
        {
            CommandUtil.AddCommandHandler(this, ApplicationCommands.Open, OnOpen, CanOpen);
            CommandUtil.AddCommandHandler(this, Convert, OnConvert, CanConvert);
        }

        public MainViewModel GetModel()
        {
            return DataContext as MainViewModel;
        }

        protected override void OnPreviewDrop(DragEventArgs e)
        {
            e.Handled = true;
            var fileNames = e.Data.GetData(DataFormats.FileDrop, true) as string[];
            DocListBox.AddDoc.Execute(fileNames, IDC_DocList);
        }

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

        private void OnConvert(object sender, ExecutedRoutedEventArgs e)
        {
            int errCnt = 0;
            foreach (var doc in GetModel().Docs)
            {
                ErrorStatus status = pdfToOffice.DoWordConversion(doc.FilePath, "");
                if (status != ErrorStatus.Success)
                {
                    errCnt++;
                }
            }

            if (GetModel().ShowMsg)
            {
                if (errCnt == 0)
                {
                    MessageBox.Show(Utils.GetErrorMessage(0));
                }
                else
                {
                    MessageBox.Show(Utils.GetErrorMessage(-100));
                }
            }
        }

        private void CanConvert(object sender, CanExecuteRoutedEventArgs e)
        {
            if (GetModel().Status == AppStatus.Init)
            {
                e.CanExecute = false;
                return;
            }

            if (GetModel().Docs.Count == 0)
            {
                e.CanExecute = false;
                return;
            }

            e.CanExecute = true;
        }
    }
}
