#pragma once

#include <etl/expected.h>

#include "middleware/core/message_allocator.h"
#include "middleware/core/middleware_message.h"
#include "middleware/core/skeleton_base.h"
#include "middleware/core/types.h"

namespace middleware::core
{

class EventSender
{
  public:
    EventSender(SkeletonBase& skeleton) : skeleton_(skeleton) {}
    ~EventSender() = default;
    EventSender(const EventSender& other) = delete;
    EventSender& operator=(const EventSender& other) = delete;
    EventSender(EventSender&& other) = delete;
    EventSender& operator=(EventSender&& other) = delete;

    template <typename T>
    [[nodiscard]] HRESULT send(const T& data, const MemberId memberId) const
    {
        HRESULT ret = HRESULT::NotRegistered;
        if (skeleton_.isInitialized())
        {
            auto msg = MiddlewareMessage::createEvent(
                skeleton_.getServiceId(), memberId, skeleton_.getInstanceId(), skeleton_.getSourceClusterId());
            ret = MessageAllocator::getInstance().allocate(
                data, msg, static_cast<uint8_t>(skeleton_.getClusterConnections().size()));
            if (ret == HRESULT::Ok)
            {
                ret = sendToClusters_(msg);
            }
        }

        return ret;
    }

    [[nodiscard]] HRESULT send(const MemberId memberId) const;

  private:
    [[nodiscard]] HRESULT sendToClusters_(MiddlewareMessage& msg) const;

    SkeletonBase& skeleton_;
};

}  // namespace middleware::core
