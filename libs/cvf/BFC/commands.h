#pragma once

#include"BFC/def.h"
//#include"BFC/argv.h"
#include<string>

namespace ff {
	class ArgSet;
}


using ff::ArgSet;
using std::string;

_FF_BEG


class _BFCS_API FFCommand
{
public:
	enum
	{
		FUNCT_APP0 = 10, //should be the same as vfxEHT_APICMD_APP1
		FUNCT_APP1
	};

	static int getFuncType(void(*)(ArgSet&))
	{
		return FUNCT_APP1;
	}
	static int getFuncType(void(*)())
	{
		return FUNCT_APP0;
	}

	class _BFCS_API Command
	{
	public:
		const char*			  m_cmd;			//related command, empty if not used
		void*				  m_func;           //the callback function
		int					  m_funcType;       //the type of @m_func
		const char*			  m_defArgStr;      //default command-line argument
		const char*			  m_shortHelp;      //a short help
		const char*           m_example;        //example usages
		void				 *m_pCustom;
	public:
		Command(const char *cmd = "", void *func = nullptr, int funcType = 0, const char *defArg = "", const char *shortHelp = "", const char *example = "", void *pCustom = NULL)
			:m_cmd(cmd), m_func(func), m_funcType(funcType), m_defArgStr(defArg), m_shortHelp(shortHelp), m_example(example), m_pCustom(pCustom)
		{
		}
	};

	class _BFCS_API CommandList
	{
	public:
		const Command   *m_cmds;
		int			     m_count;
		CommandList		*m_pNext;
	public:
		CommandList(const Command *cmds, int count);
	};

	// static CommandList *g_cmdList;
};

_BFCS_API void exec(const char *cmd);

#define _FF_CMD_TOKENPASTE(x, y) x ## y
#define _FF_CMD_TOKENPASTE2(x, y) _FF_CMD_TOKENPASTE(x, y)
#define _FF_CMD_UNIQUE_NAME(x) _FF_CMD_TOKENPASTE2(x, __LINE__)

#define FF_ADD_COMMANDS(cmdsTab) \
	static ff::FFCommand::CommandList _FF_CMD_UNIQUE_NAME(_FF_auto_obj_add_commands)(cmdsTab,sizeof(cmdsTab)/sizeof(cmdsTab[0]));

#define CMD_BEG() namespace _FF_CMD_UNIQUE_NAME(_FFcmd) { static ff::FFCommand::Command cmdtab[]={

#define CMD(cmd, func, defArg, shortHelp, example) ff::FFCommand::Command(cmd,func,ff::FFCommand::getFuncType(func),defArg,shortHelp,example,nullptr),
#define CMD0(cmd,func) CMD(cmd,func,"","","")

#define CMD_END() }; FF_ADD_COMMANDS(cmdtab); }



_FF_END

