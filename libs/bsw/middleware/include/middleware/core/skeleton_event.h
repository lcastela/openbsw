#pragma once

#include <etl/span.h>
#include <etl/type_traits.h>
#include <etl/utility.h>

#include "middleware/core/event_sender.h"
#include "middleware/core/skeleton_base.h"
#include "middleware/core/types.h"

namespace middleware::core
{

template <typename Type, MemberId MEMBER_ID>
class SkeletonEvent
{
  public:
    using EventType = Type;

    SkeletonEvent(SkeletonBase& skeleton) : eventSender_(skeleton) {}

    [[nodiscard]] HRESULT send(const Type& data) const { return eventSender_.send<EventType>(data, MEMBER_ID); }

  private:
    EventSender eventSender_;
};

template <MemberId MEMBER_ID>
class SkeletonEvent<void, MEMBER_ID>
{
  public:
    using EventType = void;

    SkeletonEvent(SkeletonBase& skeleton) : eventSender_(skeleton) {}

    [[nodiscard]] HRESULT send() const { return eventSender_.send(MEMBER_ID); }

  private:
    EventSender eventSender_;
};

}  // namespace middleware::core
