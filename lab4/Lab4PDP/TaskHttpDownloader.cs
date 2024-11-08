using System;
using System.Collections.Generic;
using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading.Tasks;

namespace Lab4PDP
{
	

	class HttpFileDownloader
	{
		private static readonly Encoding Encoding = Encoding.ASCII;

		public static Task<byte[]> DownloadFileAsync(string url)
		{
			Uri uri = new Uri(url);
			string host = uri.Host;
			int port = 80;
			string path = uri.AbsolutePath;

			return Task.Run(() => DownloadFileInternal(host, port, path));
		}

		private static byte[] DownloadFileInternal(string host, int port, string path)
		{
			using (Socket socket = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp))
			{
				Connect(socket, host, port).Wait();
				SendHttpRequest(socket, host, path).Wait();
				return ReceiveHttpResponse(socket).Result;
			}
		}

		private static Task Connect(Socket socket, string host, int port)
		{
			var tcs = new TaskCompletionSource<object>();

			socket.BeginConnect(host, port, ar =>
			{
				try
				{
					socket.EndConnect(ar);
					tcs.SetResult(null);
				}
				catch (Exception ex)
				{
					tcs.SetException(ex);
				}
			}, null);

			return tcs.Task;
		}

		private static Task SendHttpRequest(Socket socket, string host, string path)
		{
			string request = $"GET {path} HTTP/1.1\r\n" +
							 $"Host: {host}\r\n" +
							 "Connection: close\r\n\r\n";

			byte[] requestBytes = Encoding.GetBytes(request);
			var tcs = new TaskCompletionSource<object>();

			socket.BeginSend(requestBytes, 0, requestBytes.Length, SocketFlags.None, ar =>
			{
				try
				{
					socket.EndSend(ar);
					tcs.SetResult(null);
				}
				catch (Exception ex)
				{
					tcs.SetException(ex);
				}
			}, null);

			return tcs.Task;
		}

		private static Task<byte[]> ReceiveHttpResponse(Socket socket)
		{
			var tcs = new TaskCompletionSource<byte[]>();
			var responseBytes = new List<byte>();

			ReceiveNextChunk(socket, responseBytes, tcs);

			return tcs.Task;
		}

		private static void ReceiveNextChunk(Socket socket, List<byte> responseBytes, TaskCompletionSource<byte[]> tcs)
		{
			byte[] buffer = new byte[1024];

			socket.BeginReceive(buffer, 0, buffer.Length, SocketFlags.None, ar =>
			{
				try
				{
					int bytesRead = socket.EndReceive(ar);

					if (bytesRead > 0)
					{
						responseBytes.AddRange(new ArraySegment<byte>(buffer, 0, bytesRead));

						ReceiveNextChunk(socket, responseBytes, tcs);
					}
					else
					{
						tcs.SetResult(responseBytes.ToArray());
					}
				}
				catch (Exception ex)
				{
					tcs.SetException(ex);
				}
			}, null);
		}

		public static Task DownloadMultipleFilesAsync(List<string> urls, string downloadFolder)
		{
			var downloadTasks = new List<Task>();

			foreach (string url in urls)
			{
				string fileName = Path.Combine(downloadFolder, Path.GetFileName(new Uri(url).AbsolutePath));
				downloadTasks.Add(DownloadAndSaveFileAsync(url, fileName));
			}

			return Task.WhenAll(downloadTasks);
		}

		private static Task DownloadAndSaveFileAsync(string url, string savePath)
		{
			return Task.Run(async () =>
			{
				byte[] fileData = await DownloadFileAsync(url);
				File.WriteAllBytes(savePath, fileData);
				Console.WriteLine($"Downloaded {url} to {savePath}");
			});
		}

		static void Main(string[] args)
		{
			var urls = new List<string>
			{
				"http://example.com/file1.txt",
				"http://example.com/file2.txt",
				"http://example.com/file3.txt"
			};

			string downloadFolder = Path.Combine(Environment.CurrentDirectory, "downloads");
			Directory.CreateDirectory(downloadFolder);

			DownloadMultipleFilesAsync(urls, downloadFolder).Wait();

			Console.WriteLine("All downloads completed!");
		}
	}
}
