using System;
using System.IO;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using Microsoft.Win32;
using static PdfToOfficeApp.MainModel;

namespace PdfToOfficeApp
{
    /// <summary>
    /// MainWindow.xaml에 대한 상호 작용 논리
    /// </summary>
    public partial class MainWindow : Window
    {
        public MainViewModel vm;

        public MainWindow()
        {
            InitializeComponent();

            vm = new MainViewModel();
            this.DataContext = vm;
            if (vm.CloseAction == null)
                vm.CloseAction = new Action(() => this.Close());
        }

        protected override void OnInitialized(EventArgs e)
        {
            base.OnInitialized(e);
            AddCommandHandlers(AddFileCommand, OnAddFile, CanAddFile);
            AddCommandHandlers(RemoveFileCommand, OnRemoveFile, CanRemoveFile);
            AddCommandHandlers(OpenFolderCommand, OnOpenFolder, CanOpenFolder);
            AddCommandHandlers(ConvertCommand, OnConvert, CanConvert);
            AddCommandHandlers(CancelCommand, OnCancel, CanCancel);
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
        // 파일 위로 이동 커맨드
        public static RoutedCommand UpFileCommand = new RoutedCommand("UpFileCommand", typeof(Button));
        // 파일 아래로 이동 커맨드
        public static RoutedCommand DownFileCommand = new RoutedCommand("DownFileCommand", typeof(Button));
        // 취소 커맨드
        public static RoutedCommand CancelCommand = new RoutedCommand("CancelCommand", typeof(Button));

        private void OnConvert(object sender, ExecutedRoutedEventArgs e)
        {
            for (int i = 0; i < vm.FileInformations.Count; i++)
            {
                string path = vm.FileInformations[i].StrFilePath + "\\" + vm.FileInformations[i].StrFileName;
                int resultCode = vm.RunSample(path);
                if (resultCode != 0)
                {
                    MessageBox.Show("Conversion is failed.");
                    return;
                }
            }

            MessageBox.Show("Conversion is done.");
        }

        private void CanConvert(object sender, CanExecuteRoutedEventArgs e)
        {
            // 목록에 파일 있는지 확인
            if (vm.FileInformations.Count == 0)
            {
                e.CanExecute = false;
                return;
            }

            e.CanExecute = true;
        }

        // 파일 추가
        private void OnAddFile(object sender, ExecutedRoutedEventArgs e)
        {
            OpenFileDialog openFileDialog = new OpenFileDialog();
            openFileDialog.DefaultExt = ".pdf";
            openFileDialog.Filter = "PDF 문서 (*.pdf)|*.pdf";
            openFileDialog.Multiselect = true;

            if (openFileDialog.ShowDialog() == true)
            {
                foreach (string file in openFileDialog.FileNames)
                {
                    FileInfo fileInfo = new FileInfo(file);

                    FileInformation fileInformation = new FileInformation();

                    fileInformation.StrFileName = fileInfo.Name;
                    fileInformation.StrFilePath = fileInfo.DirectoryName;
                    fileInformation.StrFileSize = fileInfo.Length.ToString() + " Bytes";

                    vm.FileInformations.Add(fileInformation);
                }
            }
        }

        private void CanAddFile(object sender, CanExecuteRoutedEventArgs e)
        {
            e.CanExecute = true;
        }

        // 파일 추가
        private void OnRemoveFile(object sender, ExecutedRoutedEventArgs e)
        {
            vm.FileInformations.Remove(vm.SeletedFileInfo);
        }

        private void CanRemoveFile(object sender, CanExecuteRoutedEventArgs e)
        {
            // 선택된 항목 유무 확인
            if (vm.SeletedFileInfo != null)
                e.CanExecute = true;
            else
                e.CanExecute = false;
        }

        // 파일 추가
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
