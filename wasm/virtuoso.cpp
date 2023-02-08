#define VIRTUOSO_COMPILER
#include "Virtuoso.h"
#include "miniz.h"
#include "JSONOrderedEncoder.h"
#include "StreamEncoderDecoder.h"
#include <stdio.h>

using namespace Virtuoso;

enum {
    kSequence = 0,
    kSequences = 1,
    kSettings = 2
};

extern "C" char* encodeData(const uint8_t* data, size_t len)
{
    return StreamEncoder::compressAndBase64Encode(data, len);
}

extern "C" char* decodeData(const uint8_t* data, size_t len)
{
    return StreamEncoder::base64DecodeAndDecompress(data, len, false);
}

extern "C" char* encodeText(const uint8_t* data, size_t len)
{
    return StreamEncoder::compressAndBase64Encode(data, len);
}

extern "C" char* decodeText(const uint8_t* data, size_t len)
{
    return StreamEncoder::base64DecodeAndDecompress(data, len, true);
}

extern "C" char* encodeJSON(int type, const char* str, size_t len)
{
    JSONOrderedEncoder encoder(str, len);
    size_t maxOutput = 200*1024;
    uint8_t* output = (uint8_t*)malloc(maxOutput);
    char* result = nullptr;
    if (output != nullptr)
    {
        size_t encodedJSONSize = 0;
        if (type == kSequence)
            encodedJSONSize = encoder.encodeSequenceJSON(output, maxOutput);
        else if (type == kSequences)
            encodedJSONSize = encoder.encodeSequencesJSON(output, maxOutput);
        else if (type == kSettings)
            encodedJSONSize = encoder.encodeSettingsJSON(output, maxOutput);
        if (encodedJSONSize > 0)
        {
            result = StreamEncoder::base64Encode(output, encodedJSONSize);
        }
        free(output);
    }
    return result;
}

extern "C" char* deflateJSON(int type, const char* str, size_t len)
{
    JSONOrderedEncoder encoder(str, len);
    size_t maxOutput = 200*1024;
    uint8_t* output = (uint8_t*)malloc(maxOutput);
    char* result = nullptr;
    if (output != nullptr)
    {
        size_t encodedJSONSize = 0;
        if (type == kSequence)
            encodedJSONSize = encoder.encodeSequenceJSON(output, maxOutput);
        else if (type == kSequences)
            encodedJSONSize = encoder.encodeSequencesJSON(output, maxOutput);
        else if (type == kSettings)
            encodedJSONSize = encoder.encodeSettingsJSON(output, maxOutput);
        if (encodedJSONSize > 0)
        {
            result = StreamEncoder::compressAndBase64Encode(output, encodedJSONSize);
        }
        free(output);
    }
    return result;
}

static int asnprintf(char* &buf, size_t &remaining, const char* fmt, ...)
{
    va_list arg;
    va_start(arg, fmt);
    int cnt = vsnprintf(buf, remaining, fmt, arg);
    va_end(arg);
    size_t len = strlen(buf);
    buf += len;
    remaining -= len;
    return cnt;
}

static bool decodeSequenceJSON(uint8_t* p, char* out, size_t remaining)
{
    if (*p++ != JSONOrderedEncoder::kSEQUENCE_START)
        return false;
    asnprintf(out, remaining, "{");
    if (*p++ != JSONOrderedEncoder::kSEQID)
        return false;
    char* str = (char*)p;
    size_t len = strlen(str);
    asnprintf(out, remaining, "\"seqID\":\"%s\",", str);
    p += len+1;
    if (*p++ != JSONOrderedEncoder::kSEQNAM)
        return false;
    str = (char*)p;
    len = strlen(str);
    asnprintf(out, remaining, "\"seqNam\":\"%s\",", str);
    p += len+1;
    if (*p++ != JSONOrderedEncoder::kFRAMES_START)
        return false;
    asnprintf(out, remaining, "\"seqframes\":[");
    bool firstFrame = true;
    bool firstFrameField = true;
    bool firstServoField = true;
    uint8_t tag;
    do
    {
        tag = *p++;
        switch (tag)
        {
            case JSONOrderedEncoder::kFRAME_START:
                asnprintf(out, remaining, (firstFrame) ? "{" : ",{");
                firstFrameField = true;
                firstFrame = false;
                break;
            case JSONOrderedEncoder::kFRAME_END:
                asnprintf(out, remaining, "}");
                break;
            case JSONOrderedEncoder::kFRBEG:
                str = (char*)p;
                len = strlen(str);
                if (!firstFrameField)
                    asnprintf(out, remaining, ",");
                asnprintf(out, remaining, "\"frbeg\":\"%s\"", str);
                firstFrameField = false;
                p += len+1;
                break;
            case JSONOrderedEncoder::kFRDUR:
                if (!firstFrameField)
                    asnprintf(out, remaining, ",");
                asnprintf(out, remaining, "\"frdur\":%u", *(uint16_t*)p);
                firstFrameField = false;
                p += sizeof(uint16_t);
                break;
            case JSONOrderedEncoder::kFREND:
                if (!firstFrameField)
                    asnprintf(out, remaining, ",");
                str = (char*)p;
                len = strlen(str);
                asnprintf(out, remaining, "\"frend\":\"%s\"", str);
                firstFrameField = false;
                p += len+1;
                break;
            case JSONOrderedEncoder::kFRNAM:
                if (!firstFrameField)
                    asnprintf(out, remaining, ",");
                str = (char*)p;
                len = strlen(str);
                asnprintf(out, remaining, "\"frnam\":\"%s\"", str);
                firstFrameField = false;
                p += len+1;
                break;
            case JSONOrderedEncoder::kSERVOS_START:
                break;
            case JSONOrderedEncoder::kSERVOS_END:
                break;
            case JSONOrderedEncoder::kSRVNUM:
                if (!firstFrameField)
                    asnprintf(out, remaining, ",");
                firstFrameField = false;
                asnprintf(out, remaining, "\"%u\":{", *(uint16_t*)p);
                p += sizeof(uint16_t);
                firstServoField = true;
                break;
            case JSONOrderedEncoder::kSRVDUR:
                if (!firstServoField)
                    asnprintf(out, remaining, ",");
                asnprintf(out, remaining, "\"srvdur\":%u", *(uint16_t*)p);
                firstServoField = false;
                p += sizeof(uint16_t);
                break;
            case JSONOrderedEncoder::kSRVEAS:
                if (!firstServoField)
                    asnprintf(out, remaining, ",");
                asnprintf(out, remaining, "\"srveas\":%u", *(uint8_t*)p);
                firstServoField = false;
                p += sizeof(uint8_t);
                break;
            case JSONOrderedEncoder::kSRVENA:
                if (!firstServoField)
                    asnprintf(out, remaining, ",");
                asnprintf(out, remaining, "\"srvena\":%s", *(uint8_t*)p ? "true" : "false");
                firstServoField = false;
                p += sizeof(uint8_t);
                break;
            case JSONOrderedEncoder::kSRVPOS:
                if (!firstServoField)
                    asnprintf(out, remaining, ",");
                asnprintf(out, remaining, "\"srvpos\":%u}", *(uint16_t*)p);
                firstServoField = false;
                p += sizeof(uint16_t);
                break;
        }
    }
    while (tag != JSONOrderedEncoder::kSEQUENCE_END);
    asnprintf(out, remaining, "]");
    asnprintf(out, remaining, "}");
    return true;
}

extern "C" char* decodeJSON(int type, const uint8_t* str, size_t len)
{
    uint8_t* buffer = (uint8_t*)StreamEncoder::base64Decode(str, len);
    if (buffer == nullptr)
        return nullptr;
    size_t bufferSize = 200*1024;
    char* result = (char*)malloc(bufferSize+1);
    if (result == nullptr)
    {
        free(buffer);
        return nullptr;
    }
    *result = '\0';
    if (type == 0)
    {
        if (!decodeSequenceJSON(buffer, result, bufferSize))
        {
            free(result);
            result = nullptr;
        }
    }
    free(buffer);
    return result;
}

extern "C" char* inflateJSON(int type, const uint8_t* str, size_t len)
{
    uint8_t* buffer = (uint8_t*)StreamEncoder::base64DecodeAndDecompress(str, len);
    if (buffer == nullptr)
        return nullptr;
    size_t bufferSize = 200*1024;
    char* result = (char*)malloc(bufferSize+1);
    if (result == nullptr)
    {
        free(buffer);
        return nullptr;
    }
    *result = '\0';
    if (type == 0)
    {
        if (!decodeSequenceJSON(buffer, result, bufferSize))
        {
            free(result);
            result = nullptr;
        }
    }
    free(buffer);
    return result;
}

extern "C" char* compile(const char* str, int len)
{
    try {
        Virtuoso::Program program(str);
        size_t size;
        std::vector<uint8_t> byteCodes;
        std::vector<uint16_t> subroutines;
        program.getByteCodes(byteCodes, subroutines);

        size_t virtuosoSubroutineSize = 128 * sizeof(uint16_t);
        size_t virtuosoBlockSize = virtuosoSubroutineSize + byteCodes.size();
        uint8_t* virtuosoBlock = (uint8_t*)malloc(virtuosoBlockSize);
        if (virtuosoBlock == nullptr)
        {
            fprintf(stderr, "ERROR:FAILED TO ALLOCATE RESULT MEMORY\n");
            return nullptr;
        }
        uint8_t* virtuosoBytes = virtuosoBlock + virtuosoSubroutineSize;
        uint8_t* virtuosoSubroutine = virtuosoBlock;
        memset(virtuosoBlock, '\0', virtuosoBlockSize);
        *virtuosoSubroutine++ = (subroutines.size()&0xFF);
        *virtuosoSubroutine++ = (subroutines.size()>>8);
        for (auto sub : subroutines)
        {
            *virtuosoSubroutine++ = (sub&0xFF);
            *virtuosoSubroutine++ = (sub>>8);
        }
        memcpy(virtuosoBytes, byteCodes.data(), byteCodes.size());
        char* output = StreamEncoder::base64Encode(virtuosoBlock, virtuosoBlockSize);
        free(virtuosoBlock);
        if (output == nullptr)
        {
            fprintf(stderr, "ERROR:FAILED TO ALLOCATE RESULT MEMORY\n");
            return nullptr;
        }
        return output;
    } catch (std::exception& ex) {
        fprintf(stderr, "ERROR:%s\n", ex.what());
    } catch (std::string& ex) {
        fprintf(stderr, "ERROR:%s\n", ex.c_str());
    } catch (const char* ex) {
        fprintf(stderr, "ERROR:%s\n", ex);
    } catch (...) {
        fprintf(stderr, "ERROR:Unknown exception.\n");
    }
    return nullptr;
}
