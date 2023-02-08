
class JSONOrderedEncoder
{
public:
    enum {
        /* Groups */
        kSEQUENCES_START = 1,
        kSEQUENCES_END = 2,
        kSEQUENCE_START = 3,
        kSEQUENCE_END = 4,
        kFRAMES_START = 5,
        kFRAMES_END = 6,
        kFRAME_START = 7,
        kFRAME_END = 8,
        kSERVOS_START = 9,
        kSERVOS_END = 0xa,
        kSETTINGS_START = 0xb,
        kSETTINGS_END = 0xc,
        kSETTING_START = 0xd,
        kSETTING_END = 0xe,

        /* Sequence fields */
        kSEQID = 0xf,
        kSEQNAM = 0x10,

        /* Frame fields */
        kFRBEG = 0x11,
        kFRDUR = 0x12,
        kFREND = 0x13,
        kFRNAM = 0x14,

        /* Servo fields */
        kSRVNUM = 0x15,
        kSRVDUR = 0x16,
        kSRVEAS = 0x17,
        kSRVENA = 0x18,
        kSRVPOS = 0x19,

        /* Settings fields */
        //kSRVENA,
        kSRVERR = 0x1a,
        kSRVMAX = 0x1b,
        kSRVMIN = 0x1c,
        kSRVNAM = 0x1d,
        kSRVUSEERR = 0x1e
    };
    JSONOrderedEncoder(const char* text, size_t len) :
        fText(text),
        fPtr(text),
        fEnd(fPtr + len),
        fLength(len)
    {
    }

    char read()
    {
        char ch = fPushBack;
        if (ch == 0 && !fError && fLength > 0)
        {
            if (fPtr < fEnd)
            {
                ch = *fPtr++;
                fLength--;
            }
            else
            {
                fError = true;
            }
        }
        fPushBack = 0;
        return ch;
    }

    bool expect(char ch)
    {
        char c = read();
        if (c == ch)
            return true;
        fError = true;
        fprintf(stderr, "OUT: EXPECTED %c but got %c\n", ch, c);
        return false;
    }

    bool expectKeyword(const char* key)
    {
        const char* keyword = key;
        while (*key != '\0')
        {
            if (!expect(*key++))
            {
                fprintf(stderr,"OUT:EXPECTED %s\n", keyword);
                return false;
            }
        }
        return true;
    }

    bool expectKey(const char* key)
    {
        // fprintf(stderr, "OUT:EXPECT %s\n", key);
        if (!expect('"'))
            return false;
        if (!expectKeyword(key))
            return false;
        if (!expect('"'))
            return false;
        if (!expect(':'))
            return false;
        return true;
    }

    char* expectString(char* buf, size_t maxlen)
    {
        char* p = buf;
        char* end = buf + maxlen-1;
        if (expect('"'))
        {
            while (!fError)
            {
                char ch = read();
                if (fError || ch == '"')
                    break;
                if (p < end)
                    *p++ = ch;
            }
        }
        *p = '\0';
        return buf;
    }

    bool ignoreString()
    {
        char buf[2];
        expectString(buf, sizeof(buf));
        return !fError;
    }

    long expectInteger()
    {
        bool negative = false;
        long result = 0;
        char ch = read();
        if (ch == '-')
            negative = true;
        else
            pushBack(ch);
        while (!fError)
        {
            ch = read();
            if (fError)
                break;
            if (!isdigit(ch))
            {
                pushBack(ch);
                break;
            }
            result = result * 10 + (ch - '0');
        }
        return negative ? -result : result;
    }

    long expectStringInteger()
    {
        long value = 0;
        expect('"');
        value = expectInteger();
        expect('"');
        return value;
    }

    long expectPossibleStringInteger()
    {
        char ch = read();
        pushBack(ch);
        return (ch == '"') ? expectStringInteger() : expectInteger();
    }

    bool expectBoolean()
    {
        char ch = read();
        pushBack(ch);
        if (ch == 't' && expectKeyword("true"))
            return true;
        if (ch == 'f' && expectKeyword("false"))
            return false;
        return false;
    }

    inline void pushBack(char ch)
    {
        fPushBack = ch;
    }

    inline bool hasError()
    {
        return fError;
    }

    template<typename T>
    void write(const T &t)
    {
        if (fOut+sizeof(t) < fOutEnd)
        {
            memcpy(fOut, &t, sizeof(t)); fOut += sizeof(t);
        }
        else
        {
            fError = true;
        }
    }

    inline void writeTag(uint8_t b)
    {
        write(b);
    }

    void outString(const char* str)
    {
        while (*str != '\0')
            write(*str++);
        writeTag(*str);
    }

    size_t encodeSettingsJSON(uint8_t* outputBuffer, size_t maxLen)
    {
        char str[64];
        fOut = outputBuffer;
        fOutEnd = fOut + maxLen;
        if (!expect('['))
            return 0;
        writeTag(kSETTINGS_START);
        for (;;)
        {
            char ch = read();
            pushBack(ch);
            if (ch == ']')
                break;
            if (!expect('{'))
                return 0;
            writeTag(kSETTING_START);
            if (expectKey("srvena"))
            {
                bool servoEnabled = expectBoolean();
                writeTag(kSRVENA);
                write(servoEnabled);
            }
            if (!expect(','))
                return 0;
            if (expectKey("srverr"))
            {
                uint16_t srverr = uint16_t(expectPossibleStringInteger());
                writeTag(kSRVERR);
                write(srverr);
            }
            if (!expect(','))
                return 0;
            if (expectKey("srvmax"))
            {
                uint16_t srvmax = uint16_t(expectPossibleStringInteger());
                writeTag(kSRVMAX);
                write(srvmax);
            }
            if (!expect(','))
                return 0;
            if (expectKey("srvmin"))
            {
                uint16_t srvmin = uint16_t(expectPossibleStringInteger());
                writeTag(kSRVMIN);
                write(srvmin);
            }
            if (!expect(','))
                return 0;
            if (expectKey("srvnam"))
            {
                expectString(str, sizeof(str));
                writeTag(kSRVNAM);
                outString(str);
            }
            if (!expect(','))
                return 0;
            if (expectKey("srvuseerr"))
            {
                bool errorEnabled = expectBoolean();
                writeTag(kSRVUSEERR);
                write(errorEnabled);
            }
            if (!expect('}'))
                return 0;
            writeTag(kSETTING_END);

            ch = read();
            pushBack(ch);
            if (ch == ']')
                break;
            if (!expect(','))
                return 0;
        }
        if (!expect(']'))
            return 0;
        writeTag(kSETTINGS_END);
        return fOut - outputBuffer;
    }

    size_t encodeSequencesJSON(uint8_t* outputBuffer, size_t maxLen)
    {
        fOut = outputBuffer;
        fOutEnd = outputBuffer + maxLen;
        if (!expect('['))
            return 0;
        writeTag(kSEQUENCES_START);
        for (;;)
        {
            char ch = read();
            pushBack(ch);
            if (ch == '{' && !encodeSequenceJSON(fOut, (outputBuffer + maxLen) - fOut))
                return 0;
            ch = read();
            pushBack(ch);
            if (ch == ']')
                break;
            if (!expect(','))
                return 0;
        }
        if (!expect(']'))
            return 0;
        writeTag(kSEQUENCES_END);
        return fOut - outputBuffer;
    }

    size_t encodeSequenceJSON(uint8_t* outputBuffer, size_t maxLen)
    {
        char str[64];
        fOut = outputBuffer;
        fOutEnd = fOut + maxLen;
        if (!expect('{'))
            return 0;
        writeTag(kSEQUENCE_START);
        if (expectKey("seqID"))
        {
            expectString(str, sizeof(str));
            writeTag(kSEQID);
            outString(str);
            // fprintf(stderr, "Seq ID: \"%s\"\n", str);
        }
        if (!expect(','))
            return 0;
        if (expectKey("seqNam"))
        {
            expectString(str, sizeof(str));
            writeTag(kSEQNAM);
            outString(str);
            // fprintf(stderr, "Seq Name: \"%s\"\n", str);
        }
        if (!expect(','))
            return 0;
        if (expectKey("seqframes") && expect('['))
        {
            writeTag(kFRAMES_START);
            for (;;)
            {
                uint16_t frameDuration = 0;

                if (!expect('{'))
                    return 0;
                writeTag(kFRAME_START);
                if (expectKey("frbeg"))
                {
                    expectString(str, sizeof(str));
                    writeTag(kFRBEG);
                    outString(str);
                    //fprintf(stderr, "Serial begin: \"%s\"\n", str);
                }
                if (!expect(','))
                    return 0;
                if (expectKey("frdur"))
                {
                    frameDuration = uint16_t(expectPossibleStringInteger());
                    writeTag(kFRDUR);
                    write(frameDuration);
                }
                if (!expect(','))
                    return 0;
                if (expectKey("frend"))
                {
                    expectString(str, sizeof(str));
                    writeTag(kFREND);
                    outString(str);
                }
                if (!expect(','))
                    return 0;
                if (expectKey("frnam"))
                {
                    expectString(str, sizeof(str));
                    writeTag(kFRNAM);
                    outString(str);
                }
                char ch = read();
                if (ch == ',')
                {
                    ch = read();
                    if (ch == '"')
                    {
                        writeTag(kSERVOS_START);

                        // Parse Servos
                        pushBack(ch);
                        while (!hasError())
                        {
                            uint16_t servoIndex = uint16_t(expectStringInteger());
                            writeTag(kSRVNUM);
                            write(servoIndex);
                            //fprintf(stderr, "Servo #%u\n", servoNumber);
                            if (!expect(':'))
                                return 0;
                            if (!expect('{'))
                                return 0;
                            if (expectKey("srvdur"))
                            {
                                uint16_t servoDuration = uint16_t(expectPossibleStringInteger());
                                writeTag(kSRVDUR);
                                write(servoDuration);
                                //fprintf(stderr, "Servo Duration: %u\n", servoDuration);
                            }
                            if (!expect(','))
                                return 0;
                            if (expectKey("srveas"))
                            {
                                uint8_t servoEasing = uint8_t(expectPossibleStringInteger());
                                writeTag(kSRVEAS);
                                write(servoEasing);
                                //fprintf(stderr, "Servo Ease: %u\n", servoEasing);
                            }
                            if (!expect(','))
                                return 0;
                            if (expectKey("srvena"))
                            {
                                bool servoEnabled = expectBoolean();
                                writeTag(kSRVENA);
                                write(servoEnabled);
                                //fprintf(stderr, "Servo Enabled: %s\n", (servoEnabled ? "true" : "false"));
                            }
                            if (!expect(','))
                                return 0;
                            if (expectKey("srvpos"))
                            {
                                uint16_t servoPos = uint16_t(expectPossibleStringInteger());
                                writeTag(kSRVPOS);
                                write(servoPos);
                                //fprintf(stderr, "Servo Pos: %u\n", servoPos);
                            }
                            if (!expect('}'))
                                return 0;
                            ch = read();
                            if (ch == '}')
                                break;
                            if (ch != ',')
                                return 0;
                        }
                        writeTag(kSERVOS_END);
                    }
                }
                if (ch == '}')
                {
                    writeTag(kFRAME_END);
                    ch = read();
                    // end of frames
                    if (ch == ']')
                    {
                        writeTag(kFRAMES_END);
                        break;
                    }
                    // check if another frame
                    if (ch != ',')
                        return 0;
                    // reloop
                }
                else
                {
                    return 0;
                }
            }
            if (!expect('}'))
                return 0;
            writeTag(kSEQUENCE_END);
        }
        return fOut - outputBuffer;
    }

protected:
    const char* fText = nullptr;
    const char* fPtr = nullptr;
    const char* fEnd = nullptr;
    size_t fLength = 0;
    char fPushBack = 0;
    bool fError = false;
    uint8_t* fOut;
    uint8_t* fOutEnd;
};
