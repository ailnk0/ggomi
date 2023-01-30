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
        public static readonly string SAMPLE_PATH = "../sample/sample.pdf";

        protected MainWindow window = null;
        protected MainViewModel model = null;

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

            string[] fileNames = { SAMPLE_PATH };
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

            Doc doc = new Doc
            {
                FilePath = SAMPLE_PATH
            };
            model.Docs.Add(doc);
            model.AppStatus = APP_STATUS.READY;
            model.ConvFileType = FILE_TYPE.DOCX;

            canExecute = MainWindow.ConvertCommand.CanExecute(null, window);
            Assert.AreEqual(true, canExecute);

            MainWindow.ConvertCommand.Execute(true, window);

            bool isConverted = model.Docs[0].ResCode == RES_CODE.Success;
            Assert.AreEqual(true, isConverted);
        }

        [TestMethod]
        public void TestRemoveFileCommand()
        {
            bool canExecute = MainWindow.RemoveFileCommand.CanExecute(null, window);
            Assert.AreEqual(false, canExecute);

            Doc doc = new Doc
            {
                FilePath = SAMPLE_PATH
            };
            model.Docs.Add(doc);
            model.AppStatus = APP_STATUS.READY;
            model.SelectedItems = model.Docs;

            canExecute = MainWindow.RemoveFileCommand.CanExecute(null, window);
            Assert.AreEqual(true, canExecute);

            MainWindow.RemoveFileCommand.Execute(false, window);

            bool isRemoved = model.Docs.Count == 0;
            Assert.AreEqual(true, isRemoved);
        }

        [TestMethod]
        public void TestGetOutPath()
        {
            string sourcePath = SAMPLE_PATH;
            string outPath = null;

            model.ConvFileType = FILE_TYPE.DOCX;
            model.IsOverwrite = true;
            outPath = PdfToOfficeApp.Util.PathManager.GetOutPath(model, sourcePath);
            Assert.AreEqual("../sample/sample.docx", outPath);

            model.IsOverwrite = false;
            File.Delete("../sample/sample (1).docx");
            File.Create("../sample/sample.docx").Close();
            outPath = PdfToOfficeApp.Util.PathManager.GetOutPath(model, sourcePath);
            Assert.AreEqual(Path.GetFullPath("../sample/sample (1).docx"), Path.GetFullPath(outPath));

            model.UserDir = "../sample/UserDir/";
            model.IsSaveToUserDir = true;
            outPath = PdfToOfficeApp.Util.PathManager.GetOutPath(model, sourcePath);
            Assert.AreEqual("../sample/UserDir/sample.docx", outPath);
        }
    }
}
