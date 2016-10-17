// An NSIS plugin providing Perl compatible regular expression functions.
// 
// A simple wrapper around the excellent PCRE library which was written by
// Philip Hazel, University of Cambridge.
// 
// For those that require documentation on how to construct regular expressions,
// please see http://www.pcre.org/
// 
// _____________________________________________________________________________
// 
// Copyright (c) 2007 Computerway Business Solutions Ltd.
// Copyright (c) 2005 Google Inc.
// Copyright (c) 1997-2006 University of Cambridge
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 
//     * Redistributions of source code must retain the above copyright notice,
//       this list of conditions and the following disclaimer.
// 
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
// 
//     * Neither the name of the University of Cambridge nor the name of Google
//       Inc. nor the name of Computerway Business Solutions Ltd. nor the names
//       of their contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
// 
// Core PCRE Library Written by:       Philip Hazel, University of Cambridge
// C++ Wrapper functions by:           Sanjay Ghemawat, Google Inc.
// Support for PCRE_XXX modifiers by:  Giuseppe Maxia
// NSIS integration by:                Rob Stocks, Computerway Business Solutions Ltd.

#include <windows.h>
#include <stdio.h>
#include "exdll.h"
#include <pcre.h>
#include <pcrecpp.h>

HINSTANCE g_hInstance;
HWND g_hwndParent;
pcrecpp::RE_Options g_options;
pcrecpp::RE *g_reFaC = NULL;
string g_inputFaC = "";

inline void pushint(int i)
{
	TCHAR intstr[32];
	wsprintf(intstr, _T("%d"), i);
	pushstring(intstr);
}

inline bool convertboolstr(TCHAR* s)
{
	return (_tcscmp(_T("1"), s)==0)
			|| (_tcscmp(_T("true"), s)==0)
			|| (_tcscmp(_T("TRUE"), s)==0)
			|| (_tcscmp(_T("True"), s)==0);
}

inline void ResetFind()
{
	if (g_reFaC!=NULL)
	{
		delete g_reFaC;
		g_reFaC = NULL;
	}
}

void SetOrClearOption(TCHAR* opt, bool set)
{
	if (_tcscmp(_T("CASELESS"), opt)==0) g_options.set_caseless(set);
	if (_tcscmp(_T("MULTILINE"), opt)==0) g_options.set_multiline(set);
	if (_tcscmp(_T("DOTALL"), opt)==0) g_options.set_dotall(set);
	if (_tcscmp(_T("EXTENDED"), opt)==0) g_options.set_extended(set);
	if (_tcscmp(_T("DOLLAR_ENDONLY"), opt)==0) g_options.set_dollar_endonly(set);
	if (_tcscmp(_T("EXTRA"), opt)==0) g_options.set_extra(set);
	if (_tcscmp(_T("UTF8"), opt)==0) g_options.set_utf8(set);
	if (_tcscmp(_T("UNGREEDY"), opt)==0) g_options.set_ungreedy(set);
	if (_tcscmp(_T("NO_AUTO_CAPTURE"), opt)==0) g_options.set_no_auto_capture(set);
	if (_tcscmp(_T("i"), opt)==0) g_options.set_caseless(set);
	if (_tcscmp(_T("m"), opt)==0) g_options.set_multiline(set);
	if (_tcscmp(_T("s"), opt)==0) g_options.set_dotall(set);
	if (_tcscmp(_T("x"), opt)==0) g_options.set_extended(set);
}

bool PerformMatch(pcrecpp::RE *re, string *_subject, pcrecpp::RE::Anchor anchor, int stackoffset, bool requireCaptures)
{
	string* results = NULL;
	int captures;
	bool matched = false;
	pcrecpp::StringPiece subject(*_subject);

	captures = re->NumberOfCapturingGroups();
	if (captures<0) captures = 0;
	if (captures==0 && requireCaptures)
	{
		pushstring(_T("error No capture groups specified in pattern."));
	}
	else
	{
		results = new string[captures];		
		const pcrecpp::Arg* *args = new const pcrecpp::Arg*[captures];
		for (int i = 0; i < captures; i++)
		{
			args[i] = new pcrecpp::Arg(&results[i]);
		}
		int consumed;
		matched = re->DoMatch(subject, anchor, &consumed, args, captures);
		for (int i = 0; i < captures; i++)
		{
			delete args[i];
		}
		delete[] args;

		if (matched)
		{
			subject.remove_prefix(consumed);

			TCHAR** savestack = NULL;
			if (stackoffset>0)
			{
				savestack = new TCHAR*[stackoffset];
				for (int i = 0; i < stackoffset; i++)
				{
					savestack[i] = new TCHAR[g_stringsize];
					popstring(savestack[i]);
				}
			}

			for (int i = (captures-1); i >= 0; i--)
			{
				PushStringA(results[i].c_str());
			}

			if (stackoffset>0)
			{
				for (int i = (stackoffset-1); i >= 0; i--)
				{
					pushstring(savestack[i]);
					delete[] savestack[i];
				}
				delete[] savestack;
			}

			delete[] results;
		}

		if (matched) pushint(captures);
		pushstring((matched)?_T("true"):_T("false"));
	}

	subject.CopyToString(_subject);

	return matched;
}


BOOL WINAPI DllMain(HANDLE hInst, ULONG ul_reason_for_call, LPVOID lpReserved)
{
	g_hInstance=(HINSTANCE)hInst;
	if (DLL_PROCESS_DETACH == ul_reason_for_call)
	{
		ResetFind();
	}
	return TRUE;
}


extern "C" __declspec(dllexport)
void RECheckPattern(HWND hwndParent, int string_size, TCHAR *variables, stack_t **stacktop, extra_parameters *extra)
{
	g_hwndParent=hwndParent;

	EXDLL_INIT();

	char* pattern = new char[string_size];
	PopStringA(pattern);

	pcrecpp::RE re(pattern, g_options);

	string errorstr = re.error();

	PushStringA(errorstr.c_str());

	delete[] pattern;
}

extern "C" __declspec(dllexport)
void REQuoteMeta(HWND hwndParent, int string_size, TCHAR *variables, stack_t **stacktop, extra_parameters *extra)
{
	g_hwndParent=hwndParent;

	EXDLL_INIT();

	char* toquote = new char[string_size];

	PopStringA(toquote);

	string quoted = pcrecpp::RE::QuoteMeta(toquote);

	PushStringA(quoted.c_str());

	delete[] toquote;
}


extern "C" __declspec(dllexport)
void REFind(HWND hwndParent, int string_size, TCHAR *variables, stack_t **stacktop, extra_parameters *extra)
{
	g_hwndParent=hwndParent;

	EXDLL_INIT();

	if (g_options.no_auto_capture())
	{
		pushstring(_T("error FindAndConsume called with NO_AUTO_CAPTURE"));
		return;
	}

	char* pattern = new char[string_size];
	char* subject = new char[string_size];
	TCHAR* stackoffsetstr = new TCHAR[string_size];
	int stackoffset = 0;

	PopStringA(pattern);
	PopStringA(subject);
	popstring(stackoffsetstr);

	if (_tcslen(stackoffsetstr)>0)
	{
		stackoffset = _ttoi(stackoffsetstr);
	}

	ResetFind();

	g_reFaC = new pcrecpp::RE(pattern, g_options);
	g_inputFaC = (string)(subject);

	string errorstr = g_reFaC->error();

	if (errorstr.empty())
	{
		if (g_reFaC->NumberOfCapturingGroups()==0)
		{
			string newpattern = "("+g_reFaC->pattern()+")";
			delete g_reFaC;
			g_reFaC = new pcrecpp::RE(newpattern, g_options);
		}
		PerformMatch(g_reFaC, &g_inputFaC, g_reFaC->UNANCHORED, stackoffset, true);
	}
	else
	{
		errorstr.insert(0, "error ");
		PushStringA(errorstr.c_str());
	}

	delete[] stackoffsetstr;
	delete[] subject;
	delete[] pattern;
}

extern "C" __declspec(dllexport)
void REFindNext(HWND hwndParent, int string_size, TCHAR *variables, stack_t **stacktop, extra_parameters *extra)
{
	g_hwndParent=hwndParent;

	EXDLL_INIT();

	TCHAR* stackoffsetstr = new TCHAR[string_size];
	int stackoffset = 0;

	popstring(stackoffsetstr);

	if (_tcsclen(stackoffsetstr)>0)
	{
		stackoffset = _ttoi(stackoffsetstr);
	}

	if (g_reFaC!=NULL)
	{
		PerformMatch(g_reFaC, &g_inputFaC, g_reFaC->UNANCHORED, stackoffset, true);
	}
	else
	{
		pushstring(_T("error FindAndConsume must be called before FindAndConsumeNext."));
	}

	delete[] stackoffsetstr;
}

extern "C" __declspec(dllexport)
void REFindClose(HWND hwndParent, int string_size, TCHAR *variables, stack_t **stacktop, extra_parameters *extra)
{
	g_hwndParent=hwndParent;

	EXDLL_INIT();

	ResetFind();
}

extern "C" __declspec(dllexport)
void REMatches(HWND hwndParent, int string_size, TCHAR *variables, stack_t **stacktop, extra_parameters *extra)
{
	g_hwndParent=hwndParent;

	EXDLL_INIT();

	char* pattern = new char[string_size];
	char* subject = new char[string_size];
	TCHAR* partialstr = new TCHAR[string_size];
	TCHAR* stackoffsetstr = new TCHAR[string_size];
	int stackoffset = 0;

	PopStringA(pattern);
	PopStringA(subject);
	popstring(partialstr);
	popstring(stackoffsetstr);

	if (_tcslen(stackoffsetstr)>0)
	{
		stackoffset = _ttoi(stackoffsetstr);
	}

	pcrecpp::RE re(pattern, g_options);

	string errorstr = re.error();

	if (errorstr.empty())
	{
		string input(subject);
		pcrecpp::RE::Anchor a = pcrecpp::RE::ANCHOR_BOTH;
		if (convertboolstr(partialstr) || g_options.multiline())
			a = pcrecpp::RE::UNANCHORED;
		PerformMatch(&re, &input, a, stackoffset, false);
	}
	else
	{
		errorstr.insert(0, "error ");
		PushStringA(errorstr.c_str());
	}
	delete[] stackoffsetstr;
	delete[] partialstr;
	delete[] pattern;
	delete[] subject;
}

extern "C" __declspec(dllexport)
void REReplace(HWND hwndParent, int string_size, TCHAR *variables, stack_t **stacktop, extra_parameters *extra)
{
	g_hwndParent=hwndParent;

	EXDLL_INIT();

	char* pattern = new char[string_size];
	char* subject = new char[string_size];
	char* replacement = new char[string_size];
	TCHAR* replaceall = new TCHAR[string_size];

	PopStringA(pattern);
	PopStringA(subject);
	PopStringA(replacement);
	popstring(replaceall);

	pcrecpp::RE re(pattern, g_options);

	string errorstr = re.error();
	if (errorstr.empty())
	{
		string subj = (string)subject;
		bool success = false;
		
		if (convertboolstr(replaceall))
		{
			success = (re.GlobalReplace(replacement, &subj)>0);
		}
		else
		{
			success = re.Replace(replacement, &subj);
		}

		if (success)
		{	
			PushStringA(subj.c_str());
			pushstring(_T("true"));
		}
		else
		{
			pushstring(_T("false"));
		}
	}
	else
	{
		errorstr.insert(0, "error ");
		PushStringA(errorstr.c_str());
	}

	delete[] replaceall;
	delete[] replacement;
	delete[] pattern;
	delete[] subject;
}

extern "C" __declspec(dllexport)
void REClearAllOptions(HWND hwndParent, int string_size, TCHAR *variables, stack_t **stacktop, extra_parameters *extra)
{
	g_hwndParent=hwndParent;

	EXDLL_INIT();

	g_options.set_all_options(0);
}

extern "C" __declspec(dllexport)
void REClearOption(HWND hwndParent, int string_size, TCHAR *variables, stack_t **stacktop, extra_parameters *extra)
{
	g_hwndParent=hwndParent;

	EXDLL_INIT();

	TCHAR* pcreopt = new TCHAR[string_size];
	popstring(pcreopt);

	SetOrClearOption(pcreopt, false);

	delete[] pcreopt;
}

extern "C" __declspec(dllexport)
void RESetOption(HWND hwndParent, int string_size, TCHAR *variables, stack_t **stacktop, extra_parameters *extra)
{
	g_hwndParent=hwndParent;

	EXDLL_INIT();

	TCHAR* pcreopt = new TCHAR[string_size];
	popstring(pcreopt);

	SetOrClearOption(pcreopt, true);

	delete[] pcreopt;
}

extern "C" __declspec(dllexport)
void REGetOption(HWND hwndParent, int string_size, TCHAR *variables, stack_t **stacktop, extra_parameters *extra)
{
	g_hwndParent=hwndParent;

	EXDLL_INIT();

	TCHAR* pcreopt = new TCHAR[string_size];
	bool set = false;

	popstring(pcreopt);

	if (_tcscmp(_T("CASELESS"), pcreopt)==0)
		set = g_options.caseless();
	else if (_tcscmp(_T("MULTILINE"), pcreopt)==0)
		set = g_options.multiline();
	else if (_tcscmp(_T("DOTALL"), pcreopt)==0)
		set = g_options.dotall();
	else if (_tcscmp(_T("EXTENDED"), pcreopt)==0)
		set = g_options.extended();
	else if (_tcscmp(_T("DOLLAR_ENDONLY"), pcreopt)==0)
		set = g_options.dollar_endonly();
	else if (_tcscmp(_T("EXTRA"), pcreopt)==0)
		set = g_options.extra();
	else if (_tcscmp(_T("UTF8"), pcreopt)==0)
		set = g_options.utf8();
	else if (_tcscmp(_T("UNGREEDY"), pcreopt)==0)
		set = g_options.ungreedy();
	else if (_tcscmp(_T("NO_AUTO_CAPTURE"), pcreopt)==0)
		set = g_options.no_auto_capture();
	else if (_tcscmp(_T("i"), pcreopt)==0)
		set = g_options.caseless();
	else if (_tcscmp(_T("m"), pcreopt)==0)
		set = g_options.multiline();
	else if (_tcscmp(_T("s"), pcreopt)==0)
		set = g_options.dotall();
	else if (_tcscmp(_T("x"), pcreopt)==0)
		set = g_options.extended();

	delete[] pcreopt;

	pushstring(set?_T("true"):_T("false"));
}

