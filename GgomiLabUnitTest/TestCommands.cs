using System.IO;
using GgomiLab;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace GgomiLabUnitTest
{
    [TestClass]
    public class TestCommands
    {
        [TestMethod]
        public void TestAddDoc()
        {
            MainWindow window = new MainWindow();
            window.GetModel().ShowMsg = false;
            DocListBox docListBox = window.FindName("IDC_DocList") as DocListBox;

            Assert.AreEqual(true, DocListBox.AddDoc.CanExecute(null, docListBox));

            string[] fileNames = { "a.pdf", "b.pdf", "c.pdf" };
            DocListBox.AddDoc.Execute(fileNames, docListBox);

            Assert.AreEqual(fileNames.Length, window.GetModel().Docs.Count);
            for (int i = 0; i < fileNames.Length; i++)
            {
                Assert.AreEqual(fileNames[i], window.GetModel().Docs[i].FilePath);
            }
        }

        [TestMethod]
        public void TestConvert()
        {
            MainWindow window = new MainWindow();
            window.GetModel().ShowMsg = false;
            DocListBox docListBox = window.FindName("IDC_DocList") as DocListBox;

            Assert.AreEqual(false, MainWindow.Convert.CanExecute(null, window));

            string[] fileNames = { "../sample/HOffice2022_Brochure_KR.pdf" };
            DocListBox.AddDoc.Execute(fileNames, docListBox);

            Assert.AreEqual(true, MainWindow.Convert.CanExecute(null, window));

            MainWindow.Convert.Execute(null, window);

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
