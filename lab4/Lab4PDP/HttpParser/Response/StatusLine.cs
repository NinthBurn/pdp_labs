using Lab4PDP.HttpParser;
using System;
using System.Text.RegularExpressions;

namespace HttpParser.Models
{
    public class StatusLine
    {
        public string StatusCode { get; set; }
        public string ReasonPhrase { get; set; }
        public string HttpVersion { get; set; }

        private readonly string[] validHttpVerbs = { "GET", "POST" };

        public StatusLine(string[] lines)
        {
            var firstLine = lines[0].Split(' ');
            ValidateRequestLine(firstLine);

            SetHttpVersion(firstLine[0]);
            SetStatusCode(firstLine[1]);

            for(int i = 2; i < firstLine.Length; i++)
            {
                ReasonPhrase += firstLine[i].Trim() + " ";
            }
        }

        private void ValidateRequestLine(string[] firstLine)
        {
            if (firstLine.Length < 3)
                throw new HttpParserException("Status line is not in a valid format", "ValidateStatusLine", string.Join(" ", firstLine));
        }

        private void SetStatusCode(string code)
        {
            code = code.Trim();
			if(!Regex.IsMatch(code, @"[1-5]\d\d"))
				throw new HttpParserException("Status code is not in a valid format", "ValidateStatusLine", string.Join(" ", code));

			StatusCode = code;
		}

        private void SetHttpVersion(string version)
        {
            if (!Regex.IsMatch(version, @"HTTP/\d.\d"))
            {
                HttpVersion = "HTTP/1.1";
            }

            HttpVersion = version.Trim();
        }
    }
}
