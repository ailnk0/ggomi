using System.IO;
using System.Linq;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using PdfToOfficeApp;

namespace PdfToOfficeUnitTest
{
    [TestClass]
    public class PdfToOfficeUnitTest
    {
        [TestMethod]
        public void TestAddFileCommand()
        {
            MainWindow window = new MainWindow();
            window.GetModel().ShowMsg = false;

            Assert.AreEqual(true, MainWindow.AddFileCommand.CanExecute(null, window));

            string[] strFileNames = { "test1.pdf", "test2.pdf" };
            MainWindow.AddFileCommand.Execute(strFileNames, window);

            for (int i = 0; i < strFileNames.Length; i++)
            {
                var a = window.GetModel().Docs.Any(e => (e.FilePath == strFileNames[i]));

                Assert.AreEqual(true, a);
            }
        }

        // 파일 변환
        [TestMethod]
        public void TestConvertCommand()
        {
            MainWindow window = new MainWindow();
            window.GetModel().ShowMsg = false;
            Assert.AreEqual(false, MainWindow.ConvertCommand.CanExecute(null, window));

            string[] FileNames = { "../sample/HOffice2022_Brochure_KR.pdf" };
            MainWindow.AddFileCommand.Execute(FileNames, window);

            window.GetModel().Status = AppStatus.Ready;
            Assert.AreEqual(true, MainWindow.ConvertCommand.CanExecute(null, window));

            MainWindow.ConvertCommand.Execute(false, window);

            string convertedDocPath = "../sample/HOffice2022_Brochure_KR.docx";
            bool isConvertSuccess = File.Exists(convertedDocPath);
            if (isConvertSuccess)
            {
                File.Delete(convertedDocPath);
            }

            Assert.AreEqual(true, isConvertSuccess);
        }
    }
}
