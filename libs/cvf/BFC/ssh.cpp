
#include"BFC/ssh.h"
#include"BFC/err.h"
#include"BFC/stdf.h"
#include"libssh/libssh.h"
#include<memory>
#include<mutex>

_FF_BEG

#if 0
int verify_knownhost(ssh_session session)
{
	int state, hlen;
	unsigned char *hash = NULL;
	char *hexa;
	char buf[1000];
	state = ssh_is_server_known(session);
	hlen = ssh_get_pubkey_hash(session, &hash);
	if (hlen < 0)
		return -1;
	switch (state)
	{
	case SSH_SERVER_KNOWN_OK:
		break; /* ok */
	case SSH_SERVER_KNOWN_CHANGED:
		fprintf(stderr, "Host key for server changed: it is now:\n");
		ssh_print_hexa("Public key hash", hash, hlen);
		fprintf(stderr, "For security reasons, connection will be stopped\n");
		free(hash);
		return -1;
	case SSH_SERVER_FOUND_OTHER:
		fprintf(stderr, "The host key for this server was not found but an other"
			"type of key exists.\n");
		fprintf(stderr, "An attacker might change the default server key to"
			"confuse your client into thinking the key does not exist\n");
		free(hash);
		return -1;
	case SSH_SERVER_FILE_NOT_FOUND:
		fprintf(stderr, "Could not find known host file.\n");
		fprintf(stderr, "If you accept the host key here, the file will be"
			"automatically created.\n");
		/* fallback to SSH_SERVER_NOT_KNOWN behavior */
	case SSH_SERVER_NOT_KNOWN:
		hexa = ssh_get_hexa(hash, hlen);
		fprintf(stderr, "The server is unknown. Do you trust the host key?\n");
		fprintf(stderr, "Public key hash: %s\n", hexa);
		//ssh_free(hexa);
		/*if (fgets(buf, sizeof(buf), stdin) == NULL)
		{
		free(hash);
		return -1;
		}*/
		strcpy(buf, "yes");

		if (strnicmp(buf, "yes", 3) != 0)
		{
			free(hash);
			return -1;
		}
		if (ssh_write_knownhost(session) < 0)
		{
			fprintf(stderr, "Error %s\n", strerror(errno));
			free(hash);
			return -1;
		}
		break;
	case SSH_SERVER_ERROR:
		fprintf(stderr, "Error %s", ssh_get_error(session));
		free(hash);
		return -1;
	}
	//free(hash);
	return 0;
}
#endif

class SSHSession::CImpl
{
public:
	class Channel
	{
	public:
		CImpl        *_site = NULL;
		ssh_channel  _channel = NULL;
		bool         _occupied = false;
	private:
		void _readChannel1(ssh_channel channel, FILE *ostream, std::string &msg, int is_stderr, bool waitCmdEnd)
		{
			char buffer[256];
			while (true)
			{
				int nbytes = 0;
				
				{
					std::lock_guard<std::mutex> _lock(_site->_sshMutex);
					nbytes = ssh_channel_read_nonblocking(channel, buffer, sizeof(buffer), is_stderr);
				}

				if (nbytes < 0)
					FF_EXCEPTION(ERR_OP_FAILED,"read channel failed");

				if (nbytes > 0)
				{
					msg.append(buffer, nbytes);
					if (ostream)
						fwrite(buffer, 1, nbytes, ostream);

					if (waitCmdEnd)
					{
						int i = nbytes - 1;
						for (; i >= 0 && buffer[i] > 0; --i);
						if (i >= 0) //encounter char <=0
							break;
					}
				}
				else
				{
					if (!waitCmdEnd)
						break;
					else
						_sleep(10);
				}
			}
		}
		void _readChannel(ssh_channel channel, FILE *ostream, std::string &msg, bool waitCmdEnd)
		{
			

			if (waitCmdEnd)
			{
				std::lock_guard<std::mutex> _lock(_site->_sshMutex);
				const char *cmde = "echo -e \"\\0377\"\n"; //write -1 into the channel
				//const char *cmde = "echo -e \"\\0\"\n"; //write 0 into the channel
				ssh_channel_write(channel, cmde, (uint32_t)strlen(cmde));
			}
			_readChannel1(channel, ostream, msg, 0, waitCmdEnd);
			_readChannel1(channel, ostream, msg, 1, false);
		}
	public:
		~Channel()
		{
			this->close();
		}

		void create(ssh_session session, const std::string &initCmds, CImpl *site)
		{
			int rc;
			ssh_channel channel = NULL;
			
			{
				std::lock_guard<std::mutex> _lock(site->_sshMutex);

				channel = ssh_channel_new(session);
				if (channel)
				{
					rc = ssh_channel_open_session(channel);
					if (rc == SSH_OK)
						rc = ssh_channel_request_shell(channel);

					if (rc != SSH_OK)
					{
						const char *err = ssh_get_error(channel);
						ssh_channel_free(channel);
						channel = NULL;
					}
				}
			}
			if (!channel)
				FF_EXCEPTION(ERR_OP_FAILED, "failed to open ssh channel");

			{
				this->close();
				_channel = channel;
				_site = site;
				_occupied = false;
			}

			if(!initCmds.empty())
				this->exec(initCmds, NULL); //this will also clear system message
			else
			{//clear system message
				std::string msg;
				_readChannel(_channel, NULL, msg, true);
			}
		}
		void close()
		{
			if (_channel)
			{
				ssh_channel_close(_channel);
				ssh_channel_free(_channel);
				_channel = NULL;
			}
		}
		bool _sendCmd(const std::string &cmd)
		{
			std::lock_guard<std::mutex> _lock(_site->_sshMutex);

			return ssh_channel_write(_channel, cmd.c_str(), (uint32_t)cmd.size()) == cmd.size() &&
				ssh_channel_write(_channel, "\n", 1) == 1;
		}
		std::string exec(const std::string &cmd, FILE *ostream)
		{
			std::string msg;
			if (!_sendCmd(cmd) )
			{
				FF_EXCEPTION(ERR_OP_FAILED, "failed to execute ssh command");
			}
			else
				_readChannel(_channel, ostream, msg, true);

			return msg;
		}
	};

	typedef std::shared_ptr<Channel> _ChannelPtr;

	ssh_session  _session=NULL;
	Options      _options;
	std::mutex   _sshMutex;
	std::vector<_ChannelPtr>  _channels;
	std::mutex   _channelMutex;

	Channel* _getIdleChannel()
	{
		std::lock_guard<std::mutex> lock(_channelMutex);

		Channel *ptr = nullptr;
		while (!ptr)
		{
			size_t i = 0;
			for (; i < _channels.size(); ++i)
			{
				if (!_channels[i]->_occupied)
					break;
			}
			if (i == _channels.size())
			{
				if ((int)_channels.size() < _options.maxChannels)
				{
					_ChannelPtr p(new Channel);
					p->create(_session, _options.channelInitCmds, this);

					_channels.push_back(p);
				}
			}
			if (i < _channels.size())
				ptr = _channels[i].get();
			else
				_sleep(10);
		}
		if (ptr)
			ptr->_occupied = true;
		else
			FF_EXCEPTION(ERR_OP_FAILED, "");

		return ptr;
	}
	void _freeIdleChannel(Channel *ptr)
	{
		if (ptr)
		{
			std::lock_guard<std::mutex> lock(_channelMutex);
			ptr->_occupied = false;
		}
	}

	struct AutoChannelFree
	{
		Channel *channel=NULL;
		CImpl   *site=NULL;
	public:
		AutoChannelFree(Channel *_channel, CImpl *_site)
			:channel(_channel),site(_site)
		{}
		~AutoChannelFree()
		{
			if (site && channel)
				site->_freeIdleChannel(channel);
		}
	};
public:
	CImpl()
	{
	}
	~CImpl()
	{
		this->clear();
	}
	void clear()
	{
		_channels.clear();

		if (_session)
		{
			ssh_disconnect(_session);
			ssh_free(_session);
			_session = NULL;
		}
	}
	void connect(const std::string &host, const std::string &userName, const std::string &password, Options op)
	{
		std::string err;
		ssh_session session = ssh_new();
		int rc = 0;
		if (session)
		{
			ssh_options_set(session, SSH_OPTIONS_HOST, host.c_str());
			//ssh_options_set(my_ssh_session, SSH_OPTIONS_LOG_VERBOSITY, &verbosity);
			ssh_options_set(session, SSH_OPTIONS_PORT, &op.port);

			rc = ssh_connect(session);
			if (rc != SSH_OK)
			{
				err = ssh_get_error(session);
				ssh_free(session);
				session = NULL;
			}
		}
		if (session)
		{
			rc = ssh_userauth_password(session, userName.c_str(), password.c_str());
			if (rc != SSH_AUTH_SUCCESS)
			{
				err = ssh_get_error(session);
				ssh_disconnect(session);
				ssh_free(session);
				session = NULL;
			}
		}

		if (!session)
			FF_EXCEPTION(ERR_GENERIC, ff::StrFormat("failed to connect to ssh host: %s", err.empty()? "..." :err.c_str()).c_str());
		
		{
			this->clear();
			_session = session;
			_options = op;
		}
	}

	std::string exec(const std::string &cmd, FILE *ostream)
	{
		AutoChannelFree a(this->_getIdleChannel(), this);

		return a.channel->exec(cmd, ostream);
	}
};

SSHSession::SSHSession()
{
	m_impl = new CImpl;
}
SSHSession::~SSHSession()
{
	delete m_impl;
}
void SSHSession::connect(const std::string &host, const std::string &userName, const std::string &password, Options op)
{
	m_impl->connect(host, userName, password, op);
}
std::string SSHSession::exec(const std::string &cmd, FILE *ostream)
{
	return m_impl->exec(cmd, ostream);
}
int SSHSession::nchannels()
{
	return (int)m_impl->_channels.size();
}

_FF_END

#pragma comment(lib,"ssh.lib")
