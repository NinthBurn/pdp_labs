using HttpParser;
using Lab4PDP;
using Lab4PDP.HttpParser.Response;
using System;
using System.Collections.Generic;
using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading;

class EventDrivenHttpDownloader
{
	private CountdownEvent downloadCountdown;

	public void StartDownloads(List<string> urls)
	{
		downloadCountdown = new CountdownEvent(urls.Count);

		foreach (var url in urls)
		{
			var task = new DownloadTask
			{
				Url = url,
				Host = new Uri(url).Host,
				Port = 80,
				ResponseStream = new MemoryStream(),
			};

			task.Socket = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
			task.Socket.BeginConnect(task.Host, task.Port, OnConnect, task);
		}

		downloadCountdown.Wait();
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
		Directory.CreateDirectory(Path.Combine(Directory.GetCurrentDirectory(), "event_driven"));
		string filename = Path.Combine("event_driven", task.Host + ".html"); 

		string raw = Encoding.UTF8.GetString(task.ResponseStream.ToArray());
		ParsedHttpResponse req = Parser.ParseRawResponse(raw);
		
		File.WriteAllBytes(filename, Encoding.ASCII.GetBytes(req.ResponseBody));
		Console.WriteLine($"Response saved to {filename}");
	}

	private void Cleanup(DownloadTask task)
	{
		if (task.Socket != null)
		{
			task.Socket.Close();
		}
		downloadCountdown.Signal();
	}
}
