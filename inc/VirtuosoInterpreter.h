#include "ReelTwo.h"
#include "core/SetupEvent.h"
#include "core/AnimatedEvent.h"
#include "core/CommandEvent.h"
#include "ServoSequencer.h"
#include "VirtuosoOpcode.h"

namespace Virtuoso {

class Virtuosopreter : public AnimatedEvent
{
public:
    enum
    {
        ERROR_SERIAL_SIGNAL = 1<<0,
        ERROR_SERIAL_OVERRUN = 1<<1,
        ERROR_SERIAL_BUFFER = 1<<2,
        ERROR_SERIAL_CRC = 1<<3,
        ERROR_SERIAL_PROTOCOL = 1<<4,
        ERROR_SERIAL_TIMEOUT = 1<<5,
        ERROR_SCRIPT_STACK_ERROR = 1<<6,
        ERROR_CALL_STACK_ERROR = 1<<7,
        ERROR_PROGRAM_COUNTER_ERROR = 1<<8,
        ERROR_INVALID = 1<<9
    };

    Virtuosopreter(
            ServoSequencer &sequencer) :
        fServoSequencer(&sequencer),
        fDispatch(&sequencer.dispatch())
    {
        fErrors |= ERROR_INVALID;
    }

    void setBlock(const esp_partition_t* block)
    {
        fBlock = block;
        reset();
    }

    /** \brief Reset the controller.
     *
     * Reset the controller.
     */
    void reset()
    {
        fPC = 0;
        fSP = 0;
        fCSP = 0;
        fErrors = 0;

        uint32_t magicSignature;
        size_t virtuosoLength;
        if (fBlock == nullptr ||
            esp_partition_read(fBlock, 0, &magicSignature, sizeof(magicSignature)) != ESP_OK ||
            magicSignature != VIRTUOSO_MAGIC_SIGNATURE ||
            esp_partition_read(fBlock, sizeof(magicSignature), &virtuosoLength, sizeof(virtuosoLength)) != ESP_OK)
        {
            fErrors |= ERROR_INVALID;
        printf("invalid1 magicSignature=0x%08X\n",magicSignature);
            return;
        }
        printf("magicSignature=0x%08X\n",magicSignature);
        printf("magicSignature=0x%08X\n",magicSignature);
        fSubOffset = sizeof(magicSignature) + sizeof(virtuosoLength);
        if (esp_partition_read(fBlock, fSubOffset, &fSubCount, sizeof(fSubCount)) != ESP_OK ||
            fSubCount > 127)
        {
            fErrors |= ERROR_INVALID;
        printf("invalid2\n");
            return;
        }
        fSubOffset += sizeof(fSubCount);
        fByteCodeOffset = fSubOffset + 127 * sizeof(uint16_t);
        fByteCodeSize = virtuosoLength - fByteCodeOffset;

        fFlags = kRunning;
        printf("running\n");
    }

    /** \brief Stops the script.
     *
     * Stops the script, if it is currently running.
     */
    void stopScript()
    {
        fFlags = kEnded;
    }

    /** \brief Starts loaded script at specified \a subroutineNumber location.
     *
     * @param subroutineNumber A subroutine number defined in script's compiled
     * code.
     */
    void restartScript(unsigned subroutineNumber = 0)
    {
        reset();
        fPC = getSubroutine(subroutineNumber);
        fFlags = kRunning;
    }

    /** \brief Starts loaded script at specified \a subroutineNumber location
     *  after loading \a parameter on to the stack.
     *
     * @param subroutineNumber A subroutine number defined in script's compiled
     * code.
     *
     * @param parameter A number from 0 to 16383.
     *
     * Similar to the \p restartScript function, except it loads the parameter
     * on to the stack before starting the script at the specified subroutine
     * number location.
     */
    void restartScriptWithParameter(unsigned subroutineNumber, unsigned parameter)
    {
        reset();
        push(parameter);
        fPC = getSubroutine(subroutineNumber);
        fFlags = kRunning;
    }

    /**
      * Runs through a single step of any active animation script.
      */
    virtual void animate() override;

    /** \brief Sends the servos and outputs to home position.
     *
     * If the "On startup or error" setting for a servo or output channel is set
     * to "Ignore", the position will be unchanged.
     */
    void goHome()
    {
        // Not supported
    }

    /** \brief Sets the \a target of the servo on \a channelNumber.
     *
     * @param channelNumber A servo number from 0 to 127.
     *
     * @param target A number from 0 to 16383.
     *
     * If the channel is configured as a servo, then the target represents the
     * pulse width to transmit in units of quarter-microseconds. A \a target
     * value of 0 tells the Virtuoso to stop sending pulses to the servo.
     */
    void setTarget(unsigned channel, unsigned pulseQuarterMS)
    {
        printf("SERVO{%u}: %u => %u.%u\n", channel, pulseQuarterMS, pulseQuarterMS >> 2, (pulseQuarterMS&3)*25);
        if (fDispatch != nullptr)
            fDispatch->moveToPulse(channel, pulseQuarterMS >> 2);
    }

    /** \brief Sets the \a speed limit of \a channelNumber.
     *
     * @param channelNumber A servo number from 0 to 127.
     *
     * @param speed A number from 0 to 16383.
     *
     * Limits the speed a servo channel’s output value changes.
     */
    void setSpeed(unsigned channel, unsigned speed)
    {
        printf("SPEED{%u}: %u\n", channel, speed);
    }

     /** \brief Sets the \a acceleration limit of \a channelNumber.
     *
     * @param channelNumber A servo number from 0 to 127.
     *
     * @param acceleration A number from 0 to 16383.
     *
     * Limits the acceleration a servo channel’s output value changes.
     */
    void setAccelleration(unsigned channel, unsigned accelleration)
    {
        printf("ACCELERATION{%u}: %u\n", channel, accelleration);
    }

    void delayMS(unsigned ms)
    {
        fFlags = kWaiting;
        fWaitUntil = millis() + ms;
        printf("delayUntil %u\n", ms);
    }

    /** \brief Gets the position of \a channelNumber.
     *
     * @param channelNumber A servo number from 0 to 127.
     *
     * @return two-byte position value
     *
     * If channel is configured as a servo, then the position value represents
     * the current pulse width transmitted on the channel in units of
     * quarter-microseconds.
     */
    uint16_t getPosition(unsigned channel)
    {
        printf("GET POSITION: %u\n", channel);
        return 499;
    }

    /** \brief Gets the moving state for all configured servo channels.
     *
     * @return 1 if at least one servo limited by speed or acceleration is still
     * moving, 0 if not.
     *
     * Determines if the servo outputs have reached their targets or are still
     * changing and will return 1 as as long as there is at least one servo that
     * is limited by a speed or acceleration setting.
     */
    bool getMovingState(unsigned channel)
    {
        printf("GET MOVING STATE: %u\n", channel);
        return false;
    }

    /** \brief Gets if the script is running or stopped.
     *
     * @return 1 if script is stopped, 0 if running.
     */
    bool getScriptStatus()
    {
        return (fFlags != kRunning);
    }

    /** \brief Gets the error register.
     *
     * @return Two-byte error code.
     */
    uint16_t getErrors()
    {
        return fErrors;
    }

    void ledState(bool on)
    {
        printf("LED: %s\n", (on ? "ON" : "OFF"));
    }

    void serialSendByte(uint8_t byte)
    {
        printf("SEND BYTE: 0x%02X\n", byte);
    }

private:
    enum
    {
        kWaiting = 0,
        kRunning = 1,
        kEnded = 0xFF
    };

    inline uint8_t getByteCode(size_t index)
    {
        uint8_t b = 0;
        if (index >= fByteCodeSize ||
            esp_partition_read(fBlock, fByteCodeOffset + index, &b, sizeof(b)) != ESP_OK)
        {
            fErrors |= ERROR_PROGRAM_COUNTER_ERROR;
        }
        return b;
    }

    inline uint16_t getSubroutine(size_t index)
    {
        uint16_t sub = 0;
        printf("getSubroutine(%d) fSubCount=%d fSubOffset=%d\n", index, fSubCount, fSubOffset);
        if (index >= fSubCount ||
            esp_partition_read(fBlock, fSubOffset + index*sizeof(uint16_t), &sub, sizeof(sub)) != ESP_OK)
        {
            printf("Error\n");
            fErrors |= ERROR_PROGRAM_COUNTER_ERROR;
        }
        printf("sub=%04X\n", sub);
        return sub;
    }

    void push(int16_t val)
    {
        if (fSP >= fStack.size())
        {
            fErrors |= ERROR_SCRIPT_STACK_ERROR;
            return;
        }
        fStack[fSP++] = val;
    }

    int16_t pop()
    {
        if (fSP == 0)
        {
            fErrors |= ERROR_SCRIPT_STACK_ERROR;
            return 0;
        }
        return fStack[--fSP];
    }

    void pushCallstack(uint16_t val)
    {
        if (fCSP >= fCallStack.size())
        {
            fErrors |= ERROR_CALL_STACK_ERROR;
            return;
        }
        fCallStack[fCSP++] = val;
    }

    uint16_t popCallstack()
    {
        if (fCSP == 0)
        {
            fErrors |= ERROR_CALL_STACK_ERROR;
            return 0;
        }
        return fCallStack[--fCSP];
    }

    unsigned fPC = 0;
    unsigned fSP = 0;
    unsigned fCSP = 0;
    uint8_t fFlags = kEnded;
    const esp_partition_t* fBlock = nullptr;
    ServoSequencer* fServoSequencer = nullptr;
    ServoDispatch* fDispatch = nullptr;
    uint32_t fSubOffset = 0;
    uint16_t fSubCount = 0;
    uint32_t fByteCodeOffset = 0;
    uint16_t fByteCodeSize = 0;
    std::array<int16_t, 126> fStack;
    std::array<uint16_t, 126> fCallStack;
    uint32_t fWaitUntil = 0;
    uint8_t fErrors = 0;
};

void Virtuosopreter::animate()
{
    while (!fErrors)
    {
        if (fFlags == kWaiting)
        {
            if (millis() < fWaitUntil)
                return;
            printf("Wait done\n");
            fFlags = kRunning;
        }
        else if (fFlags == kEnded)
        {
            return;
        }
        Opcode opcode = (Opcode)getByteCode(fPC++);
        printf("%04X:%d\n", fPC-1, int(opcode));
        switch (opcode)
        {
            case Opcode::LITERAL:
            {
                push((getByteCode(fPC+1) << 8) | getByteCode(fPC));
                fPC += 2;
                break;
            }
            case Opcode::LITERAL8:
            {
                push(getByteCode(fPC++));
                break;
            }
            case Opcode::LITERAL_N:
            {
                auto count = getByteCode(fPC++) / 2;
                while (count > 0)
                {
                    push((getByteCode(fPC+1) << 8) | getByteCode(fPC));
                    count--;
                    fPC += 2;
                }
                break;
            }
            case Opcode::LITERAL8_N:
            {
                auto count = getByteCode(fPC++);
                while (count > 0)
                {
                    push(getByteCode(fPC++));
                    count--;
                }
                break;
            }
            case Opcode::RETURN:
            {
                fPC = popCallstack();
                break;
            }
            case Opcode::DELAY:
            {
                uint16_t timeMS = pop();
                delayMS(timeMS);
                break;
            }
            case Opcode::SERVO:
            {
                uint16_t servoNum = pop();
                uint16_t pulseValQuarterMS = pop();
                if (pulseValQuarterMS != 0)
                {
                    setTarget(servoNum, pulseValQuarterMS);
                }
                break;
            }
            case Opcode::QUIT:
            {
                fFlags = kEnded;
                goto done;
            }
            case Opcode::DROP:
            {
                pop();
                break;
            }
            case Opcode::DEPTH:
            {
                push(fSP);
                break;
            }
            case Opcode::DUP:
            {
                auto val = pop();
                push(val);
                push(val);
                break;
            }
            case Opcode::SWAP:
            {
                auto rhs = pop();
                auto lhs = pop();
                push(rhs);
                push(lhs);
                break;
            }

            // Unary
            case Opcode::BITWISE_NOT:
            {
                auto val = pop();
                push(~val);
                break;
            }
            case Opcode::LOGICAL_NOT:
            {
                auto val = pop();
                push(!val);
                break;
            }
            case Opcode::NEGATE:
            {
                auto val = pop();
                push(-val);
                break;
            }
            case Opcode::POSITIVE:
            {
                auto val = pop();
                push(val > 0);
                break;
            }
            case Opcode::NEGATIVE:
            {
                auto val = pop();
                push(val < 0);
                break;
            }
            case Opcode::NONZERO:
            {
                auto val = pop();
                push(val != 0);
                break;
            }

            // Binary
            case Opcode::BITWISE_AND:
            {
                auto rhs = pop();
                auto lhs = pop();
                push(lhs & rhs);
                break;
            }
            case Opcode::BITWISE_OR:
            {
                auto rhs = pop();
                auto lhs = pop();
                push(lhs | rhs);
                break;
            }
            case Opcode::BITWISE_XOR:
            {
                auto rhs = pop();
                auto lhs = pop();
                push(lhs ^ rhs);
                break;
            }
            case Opcode::SHIFT_RIGHT:
            {
                auto rhs = pop();
                auto lhs = pop();
                push(lhs << rhs);
                break;
            }
            case Opcode::SHIFT_LEFT:
            {
                auto rhs = pop();
                auto lhs = pop();
                push(lhs >> rhs);
                break;
            }
            case Opcode::LOGICAL_AND:
            {
                auto rhs = pop();
                auto lhs = pop();
                push(lhs && rhs);
                break;
            }
            case Opcode::LOGICAL_OR:
            {
                auto rhs = pop();
                auto lhs = pop();
                push(lhs || rhs);
                break;
            }
            case Opcode::PLUS:
            {
                auto rhs = pop();
                auto lhs = pop();
                push(lhs + rhs);
                break;
            }
            case Opcode::MINUS:
            {
                auto rhs = pop();
                auto lhs = pop();
                push(lhs - rhs);
                break;
            }
            case Opcode::TIMES:
            {
                auto rhs = pop();
                auto lhs = pop();
                push(lhs * rhs);
                break;
            }
            case Opcode::DIVIDE:
            {
                auto rhs = pop();
                auto lhs = pop();
                push(lhs / rhs);
                break;
            }
            case Opcode::MOD:
            {
                auto rhs = pop();
                auto lhs = pop();
                push(lhs % rhs);
                break;
            }
            case Opcode::EQUALS:
            {
                auto rhs = pop();
                auto lhs = pop();
                push(lhs == rhs);
                break;
            }
            case Opcode::NOT_EQUALS:
            {
                auto rhs = pop();
                auto lhs = pop();
                push(lhs != rhs);
                break;
            }
            case Opcode::LESS_THAN:
            {
                auto rhs = pop();
                auto lhs = pop();
                push(lhs < rhs);
                break;
            }
            case Opcode::GREATER_THAN:
            {
                auto rhs = pop();
                auto lhs = pop();
                push(lhs > rhs);
                break;
            }
            case Opcode::MIN:
            {
                auto rhs = pop();
                auto lhs = pop();
                push(lhs < rhs ? lhs : rhs);
                break;
            }
            case Opcode::MAX:
            {
                auto rhs = pop();
                auto lhs = pop();
                push(lhs < rhs ? rhs : lhs);
                break;
            }
            case Opcode::LED_ON:
            {
                ledState(true);
                break;
            }
            case Opcode::LED_OFF:
            {
                ledState(false);
                break;
            }
            case Opcode::SERIAL_SEND_BYTE:
            {
                uint8_t byte = pop() & 0xFF;
                serialSendByte(byte);
                break;
            }
            case Opcode::GET_POSITION:
            {
                auto channel = pop();
                push(getPosition(channel));
                break;
            }
            case Opcode::GET_MOVING_STATE:
            {
                auto channel = pop();
                push(getMovingState(channel));
                break;
            }
            case Opcode::ACCELERATION:
            {
                auto accelleration = pop();
                auto channel = pop();
                setAccelleration(channel, accelleration);
                break;
            }
            case Opcode::SPEED:
            {
                auto speed = pop();
                auto channel = pop();
                setSpeed(channel, speed);
                break;
            }
            case Opcode::JUMP:
            {
                fPC = (getByteCode(fPC+1) << 8) | getByteCode(fPC);
                break;
            }
            case Opcode::JUMP_Z:
            {
                fPC = !pop() ? (getByteCode(fPC+1) << 8) | getByteCode(fPC) : fPC + 2;
                break;
            }
            case Opcode::CALL:
            {
                uint16_t address = (getByteCode(fPC+1) << 8) | getByteCode(fPC);
                pushCallstack(fPC + 2);
                fPC = address;
                break;
            }
            case Opcode::GET_MS:
            case Opcode::OVER:
            case Opcode::PICK:
            case Opcode::ROT:
            case Opcode::ROLL:
            case Opcode::SERVO_8BIT:
            case Opcode::PWM:
            case Opcode::PEEK:
            case Opcode::POKE:
                printf("UNSUPPORTED OPCODE : %d\n", int(opcode));
                fErrors |= ERROR_PROGRAM_COUNTER_ERROR;
                break;
            default:
                if ((uint8_t(opcode) & 0x80) != 0)
                {
                    // Call subroutine
                    pushCallstack(fPC);
                    fPC = getSubroutine(uint8_t(opcode) & 0x7F);
                }
                else
                {
                    printf("ERROR\n");
                    fErrors |= ERROR_PROGRAM_COUNTER_ERROR;
                }
                break;
        }
    }
done:
    return;
}

}
