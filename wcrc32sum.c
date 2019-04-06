/*
* The program can now work on Microsoft Windows well.
* (c) Steven Yang (https://github.com/yangzhaofeng/wcrc32sum), 2019
* The source is also in the PUBLIC DOMAIN.
* I will be glad if you can notice my name when you redistribute the code.
*/

/*
wcrc32sum.c
The program computes CRC32 ckecksums of the PCM audio data in RIFF WAV files,
using various methods.
(c) iceeLyne (iceelyne@gmail.com), 2013
This source is in the PUBLIC DOMAIN.
*/

/*
----------------------------------------------------------
wavcrc32.cpp
Version 0.22
----------------------------------------------------------
A program for counting audiodata CRC checksums
that used in EAC (Exact Audio Copy by Andre Wiethoff)
----------------------------------------------------------
This program is freeware and open-source, you may
freely redistribute or modify it, or use code in your
applications.
----------------------------------------------------------
(c) ]DichlofoS[ Systems, 2007
mailto:dmvn@mccme.ru
----------------------------------------------------------
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
//#include <tchar.h>
#define _T(x)      L ## x
#define main(a,b) wmain(a,b)
#define strcmp(a,b) wcscmp(a,b)
#define strncmp(a,b,c) wcsncmp(a,b,c)
#define strlen(a) wcslen(a)
#define fopen(a,b) _wfopen(a,b)
typedef wchar_t CHAR;
#else
typedef char CHAR;
#define _T(x) x
#define fwprintf fwprintf
#define wprintf printf
#endif

#define SAMPLE_BUFFER_SIZE 1024*4

// function prototypes
void init_crc32_table(void);
unsigned int reflect(unsigned int ref, CHAR ch);

// WAV audio format
#pragma pack(2)
// 16
typedef struct {
	unsigned short audio_format; // 1 PCM, etc
	unsigned short num_channels; // 1 mono, 2 stereo, etc
	unsigned int sample_rate; // 44100, etc
	unsigned int byte_rate; // sample_rate * num_channels * bits_per_sample / 8
	unsigned short block_align; // num_channels * bits_per_sample / 8
	unsigned short bits_per_sample; // 8, 16, etc
} wav_format;

unsigned int crc32_table[256];

// help string
const CHAR help[] =
_T("Usage: %ls [OPTION] [FILE]...\n")
//"   or: %s --check [OPTION] [FILE]\n"
_T("Print CRC32 checksums of the audio data in WAV files.\n")
//"Print or check CRC32 checksums of the audio data in WAV files.\n"
_T("With no FILE, or when FILE is -, read standard input.\n")
_T("\n")
_T("  -b, --block            compute CRC32 of all channels as a block (default)\n")
_T("  -c, --channel=<ch>     compute CRC32 of the channel <ch>\n")
_T("  -a, --all              use all samples (default)\n")
_T("  -n, --no-null          exclude null samples\n")
_T("  -l, --left-no-null     the left channel only, no null samples, (-c0 -n)\n")
_T("  -r, --report           print a CRC32 report, with extra info\n")
//"  -k, --check            check CRC32 sums against given list\n"
//"\n"
//"The following two options are useful only when verifying checksums:\n"
//"      --status            don't output anything, status code shows success\n"
//"  -w, --warn              warn about improperly formated checksum lines\n"
//"\n"
_T("      --help             display this help and exit\n")
_T("      --version          output version information and exit\n")
_T("\n")
_T("The program computes CRC32 ckecksums of the PCM audio data in RIFF WAV files,\n")
_T("using various methods.\n")
//"When checking, the input\n"
//"should be a former output of this program.  The default mode is to print\n"
//"a line with checksum, a character indicating type (`*' for binary, ` ' for\n"
//"text), and name for each FILE.\n"
;
const CHAR ver[] = _T("0.6.1");

// initialize the CRC table
void init_crc32_table(void) {
	unsigned int ulPolynomial = 0x04c11db7;

	// 256 values representing ASCII character codes.
	for (int i = 0; i <= 0xFF; i++) {
		crc32_table[i] = reflect(i, 8) << 24;
		for (int j = 0; j < 8; j++)
			crc32_table[i] = (crc32_table[i] << 1) ^ (crc32_table[i] & (1 << 31) ? ulPolynomial : 0);
		crc32_table[i] = reflect(crc32_table[i], 32);
	}
}

// reflection is a requirement for the official CRC-32 standard.
unsigned int reflect(unsigned int ref, CHAR ch) {
	unsigned int value = 0;

	// swap bit 0 for bit 7
	// bit 1 for bit 6, etc.
	for (int i = 1; i < (ch + 1); i++) {
		if (ref & 1)
			value |= 1 << (ch - i);
		ref >>= 1;
	}
	return value;
}

#define FILLA(A, L, V) for(unsigned int i = 0; i < L; i ++) {A[i] = V;}

int main(int argc, CHAR** argv) {
#ifdef _WIN32
	_setmode(_fileno(stdin), _O_BINARY);
	//_setmode(_fileno(stdout), _O_BINARY);
	//_setmode(_fileno(stderr), _O_BINARY);
	_setmode(_fileno(stdout), _O_U16TEXT);
#endif
	init_crc32_table();
	CHAR cmode = _T('b');
	CHAR smode = _T('a');
	CHAR cflag[10];
	FILLA(cflag, 10, 0);

	CHAR is_opt = 1;
	int fc = 0;
	for (int n = 1; n <= argc; n++) {
		const CHAR *arg = NULL;
		FILE* f = NULL;
		if (n == argc) {
			if (fc == 0) {
				arg = _T("-");
				f = stdin;
			}
			else break;
		}
		else {
			// parse options
			arg = argv[n];
			if (strcmp(_T("--"), arg) == 0) {
				is_opt = 0;
				continue;
			}
			else if (strcmp(_T("-"), arg) == 0) {
				fc++;
				f = stdin;
			}
			else if (is_opt && strncmp(_T("--"), arg, 2) == 0) {
				if (strcmp(_T("--block"), arg) == 0) {
					cmode = _T('b');
					FILLA(cflag, 10, 0);
				}
				else if (strncmp(_T("--channel="), arg, 10) == 0) {
					if (strlen(arg) == 11) {
						arg += 10;
						if (*arg == _T('l') || *arg == _T('L')) cflag[0] = 1;
						else if (*arg == _T('r') || *arg == _T('R')) cflag[1] = 1;
						else {
							CHAR c = *arg - _T('0');
							if (c >= 0 && c < 10) cflag[c] = 1;
							else fwprintf(stderr, _T("*Bad channel option\n"));
						}
						cmode = _T('c');
					}
					else fwprintf(stderr, _T("*Bad channel option\n"));
				}
				else if (strcmp(_T("--all"), arg) == 0) {
					smode = _T('a');
				}
				else if (strcmp(_T("--no-null"), arg) == 0) {
					smode = _T('n');
				}
				else if (strcmp(_T("--left-no-null"), arg) == 0) {
					cmode = _T('c');
					FILLA(cflag, 10, 0);
					cflag[0] = 1;
					smode = _T('n');
				}
				else if (strcmp(_T("--report"), arg) == 0) cmode = _T('r');
				else if (strcmp(_T("--help"), arg) == 0) {
					wprintf(help, argv[0]);
					return 0;
				}
				else if (strcmp(_T("--version"), arg) == 0) {
					wprintf(ver);
					return 0;
				}
				else {
					fwprintf(stderr, _T("*Unknown option: %s\n"), arg);
					return 1;
				}
				continue;
			}
			else if (is_opt && strncmp(_T("-"), arg, 1) == 0) {
				if (strcmp(_T("-b"), arg) == 0) {
					cmode = _T('b');
					FILLA(cflag, 10, 0);
				}
				else if (strncmp(_T("-c"), arg, 2) == 0) {
					if (strlen(arg) == 3) {
						arg += 2;
						if (*arg == _T('l') || *arg == _T('L')) cflag[0] = 1;
						else if (*arg == _T('r') || *arg == _T('R')) cflag[1] = 1;
						else {
							CHAR c = *arg - _T('0');
							if (c >= 0 && c < 10) cflag[c] = 1;
							else fwprintf(stderr, _T("*Bad channel option\n"));
						}
						cmode = _T('c');
					}
					else fwprintf(stderr, _T("*Bad channel option\n"));
				}
				else if (strcmp(_T("-a"), arg) == 0) {
					smode = _T('a');
				}
				else if (strcmp(_T("-n"), arg) == 0) {
					smode = _T('n');
				}
				else if (strcmp(_T("-l"), arg) == 0) {
					cmode = _T('c');
					FILLA(cflag, 10, 0);
					cflag[0] = 1;
					smode = _T('n');
				}
				else if (strcmp(_T("-r"), arg) == 0) cmode = _T('r');
				else {
					fwprintf(stderr, _T("*Unknown option: %s\n"), arg);
					return 1;
				}
				continue;
			}
			else {
				fc++;
				f = fopen(arg, _T("rb"));
			}
		}
		if (!f) {
			fwprintf(stderr, _T("*Can not open file: %s\n"), arg);
			return 2;
		}
		if (cmode == _T('r')) wprintf(_T("File: %s\n"), arg);

		// check RIFF WAV header
		unsigned int rn = 0;
		unsigned int code = 0;
		wav_format fmt;
		unsigned int data_size = 0;

		rn = fread(&code, 1, 4, f); // chunk ID, "RIFF"
		if (rn != 4 || code != 0x46464952) {
			fwprintf(stderr, _T("*Not a RIFF file\n"));
			return 2;
		}
		rn = fread(&code, 1, 4, f); // chunk size
		if (rn != 4) {
			fwprintf(stderr, _T("*Corrupted file\n"));
			return 2;
		}
		rn = fread(&code, 1, 4, f); // format, "WAVE"
		if (rn != 4 || code != 0x45564157) {
			fwprintf(stderr, _T("*Not a RIFF WAV file\n"));
			return 2;
		}
		rn = fread(&code, 1, 4, f); // subchunk ID, "fmt "
		if (rn != 4 || code != 0x20746d66) {
			fwprintf(stderr, _T("*Missing WAV format chunk\n"));
			return 2;
		}
		rn = fread(&code, 1, 4, f); // subchunk size, 16 for PCM, no extra params
		if (rn != 4 || code != 0x10) {
			fwprintf(stderr, _T("*Bad WAV format chunk size\n"));
			return 2;
		}

		rn = fread(&fmt, 1, 16, f); // format info
		if (rn != 16) {
			fwprintf(stderr, _T("*Corrupted file\n"));
			return 2;
		}
		unsigned short bal = fmt.block_align;
		unsigned short bps = fmt.bits_per_sample / 8; // BYTEs per sample
		unsigned short nch = fmt.num_channels;
		if (fmt.bits_per_sample % 8 != 0) {
			fwprintf(stderr, _T("*Bits per sample not multiple of 8\n"));
			return 2;
		}
		if (bal != bps * nch) {
			fwprintf(stderr, _T("*Inconsistent WAV format\n"));
			return 2;
		}
		if (fmt.byte_rate != fmt.sample_rate * bal) {
			fwprintf(stderr, _T("*Inconsistent WAV format\n"));
			return 2;
		}
		if (cmode == _T('r')) {
			wprintf(_T("WAV header info:\n"));
			wprintf(_T("  format: %u\n", fmt.audio_format));
			wprintf(_T("  channels: %u\n", fmt.num_channels));
			wprintf(_T("  sample rate: %u\n", fmt.sample_rate));
			wprintf(_T("  byte rate: %u\n", fmt.byte_rate));
			wprintf(_T("  block align: %u\n", fmt.block_align));
			wprintf(_T("  bits per sample: %u\n", fmt.bits_per_sample));
		}
		if (fmt.audio_format != 1) {
			fwprintf(stderr, _T("*Not a RIFF PCM WAV file\n"));
			return 2;
		}

		rn = fread(&code, 1, 4, f); // subchunk ID, "data"
		if (rn != 4 || code != 0x61746164) {
			fwprintf(stderr, _T("*Missing WAV data chunk\n"));
			return 2;
		}
		rn = fread(&data_size, 1, 4, f); // chunk size
		if (rn != 4) {
			fwprintf(stderr, _T("*Corrupted file\n"));
			return 2;
		}
		if (data_size % bal != 0) {
			fwprintf(stderr, _T("*Data not aligned\n"));
			fclose(f);
			return 2;
		}
		if (cmode == _T('r')) wprintf(_T("RIFF WAV header checked and audio data size is %u bytes\n", data_size));

		unsigned int crc = 0xffffffff;
		unsigned int crcc[10];
		FILLA(crcc, 10, 0xffffffff);
		unsigned int ts = 0;
		unsigned int rs = bal * SAMPLE_BUFFER_SIZE;
		unsigned int frs = 0;
		unsigned char *buffer = malloc(bal * SAMPLE_BUFFER_SIZE);

		if (cmode == _T('b')) {
			if (smode == _T('a')) {
				if (data_size > 0) {
					if (data_size - ts < rs) rs = data_size - ts;
				}
				while (rs > 0) {
					frs = fread(buffer, 1, rs, f);
					if (frs != rs) {
						if (data_size == 0) {
							rs = 0;
							if (frs % bal != 0) {
								fwprintf(stderr, _T("*Data not aligned"));
								free(buffer);
								fclose(f);
								return 2;
							}
						}
						else {
							fwprintf(stderr, _T("*Corrupted file"));
							free(buffer);
							fclose(f);
							return 2;
						}
					}

					for (unsigned int i = 0; i < frs; i++) {
						crc = (crc >> 8) ^ crc32_table[(crc & 0xFF) ^ buffer[i]];
					}

					ts += frs;
					if (data_size > 0) {
						if (data_size - ts < rs) rs = data_size - ts;
					}
				}
				crc = crc ^ 0xffffffff;
				wprintf(_T("%08X ** %s\n"), crc, arg);
			}
			else {
				if (data_size > 0) {
					if (data_size - ts < rs) rs = data_size - ts;
				}
				while (rs > 0) {
					frs = fread(buffer, 1, rs, f);
					if (frs != rs) {
						if (data_size == 0) {
							rs = 0;
							if (frs % bal != 0) {
								fwprintf(stderr, _T("*Data not aligned"));
								free(buffer);
								fclose(f);
								return 2;
							}
						}
						else {
							fwprintf(stderr, _T("*Corrupted file"));
							free(buffer);
							fclose(f);
							return 2;
						}
					}

					for (unsigned int i = 0; i < frs; i += bal) {
						for (short j = 0; j < bal; j += bps) {
							CHAR v = 0;
							for (unsigned int k = 0; k < bps; k++) {
								if (buffer[i + j + k]) {
									v = 1;
									break;
								}
							}
							if (v) for (unsigned int k = 0; k < bps; k++) crc = (crc >> 8) ^ crc32_table[(crc & 0xFF) ^ buffer[i + j + k]];
						}
					}

					ts += frs;
					if (data_size > 0) {
						if (data_size - ts < rs) rs = data_size - ts;
					}
				}
				crc = crc ^ 0xffffffff;
				wprintf(_T("%08X *+ %s\n"), crc, arg);
			}
		}
		else if (cmode == _T('c')) {
			if (smode == _T('a')) {
				if (data_size > 0) {
					if (data_size - ts < rs) rs = data_size - ts;
				}
				while (rs > 0) {
					frs = fread(buffer, 1, rs, f);
					if (frs != rs) {
						if (data_size == 0) {
							rs = 0;
							if (frs % bal != 0) {
								fwprintf(stderr, _T("*Data not aligned"));
								free(buffer);
								fclose(f);
								return 2;
							}
						}
						else {
							fwprintf(stderr, _T("*Corrupted file"));
							free(buffer);
							fclose(f);
							return 2;
						}
					}

					for (unsigned int i = 0; i < frs;) {
						for (unsigned int c = 0; c < nch; c++) {
							if (nch < 10 && cflag[c]) {
								for (unsigned int k = 0; k < bps; k++) crcc[c] = (crcc[c] >> 8) ^ crc32_table[(crcc[c] & 0xFF) ^ buffer[i + k]];
							}
							i += bps;
						}
					}

					ts += frs;
					if (data_size > 0) {
						if (data_size - ts < rs) rs = data_size - ts;
					}
				}
				for (unsigned int c = 0; c < nch; c++) {
					crcc[c] = crcc[c] ^ 0xffffffff;
					if (nch < 10 && cflag[c]) wprintf(_T("%08X %01u* %s\n"), crcc[c], c, arg);
				}
			}
			else {
				if (data_size > 0) {
					if (data_size - ts < rs) rs = data_size - ts;
				}
				while (rs > 0) {
					frs = fread(buffer, 1, rs, f);
					if (frs != rs) {
						if (data_size == 0) {
							rs = 0;
							if (frs % bal != 0) {
								fwprintf(stderr, _T("*Data not aligned"));
								free(buffer);
								fclose(f);
								return 2;
							}
						}
						else {
							fwprintf(stderr, _T("*Corrupted file"));
							free(buffer);
							fclose(f);
							return 2;
						}
					}

					for (unsigned int i = 0; i < frs;) {
						for (unsigned int c = 0; c < nch; c++) {
							if (nch < 10 && cflag[c]) {
								CHAR v = 0;
								for (unsigned int k = 0; k < bps; k++) {
									if (buffer[i + k]) {
										v = 1;
										break;
									}
								}
								if (v) for (unsigned int k = 0; k < bps; k++) crcc[c] = (crcc[c] >> 8) ^ crc32_table[(crcc[c] & 0xFF) ^ buffer[i + k]];
							}
							i += bps;
						}
					}

					ts += frs;
					if (data_size > 0) {
						if (data_size - ts < rs) rs = data_size - ts;
					}
				}
				for (unsigned int c = 0; c < 10; c++) {
					crcc[c] = crcc[c] ^ 0xffffffff;
					if (nch < 10 && cflag[c]) wprintf(_T("%08X %01u+ %s\n"), crcc[c], c, arg);
				}
			}
		}
		else if (cmode == _T('r')) {
			unsigned int crc = 0xffffffff;
			//unsigned int crcnb = 0xffffffff;
			unsigned int crcn = 0xffffffff;
			unsigned int crcln = 0xffffffff;

			clock_t clks = clock();

			if (data_size > 0) {
				if (data_size - ts < rs) rs = data_size - ts;
			}
			while (rs > 0) {
				frs = fread(buffer, 1, rs, f);
				if (frs != rs) {
					if (data_size == 0) {
						rs = 0;
						if (frs % bal != 0) {
							fwprintf(stderr, _T("*Data not aligned"));
							free(buffer);
							fclose(f);
							return 2;
						}
					}
					else {
						fwprintf(stderr, _T("*Corrupted file"));
						free(buffer);
						fclose(f);
						return 2;
					}
				}

				for (unsigned int i = 0; i < frs; i += bal) {
					// all channels (block), all samples
					for (unsigned int j = 0; j < bal; j++) {
						crc = (crc >> 8) ^ crc32_table[(crc & 0xFF) ^ buffer[i + j]];
					}

					//// all channels (block), skipping null blocks
					//short vb = 0;
					//for (unsigned int j = 0; j < bal; j ++) {
					//    if(buffer[i+j]) {
					//        vb = 1;
					//        break;
					//    }
					//}
					//if(vb) for(unsigned int j = 0; j < bal; j ++)
					//    crcnb = (crcnb >> 8) ^ crc32_table[(crcnb & 0xFF) ^ buffer[i+j]];

					// all channels (block), skipping null samples
					for (unsigned int j = 0; j < bal; j += bps) {
						CHAR v = 0;
						for (unsigned int k = 0; k < bps; k++) {
							if (buffer[i + j + k]) {
								v = 1;
								break;
							}
						}
						if (v) for (unsigned int k = 0; k < bps; k++) crcn = (crcn >> 8) ^ crc32_table[(crcn & 0xFF) ^ buffer[i + j + k]];
					}

					// left channel only, skipping null samples
					CHAR vl = 0;
					for (unsigned int k = 0; k < bps; k++) {
						if (buffer[i + k]) {
							vl = 1;
							break;
						}
					}
					if (vl) for (unsigned int k = 0; k < bps; k++) crcln = (crcln >> 8) ^ crc32_table[(crcln & 0xFF) ^ buffer[i + k]];
				}

				ts += frs;
				if (data_size > 0) {
					if (data_size - ts < rs) rs = data_size - ts;
				}
			}
			crc = crc ^ 0xffffffff;
			//crcnb = crcnb ^ 0xffffffff;
			crcn = crcn ^ 0xffffffff;
			crcln = crcln ^ 0xffffffff;

			wprintf(_T("Used audio data size is %u bytes\n", ts));
			wprintf(_T("CRC32 sums of:\n"));
			wprintf(_T("  all channels / all samples:     %08X (EAC: grabbing, \"no use...\" off)\n", crc));
			//wprintf(_T("  all channels / no null blocks:  %08X (EAC: no equivalent)\n", crcnb));
			wprintf(_T("  all channels / no null samples: %08X (EAC: grabbing, \"no use...\" on)\n", crcn));
			wprintf(_T("  left channel / no null samples: %08X (EAC: sound editor)\n", crcln));

			wprintf(_T("Time elapsed: %u ms\n", (unsigned int)((clock() - clks) * 1000 / CLOCKS_PER_SEC)));
			wprintf(_T("----------------\n"));
		}
		free(buffer);
		fclose(f);
	}
	return 0;
}
