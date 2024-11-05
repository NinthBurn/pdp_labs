using System.Linq;

namespace Lab4PDP.HttpParser.Request
{
    internal class RequestBody
    {
        public string Body { get; set; }

        public RequestBody(RequestLine requestLine, string[] lines)
        {
            if (requestLine.Method == "GET")
            {
                Body = SetBodyFromUrlGet(requestLine.Url);
            }
            else if (requestLine.Method == "POST")
            {
                Body = SetBodyFromPost(lines);
            }
        }

        private static string SetBodyFromUrlGet(string url)
        {
            return url.Contains('?')
                ? url.Split('?')[1]
                : null;
        }

        private static string SetBodyFromPost(string[] lines)
        {
            var index = lines.Length;

            if (index == -1)
            {
                return null;
            }

            return lines[index - 1];
        }
    }
}
