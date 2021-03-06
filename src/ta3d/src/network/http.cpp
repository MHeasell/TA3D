#include <TA3D_NameSpace.h>
#include <misc/files.h>
#include <misc/paths.h>
#include "http.h"
#include <logs/logs.h>
#include "socket.tcp.h"
#include <fstream>

namespace TA3D
{

	Http::Http()
		: bStop(false), pos(0), size(0)
	{
	}

	Http::~Http()
	{
		Thread::destroyThread();
	}

	int Http::getTransferedBytes() const
	{
		return pos;
	}

	float Http::getProgress() const
	{
		return (!size) ? 0.f : (float)(pos * 100.0 / size);
	}

	void Http::proc(void* /*param*/)
	{
		bStop = false;

		if (!Paths::Exists(Paths::ExtractFilePath(filename)))
			Paths::MakeDir(Paths::ExtractFilePath(filename));

		SocketTCP sock;
		char buffer[4096];
		String realFilename = filename;
		String tmpFile = String(filename) << ".part";
		std::ofstream f(tmpFile.c_str(), std::ios::binary);
		int count;
		int crfound = 0;
		int lffound = 0;

		if (!f.is_open())
		{
			LOG_ERROR(LOG_PREFIX_NET << "httpGetFile: Could not open file " << tmpFile << " for writing !");
			return; // Error can't open file
		}

		/* open the socket and connect to the server */
		sock.open(servername, 80);
		if (!sock.isOpen())
		{
			LOG_ERROR(LOG_PREFIX_NET << "httpGetFile: Could not open socket !");
			f.close();
			remove(tmpFile.c_str());
			return;
		}

		sock.setNonBlockingMode(true); // We want it to be able to detect end of file ;)

		String tmpBuf;
		tmpBuf << "GET " << _request << " HTTP/1.0\r\nHost:" << servername << "\nAccept: */*\r\nUser-Agent: TA3D\r\nConnection: close\r\n\r\n";

		uint32 timer(MILLISECONDS_SINCE_INIT);
		sock.send(tmpBuf.data(), tmpBuf.size());
		if (!sock.isOpen())
		{
			LOG_ERROR(LOG_PREFIX_NET << "httpGetFile: Could not send request to server !");
			f.close();
			remove(tmpFile.c_str());
			return;
		}

		pos = 0;
		size = 0;
		String header;

		while (!bStop)
		{
			timer = MILLISECONDS_SINCE_INIT;
			do
			{
				count = sock.recv(buffer, sizeof(buffer) - 1);
				if (count == 0)
				{
					if (suspend(1))
					{
						sock.close();
						f.close();
						remove(tmpFile.c_str());
						return;
					}
				}
			} while (count == 0 && MILLISECONDS_SINCE_INIT - timer < 60000 && !bStop);
			if (MILLISECONDS_SINCE_INIT - timer >= 60000 || bStop)
				sock.close();
			if (count < 0)
			{
				sock.close();
				f.close();
				if (pos == size)
				{
					Paths::Files::Copy(tmpFile, realFilename, true);
					LOG_DEBUG(LOG_PREFIX_NET << "File successfully downloaded : " << realFilename);
				}
				else
				{
					LOG_DEBUG(LOG_PREFIX_NET << "Download failed : " << realFilename);
				}
				remove(tmpFile.c_str());
				return;
			}
			if (count > 0)
			{
				/* parse out the HTTP header */
				if (lffound < 2)
				{
					int i;

					for (i = 0; i < count; ++i)
					{
						header << buffer[i];
						if (buffer[i] == 0x0D)
							++crfound;
						else
						{
							if (buffer[i] == 0x0A)
								++lffound;
							else
								/* reset the CR and LF counters back to 0 */
								crfound = lffound = 0;
						}
						if (lffound == 2)
						{
							/* i points to the second LF */
							/* output the buffer to the file */
							f.write((char*)(buffer + i + 1), count - i - 1);
							pos += count - i - 1;
							size = Math::Max(size, pos);
							break;
						}
					}
					if (lffound >= 2)
					{
						header.toLower();
						String::Size offset = header.find("content-length: ");
						if (offset != String::npos)
						{
							header.erase(0, offset + 16);
							header.trimRight();
							size = header.to_int();
							LOG_DEBUG("header = " << header);
							LOG_DEBUG("Http: size = " << size);
						}
					}
				}
				else
				{
					f.write((const char*)buffer, count);
					pos += count;
					size = Math::Max(size, pos);
				}
			}
		}
		sock.close();
		f.close();
		remove(tmpFile.c_str());
		LOG_ERROR(LOG_PREFIX_NET << "Http: Download interrupted!");
	}

	void Http::signalExitThread()
	{
		while (pDead == 0)
		{
			bStop = true;
			rest(1);
		}
	}

	bool Http::isDownloading() const
	{
		return isRunning();
	}

	void Http::get(const String& filename, const String& url)
	{
		destroyThread();
		this->filename = filename;
		String tmp = url;
		if (tmp.startsWith("http://"))
			tmp.erase(0, 7);
		this->servername = Substr(tmp, 0, tmp.find_first_of("/"));
		tmp.erase(0, this->servername.size());
		this->_request = tmp;

		bStop = false;
		pos = 0;
		size = 0;
		start();
	}

	void Http::stop()
	{
		destroyThread();
	}

	String Http::request(const String& servername, const String& _request)
	{
		SocketTCP sock;
		char buffer[4096];
		String f;
		int count;
		int crfound = 0;
		int lffound = 0;

		/* open the socket and connect to the server */
		sock.open(servername, 80);
		if (!sock.isOpen())
		{
			LOG_ERROR(LOG_PREFIX_NET << "httpRequest: Could not open socket !");
			return nullptr;
		}

		sock.setNonBlockingMode(true); // We want to be able to detect end of transmission :p

		f.clear();

		String tmpBuf;
		tmpBuf << "GET " << _request << " HTTP/1.0\r\nHost:" << servername << "\nAccept: */*\r\nUser-Agent: TA3D\r\n\r\n";

		uint32 timer(MILLISECONDS_SINCE_INIT);
		sock.send(tmpBuf.data(), tmpBuf.size());
		if (!sock.isOpen())
		{
			LOG_ERROR(LOG_PREFIX_NET << "httpRequest: Could not send request to server !");
			return nullptr;
		}

		while (true)
		{
			timer = MILLISECONDS_SINCE_INIT;
			do
			{
				count = sock.recv(buffer, sizeof(buffer) - 1);
				rest(1);
			} while (count == 0 && MILLISECONDS_SINCE_INIT - timer < 1000);
			if (MILLISECONDS_SINCE_INIT - timer >= 1000)
				sock.close();
			if (count < 0)
			{
				sock.close();
				return f;
			}
			if (count > 0)
			{
				/* parse out the HTTP header */
				if (lffound < 2)
				{
					int i;

					for (i = 0; i < count; ++i)
					{
						if (buffer[i] == 0x0D)
							++crfound;
						else
						{
							if (buffer[i] == 0x0A)
								++lffound;
							else
								/* reset the CR and LF counters back to 0 */
								crfound = lffound = 0;
						}
						if (lffound == 2)
						{
							/* i points to the second LF */
							/* NUL terminate the string and put it in the buffer string */
							buffer[count] = 0x0;
							f += buffer + i + 1;
							break;
						}
					}
				}
				else
				{
					buffer[count] = 0x0;
					f += buffer;
				}
			}
		}
		sock.close();
		return f;
	}

	bool Http::getFile(const String& filename, const String& servername, const String& _request)
	{
		SocketTCP sock;
		char buffer[4096];
		std::ofstream f(filename.c_str(), std::ios::binary);
		int count;
		int crfound = 0;
		int lffound = 0;

		if (!f.is_open())
		{
			LOG_ERROR(LOG_PREFIX_NET << "httpGetFile: Could not open file " << filename << " for writing !");
			return true; // Error can't open file
		}

		/* open the socket and connect to the server */
		sock.open(servername, 80);
		if (!sock.isOpen())
		{
			LOG_ERROR(LOG_PREFIX_NET << "httpGetFile: Could not open socket !");
			f.close();
			return true;
		}

		sock.setNonBlockingMode(true); // We want it to be able to detect end of file ;)

		String tmpBuf;
		tmpBuf << "GET " << _request << " HTTP/1.0\r\nHost:" << servername << "\nAccept: */*\r\nUser-Agent: TA3D\r\nConnection: close\r\n\r\n";

		uint32 timer(MILLISECONDS_SINCE_INIT);
		sock.send(tmpBuf.data(), tmpBuf.size());
		if (!sock.isOpen())
		{
			LOG_ERROR(LOG_PREFIX_NET << "httpGetFile: Could not send request to server !");
			f.close();
			return true;
		}

		while (true)
		{
			timer = MILLISECONDS_SINCE_INIT;
			do
			{
				count = sock.recv(buffer, sizeof(buffer) - 1);
				rest(1);
			} while (count == 0 && MILLISECONDS_SINCE_INIT - timer < 1000);
			if (MILLISECONDS_SINCE_INIT - timer >= 1000)
				sock.close();
			if (count < 0)
			{
				sock.close();
				f.close();
				return false;
			}
			if (count > 0)
			{
				/* parse out the HTTP header */
				if (lffound < 2)
				{
					int i;

					for (i = 0; i < count; ++i)
					{
						if (buffer[i] == 0x0D)
							++crfound;
						else
						{
							if (buffer[i] == 0x0A)
								++lffound;
							else
								/* reset the CR and LF counters back to 0 */
								crfound = lffound = 0;
						}
						if (lffound == 2)
						{
							/* i points to the second LF */
							/* output the buffer to the file */
							f.write((const char*)(buffer + i + 1), count - i - 1);
							break;
						}
					}
				}
				else
					f.write((const char*)buffer, count);
			}
		}
		sock.close();
		f.close();
		return true;
	}
}
