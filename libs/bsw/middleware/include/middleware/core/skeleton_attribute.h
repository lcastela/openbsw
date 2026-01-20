#pragma once

#include <etl/type_traits.h>

#include "middleware/core/event_sender.h"
#include "middleware/core/skeleton_base.h"
#include "middleware/core/types.h"

namespace middleware::core
{

template <typename T, MemberId MEMBER_ID, bool AllowsSubscriptions>
class SkeletonAttribute
{
  public:
    using AttributeType = T;

    SkeletonAttribute(SkeletonBase& skeleton) : eventSender_(skeleton) {}

    const AttributeType& get() const { return attributeValue_; }

    AttributeType& get() { return attributeValue_; }

    void set(const AttributeType& value)
    {
        // avoid excessive copying
        if (&value != &attributeValue_)
        {
            attributeValue_ = value;
        }
    }

    [[nodiscard]] etl::enable_if_t<AllowsSubscriptions, HRESULT> send(const AttributeType& attribute)
    {
        set(attribute);
        return eventSender_.send(attribute, MEMBER_ID);
    }

    [[nodiscard]] etl::enable_if_t<AllowsSubscriptions, HRESULT> send() const
    {
        return eventSender_.send(get(), MEMBER_ID);
    }

  private:
    EventSender eventSender_;
    AttributeType attributeValue_{};
};

}  // namespace middleware::core
