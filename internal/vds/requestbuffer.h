#ifndef REQUESTBUFFER_H
#define REQUESTBUFFER_H

#include <memory>

#include "vds.h"

namespace vds {

class RequestBuffer {
    private:
        const int requestSizeInBytes;
        std::unique_ptr<char[]> pointer;
    public:
        RequestBuffer(const int requestSizeInBytes)
        : requestSizeInBytes(requestSizeInBytes),
            pointer(new char[requestSizeInBytes])
        {}

        char* getPointer() {return this->pointer.get();}

        response getAsResponse() {
            response responseData{
                this->pointer.release(),
                nullptr,
                static_cast<unsigned long>(this->requestSizeInBytes)
            };
            if (responseData.data == nullptr) {
                throw std::runtime_error("Request buffer is invalid.");
            }
            return responseData;
        }

        int getSizeInBytes() const {return this->requestSizeInBytes;}

        ~RequestBuffer() {}
};

} /* namespace vds */

#endif /* REQUESTBUFFER_H */
