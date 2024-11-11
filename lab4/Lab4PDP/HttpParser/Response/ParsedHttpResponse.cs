using HttpParser.Models;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Text;

namespace Lab4PDP.HttpParser.Response
{
	public class ParsedHttpResponse
	{
		public Dictionary<string, string> Headers { get; set; }
		public string ResponseBody { get; set; }
		public StatusLine StatusLine{ get; set; }
		public int ContentLength { get; set; }
		public ParsedHttpResponse(string[] lines)
		{
			StatusLine = new StatusLine(lines);
			HttpHeaders responseHeaders = new HttpHeaders(lines);
			Headers = responseHeaders.Headers;

			ContentLength = 0;
			if(responseHeaders.Headers.ContainsKey("Content-Length"))
				ContentLength = int.Parse(responseHeaders.Headers["Content-Length"]);
					
			StringBuilder sb = new StringBuilder("");

			for(int i = responseHeaders.LastIndex + 2; i < lines.Length; i++)
				sb.Append($"{lines[i]}\n");

			ResponseBody = sb.ToString();
		}

		public override string ToString()
		{
			StringBuilder sb = new StringBuilder("");

			foreach (var header in Headers)
			{
				sb.Append($"{header.Key}: {header.Value}{Environment.NewLine}");
			}

			sb.Append(Environment.NewLine);
			sb.Append(ResponseBody);

			return sb.ToString().Trim();
		}
	}
}
