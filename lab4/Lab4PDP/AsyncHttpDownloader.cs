using HttpParser;
using Lab4PDP.HttpParser.Response;
using System;
using System.Collections.Generic;
using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading;

class AsyncHttpDownloader
{
	// List to hold ongoing download tasks
	private List<DownloadTask> downloadTasks = new List<DownloadTask>();

	// Class to track information for each download
	private class DownloadTask
	{
		public string Url { get; set; }
		public Socket Socket { get; set; }
		public string Host { get; set; }
		public int Port { get; set; }
		public byte[] RequestBuffer { get; set; }
		public byte[] ResponseBuffer { get; set; }
		public MemoryStream ResponseStream { get; set; }
		public int TotalBytesReceived { get; set; }
	}

	public void StartDownloads(List<string> urls)
	{
		foreach (var url in urls)
		{
			var task = new DownloadTask
			{
				Url = url,
				Host = new Uri(url).Host,
				Port = 80,
				ResponseStream = new MemoryStream(),
			};

			// Create a new socket and initiate connection
			task.Socket = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
			task.Socket.BeginConnect(task.Host, task.Port, OnConnect, task);
		}

		//Task.WhenAll(downloadTasks).Wait();
	}

	private void OnConnect(IAsyncResult ar)
	{
		var task = (DownloadTask)ar.AsyncState;
		try
		{
			task.Socket.EndConnect(ar);

			// Construct an HTTP GET request
			string request = $"GET {new Uri(task.Url).AbsolutePath} HTTP/1.1\r\n" +
							 $"Host: {task.Host}\r\n" +
							 "Connection: close\r\n" +
							 "\r\n";

			task.RequestBuffer = Encoding.ASCII.GetBytes(request);
			task.Socket.BeginSend(task.RequestBuffer, 0, task.RequestBuffer.Length, SocketFlags.None, OnSend, task);
		}
		catch (Exception ex)
		{
			Console.WriteLine($"Error during connection: {ex.Message}");
			Cleanup(task);
		}
	}

	private void OnSend(IAsyncResult ar)
	{
		var task = (DownloadTask)ar.AsyncState;
		try
		{
			int bytesSent = task.Socket.EndSend(ar);
			Console.WriteLine($"Sent {bytesSent} bytes to {task.Host}");

			// Allocate buffer for receiving the response
			task.ResponseBuffer = new byte[1024];
			task.Socket.BeginReceive(task.ResponseBuffer, 0, task.ResponseBuffer.Length, SocketFlags.None, OnReceive, task);
		}
		catch (Exception ex)
		{
			Console.WriteLine($"Error during sending: {ex.Message}");
			Cleanup(task);
		}
	}

	private void OnReceive(IAsyncResult ar)
	{
		var task = (DownloadTask)ar.AsyncState;
		try
		{
			int bytesReceived = task.Socket.EndReceive(ar);

			if (bytesReceived > 0)
			{
				task.ResponseStream.Write(task.ResponseBuffer, 0, bytesReceived);
				task.TotalBytesReceived += bytesReceived;

				// Continue receiving more data
				task.Socket.BeginReceive(task.ResponseBuffer, 0, task.ResponseBuffer.Length, SocketFlags.None, OnReceive, task);
			}
			else
			{
				// Finished receiving data, print the result
				Console.WriteLine($"Download complete for {task.Url}. Total bytes received: {task.TotalBytesReceived}");
				SaveResponseToFile(task);
				Cleanup(task);
			}
		}
		catch (Exception ex)
		{
			Console.WriteLine($"Error during receiving: {ex.Message}");
			Cleanup(task);
		}
	}

	private void SaveResponseToFile(DownloadTask task)
	{
		string filename = task.Host + ".html";

		string raw = Encoding.UTF8.GetString(task.ResponseStream.ToArray());
		ParsedHttpResponse req = Parser.ParseRawResponse(raw);

		// 
		//Console.WriteLine(req.ContentLength);
		//Console.WriteLine(req.ResponseBody.Length);
		
		File.WriteAllBytes(filename, Encoding.ASCII.GetBytes(req.ResponseBody));
		Console.WriteLine($"Response saved to {filename}");
	}

	private void Cleanup(DownloadTask task)
	{
		if (task.Socket != null)
		{
			task.Socket.Close();
		}
		downloadTasks.Remove(task);
	}

	public static void Main(string[] args)
	{
		var downloader = new AsyncHttpDownloader();
		var urls = new List<string>
		{
			"http://example.com",
			"http://example.org",
			"http://example.net",
			"http://google.com",
			"http://youtube.com",
			"http://wikipedia.com",
			"http://httpforever.com/",
		};

		downloader.StartDownloads(urls);

		// Prevent the main thread from exiting while the downloads are running
		Console.WriteLine("Press any key to exit...");
		Console.ReadKey();
	}
}
