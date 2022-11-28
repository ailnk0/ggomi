using System;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using Microsoft.Win32;
using PdfToOfficeAppModule;

namespace PdfToOfficeApp
{
    /// <summary>
    /// MainWindow.xaml에 대한 상호 작용 논리
    /// </summary>
    public partial class MainWindow : Window, IDisposable
    {
        private PdfToOfficeProxy pdfToOffice;

        public MainWindow()
        {
            InitializeComponent();
        }

        public void Dispose()
        {
            if (pdfToOffice != null)
            {
                pdfToOffice.Dispose();
                pdfToOffice = null;
            }

            ContentRendered -= MainWindow_ContentRendered;
        }

        protected override void OnInitialized(EventArgs e)
        {
            base.OnInitialized(e);
            AddCommandHandlers();

            ContentRendered += MainWindow_ContentRendered;

            pdfToOffice = new PdfToOfficeProxy();
            pdfToOffice.InitializeSolidFramework(); // TODO : 초기화 실패 시 예외처리 해야 함(ErrorStatus)
        }

        private void AddCommandHandlers()
        {
            Util.Commands.Add(this, ApplicationCommands.Open, OnOpen, CanOpen);
            Util.Commands.Add(this, AddFileCommand, OnAddFile, CanAddFile);
            Util.Commands.Add(this, RemoveFileCommand, OnRemoveFile, CanRemoveFile);
            Util.Commands.Add(this, ConvertCommand, OnConvert, CanConvert);
        }

        private void MainWindow_ContentRendered(object sender, EventArgs e)
        {
            string[] arrArg = Environment.GetCommandLineArgs();
            if (arrArg.Length >= 3)
            {
                WindowContextExecute(arrArg[1], arrArg[2]);
            }
        }

        protected override void OnPreviewDrop(DragEventArgs e)
        {
            e.Handled = true;
            var fileNames = e.Data.GetData(DataFormats.FileDrop, true) as string[];
            AddFileCommand.Execute(fileNames, this);
        }

        private void WindowContextExecute(string filePath, string fileFormat)
        {
            string strFilePath = filePath;
            string strFileFormat = fileFormat;

            if (strFileFormat == "HWP")
                IDC_SelectFormat.IDC_RadioButton_HWP.IsChecked = true;
            else if (strFileFormat == "XSLX")
                IDC_SelectFormat.IDC_RadioButton_XLSX.IsChecked = true;
            else if (strFileFormat == "PPTX")
                IDC_SelectFormat.IDC_RadioButton_PPTX.IsChecked = true;
            else if (strFileFormat == "DOCX")
                IDC_SelectFormat.IDC_RadioButton_DOCX.IsChecked = true;
            else if (strFileFormat == "IMAGE")
                IDC_SelectFormat.IDC_RadioButton_Image.IsChecked = true;

            if (strFilePath == null)
            {
                return;
            }

            GetModel().Status = AppStatus.Ready;

            string[] fileNames = { strFilePath };
            AddFileCommand.Execute(fileNames, this);
            ConvertCommand.Execute(GetModel().Docs, IDC_Button_PrimaryConvert);
        }

        public MainViewModel GetModel()
        {
            return DataContext as MainViewModel;
        }

        #region RoutedCommand
        // 파일 변환 커맨드
        public static RoutedCommand ConvertCommand = new RoutedCommand("ConvertCommand", typeof(Button));
        // 파일 추가 커맨드
        public static RoutedCommand AddFileCommand = new RoutedCommand("AddFileCommand", typeof(Button));
        // 파일 제거 커맨드
        public static RoutedCommand RemoveFileCommand = new RoutedCommand("RemoveFileCommand", typeof(Button));

        private void OnOpen(object sender, RoutedEventArgs e)
        {
            var dialog = new OpenFileDialog
            {
                Multiselect = true
            };
            bool? result = dialog.ShowDialog();
            if (result != true)
            {
                return;
            }

            AddFileCommand.Execute(dialog.FileNames, IDC_DocListBox);
        }

        private void CanOpen(object sender, CanExecuteRoutedEventArgs e)
        {
            e.CanExecute = true;
        }

        private void OnConvert(object sender, ExecutedRoutedEventArgs e)
        {
            int failCount = 0;
            foreach (Doc doc in GetModel().Docs)
            {
                string path = doc.FilePath;
                ErrorStatus status = pdfToOffice.DoWordConversion(path, "");
                if (status != ErrorStatus.Success)
                {
                    failCount++;
                    if (GetModel().ShowMsg)
                        MessageBox.Show(Util.String.GetMsg(status));
                }
            }

            if (failCount == 0)
            {
                if (GetModel().ShowMsg)
                    MessageBox.Show(Util.String.GetMsg(ErrorStatus.Success));
            }
        }

        private void CanConvert(object sender, CanExecuteRoutedEventArgs e)
        {
            if (GetModel().Status == AppStatus.Init)
            {
                e.CanExecute = false;
                return;
            }
            else
            {
                e.CanExecute = true;
            }
        }

        // 파일 추가
        private void OnAddFile(object sender, ExecutedRoutedEventArgs e)
        {
            string[] fileNames = e.Parameter as string[];
            if (fileNames == null)
            {
                return;
            }

            foreach (var file in fileNames)
            {
                GetModel().Docs.Add(new Doc(file));
            }
        }

        private void CanAddFile(object sender, CanExecuteRoutedEventArgs e)
        {
            e.CanExecute = true;
        }

        // 파일 제거
        private void OnRemoveFile(object sender, ExecutedRoutedEventArgs e)
        {

        }

        private void CanRemoveFile(object sender, CanExecuteRoutedEventArgs e)
        {

        }
        #endregion
    }
}
