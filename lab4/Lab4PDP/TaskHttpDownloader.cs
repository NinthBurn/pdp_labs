using HttpParser;
using Lab4PDP.HttpParser.Response;
using System;
using System.Collections.Generic;
using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading.Tasks;

namespace Lab4PDP
{
	class TaskHttpDownloader
	{
		private static readonly Encoding Encoding = Encoding.ASCII;

		public Task<byte[]> DownloadFileAsync(DownloadTask task)
		{
			return Task.Run(() => DownloadFile(task));
		}

		private Task<byte[]> DownloadFile(DownloadTask task)
		{
			return Connect(task)
				.ContinueWith(t => SendHttpRequest(task))
				.ContinueWith(t => ReceiveHttpResponse(task))
				.Unwrap(); 
		}


		private Task Connect(DownloadTask task)
		{
			var tcs = new TaskCompletionSource<object>();

			task.Socket.BeginConnect(task.Host, task.Port, ar =>
			{
				try
				{
					task.Socket.EndConnect(ar);
					tcs.SetResult(null);
				}
				catch (Exception ex)
				{
					Console.WriteLine($"Error during connection: {ex.Message}");
					Cleanup(task);
					tcs.SetException(ex);
				}
			}, task);

			return tcs.Task;
		}

		private Task SendHttpRequest(DownloadTask task)
		{
			string request = $"GET {new Uri(task.Url).AbsolutePath} HTTP/1.1\r\n" +
							 $"Host: {task.Host}\r\n" +
							 "Connection: close\r\n" +
							 "\r\n";

			task.RequestBuffer = Encoding.GetBytes(request);
			var tcs = new TaskCompletionSource<object>();

			task.Socket.BeginSend(task.RequestBuffer, 0, task.RequestBuffer.Length, SocketFlags.None, ar =>
			{
				try
				{
					int bytesSent = task.Socket.EndSend(ar);
					Console.WriteLine($"Sent {bytesSent} bytes to {task.Host}");

					tcs.SetResult(null);
				}
				catch (Exception ex)
				{
					Console.WriteLine($"Error during sending: {ex.Message}");
					Cleanup(task);
					tcs.SetException(ex);
				}
			}, null);

			return tcs.Task;
		}

		private Task<byte[]> ReceiveHttpResponse(DownloadTask task)
		{
			var tcs = new TaskCompletionSource<byte[]>();
			task.ResponseBuffer = new byte[1024];

			ReceiveNextChunk(task, tcs);

			return tcs.Task;
		}

		private void ReceiveNextChunk(DownloadTask task, TaskCompletionSource<byte[]> tcs)
		{
			task.Socket.BeginReceive(task.ResponseBuffer, 0, task.ResponseBuffer.Length, SocketFlags.None, ar =>
			{
				try
				{
					int bytesRead = task.Socket.EndReceive(ar);

					if (bytesRead > 0)
					{
						task.ResponseStream.Write(task.ResponseBuffer, 0, bytesRead);
						task.TotalBytesReceived += bytesRead;

						ReceiveNextChunk(task, tcs);
					}
					else
					{
						Console.WriteLine($"Download complete for {task.Url}. Total bytes received: {task.TotalBytesReceived}");
						Cleanup(task);
						tcs.SetResult(task.ResponseStream.ToArray());
					}
				}
				catch (Exception ex)
				{
					Console.WriteLine($"Error during receiving: {ex.Message}");
					Cleanup(task);
					tcs.SetException(ex);
				}
			}, null);
		}

		public Task DownloadMultipleFilesAsync(List<string> urls)
		{
			var downloadTasks = new List<Task>();

			foreach (string url in urls)
			{
				var task = new DownloadTask
				{
					Url = url,
					Host = new Uri(url).Host,
					Port = 80,
					ResponseStream = new MemoryStream(),
				};

				task.Socket = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);

				downloadTasks.Add(DownloadAndSaveFileAsync(task));
			}

			return Task.WhenAll(downloadTasks);
		}

		private Task DownloadAndSaveFileAsync(DownloadTask task)
		{
			Directory.CreateDirectory(Path.Combine(Directory.GetCurrentDirectory(), "tasks"));
			string filename = Path.Combine("tasks", task.Host + ".html");
			return DownloadFile(task)
					.ContinueWith(t =>
					{
						string raw = Encoding.GetString(t.Result);
						string filename = Path.Combine("tasks", task.Host + ".html");

						ParsedHttpResponse req = Parser.ParseRawResponse(raw);

						File.WriteAllBytes(filename, Encoding.GetBytes(req.ResponseBody));
						Console.WriteLine($"Response saved to {filename}");
					});
		}

		private void Cleanup(DownloadTask task)
		{
			if (task.Socket != null)
			{
				task.Socket.Close();
			}
		}
	}
}
