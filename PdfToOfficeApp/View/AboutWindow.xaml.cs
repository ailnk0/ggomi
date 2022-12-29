﻿using System.Windows;

namespace PdfToOfficeApp
{
    /// <summary>
    /// Interaction logic for AboutWindow.xaml
    /// </summary>
    public partial class AboutWindow : Window
    {
        public AboutWindow()
        {
            InitializeComponent();
        }

        public MainViewModel GetModel()
        {
            return DataContext as MainViewModel;
        }
    }
}
