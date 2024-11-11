using HttpParser;
using HttpParser.Models;

namespace Lab4PDP
{
	internal class Program
	{
		public static void Main(string[] args)
		{
			var downloader = new EventDrivenHttpDownloader();
			var urls = new List<string>
			{
				"http://example.com",
				"http://example.org",
				"http://example.net",
				"http://google.com",
				"http://youtube.com",
				"http://wikipedia.com",
				"http://httpforever.com/",
				"http://httpbin.org/",
				"http://old.reddit.com/",
				"http://www.slackware.com/",
			};

			downloader.StartDownloads(urls);

			Console.WriteLine("\n======================================");
			Console.WriteLine("\tEvent driven finished");
			Console.WriteLine("======================================\n");

			var taskHttpDownloader = new TaskHttpDownloader();
			taskHttpDownloader.DownloadMultipleFilesAsync(urls).Wait();

			Console.WriteLine("\n======================================");
			Console.WriteLine("\tTasks finished");
			Console.WriteLine("======================================\n");

			var taskAsyncHttpDownloader = new TaskAsyncHttpDownloader();
			taskAsyncHttpDownloader.DownloadMultipleFilesAsync(urls).Wait();

			Console.WriteLine("\n======================================");
			Console.WriteLine("\tTasks async finished");
			Console.WriteLine("======================================\n");

		}
	}
}
