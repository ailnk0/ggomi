using System;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

namespace PdfToOfficeApp
{
    /// <summary>
    /// Interaction logic for ConfigWindow.xaml
    /// </summary>
    public partial class ConfigWindow : Window
    {
        public static RoutedCommand SelUserDirCommand = new RoutedCommand("SelUserDirCommand", typeof(MenuItem));

        public ConfigWindow()
        {
            InitializeComponent();
        }

        protected override void OnInitialized(EventArgs e)
        {
            base.OnInitialized(e);

            AddCommandHandlers();
        }

        public MainViewModel GetModel()
        {
            return DataContext as MainViewModel;
        }

        private void AddCommandHandlers()
        {
            Util.Commands.Add(this, SelUserDirCommand, OnSelUserDir, CanSelUserDir);
        }

        private void OnSelUserDir(object sender, ExecutedRoutedEventArgs e)
        {
            using (var dialog = new System.Windows.Forms.FolderBrowserDialog())
            {
                var result = dialog.ShowDialog();
                if (result != System.Windows.Forms.DialogResult.OK)
                {
                    return;
                }

                GetModel().UserDir = dialog.SelectedPath;
            }
        }

        private void CanSelUserDir(object sender, CanExecuteRoutedEventArgs e)
        {
            if (GetModel().IsSaveToUserDir)
            {
                e.CanExecute = true;
                return;
            }

            e.CanExecute = false;
        }
    }
}
