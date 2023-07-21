#pragma once

#include"BFC/def.h"
#include<stdio.h>
#include<string>

_FF_BEG

class _BFC_API SSHSession
{
	class CImpl;

	CImpl *m_impl;
public:
	SSHSession();

	~SSHSession();

	struct Options
	{
		int port = 22;
		int maxChannels = 1;
		std::string  channelInitCmds;
	};

	void connect(const std::string &host, const std::string &userName, const std::string &password, Options op=Options());

	std::string exec(const std::string &cmd, FILE *ostream=NULL);

	int  nchannels();
};


_FF_END


