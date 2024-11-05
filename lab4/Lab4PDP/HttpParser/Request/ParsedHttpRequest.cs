using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Text;

namespace Lab4PDP.HttpParser.Request
{
	public class ParsedHttpRequest
	{
		public string Url { get; set; }
		public Dictionary<string, string> Headers { get; set; }
		public string RequestBody { get; set; }
		public Uri Uri { get; set; }
		public ParsedHttpRequest()
		{
		}

		public override string ToString()
		{
			var method = Headers["Method"];
			var version = Headers["HttpVersion"];

			var sb = new StringBuilder($"{method} {Url} {version}{Environment.NewLine}");

			var headersToIgnore = new List<string> { "Method", "HttpVersion" };

			foreach (var header in Headers)
			{
				if (headersToIgnore.Contains(header.Key)) continue;
				sb.Append($"{header.Key}: {header.Value}{Environment.NewLine}");
			}

			if (method == "POST")
			{
				sb.Append(Environment.NewLine);
				sb.Append(RequestBody);
			}

			return sb.ToString().Trim();
		}
	}
}
