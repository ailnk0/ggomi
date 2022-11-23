using Microsoft.Win32;
using System;
using System.Collections.Generic;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

namespace PdfToOfficeApp
{
    /// <summary>
    /// MainWindow.xaml에 대한 상호 작용 논리
    /// </summary>
    public partial class MainWindow : Window
    {
        public MainViewModel vm;
        public DocList DocListData;

        public static readonly DependencyProperty StatusProperty =
           DependencyProperty.Register("Status", typeof(AppStatus), typeof(MainWindow), new PropertyMetadata(AppStatus.Init));

        public AppStatus Status
        {
            get => (AppStatus)GetValue(StatusProperty);
            set => SetValue(StatusProperty, value);
        }

        public MainWindow()
        {
            InitializeComponent();
        }

        protected override void OnInitialized(EventArgs e)
        {
            base.OnInitialized(e);

            vm = new MainViewModel();
            this.DataContext = vm;
            if (vm.CloseAction == null)
                vm.CloseAction = new Action(() => this.Close());

            DocListData = IDC_DocList.ItemsSource as DocList;

            AddCommandHandlers(ApplicationCommands.Open, OnOpen, CanOpen);
            AddCommandHandlers(AddFileCommand, OnAddFile, CanAddFile);
            AddCommandHandlers(RemoveFileCommand, OnRemoveFile, CanRemoveFile);
            AddCommandHandlers(OpenFolderCommand, OnOpenFolder, CanOpenFolder);
            AddCommandHandlers(ConvertCommand, OnConvert, CanConvert);
            AddCommandHandlers(CancelCommand, OnCancel, CanCancel);

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

            Status = AppStatus.Ready;

            string[] fileNames = { strFilePath };
            AddFileCommand.Execute(fileNames, this);
            ConvertCommand.Execute(DocListData, IDC_Button_PrimaryConvert);
        }

        #region RoutedCommand
        // 파일 변환 커맨드
        public static RoutedCommand ConvertCommand = new RoutedCommand("ConvertCommand", typeof(Button));
        // 파일 추가 커맨드
        public static RoutedCommand AddFileCommand = new RoutedCommand("AddFileCommand", typeof(Button));
        // 파일 제거 커맨드
        public static RoutedCommand RemoveFileCommand = new RoutedCommand("RemoveFileCommand", typeof(Button));
        // 저장 경로 설정 커맨드
        public static RoutedCommand OpenFolderCommand = new RoutedCommand("OpenFolderCommand", typeof(Button));
        // 취소 커맨드
        public static RoutedCommand CancelCommand = new RoutedCommand("CancelCommand", typeof(Button));

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

            AddFileCommand.Execute(dialog.FileNames, IDC_DocList);
        }

        private void CanOpen(object sender, CanExecuteRoutedEventArgs e)
        {
            e.CanExecute = true;
        }

        private void OnConvert(object sender, ExecutedRoutedEventArgs e)
        {
            bool? showMessage = e.Parameter as bool?;
            foreach (Doc doc in DocListData)
            {
                string path = doc.FilePath;
                int resultCode = vm.RunSample(path);
                if (resultCode != 0)
                {
                    if (showMessage != false)
                        MessageBox.Show(Utils.GetString("IDS_Msg_Failed") + " " + resultCode.ToString());
                }
            }
            if (showMessage != false)
                MessageBox.Show("Conversion is done.");
        }

        private void CanConvert(object sender, CanExecuteRoutedEventArgs e)
        {
            if (Status == AppStatus.Init)
            {
                e.CanExecute = false;
                return;
            }
            else
            {
                if (SelectFormat.bRadioSelected)
                {
                    e.CanExecute = true;
                }
            }
        }

        // 파일 추가
        private void OnAddFile(object sender, ExecutedRoutedEventArgs e)
        {
            var fileNames = e.Parameter as string[];
            if (fileNames == null)
            {
                return;
            }

            var itemsSource = DocListData as ICollection<Doc>;
            foreach (var file in fileNames)
            {
                itemsSource.Add(new Doc(file));
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

        // 폴더 지정
        private void OnOpenFolder(object sender, ExecutedRoutedEventArgs e)
        {
            System.Windows.Forms.FolderBrowserDialog folderBrowserDialog = new System.Windows.Forms.FolderBrowserDialog();
            if (folderBrowserDialog.ShowDialog() == System.Windows.Forms.DialogResult.OK)
            {
                vm.StrSavePath = folderBrowserDialog.SelectedPath;
            }
        }

        private void CanOpenFolder(object sender, CanExecuteRoutedEventArgs e)
        {
            e.CanExecute = true;
        }

        // 취소
        private void OnCancel(object sender, ExecutedRoutedEventArgs e)
        {
            vm.CloseAction();
        }

        private void CanCancel(object sender, CanExecuteRoutedEventArgs e)
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
        #endregion
    }
}
