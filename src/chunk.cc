#include "chunk.h"

void Chunk::emit_byte(const u8 byte, const i32 line)
{
    if (lines_.len() == 0 || lines_[lines_.len() - 2] != line) {
        lines_.push(line);
        lines_.push(1);
    } else {
        lines_[lines_.len() - 1]++;
    }
    code_.push(byte);
}

i32 Chunk::add_constant(const Value val)
{
    constants_.push(val);
    return constants_.len() - 1;
}
