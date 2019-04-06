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
#endif

#define SAMPLE_BUFFER_SIZE 1024*4

// function prototypes
void init_crc32_table(void);
unsigned int reflect(unsigned int ref, wchar_t ch);

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
const char help[] =
"Usage: %ls [OPTION] [FILE]...\n"
//"   or: %s --check [OPTION] [FILE]\n"
"Print CRC32 checksums of the audio data in WAV files.\n"
//"Print or check CRC32 checksums of the audio data in WAV files.\n"
"With no FILE, or when FILE is -, read standard input.\n"
"\n"
"  -b, --block            compute CRC32 of all channels as a block (default)\n"
"  -c, --channel=<ch>     compute CRC32 of the channel <ch>\n"
"  -a, --all              use all samples (default)\n"
"  -n, --no-null          exclude null samples\n"
"  -l, --left-no-null     the left channel only, no null samples, (-c0 -n)\n"
"  -r, --report           print a CRC32 report, with extra info\n"
//"  -k, --check            check CRC32 sums against given list\n"
//"\n"
//"The following two options are useful only when verifying checksums:\n"
//"      --status            don't output anything, status code shows success\n"
//"  -w, --warn              warn about improperly formated checksum lines\n"
//"\n"
"      --help             display this help and exit\n"
"      --version          output version information and exit\n"
"\n"
"The program computes CRC32 ckecksums of the PCM audio data in RIFF WAV files,\n"
"using various methods.\n"
//"When checking, the input\n"
//"should be a former output of this program.  The default mode is to print\n"
//"a line with checksum, a character indicating type (`*' for binary, ` ' for\n"
//"text), and name for each FILE.\n"
;
const char ver[] = "0.5.0";

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
unsigned int reflect(unsigned int ref, wchar_t ch) {
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

int wmain(int argc, wchar_t** argv) {
#ifdef _WIN32
	_setmode(_fileno(stdin), _O_BINARY);
	//_setmode(_fileno(stdout), _O_BINARY);
	//_setmode(_fileno(stderr), _O_BINARY);
	_setmode(_fileno(stdout), _O_U16TEXT);
#endif
	init_crc32_table();
	wchar_t cmode = L'b';
	wchar_t smode = L'a';
	wchar_t cflag[10];
	FILLA(cflag, 10, 0);

	wchar_t is_opt = 1;
	int fc = 0;
	for (int n = 1; n <= argc; n++) {
		const wchar_t *arg = NULL;
		FILE* f = NULL;
		if (n == argc) {
			if (fc == 0) {
				arg = L"-";
				f = stdin;
			}
			else break;
		}
		else {
			// parse options
			arg = argv[n];
			if (wcscmp(L"--", arg) == 0) {
				is_opt = 0;
				continue;
			}
			else if (wcscmp(L"-", arg) == 0) {
				fc++;
				f = stdin;
			}
			else if (is_opt && wcsncmp(L"--", arg, 2) == 0) {
				if (wcscmp(L"--block", arg) == 0) {
					cmode = L'b';
					FILLA(cflag, 10, 0);
				}
				else if (wcsncmp(L"--channel=", arg, 10) == 0) {
					if (wcslen(arg) == 11) {
						arg += 10;
						if (*arg == L'l' || *arg == L'L') cflag[0] = 1;
						else if (*arg == L'r' || *arg == L'R') cflag[1] = 1;
						else {
							wchar_t c = *arg - L'0';
							if (c >= 0 && c < 10) cflag[c] = 1;
							else fprintf(stderr, "*Bad channel option\n");
						}
						cmode = L'c';
					}
					else fprintf(stderr, "*Bad channel option\n");
				}
				else if (wcscmp(L"--all", arg) == 0) {
					smode = L'a';
				}
				else if (wcscmp(L"--no-null", arg) == 0) {
					smode = L'n';
				}
				else if (wcscmp(L"--left-no-null", arg) == 0) {
					cmode = L'c';
					FILLA(cflag, 10, 0);
					cflag[0] = 1;
					smode = L'n';
				}
				else if (wcscmp(L"--report", arg) == 0) cmode = L'r';
				else if (wcscmp(L"--help", arg) == 0) {
					printf(help, argv[0]);
					return 0;
				}
				else if (wcscmp(L"--version", arg) == 0) {
					printf(ver);
					return 0;
				}
				else {
					fwprintf(stderr, L"*Unknown option: %ls\n", arg);
					return 1;
				}
				continue;
			}
			else if (is_opt && wcsncmp(L"-", arg, 1) == 0) {
				if (wcscmp(L"-b", arg) == 0) {
					cmode = L'b';
					FILLA(cflag, 10, 0);
				}
				else if (wcsncmp(L"-c", arg, 2) == 0) {
					if (wcslen(arg) == 3) {
						arg += 2;
						if (*arg == L'l' || *arg == L'L') cflag[0] = 1;
						else if (*arg == L'r' || *arg == L'R') cflag[1] = 1;
						else {
							wchar_t c = *arg - L'0';
							if (c >= 0 && c < 10) cflag[c] = 1;
							else fprintf(stderr, "*Bad channel option\n");
						}
						cmode = L'c';
					}
					else fprintf(stderr, "*Bad channel option\n");
				}
				else if (wcscmp(L"-a", arg) == 0) {
					smode = L'a';
				}
				else if (wcscmp(L"-n", arg) == 0) {
					smode = L'n';
				}
				else if (wcscmp(L"-l", arg) == 0) {
					cmode = L'c';
					FILLA(cflag, 10, 0);
					cflag[0] = 1;
					smode = L'n';
				}
				else if (wcscmp(L"-r", arg) == 0) cmode = L'r';
				else {
					fwprintf(stderr, L"*Unknown option: %ls\n", arg);
					return 1;
				}
				continue;
			}
			else {
				fc++;
				f = _wfopen(arg, L"rb");
			}
		}
		if (!f) {
			fwprintf(stderr, L"*Can not open file: %ls\n", arg);
			return 2;
		}
		if (cmode == L'r') wprintf(L"File: %ls\n", arg);

		// check RIFF WAV header
		unsigned int rn = 0;
		unsigned int code = 0;
		wav_format fmt;
		unsigned int data_size = 0;

		rn = fread(&code, 1, 4, f); // chunk ID, "RIFF"
		if (rn != 4 || code != 0x46464952) {
			fprintf(stderr, "*Not a RIFF file\n");
			return 2;
		}
		rn = fread(&code, 1, 4, f); // chunk size
		if (rn != 4) {
			fprintf(stderr, "*Corrupted file\n");
			return 2;
		}
		rn = fread(&code, 1, 4, f); // format, "WAVE"
		if (rn != 4 || code != 0x45564157) {
			fprintf(stderr, "*Not a RIFF WAV file\n");
			return 2;
		}
		rn = fread(&code, 1, 4, f); // subchunk ID, "fmt "
		if (rn != 4 || code != 0x20746d66) {
			fprintf(stderr, "*Missing WAV format chunk\n");
			return 2;
		}
		rn = fread(&code, 1, 4, f); // subchunk size, 16 for PCM, no extra params
		if (rn != 4 || code != 0x10) {
			fprintf(stderr, "*Bad WAV format chunk size\n");
			return 2;
		}

		rn = fread(&fmt, 1, 16, f); // format info
		if (rn != 16) {
			fprintf(stderr, "*Corrupted file\n");
			return 2;
		}
		unsigned short bal = fmt.block_align;
		unsigned short bps = fmt.bits_per_sample / 8; // BYTEs per sample
		unsigned short nch = fmt.num_channels;
		if (fmt.bits_per_sample % 8 != 0) {
			fprintf(stderr, "*Bits per sample not multiple of 8\n");
			return 2;
		}
		if (bal != bps * nch) {
			fprintf(stderr, "*Inconsistent WAV format\n");
			return 2;
		}
		if (fmt.byte_rate != fmt.sample_rate * bal) {
			fprintf(stderr, "*Inconsistent WAV format\n");
			return 2;
		}
		if (cmode == L'r') {
			printf("WAV header info:\n");
			printf("  format: %u\n", fmt.audio_format);
			printf("  channels: %u\n", fmt.num_channels);
			printf("  sample rate: %u\n", fmt.sample_rate);
			printf("  byte rate: %u\n", fmt.byte_rate);
			printf("  block align: %u\n", fmt.block_align);
			printf("  bits per sample: %u\n", fmt.bits_per_sample);
		}
		if (fmt.audio_format != 1) {
			fprintf(stderr, "*Not a RIFF PCM WAV file\n");
			return 2;
		}

		rn = fread(&code, 1, 4, f); // subchunk ID, "data"
		if (rn != 4 || code != 0x61746164) {
			fprintf(stderr, "*Missing WAV data chunk\n");
			return 2;
		}
		rn = fread(&data_size, 1, 4, f); // chunk size
		if (rn != 4) {
			fprintf(stderr, "*Corrupted file\n");
			return 2;
		}
		if (data_size % bal != 0) {
			fprintf(stderr, "*Data not aligned\n");
			fclose(f);
			return 2;
		}
		if (cmode == L'r') printf("RIFF WAV header checked and audio data size is %u bytes\n", data_size);

		unsigned int crc = 0xffffffff;
		unsigned int crcc[10];
		FILLA(crcc, 10, 0xffffffff);
		unsigned int ts = 0;
		unsigned int rs = bal * SAMPLE_BUFFER_SIZE;
		unsigned int frs = 0;
		unsigned char *buffer = malloc(bal * SAMPLE_BUFFER_SIZE);

		if (cmode == L'b') {
			if (smode == L'a') {
				if (data_size > 0) {
					if (data_size - ts < rs) rs = data_size - ts;
				}
				while (rs > 0) {
					frs = fread(buffer, 1, rs, f);
					if (frs != rs) {
						if (data_size == 0) {
							rs = 0;
							if (frs % bal != 0) {
								fprintf(stderr, "*Data not aligned");
								free(buffer);
								fclose(f);
								return 2;
							}
						}
						else {
							fprintf(stderr, "*Corrupted file");
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
				wprintf(L"%08X ** %ls\n", crc, arg);
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
								fprintf(stderr, "*Data not aligned");
								free(buffer);
								fclose(f);
								return 2;
							}
						}
						else {
							fprintf(stderr, "*Corrupted file");
							free(buffer);
							fclose(f);
							return 2;
						}
					}

					for (unsigned int i = 0; i < frs; i += bal) {
						for (short j = 0; j < bal; j += bps) {
							wchar_t v = 0;
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
				wprintf(L"%08X *+ %ls\n", crc, arg);
			}
		}
		else if (cmode == L'c') {
			if (smode == L'a') {
				if (data_size > 0) {
					if (data_size - ts < rs) rs = data_size - ts;
				}
				while (rs > 0) {
					frs = fread(buffer, 1, rs, f);
					if (frs != rs) {
						if (data_size == 0) {
							rs = 0;
							if (frs % bal != 0) {
								fprintf(stderr, "*Data not aligned");
								free(buffer);
								fclose(f);
								return 2;
							}
						}
						else {
							fprintf(stderr, "*Corrupted file");
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
					if (nch < 10 && cflag[c]) wprintf(L"%08X %01u* %ls\n", crcc[c], c, arg);
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
								fprintf(stderr, "*Data not aligned");
								free(buffer);
								fclose(f);
								return 2;
							}
						}
						else {
							fprintf(stderr, "*Corrupted file");
							free(buffer);
							fclose(f);
							return 2;
						}
					}

					for (unsigned int i = 0; i < frs;) {
						for (unsigned int c = 0; c < nch; c++) {
							if (nch < 10 && cflag[c]) {
								wchar_t v = 0;
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
					if (nch < 10 && cflag[c]) wprintf(L"%08X %01u+ %ls\n", crcc[c], c, arg);
				}
			}
		}
		else if (cmode == L'r') {
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
							fprintf(stderr, "*Data not aligned");
							free(buffer);
							fclose(f);
							return 2;
						}
					}
					else {
						fprintf(stderr, "*Corrupted file");
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
						wchar_t v = 0;
						for (unsigned int k = 0; k < bps; k++) {
							if (buffer[i + j + k]) {
								v = 1;
								break;
							}
						}
						if (v) for (unsigned int k = 0; k < bps; k++) crcn = (crcn >> 8) ^ crc32_table[(crcn & 0xFF) ^ buffer[i + j + k]];
					}

					// left channel only, skipping null samples
					wchar_t vl = 0;
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

			printf("Used audio data size is %u bytes\n", ts);
			printf("CRC32 sums of:\n");
			printf("  all channels / all samples:     %08X (EAC: grabbing, \"no use...\" off)\n", crc);
			//printf("  all channels / no null blocks:  %08X (EAC: no equivalent)\n", crcnb);
			printf("  all channels / no null samples: %08X (EAC: grabbing, \"no use...\" on)\n", crcn);
			printf("  left channel / no null samples: %08X (EAC: sound editor)\n", crcln);

			printf("Time elapsed: %u ms\n", (unsigned int)((clock() - clks) * 1000 / CLOCKS_PER_SEC));
			printf("----------------\n");
		}
		free(buffer);
		fclose(f);
	}
	return 0;
}
