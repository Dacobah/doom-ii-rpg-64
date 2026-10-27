#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL.h>
#include "ZipFile.h"
#include "SDLGL.h"

#ifdef N64
#include <libdragon.h>

ZipFile::ZipFile() {
	this->file = nullptr;
	this->entry_count = 0;
	this->entry = nullptr;
	this->page_count = 0;
	this->page = nullptr;
}

ZipFile::~ZipFile() {
	this->closeZipFile();
}

void ZipFile::openZipFile(const char* name) {
	(void)name;
	// DragonFS filesystem is initialized globally at boot
}

void ZipFile::closeZipFile() {
}

void ZipFile::findAndReadZipDir(int startoffset) {
	(void)startoffset;
}

uint8_t* ZipFile::readZipFileEntry(const char* name, int* sizep) {
	const char* rel = strstr(name, "Packages/");
	const char* dfsPath = rel ? rel : name;
	while (*dfsPath == '/') dfsPath++;

	int handle = dfs_open(dfsPath);
	if (handle < 0) {
		char altPath[256];
		snprintf(altPath, sizeof(altPath), "/%s", dfsPath);
		handle = dfs_open(altPath);
	}

	if (handle < 0) {
		printf("ZipFile (DragonFS): Failed to open %s (err: %d, requested: %s)\n", dfsPath, handle, name);
		if (sizep) *sizep = 0;
		return nullptr;
	}

	int size = dfs_size(handle);
	if (size <= 0) {
		printf("ZipFile (DragonFS): Invalid size %d for %s\n", size, dfsPath);
		dfs_close(handle);
		if (sizep) *sizep = 0;
		return nullptr;
	}

	uint8_t* buf = (uint8_t*)std::malloc(size);
	if (!buf) {
		printf("ZipFile: Failed to allocate %d bytes for %s\n", size, dfsPath);
		dfs_close(handle);
		if (sizep) *sizep = 0;
		return nullptr;
	}

	int readBytes = dfs_read(buf, 1, size, handle);
	dfs_close(handle);

	if (readBytes != size) {
		printf("ZipFile: Short read (%d/%d) for %s\n", readBytes, size, dfsPath);
	}

	if (sizep) *sizep = size;
	return buf;
}

#else // !N64 (Original PC implementation)

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

void Error(const char* fmt, ...)
{
	char errMsg[256];
	va_list ap;
	va_start(ap, fmt);
#ifdef _WIN32
	vsprintf_s(errMsg, sizeof(errMsg), fmt, ap);
#else
	vsnprintf(errMsg, sizeof(errMsg), fmt, ap);
#endif
	va_end(ap);

	printf("%s", errMsg);

	const SDL_MessageBoxButtonData buttons[] = {
		{ 0, 0, "Ok" },
	};
	const SDL_MessageBoxColorScheme colorScheme = {
		{
			{ 255,   0,   0 },
			{   0, 255,   0 },
			{ 255, 255,   0 },
			{   0,   0, 255 },
			{ 255,   0, 255 }
		}
	};
	const SDL_MessageBoxData messageboxdata = {
		SDL_MESSAGEBOX_ERROR,
		NULL,
		"Doom II RPG Error",
		errMsg,
		SDL_arraysize(buttons),
		buttons,
		&colorScheme
	};

	SDL_ShowMessageBox(&messageboxdata, NULL);
	exit(0);
}

static void* zip_alloc(void* ctx, unsigned int items, unsigned int size)
{
	return std::malloc(items * size);
}

static void zip_free(void* ctx, void* address)
{
	std::free(address);
}

static int _ReadByte(FILE* f)
{
	return fgetc(f);
}

static int _ReadShort(FILE* f)
{
	int v;

	v = fgetc(f);
	v |= fgetc(f) << 8;

	return v;
}

static int _ReadInt(FILE* f)
{
	int v;

	v = fgetc(f);
	v |= fgetc(f) << 8;
	v |= fgetc(f) << 16;
	v |= fgetc(f) << 24;

	return v;
}

ZipFile::ZipFile() {
	this->file = nullptr;
	this->entry_count = 0;
	this->entry = nullptr;
	this->page_count = 0;
	this->page = nullptr;
}

ZipFile::~ZipFile() {
	this->closeZipFile();
}

void ZipFile::findAndReadZipDir(int startoffset) {
	int files, count, offset, size;
	int i;

	fseek(this->file, startoffset, SEEK_SET);

	if (_ReadInt(this->file) != ZIP_END_OF_CENTRAL_DIRECTORY_SIG) {
		Error("cannot find end of central directory");
	}

	_ReadShort(this->file);
	_ReadShort(this->file);
	files = _ReadShort(this->file);
	count = _ReadShort(this->file);
	size = _ReadInt(this->file);
	offset = _ReadInt(this->file);
	_ReadShort(this->file);

	if (files != count) {
		Error("multiple disk zipfiles are not supported");
	}

	this->entry_count = count;
	this->entry = (zip_entry_t*)std::malloc(count * sizeof(zip_entry_t));

	fseek(this->file, offset, SEEK_SET);

	for (i = 0; i < count; i++)
	{
		char name[256];
		int namelength, extralength, commentsize;

		if (_ReadInt(this->file) != ZIP_CENTRAL_DIRECTORY_SIG) {
			Error("wrong zip central directory signature");
		}

		_ReadShort(this->file); // version made by
		_ReadShort(this->file); // version needed to extract
		_ReadShort(this->file); // general purpose bit flag
		_ReadShort(this->file); // compression method
		_ReadShort(this->file); // last mod file time
		_ReadShort(this->file); // last mod file date
		_ReadInt(this->file); // crc-32
		this->entry[i].csize = _ReadInt(this->file);
		this->entry[i].usize = _ReadInt(this->file);
		namelength = _ReadShort(this->file);
		extralength = _ReadShort(this->file);
		commentsize = _ReadShort(this->file);
		_ReadShort(this->file); // disk number start
		_ReadShort(this->file); // internal file attributes
		_ReadInt(this->file); // external file attributes
		this->entry[i].offset = _ReadInt(this->file);

		if (namelength >= sizeof(name)) {
			Error("zipfile entry name is too long (%d)", namelength);
		}

		fread(name, sizeof(uint8_t), namelength, this->file);
		name[namelength] = 0;

		this->entry[i].name = (char*)std::malloc(namelength + 1);
		std::memcpy(this->entry[i].name, name, namelength + 1);

		fseek(this->file, extralength, SEEK_CUR);
		fseek(this->file, commentsize, SEEK_CUR);
	}
}

void ZipFile::openZipFile(const char* name) {

	uint8_t buf[512];
	int filesize, back, maxback;
	int i, n;

	this->file = fopen(name, "rb");
	if (this->file == NULL) {
		Error("openZipFile: cannot open file %s\n", name);
	}

	fseek(this->file, 0, SEEK_END);
	filesize = (int)ftell(this->file);
	fseek(this->file, 0, SEEK_SET);

	maxback = MIN(filesize, 0xFFFF + sizeof(buf));
	back = MIN(maxback, sizeof(buf));

	while (back < maxback)
	{
		fseek(this->file, filesize - back, SEEK_SET);
		n = sizeof(buf);
		fread(buf, sizeof(uint8_t), n, this->file);
		for (i = n - 4; i > 0; i--)
		{
			if (!std::memcmp(buf + i, "PK\5\6", 4)) {
				findAndReadZipDir(filesize - back + i);
				return;
			}
		}
		back += sizeof(buf) - 4;
	}

	Error("cannot find end of central directory\n");
}

void ZipFile::closeZipFile() {
	if (this) {
		if (this->entry) {
			std::free(this->entry);
			this->entry = nullptr;
		}

		if (this->file) {
			fclose(this->file);
			this->file = nullptr;
		}
	}
}

uint8_t* ZipFile::readZipFileEntry(const char* name, int* sizep) {
	zip_entry_t* entry = nullptr;
	int i, sig, general, method, namelength, extralength;
	uint8_t* cdata;
	int code;

	for (i = 0; i < this->entry_count; i++)
	{
		zip_entry_t* entryTmp = this->entry + i;
		if (!SDL_strcasecmp(name, entryTmp->name)) {
			entry = this->entry + i;
			break;
		}
	}

	if (entry == NULL) {
		Error("did not find the %s file in the zip file", name);
	}

	fseek(this->file, entry->offset, SEEK_SET);

	sig = _ReadInt(this->file);
	if (sig != ZIP_LOCAL_FILE_SIG) {
		Error("wrong zip local file signature (0x%x)", sig);
	}

	_ReadShort(this->file);
	general = _ReadShort(this->file);
	if (general & ZIP_ENCRYPTED_FLAG) {
		Error("zipfile content is encrypted");
	}

	method = _ReadShort(this->file);
	_ReadShort(this->file);
	_ReadShort(this->file);
	_ReadInt(this->file);
	_ReadInt(this->file);
	_ReadInt(this->file);
	namelength = _ReadShort(this->file);
	extralength = _ReadShort(this->file);

	fseek(this->file, namelength + extralength, SEEK_CUR);

	cdata = (uint8_t*)std::malloc(entry->csize);
	fread(cdata, sizeof(uint8_t), entry->csize, this->file);

	if (method == 0)
	{
		*sizep = entry->csize;
		return cdata;
	}
	else if (method == 8)
	{
		uint8_t* udata = (uint8_t*)std::malloc(entry->usize);

		z_stream stream;
		std::memset(&stream, 0, sizeof stream);

		stream.next_in = cdata;
		stream.avail_in = entry->csize;
		stream.next_out = udata;
		stream.avail_out = entry->usize;
		stream.zalloc = zip_alloc;
		stream.zfree = zip_free;
		stream.opaque = Z_NULL;

		code = inflateInit2(&stream, -15);
		if (code != Z_OK) {
			Error("inflateInit2 returned %d", code);
		}

		code = inflate(&stream, Z_FINISH);
		if (code != Z_STREAM_END) {
			inflateEnd(&stream);
			Error("inflate returned %d", code);
		}

		code = inflateEnd(&stream);
		if (code != Z_OK) {
			Error("inflateEnd returned %d", code);
		}

		std::free(cdata);
		*sizep = entry->usize;
		return udata;
	}
	else
	{
		Error("unknown compression method %d", method);
	}

	return nullptr;
}

#endif
