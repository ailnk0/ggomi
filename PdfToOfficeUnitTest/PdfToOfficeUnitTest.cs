using System.IO;
using System.Linq;
using System.Windows;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using PdfToOfficeApp;
using PdfToOfficeAppModule;

namespace PdfToOfficeUnitTest
{
    [TestClass]
    public class PdfToOfficeUnitTest
    {
        MainWindow window = null;
        MainViewModel model = null;

        [TestInitialize]
        public void TestInitialize()
        {
            if (Application.Current == null)
            {
                App app = new App();
                app.InitializeComponent();
            }

            window = new MainWindow();

            model = window.GetModel();
            model.Init();
            model.ShowMsg = false;
            model.IsOverwrite = true;
            model.IsSaveToUserDir = false;
        }

        [TestCleanup]
        public void TestCleanup()
        {
            if (window != null)
            {
                window.Dispose();
                window = null;
            }
        }

        [TestMethod]
        public void TestAddFileCommand()
        {
            bool canExecute = MainWindow.AddFileCommand.CanExecute(null, window);
            Assert.AreEqual(true, canExecute);

            string[] fileNames = { "../sample/HOffice2022_Brochure_KR.pdf" };
            MainWindow.AddFileCommand.Execute(fileNames, window);

            for (int i = 0; i < fileNames.Length; i++)
            {
                var isExist = model.Docs.Any(e => (e.FilePath == fileNames[i]));
                Assert.AreEqual(true, isExist);
            }
        }

        [TestMethod]
        public void TestConvertCommand()
        {
            bool canExecute = MainWindow.ConvertCommand.CanExecute(null, window);
            Assert.AreEqual(false, canExecute);

            model.Docs.Add(new Doc("../sample/HOffice2022_Brochure_KR.pdf"));
            model.AppStatus = APP_STATUS.READY;
            model.ConvFileType = FILE_TYPE.DOCX;

            canExecute = MainWindow.ConvertCommand.CanExecute(null, window);
            Assert.AreEqual(true, canExecute);

            MainWindow.ConvertCommand.Execute(true, window);

            string outPath = "../sample/HOffice2022_Brochure_KR.docx";
            bool isConverted = File.Exists(outPath);
            if (isConverted)
            {
                File.Delete(outPath);
            }

            Assert.AreEqual(true, isConverted);
        }

        [TestMethod]
        public void TestRemoveFileCommand()
        {
            bool canExecute = MainWindow.RemoveFileCommand.CanExecute(null, window);
            Assert.AreEqual(false, canExecute);

            model.Docs.Add(new Doc("../sample/HOffice2022_Brochure_KR.pdf"));
            model.AppStatus = APP_STATUS.READY;
            model.SelectedItems = model.Docs;

            canExecute = MainWindow.RemoveFileCommand.CanExecute(null, window);
            Assert.AreEqual(true, canExecute);

            MainWindow.RemoveFileCommand.Execute(false, window);

            bool isRemoved = model.Docs.Count == 0;
            Assert.AreEqual(true, isRemoved);
        }
    }
}
