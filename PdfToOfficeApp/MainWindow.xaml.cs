using System;
using System.ComponentModel;
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
        private BackgroundWorker worker = new BackgroundWorker();

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
            ErrorStatus result = pdfToOffice.InitializeSolidFramework();
            if (result != ErrorStatus.Success)
            {
                if (GetModel().ShowMsg)
                {
                    string strErr = string.Format("초기화를 실패하였습니다.\nError Code : {0}", result.ToString());
                    MessageBox.Show(strErr);
                    return;
                }
            }

            worker.DoWork += Worker_DoWork;
            worker.RunWorkerCompleted += Worker_RunWorkerCompleted;
            worker.ProgressChanged += Worker_ProgressChanged;
            worker.WorkerReportsProgress = true;
            worker.WorkerSupportsCancellation = true;
        }

        private void Worker_ProgressChanged(object sender, ProgressChangedEventArgs e)
        {

        }

        // 작업 완료
        private void Worker_RunWorkerCompleted(object sender, RunWorkerCompletedEventArgs e)
        {
            string strResult;
            if (e.Error != null)
            {
                strResult = e.Error.Message;
            }
            else if (e.Cancelled)
            {
                strResult = "취소되었습니다.";
            }
            else if ((int)e.Result > 0)
            {
                strResult = (int)e.Result + "개 변환을 실패하였습니다.";
            }
            else
            {
                strResult = Util.String.GetMsg(ErrorStatus.Success);
            }

            if (GetModel().ShowMsg)
            {
                MessageBox.Show(strResult);
            }
        }

        private void Worker_DoWork(object sender, DoWorkEventArgs e)
        {
            int failCount = 0;

            foreach (Doc doc in (DocList)e.Argument)
            {
                if (worker.CancellationPending)
                {
                    e.Cancel = true;
                    return;
                }

                string path = doc.FilePath;
                ErrorStatus status = pdfToOffice.DoWordConversion(path, "");
                if (status != ErrorStatus.Success)
                {
                    failCount++;
                    Dispatcher.BeginInvoke(new Action(delegate
                    {
                        if (GetModel().ShowMsg)
                            MessageBox.Show(Util.String.GetMsg(status));
                    }));
                }
            }

            e.Result = failCount;
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
            DocList docList = GetModel().Docs;
            worker.RunWorkerAsync(docList);
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
