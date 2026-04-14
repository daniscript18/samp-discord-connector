//----------------------------------------------------------
//
//   SA-MP Multiplayer Modification For GTA:SA
//   Copyright 2014 SA-MP Team, Dan, maddinat0r
//
//----------------------------------------------------------

#include <cstring>
#include <cstdlib>

//----------------------------------------------------------

#include <assert.h>

//----------------------------------------------------------

#include "amx2.h"

//----------------------------------------------------------

int AMXAPI amx_PushAddress(AMX *amx, cell *address) 
{
	AMX_HEADER *hdr;
	unsigned char *data;
	cell xaddr;
	/* reverse relocate the address */
	assert(amx != NULL);
	hdr = (AMX_HEADER *) amx->base;
	assert(hdr != NULL);
	assert(hdr->magic == AMX_MAGIC);
	data = (amx->data != NULL) ? amx->data : amx->base + (int) hdr->dat;
	xaddr = (cell) ((unsigned char*) address-data);
	if ((ucell) xaddr >= (ucell) amx->stp) 
	{
		return AMX_ERR_MEMACCESS;
	}
	return amx_Push(amx,xaddr);
}

void AMXAPI amx_Redirect(AMX *amx, char *from, ucell to, AMX_NATIVE *store) 
{
	AMX_HEADER *hdr = (AMX_HEADER*) amx->base;
	AMX_FUNCSTUB *func;
	for (int idx = 0, num = NUMENTRIES(hdr, natives, libraries); idx != num; ++idx) 
	{
		func = GETENTRY(hdr, natives, idx);
		if (!strcmp(from, GETENTRYNAME(hdr, func))) 
		{
			if (store) 
			{
				*store = (AMX_NATIVE) func->address;
			}
			func->address = to;
			return;
		}
	}
}

int AMXAPI amx_GetCString(AMX *amx, cell param, char *&dest) 
{
	cell *ptr;
	amx_GetAddr(amx, param, &ptr);
	int len;
	amx_StrLen(ptr, &len);
	dest = (char*) malloc((len + 1) * sizeof(char));
	if (dest != NULL) 
	{
		amx_GetString(dest, ptr, 0, UNLIMITED);
		dest[len] = 0;
		return len;
	}
	return 0;
}

int AMXAPI amx_SetCString(AMX *amx, cell param, const char *str, int len) 
{
	cell *dest;
	int error;
	if ((error = amx_GetAddr(amx, param, &dest)) != AMX_ERR_NONE)
		return error;

	return amx_SetString(dest, str, 0, 0, len);
}

#if defined __cplusplus

namespace
{
	bool IsValidUtf8(std::string const &input)
	{
		unsigned int expected_continuation = 0;

		for (unsigned char byte : input)
		{
			if (expected_continuation != 0)
			{
				if ((byte & 0xC0) != 0x80)
					return false;

				--expected_continuation;
				continue;
			}

			if ((byte & 0x80) == 0)
				continue;

			if ((byte & 0xE0) == 0xC0)
			{
				if (byte < 0xC2)
					return false;
				expected_continuation = 1;
			}
			else if ((byte & 0xF0) == 0xE0)
			{
				expected_continuation = 2;
			}
			else if ((byte & 0xF8) == 0xF0)
			{
				if (byte > 0xF4)
					return false;
				expected_continuation = 3;
			}
			else
			{
				return false;
			}
		}

		return expected_continuation == 0;
	}

	void AppendUtf8(std::string &output, unsigned int codepoint)
	{
		if (codepoint <= 0x7F)
		{
			output.push_back(static_cast<char>(codepoint));
		}
		else if (codepoint <= 0x7FF)
		{
			output.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
			output.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
		}
		else
		{
			output.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
			output.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
			output.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
		}
	}

	unsigned int Cp1252ToUnicode(unsigned char byte)
	{
		static const unsigned int cp1252_controls[32] = {
			0x20AC, 0xFFFD, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021,
			0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0xFFFD, 0x017D, 0xFFFD,
			0xFFFD, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
			0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0xFFFD, 0x017E, 0x0178,
		};

		if (byte >= 0x80 && byte <= 0x9F)
			return cp1252_controls[byte - 0x80];

		return byte;
	}

	std::string EnsureUtf8(std::string input)
	{
		if (IsValidUtf8(input))
			return input;

		std::string output;
		output.reserve(input.size() * 2);
		for (unsigned char byte : input)
			AppendUtf8(output, Cp1252ToUnicode(byte));
		return output;
	}
}

std::string AMXAPI amx_GetCppString(AMX *amx, cell param) 
{
	cell *addr = nullptr;
	amx_GetAddr(amx, param, &addr);

	int len = 0;
	amx_StrLen(addr, &len);

	std::string string(len, ' ');
	amx_GetString(&string[0], addr, 0, len + 1);

	return EnsureUtf8(std::move(string));
}

int AMXAPI amx_SetCppString(AMX *amx, cell param, const std::string &str, size_t maxlen)
{
	cell *dest = nullptr;
	int error;
	if ((error = amx_GetAddr(amx, param, &dest)) != AMX_ERR_NONE)
		return error;

	return amx_SetString(dest, str.c_str(), 0, 0, maxlen);
}

#endif // __cplusplus

//----------------------------------------------------------
// EOF
