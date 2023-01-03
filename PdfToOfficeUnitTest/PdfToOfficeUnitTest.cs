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
            Assert.AreEqual(true, MainWindow.AddFileCommand.CanExecute(null, window));

            string[] fileNames = { "../sample/HOffice2022_Brochure_KR.pdf" };
            MainWindow.AddFileCommand.Execute(fileNames, window);

            for (int i = 0; i < fileNames.Length; i++)
            {
                var a = window.GetModel().Docs.Any(e => (e.FilePath == fileNames[i]));

                Assert.AreEqual(true, a);
            }
        }

        [TestMethod]
        public void TestConvertCommand()
        {
            Assert.AreEqual(false, MainWindow.ConvertCommand.CanExecute(null, window));

            model.ConvFileType = FILE_TYPE.DOCX;
            model.Docs.Add(new Doc("../sample/HOffice2022_Brochure_KR.pdf"));
            model.AppStatus = APP_STATUS.READY;

            Assert.AreEqual(true, MainWindow.ConvertCommand.CanExecute(null, window));

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
            Assert.AreEqual(false, MainWindow.RemoveFileCommand.CanExecute(null, window));

            model.Docs.Add(new Doc("../sample/HOffice2022_Brochure_KR.pdf"));
            model.SelectedItems = model.Docs;
            model.AppStatus = APP_STATUS.READY;

            Assert.AreEqual(true, MainWindow.RemoveFileCommand.CanExecute(null, window));

            MainWindow.RemoveFileCommand.Execute(false, window);

            bool isRemoved = window.GetModel().Docs.Count == 0;
            Assert.AreEqual(true, isRemoved);
        }
    }
}
