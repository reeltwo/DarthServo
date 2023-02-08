///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Base64.h"
//#include "miniz.h"

#define STREAM_ENCODER_MAGIC 0x6c616D53
#define STREAM_HEADER_SIZE   sizeof(uint32_t)*3

class StreamEncoder
{
public:
	static char* compressAndBase64Encode(const uint8_t* data, size_t len)
	{
	    mz_ulong outputSize = mz_compressBound(len);
	    uint8_t* output = (uint8_t*)malloc(outputSize + STREAM_HEADER_SIZE);
	    if (output == nullptr)
	        return nullptr;
	    if (mz_compress(output + STREAM_HEADER_SIZE, &outputSize, data, len) != MZ_OK)
	    {
	        // Compression failed for some reason. Shouldn't really happen.
	        free(output);
	        return nullptr;
	    }
	    // Write the compressed buffer over the original data
	    // with our header so we can base64 encode everything
	    ((uint32_t*)output)[0] = STREAM_ENCODER_MAGIC;
	    ((uint32_t*)output)[1] = len;
	    ((uint32_t*)output)[2] = outputSize;

	    // Add our header to the output size
	    outputSize += STREAM_HEADER_SIZE;

	    size_t outputBase64Size = Base64::encodeLength((char*)output, outputSize);
	    char* outputBase64 = (char*)malloc(outputBase64Size);
	    if (outputBase64 != nullptr)
	    {
	        Base64::encode((char*)output, outputSize, outputBase64);
	    }
	    free(output);
	    return outputBase64;
	}

    static char* base64Encode(const uint8_t* data, size_t len)
    {
        size_t outputBase64Size = Base64::encodeLength((char*)data, len);
        char* outputBase64 = (char*)malloc(outputBase64Size);
        if (outputBase64 != nullptr)
        {
            Base64::encode((char*)data, len, outputBase64);
        }
        return outputBase64;
    }

	static char* base64DecodeAndDecompress(const uint8_t* encodedBuffer, size_t &len, bool appendZero = false)
	{
	    size_t decodeSize = Base64::decodeLength((char*)encodedBuffer);
	    char* sourceBuffer = (char*)malloc(decodeSize);
	    if (sourceBuffer == nullptr)
        {
            len = 0;
	    	return nullptr;
        }
        decodeSize = Base64::decode((char*)encodedBuffer, sourceBuffer);

	    if (decodeSize < STREAM_HEADER_SIZE ||
	    	((uint32_t*)sourceBuffer)[0] != STREAM_ENCODER_MAGIC)
		{
            len = 0;
			free(sourceBuffer);
		    return nullptr;
		}
		mz_ulong outputLen = ((uint32_t*)sourceBuffer)[1];
		mz_ulong sourceLen = ((uint32_t*)sourceBuffer)[2];

		uint8_t* outputBuffer = (uint8_t*)malloc(outputLen + (appendZero ? 1 : 0));
	    if (outputBuffer == nullptr)
	    {
            len = 0;
	    	free(sourceBuffer);
	    	return nullptr;
	    }
		int err = mz_uncompress2(outputBuffer, &outputLen, (uint8_t*)sourceBuffer + STREAM_HEADER_SIZE, &sourceLen);
		free(sourceBuffer);
		if (err != MZ_OK)
		{
            len = 0;
			free(outputBuffer);
			return nullptr;
		}
        len = size_t(outputLen);
        if (appendZero)
            outputBuffer[len] = 0;
		return (char*)outputBuffer;
   	}

    static char* base64Decode(const uint8_t* encodedBuffer, size_t &len, bool appendZero = false)
    {
        size_t decodeSize = Base64::decodeLength((char*)encodedBuffer);
        char* decodedBuffer = (char*)malloc(decodeSize+1);
        len = 0;
        if (decodedBuffer != nullptr)
        {
            decodeSize = Base64::decode((char*)encodedBuffer, decodedBuffer);
            if (appendZero)
                decodedBuffer[decodeSize] = 0;
        }
        return decodedBuffer;
    }
};

#ifdef ESP32
class FileStream
{
public:
    FileStream()
    {
        fError = true;
    }

    FileStream(fs::FS &fs, const char* path)
    {
        reset(fs, path);
    }

    ~FileStream()
    {
        close();
    }

    bool hasError()
    {
        return !fFile || fError;
    }

    template<typename T>
    T &read(T &t)
    {
        fError = (fError || readBytes((uint8_t*)&t, sizeof(t)) != sizeof(t));
        return t;
    }

    template<typename T>
    const T &write(const T &t)
    {
        fError = (fError || writeBytes((uint8_t*)&t, sizeof(t)) != sizeof(t));
        return t;
    }

    virtual size_t readBytes(void* dst, size_t len)
    {
        size_t readBytes = 0;
        if (!fError)
        {
            readBytes = fFile.read((uint8_t*)dst, len);
            fError = (len != readBytes);
        }
        if (!fError)
            fOffset += readBytes;
        return readBytes;
    }

    virtual size_t writeBytes(const void* src, size_t len)
    {
        size_t written = 0;
        if (!fError)
        {
            written = fFile.write((uint8_t*)src, len);
            fError = (len != written);
        }
        if (!fError)
            fOffset += written;
        return written;
    }

    void readString(char* dst, size_t maxLen)
    {
        char ch = 0;
        char* str = dst;
        while (read(ch) != '\0')
        {
            if (str - dst < maxLen-1)
                *str++ = ch;
        }
        *str = '\0';
    }

    void writeString(const char* src)
    {
        while (*src != '\0')
        {
            write(*src++);
        }
        write(*src);
    }

    virtual size_t fileSize()
    {
        return fFile ? fFile.size() : 0;
    }

    virtual size_t size()
    {
        return fOffset;
    }

    virtual void close()
    {
        if (fFile)
            fFile.close();
    }

    virtual void reset(fs::FS &fs, const char* path)
    {
        close();
        fFile = fs.open(path);
        if (!fFile)
        {
            printf("FAILED TO OPEN: %s\n", path);
        }
        fOffset = 0;
        fError = (!fFile) ? true : false;
    }

protected:
    File fFile;
    bool fError = false;
    uint32_t fOffset = 0;
};

class CompressedFileInputStream: public FileStream
{
public:
    CompressedFileInputStream()
    {
    }

    CompressedFileInputStream(fs::FS &fs, const char* path) :
        FileStream(fs, path)
    {
    }

    ~CompressedFileInputStream()
    {
        close();
    }

    virtual size_t readBytes(void* dst, size_t len) override
    {
        uint8_t* dptr = (uint8_t*)dst;
        // Check if our read can be completed from the output buffer
        while (!fError && len != 0)
        {
            while (fDecodePtr < fStream.next_out && len != 0)
            {
                uint8_t b = *fDecodePtr++;
                // printf("%02X ", b);
                *dptr++ = b;
                len--;
                fDecompressedOffset++;
            }
            if (len != 0)
            {
                // Need to decompress more
                if (!fillBuffer())
                    break;
            }
        }
        return dptr - (uint8_t*)dst;
    }

    bool fillBuffer()
    {
        if (fError || !fFile)
            return false;
        size_t inputBytes = std::min(fCompressedSize - fOffset, sizeof(fInputBuffer));
        inputBytes = FileStream::readBytes(&fInputBuffer[fStream.avail_in], inputBytes - fStream.avail_in);
        fStream.avail_in = inputBytes;
        fStream.next_in = fInputBuffer;
        fStream.avail_out = sizeof(fOutputBuffer);
        fStream.next_out = fOutputBuffer;
        fDecodePtr = fOutputBuffer;

        printf("fStream.total_in: %u\n", uint32_t(fStream.total_in));
        printf("fStream.avail_in: %u\n", uint32_t(fStream.avail_in));
        printf("fCompressedSize:  %u\n", uint32_t(fCompressedSize));
        printf("FLUSH: %s\n", (fStream.total_in + fStream.avail_in == fCompressedSize) ?
                "MZ_FINISH" : "MZ_PARTIAL_FLUSH");
        int err = mz_inflate(&fStream,
            (fStream.total_in + fStream.avail_in == fCompressedSize) ?
                MZ_FINISH : MZ_PARTIAL_FLUSH);
        if (fStream.avail_in != 0)
        {
            memmove(fInputBuffer, fStream.next_in, fStream.avail_in);
        }
        switch (err)
        {
            case MZ_OK:
            case MZ_STREAM_END:
                break;

            case MZ_BUF_ERROR:
            case MZ_DATA_ERROR:
            case MZ_MEM_ERROR:
            default:
                // Error out
                printf("inflate err: %d\n", err);
                fError = true;
                return false;
        }
        return true;
    }

    virtual size_t writeBytes(const void* src, size_t len) override
    {
        // Only reading is supported
        fError = true;
        return 0;
    }

    virtual void close() override
    {
        FileStream::close();
        if (fStreamInit)
        {
            mz_inflateEnd(&fStream);
            fStreamInit = false;
        }
    }

    virtual size_t fileSize() override
    {
        return fUncompressedSize;
    }

    virtual size_t size()
    {
        return fDecompressedOffset;
    }

    virtual void reset(fs::FS &fs, const char* path) override
    {
        // Avoid reallocating the stream buffer if already allocated
        bool streamInit = fStreamInit;
        fStreamInit = false;
        FileStream::reset(fs, path);
        fStreamInit = streamInit;

        if (!fStreamInit)
        {
            memset(&fStream, '\0', sizeof(fStream));
            fError = (mz_inflateInit2(&fStream, MZ_DEFAULT_WINDOW_BITS) != Z_OK);
            fStreamInit = true;
        }
        else
        {
            mz_deflateReset(&fStream);
        }
        uint32_t magicSignature = 0;
        FileStream::readBytes(&magicSignature, sizeof(magicSignature));
        fError = (fError || magicSignature != STREAM_ENCODER_MAGIC);
        FileStream::readBytes(&fUncompressedSize, sizeof(fUncompressedSize));
        FileStream::readBytes(&fCompressedSize, sizeof(fCompressedSize));
        fStream.next_out = fDecodePtr = fOutputBuffer;
        fDecompressedOffset = fOffset = 0;
    }

private:
    mz_stream fStream;
    bool fStreamInit = false;
    uint32_t fDecompressedOffset = 0;
    uint8_t fInputBuffer[32];
    uint8_t fOutputBuffer[4096];
    uint32_t fUncompressedSize = 0;
    uint32_t fCompressedSize = 0;
    uint8_t* fDecodePtr = nullptr;
};

class PartitionStream
{
public:
    PartitionStream(const esp_partition_t* partition = nullptr) :
        fPartition(partition),
        fLimit(partition ? partition->size : 0)
    {
    }

    bool hasError()
    {
        return fError;
    }

    template<typename T>
    T &read(T &t)
    {
        fError = (fError || fOffset + sizeof(t) > fLimit ||
            esp_partition_read(fPartition, fOffset, &t, sizeof(t)) != ESP_OK);
        if (!fError)
            fOffset += sizeof(t);
        return t;
    }

    template<typename T>
    const T &write(const T &t)
    {
        fError = (fError || fOffset + sizeof(t) > fLimit ||
            esp_partition_write(fPartition, fOffset, &t, sizeof(t)) != ESP_OK);
        if (!fError)
            fOffset += sizeof(t);
        return t;
    }

    size_t readBytes(void* dst, size_t len)
    {
        fError = (fError || fOffset + len > fLimit ||
            esp_partition_read(fPartition, fOffset, dst, len) != ESP_OK);
        if (!fError)
        {
            fOffset += len;
            return len;
        }
        return 0;
    }

    void writeBytes(const void* src, size_t len)
    {
        fError = (fError || fOffset + len > fLimit ||
            esp_partition_write(fPartition, fOffset, src, len) != ESP_OK);
        if (!fError)
            fOffset += len;
    }

    void readString(char* dst, size_t maxLen)
    {
        char ch = 0;
        char* str = dst;
        while (read(ch) != '\0')
        {
            if (str - dst < maxLen-1)
                *str++ = ch;
        }
        *str = '\0';
    }

    void writeString(const char* src)
    {
        while (*src != '\0')
        {
            write(*src++);
        }
        write(*src);
    }

    bool erase()
    {
        fError = (esp_partition_erase_range(fPartition, 0, fLimit) != ESP_OK);
        return !fError;
    }

    size_t size()
    {
        return fOffset;
    }

    void reset(const esp_partition_t* partition = nullptr)
    {
        if (partition != nullptr)
        {
            fPartition = partition;
            fLimit = partition->size;
        }
        fOffset = 0;
        fError = false;
    }

    bool fError = false;

protected:
    const esp_partition_t* fPartition;
    uint32_t fOffset = 0;
    uint32_t fLimit = 0;
};

class StreamBase64Encoder
{
public:
    StreamBase64Encoder(const esp_partition_t* partition, uint32_t magicSignature) :
        fPartition(partition),
        fMagicSignature(magicSignature),
        fDataLength(0),
        fOffset(0)
    {
    }

    StreamBase64Encoder(fs::FS &fs, const char* path)
    {
        reset(fs, path);
    }

    StreamBase64Encoder(File file) :
        fPartition(nullptr),
        fMagicSignature(0),
        fDataLength(0),
        fOffset(0)
    {
        reset(file);
    }

    ~StreamBase64Encoder()
    {
        close();
        if (fFile)
            fFile.close();
    }

    void close()
    {
        if (fFile)
            fFile.close();
    }

    void reset(fs::FS &fs, const char* path)
    {
        reset(fs.open(path));
    }

    void reset(File file)
    {
        close();
        fFile = file;
        fOffset = 0;
        fDataLength = fFile.size();
        fPartition = nullptr;
        fMagicSignature = 0;
    }

    bool read()
    {
        if (fFile)
            return true;

        uint32_t magicSignature = 0;
        fDataLength = fOffset = 0;
        if (fPartition != nullptr &&
            esp_partition_read(fPartition, fOffset, &magicSignature, sizeof(magicSignature)) == ESP_OK &&
            magicSignature == fMagicSignature)
        {
            fOffset += sizeof(magicSignature);
            if (esp_partition_read(fPartition, fOffset, &fDataLength, sizeof(fDataLength)) == ESP_OK &&
                fDataLength < fPartition->size)
            {
                fOffset += sizeof(fDataLength);
                return true;
            }
        }
        fDataLength = fOffset = 0;
        return false;
    }

    bool write(Print& out)
    {
        while (fDataLength != 0)
        {
            // Output buffer size needs to be a multiple of 3 for base64 encoding.
            char buffer[1026];
            size_t remaining = std::min(fDataLength, sizeof(buffer));
            if (fFile)
            {
                remaining = fFile.read((uint8_t*)buffer, remaining);
            }
            else if (fPartition == nullptr ||
                     esp_partition_read(fPartition, fOffset, buffer, remaining) != ESP_OK)
            {
                fDataLength = fOffset = 0;
                return false;
            }
            char encodeBuffer[1400];
            size_t encodeLength = Base64::encode(buffer, remaining, encodeBuffer);
            out.write(encodeBuffer, encodeLength-1);
            fDataLength -= remaining;
            fOffset += remaining;
        }
        return (fDataLength != 0);
    }

private:
    const esp_partition_t* fPartition = nullptr;
    uint32_t fMagicSignature = 0;
    size_t fDataLength = 0;
    size_t fOffset = 0;
    File fFile;
};

class StreamBase64Decoder
{
public:
	StreamBase64Decoder(uint32_t magicSignature = 0) :
		fMagicSignature(magicSignature)
	{}

    void init(uint32_t magicSignature, const esp_partition_t* partition)
    {
        fOffset = 0;
        fMagicSignature = magicSignature;
        fPartition = partition;
        fSuccess = false;
    }

    void init(fs::FS &fs, const char* path)
    {
        fOffset = 0;
        fMagicSignature = 0;
        fPartition = nullptr;
        fSuccess = false;
        fFile = fs.open(path, FILE_WRITE);
    }

    bool received(uint8_t* encodedBuffer, size_t len)
    {
        if (fError || fSuccess)
            return false;
       	if (fOffset == 0 && !fFile)
       	{
            if (fPartition != nullptr ||
                esp_partition_erase_range(fPartition, 0, fPartition->size) != ESP_OK)
            {
                // Failed to format partition, error out
                fError = true;
                return false;
            }
            fOffset += sizeof(uint32_t) * 2;
       	}
        char buffer[256+1];
        for (unsigned i = 0; i <= len / 16; i += 16)
        {
            size_t decodedBytes = Base64::decode((char*)encodedBuffer, buffer, sizeof(buffer)-1);
            if (fFile)
            {
                size_t wroteBytes = fFile.write((uint8_t*)buffer, decodedBytes);
                if (wroteBytes != decodedBytes)
                {
                    printf("Failed to write %u [%u written]\n", decodedBytes, wroteBytes);
                    fError = true;
                    return false;
                }
            }
            else if (fPartition != nullptr ||
                     esp_partition_write(fPartition, fOffset, buffer, decodedBytes) != ESP_OK)
            {
            	// Failed to write data, error out
            	fError = true;
                return false;
            }
            encodedBuffer += sizeof(buffer)-1;
            fOffset += decodedBytes;
        }
        return true;
    }

    bool end()
    {
    	// bytes written so far
    	uint32_t len = fOffset;
        if (fFile)
        {
            fFile.close();
        }
        else if (fPartition == nullptr ||
                 esp_partition_write(fPartition, 0, &fMagicSignature, sizeof(fMagicSignature)) != ESP_OK ||
                 esp_partition_write(fPartition, sizeof(uint32_t), &len, sizeof(len)) != ESP_OK)
        {
            // Failed to write header length, error out and erase partition
            fError = true;
            if (fPartition != nullptr)
            {
                esp_partition_erase_range(fPartition, 0, fPartition->size);
                uint32_t magicSignature = 0;
                esp_partition_write(fPartition, 0, &magicSignature, sizeof(magicSignature));
            }
            return false;
        }
        else
        {
            // Partition storage has a header
            len -= sizeof(uint32_t) * 2;
        }
        fSuccess = true;
        return true;
    }

    bool successful()
    {
        return fSuccess;
    }

private:
	uint32_t fMagicSignature = 0;
    const esp_partition_t* fPartition = nullptr;
    uint32_t fOffset = 0;
    bool fError = false;
    bool fSuccess = false;
    File fFile;
};

class StreamDecompressor
{
public:
	StreamDecompressor(uint32_t magicSignature = 0) :
		fMagicSignature(magicSignature)
	{}

    void init(uint32_t magicSignature, const esp_partition_t* partition)
    {
        fOffset = 0;
        fMagicSignature = magicSignature;
        fPartition = partition;
        fUncompressedSize = 0;
        fCompressedSize = 0;
        fSuccess = false;
        fError = false;
        memset(&fStream, '\0', sizeof(fStream));
    }

    void init(fs::FS &fs, const char* path)
    {
        fOffset = 0;
        fMagicSignature = 0;
        fPartition = nullptr;
        fUncompressedSize = 0;
        fCompressedSize = 0;
        fSuccess = false;
        fError = false;
        fFile = fs.open(path, FILE_WRITE);
        memset(&fStream, '\0', sizeof(fStream));
    }

    void init(fs::FS &fs, const char* path, const char* compressedPath)
    {
        init(fs, path);
        fCompressedFile = fs.open(compressedPath, FILE_WRITE);
    }

    bool received(uint8_t* encodedBuffer, size_t len)
    {
        if (fError || fSuccess)
            return false;
        uint8_t outBuffer[1024];
        bool finished = false;
        constexpr size_t bufIncrements = 256;
        char buffer[bufIncrements*2];
        for (unsigned i = 0; !finished && i <= len / 16; i += 16)
        {
            // printf("CHUNK %d of %d\n", i, (len / bufIncrements)+1);
            size_t decodedBytes;
            decodedBytes = Base64::decode((char*)encodedBuffer, &buffer[fStream.avail_in], bufIncrements);
            if (fCompressedFile)
            {
                fCompressedFile.write((uint8_t*)&buffer[fStream.avail_in], decodedBytes);
            }
            decodedBytes += fStream.avail_in;

            uint32_t decodeOffset = 0;
            if (fOffset == 0)
            {
                // Need a minimum of our header size and the magic signature must match
                if (decodedBytes < STREAM_HEADER_SIZE || ((uint32_t*)buffer)[0] != STREAM_ENCODER_MAGIC)
                {
                    fError = true;
                    return false;
                }
                fUncompressedSize = ((uint32_t*)buffer)[1];
                fCompressedSize = ((uint32_t*)buffer)[2];
                decodeOffset += STREAM_HEADER_SIZE;
                decodedBytes -= decodeOffset;
                printf("uncompressedSize: %u\n", fUncompressedSize);
                printf("compressedSize:   %u\n", fCompressedSize);
                if (mz_inflateInit2(&fStream, MZ_DEFAULT_WINDOW_BITS) != Z_OK)
                {
                    fError = true;
                    return false;
                }
            }
            // Now we can start decompression

            fStream.avail_in = decodedBytes;
            fStream.next_in = (uint8_t*)buffer + decodeOffset;
            fStream.avail_out = sizeof(outBuffer);
            fStream.next_out = outBuffer;
            //UInt8* outBuf = (UInt8*)&(outputBuf->fByte[inOffset]);

            // for (unsigned qi = 0; qi < fStream.avail_in; qi++)
            // {
            //     printf("%02X ", fStream.next_in[qi]);
            // }
            // printf("\n");
            // printf("decodedBytes : %d\n", decodedBytes);
            if (fStream.total_in + fStream.avail_in == fCompressedSize)
            {
                finished = true;
            }
            int err = mz_inflate(&fStream,
                (fStream.total_in + fStream.avail_in == fCompressedSize) ?
                    MZ_FINISH : MZ_PARTIAL_FLUSH);
            size_t amountWritten = sizeof(outBuffer) - fStream.avail_out;
            if (fStream.avail_in != 0)
            {
                // We have left over bytes - prefill the decode buffer
                memmove(buffer, fStream.next_in, fStream.avail_in);
                // printf("PARTIAL BUFFER: %d\n", fStream.avail_in);
            }
            switch (err)
            {
                case MZ_OK:
                case MZ_STREAM_END:
                    break;

                case MZ_BUF_ERROR:
                case MZ_DATA_ERROR:
                case MZ_MEM_ERROR:
                default:
                    // Error out
                    printf("inflate err: %d\n", err);
                    fError = true;
                    return false;
            }

            if (fOffset == 0 && !fFile)
            {
                if (fPartition == nullptr ||
                    esp_partition_erase_range(fPartition, 0, fPartition->size) != ESP_OK)
                {
                    // Failed to format partition, error out
                    fError = true;
                    return false;
                }
                fOffset += sizeof(uint32_t) * 2;
            }
            if (fFile)
            {
                size_t wroteBytes = fFile.write(outBuffer, amountWritten);
                if (wroteBytes != amountWritten)
                {
                    printf("Failed to write %u [%u written]\n", amountWritten, wroteBytes);
                    fError = true;
                    return false;
                }
            }
            else if (fPartition == nullptr ||
                     esp_partition_write(fPartition, fOffset, outBuffer, amountWritten) != ESP_OK)
            {
                // Failed to write output buffer, error out
                fError = true;
                return false;
            }
            encodedBuffer += bufIncrements;
            fOffset += amountWritten;
        }
        // Successful
        return true;
    }

    bool end()
    {
        if (fStream.next_in != nullptr)
        {
            mz_inflateEnd(&fStream);
            memset(&fStream, '\0', sizeof(fStream));
        }
        if (fCompressedFile)
            fCompressedFile.close();
        if (fFile)
        {
            fFile.close();
        }
        else if (fPartition == nullptr ||
            esp_partition_write(fPartition, 0, &fMagicSignature, sizeof(fMagicSignature)) != ESP_OK ||
            esp_partition_write(fPartition, sizeof(uint32_t), &fOffset, sizeof(fOffset)) != ESP_OK)
        {
            // Failed to write header length, error out and erase partition
            fError = true;
            if (fPartition != nullptr)
            {
                esp_partition_erase_range(fPartition, 0, fPartition->size);
                uint32_t magicSignature = 0;
                esp_partition_write(fPartition, 0, &magicSignature, sizeof(magicSignature));
            }
            return false;
        }
        fSuccess = true;
        return true;
    }

    bool successful()
    {
        return fSuccess;
    }

private:
	uint32_t fMagicSignature = 0;
    const esp_partition_t* fPartition = nullptr;
    uint32_t fUncompressedSize = 0;
    uint32_t fCompressedSize = 0;
    uint32_t fOffset = 0;
    bool fError = false;
    bool fSuccess = false;
    mz_stream fStream;
    File fFile;
    File fCompressedFile;
};
#endif
