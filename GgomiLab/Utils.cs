namespace GgomiLab
{
    public class Utils
    {
        // TODO : 에러 메세지는 리소스이므로 Xaml 리소스에서 string을 불러오도록 구현해야한다.
        public static string GetErrorMessage(int code)
        {
            switch (code)
            {
                case -1:
                    return "Invalid License";
                case 0:
                    return "Success";
                default:
                    return "Unknown Error";
            }
        }
    }
}
