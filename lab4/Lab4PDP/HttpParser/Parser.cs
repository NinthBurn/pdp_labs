using HttpParser.Models;
using Lab4PDP.HttpParser;
using Lab4PDP.HttpParser.Request;
using Lab4PDP.HttpParser.Response;
using System;

namespace HttpParser
{
    public static class Parser
    {
        public static ParsedHttpRequest ParseRawRequest(string raw)
        {
            try
            {
                var lines = SplitLines(raw);

                var requestLine = new RequestLine(lines);
                var requestHeaders = new HttpHeaders(lines);
                requestHeaders.AddHeader("Method", requestLine.Method);
                requestHeaders.AddHeader("HttpVersion", requestLine.HttpVersion);

                var requestBody = new RequestBody(requestLine, lines);

                var parsed = new ParsedHttpRequest()
                {
                    Url = requestLine.Url,
                    Uri = new Uri(requestLine.Url),
                    Headers = requestHeaders.Headers,
                    RequestBody = requestBody.Body
                };

                return parsed;
            }
            catch (HttpParserException c)
            {
                Console.WriteLine($"Could not parse the raw request. {c.Message}");
                throw;
            }
            catch (Exception e)
            {
                Console.WriteLine($"Unhandled error parsing the raw request: {raw}\r\nError {e.Message}");
                throw;
            }
        }

		public static ParsedHttpResponse ParseRawResponse(string raw)
		{
			try
			{
				string[] lines = SplitLines(raw);
				ParsedHttpResponse parsed = new ParsedHttpResponse(lines);

				return parsed;
			}
			catch (HttpParserException c)
			{
				Console.WriteLine($"Could not parse the raw response. {c.Message}");
				throw;
			}
			catch (Exception e)
			{
				Console.WriteLine($"Unhandled error parsing the raw response: {raw}\r\nError {e.Message}");
				throw;
			}
		}
		
        private static string[] SplitLines(string raw)
        {
            return raw
                .TrimEnd('\r', '\n')
                .Split(new[] { "\\n", "\n", "\r\n" }, StringSplitOptions.None);
        }
    }
}
