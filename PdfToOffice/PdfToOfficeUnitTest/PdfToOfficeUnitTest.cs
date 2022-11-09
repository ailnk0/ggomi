using System.IO;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using PdfToOfficeApp;
using static PdfToOfficeApp.MainModel;

namespace PdfToOfficeUnitTest
{
    [TestClass]
    public class PdfToOfficeUnitTest
    {
        [TestMethod]
        public void TestMethod1()
        {
            PdfToOffice pdfToOffice = new PdfToOffice();
            string path = "";
            int result = pdfToOffice.RunSample(path);

            Assert.AreEqual(0, result);
        }

        // 파일 추가
        [TestMethod]
        public void TestAddFile()
        {
            MainWindow mw = new MainWindow();

            MainWindow.AddFileCommand.Execute(mw.vm.FileInformations, mw);
            Assert.AreEqual(2, mw.vm.FileInformations.Count);
        }

        // 파일 삭제
        [TestMethod]
        public void TestRemoveFile()
        {
            MainWindow mw = new MainWindow();
            Assert.AreEqual(false, MainWindow.RemoveFileCommand.CanExecute(null, mw));
            for (int i = 0; i < 3; i++)
            {
                FileInformation fileInformation = new FileInformation();

                fileInformation.StrFileName = "HOffice2022_Brochure_KR.pdf";
                fileInformation.StrFilePath = "../sample";
                fileInformation.StrFileSize = i.ToString() + " Bytes";

                mw.vm.FileInformations.Add(fileInformation);
            }
            mw.vm.SeletedFileInfo = mw.vm.FileInformations[1];
            Assert.AreEqual(true, MainWindow.RemoveFileCommand.CanExecute(null, mw));

            MainWindow.RemoveFileCommand.Execute(mw.vm.SeletedFileInfo, mw);
            Assert.AreEqual(2, mw.vm.FileInformations.Count);
        }

        // 파일 변환
        [TestMethod]
        public void TestConvertCommand()
        {
            MainWindow mw = new MainWindow();
            Assert.AreEqual(false, MainWindow.ConvertCommand.CanExecute(null, mw));

            for (int i = 0; i < 3; i++)
            {
                FileInformation fileInformation = new FileInformation();

                fileInformation.StrFileName = "HOffice2022_Brochure_KR.pdf";
                fileInformation.StrFilePath = "../sample";
                fileInformation.StrFileSize = i.ToString() + " Bytes";

                mw.vm.FileInformations.Add(fileInformation);
            }
            Assert.AreEqual(true, MainWindow.ConvertCommand.CanExecute(null, mw));

            MainWindow.ConvertCommand.Execute(false, mw);

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
