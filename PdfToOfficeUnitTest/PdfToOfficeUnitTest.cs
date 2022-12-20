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

        // 파일 삭제
        [TestMethod]
        public void TestRemoveFileCommand()
        {
            MainWindow window = new MainWindow();
            window.GetModel().ShowMsg = false;
            // 초기 상태 버튼 비활성화
            Assert.AreEqual(false, MainWindow.RemoveFileCommand.CanExecute(null, window));

            // 파일 추가
            string[] FileNames = { "../sample/HOffice2022_Brochure_KR.pdf" };
            MainWindow.AddFileCommand.Execute(FileNames, window);

            // 선택 전 버튼 비활성화
            Assert.AreEqual(false, MainWindow.RemoveFileCommand.CanExecute(null, window));

            var obj = (DocListBox)window.FindName("IDC_DocListBox");
            obj.SelectedIndex = 0;

            // 선택 후 버튼 활성화
            Assert.AreEqual(true, MainWindow.RemoveFileCommand.CanExecute(null, window));

            // 파일 삭제
            MainWindow.RemoveFileCommand.Execute(false, window);

            bool isRemoveSuccess;
            if (window.GetModel().Docs.Count > 0)
                isRemoveSuccess = false;
            else
                isRemoveSuccess = true;

            Assert.AreEqual(true, isRemoveSuccess);
        }
    }
}
