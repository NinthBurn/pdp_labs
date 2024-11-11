using System.Net.Sockets;

namespace Lab4PDP
{
	public class DownloadTask
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
}
