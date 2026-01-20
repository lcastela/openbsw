#pragma once

#include <etl/vector.h>

#include "middleware/core/middleware_message.h"
#include "middleware/core/skeleton_base.h"
#include "middleware/core/types.h"

namespace middleware::core
{

class ResponseBufferBase
{
  public:
    struct SkeletonResponseInfo
    {
        AddressId addressId{INVALID_ADDRESS_ID};
        ClusterId targetClusterId{static_cast<ClusterId>(INVALID_CLUSTER_ID)};
        RequestId requestId{INVALID_REQUEST_ID};
    };

    ResponseBufferBase(const SkeletonBase& skeleton, etl::ivector<SkeletonResponseInfo>& responses);

    SkeletonResponseInfo* getAvailableResponse(const AddressId addressId,
                                               const ClusterId targetClusterId,
                                               const RequestId requestId);

    HRESULT cancelResponse(SkeletonResponseInfo& response);

    bool isResponseIteratorValid(SkeletonResponseInfo* const iterator) const;

  protected:
    HRESULT sendResponse(MiddlewareMessage& msg, SkeletonResponseInfo& response, bool handleResponseFailure);

    constexpr MiddlewareMessage generateResponseMessageHeader(const SkeletonResponseInfo& responseInfo,
                                                              const MemberId memberId) const
    {
        return MiddlewareMessage::createResponse(
            {skeleton_.getServiceId(), memberId, responseInfo.requestId, skeleton_.getInstanceId()},
            skeleton_.getSourceClusterId(),
            responseInfo.targetClusterId,
            responseInfo.addressId);
    }

    void makeResponseAvailable(SkeletonResponseInfo& response);

  private:
    const SkeletonBase& skeleton_;
    etl::ivector<SkeletonResponseInfo>& responses_;
};

}  // namespace middleware::core
