#include <functional>

class lzss_decoder {
public:
    /**
     * Constructor: Initializes the decoder with a callback function for output.
     * @param putc_cbk A callback function to handle the output characters.
     */
    lzss_decoder(std::function<void(uint8_t)> putc_cbk)
            : decode_callback_(std::move(putc_cbk)) {

        // Initialize the buffer with spaces
        memset(buffer, ' ', N - F);

        r_ = N - F;
    }

    /**
     * Enum representing the status of the decoding process.
     */
    enum status : uint8_t {
        DONE,
        IN_PROGRESS,
        NOT_COMPLETED
    };

    /**
     * Decodes the provided buffer until it ends, then pauses the process.
     * @param buffer The input buffer containing compressed data.
     * @param size The size of the input buffer.
     * @return DONE if decompression is complete, NOT_COMPLETED if more data is needed.
     */
    status decode(uint8_t* buffer, uint32_t size) {
        buffer_ = buffer;
        available_ += size;

        status res;

        while ((res = handle_state()) == IN_PROGRESS);

        buffer_ = nullptr;

        return res;
    }

    static const int LZSS_EOF = -1;
    static const int LZSS_BUFFER_EMPTY = -2;

private:
    /**
     * Handles the current state_ of the FSM and processes the input data.
     * @return The current status of the decoding process.
     */
    status handle_state() {
        status res = IN_PROGRESS;
        int c = getbit(bits_required());

        if (c == LZSS_BUFFER_EMPTY) {
            res = NOT_COMPLETED;
        } else if (c == LZSS_EOF) {
            res = DONE;
            state_ = FSM_STATES::FSM_EOF;
        } else {
            switch (state_) {
                case FSM_STATES::FSM_0:
                    state_ = c ? FSM_STATES::FSM_1 : FSM_STATES::FSM_2;
                    break;
                case FSM_STATES::FSM_1:
                    putc(c);
                    buffer[r_++] = c;
                    r_ &= (N - 1);
                    state_ = FSM_STATES::FSM_0;
                    break;
                case FSM_STATES::FSM_2:
                    i_ = c;
                    state_ = FSM_STATES::FSM_3;
                    break;
                case FSM_STATES::FSM_3: {
                    int j = c;
                    for (int k = 0; k <= j + 1; k++) {
                        c = buffer[(i_ + k) & (N - 1)];
                        putc(c);
                        buffer[r_++] = c;
                        r_ &= (N - 1);
                    }
                    state_ = FSM_STATES::FSM_0;
                    break;
                }
                case FSM_STATES::FSM_EOF:
                    break;
            }
        }
        return res;
    }

    /**
     * Gets a specified number of bits from the input buffer.
     * @param n The number of bits to retrieve.
     * @return The retrieved bits, or an error code if insufficient data.
     */
    int getbit(uint8_t n) {
        int x, c;
        while (buf_size_ < n) {
            switch (c = getc()) {
                case LZSS_EOF:
                case LZSS_BUFFER_EMPTY:
                    return c;
            }
            buf_ <<= 8;
            buf_ |= (uint8_t)c;
            buf_size_ += sizeof(uint8_t) * 8;
        }
        x = buf_ >> (buf_size_ - n);
        buf_ &= (1 << (buf_size_ - n)) - 1;
        buf_size_ -= n;
        return x;
    }

    /**
     * Gets a single character from the input buffer.
     * @return The retrieved character, or an error code if the buffer is empty.
     */
    int getc() {
        int c;
        if (buffer_ == nullptr || available_ == 0) {
            c = LZSS_BUFFER_EMPTY;
        } else {
            c = *buffer_;
            buffer_++;
            available_--;
        }
        return c;
    }

    /**
     * Outputs a character using the callback function.
     * @param c The character to output.
     */
    inline void putc(const uint8_t c) {
        if (decode_callback_) {
            decode_callback_(c);
        }
    }

    /**
     * Gets the number of bits required by the FSM in the current state_.
     * @param s The current state_ of the FSM.
     * @return The number of bits required.
     */
    uint8_t bits_required() {
        switch (state_) {
            case FSM_STATES::FSM_0:
                return 1;
            case FSM_STATES::FSM_1:
                return 8;
            case FSM_STATES::FSM_2:
                return EI;
            case FSM_STATES::FSM_3:
                return EJ;
            default:
                return 0;
        }
    }

private:
    // Constants for buffer sizes
    static constexpr int EI = 11;              // Typically 10..13
    static constexpr int EJ = 4;               // Typically 4..5
    static constexpr int N  = (1 << EI);       // Buffer size
    static constexpr int F  = ((1 << EJ) + 1); // Lookahead buffer size

    // FSM state_ variables
    enum class FSM_STATES {
        FSM_0 = 0,
        FSM_1 = 1,
        FSM_2 = 2,
        FSM_3 = 3,
        FSM_EOF
    };

    // Algorithm-specific buffer used to store text for reference and copying
    uint8_t buffer[N * 2] = {};

    // Input buffer and available_ bytes
    uint8_t* buffer_ = nullptr;
    uint32_t available_ = 0;

    // Variables for getbit function
    uint32_t buf_ = 0, buf_size_ = 0;

    FSM_STATES state_ = FSM_STATES::FSM_0;

    // Variables used in decoding sessions
    int i_ = 0, r_ = 0;

    // Callback function for output characters
    std::function<void(uint8_t)> decode_callback_;
};