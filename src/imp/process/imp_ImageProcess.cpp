#include "imp_ImageProcess.hpp"

#include <bc7enc.h>

namespace imp
{
    void Process(std::span<ImageProcessInput> inputs, ImageProcessFlags flags, ImageView& output)
    {
        for (uint32_t i = 1; i < inputs.size(); ++i) {
            if (inputs.begin()[i - 1].buffer.size != inputs.begin()[i].buffer.size) {
                throw std::runtime_error("Mismatched input sizes, not supported currently");
            }
        }

        if (output.size != inputs.begin()->buffer.size) {
            throw std::runtime_error("Mismatched input/output size");
        }


    }
}