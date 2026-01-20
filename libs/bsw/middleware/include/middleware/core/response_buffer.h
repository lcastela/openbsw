#pragma once

#include <cstdint>

#include <etl/optional.h>
#include <etl/type_traits.h>
#include <etl/vector.h>

#include "middleware/core/message_allocator.h"
#include "middleware/core/middleware_message.h"
#include "middleware/core/response_buffer_base.h"
#include "middleware/core/types.h"

// suppress misra 5.2.9 EOF: Conversions from pointers ok.
namespace middleware::core
{

template <typename T, MemberId MemberIdValue>
struct ResponseTraits
{
    using ResponseType = T;
    static constexpr MemberId METHOD_MEMBER_ID = MemberIdValue;
};

template <typename Traits, uint16_t RESPONSE_LIMIT>
class ResponseBuffer : public ResponseBufferBase
{
  public:
    using Base = ResponseBufferBase;
    using ResponseTraits = Traits;
    using ResponseType = typename ResponseTraits::ResponseType;
    using ResponseContainer = etl::vector<Base::SkeletonResponseInfo, RESPONSE_LIMIT>;

    ResponseBuffer(const SkeletonBase& skeleton) : Base(skeleton, responses_) {}

    HRESULT respond(SkeletonResponseInfo& response, const ResponseType& result, const bool handleResponseFailure = true)
    {
        auto ret = HRESULT::ResponseBufferFutureNotFound;

        if (Base::isResponseIteratorValid(&response))
        {
            MiddlewareMessage msg = Base::generateResponseMessageHeader(response, Traits::METHOD_MEMBER_ID);
            ret = MessageAllocator::getInstance().allocate(result, msg);
            if (ret == HRESULT::Ok)
            {
                ret = Base::sendResponse(msg, response, handleResponseFailure);
            }
        }

        return ret;
    }

  private:
    ResponseContainer responses_;
};

}  // namespace middleware::core
