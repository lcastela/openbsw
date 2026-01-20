#include "middleware/core/response_buffer_base.h"

#include <etl/algorithm.h>
#include <etl/vector.h>

#include "middleware/core/message_allocator.h"
#include "middleware/core/middleware_message.h"
#include "middleware/core/skeleton_base.h"
#include "middleware/core/types.h"

namespace middleware::core
{

ResponseBufferBase::ResponseBufferBase(const SkeletonBase& skeleton, etl::ivector<SkeletonResponseInfo>& responses)
    : skeleton_(skeleton), responses_(responses)
{
}

ResponseBufferBase::SkeletonResponseInfo* ResponseBufferBase::getAvailableResponse(const AddressId addressId,
                                                                                   const ClusterId targetClusterId,
                                                                                   const RequestId requestId)
{
    SkeletonResponseInfo* availableResponse = nullptr;
    if (!responses_.full())
    {
        SkeletonResponseInfo newResponse{addressId, targetClusterId, requestId};
        availableResponse = &responses_.emplace_back(newResponse);
    }

    return availableResponse;
}

void ResponseBufferBase::makeResponseAvailable(SkeletonResponseInfo& response)
{
    responses_.erase(&response);
}

HRESULT ResponseBufferBase::cancelResponse(SkeletonResponseInfo& response)
{
    HRESULT ret = HRESULT::ResponseBufferFutureNotFound;
    if (isResponseIteratorValid(&response))
    {
        makeResponseAvailable(response);
        ret = HRESULT::Ok;
    }

    return ret;
}

bool ResponseBufferBase::isResponseIteratorValid(SkeletonResponseInfo* const iterator) const
{
    SkeletonResponseInfo* ret =
        etl::find_if(responses_.begin(), responses_.end(), [&iterator](const SkeletonResponseInfo& response) {
            return &response == iterator;
        });

    return ret != responses_.end();
}

HRESULT ResponseBufferBase::sendResponse(MiddlewareMessage& msg,
                                         SkeletonResponseInfo& response,
                                         const bool handleResponseFailure)
{
    const HRESULT res = skeleton_.sendMessage(msg);
    if (res == HRESULT::Ok)
    {
        makeResponseAvailable(response);
    }
    else
    {
        MessageAllocator::deallocate(msg);
        if (handleResponseFailure)
        {
            static_cast<void>(cancelResponse(response));
        }
    }

    return res;
}

}  // namespace middleware::core
