#include "gtest/gtest.h"
#include "middleware/core/skeleton_base.h"
#include "middleware/core/types.h"

class SkeletonMock : public ::middleware::core::SkeletonBase
{

  public:
    SkeletonMock(::middleware::core::ServiceId serviceId, ::middleware::core::InstanceId instanceId)
        : serviceId_(serviceId), middleware::core::SkeletonBase()
    {
        this->setInstanceId(instanceId);
    }
    ::middleware::core::ServiceId getServiceId() const final { return serviceId_; }
    virtual ::middleware::core::HRESULT onNewMessageReceived(const ::middleware::core::MiddlewareMessage& msg)
    {
        return ::middleware::core::HRESULT::NotImplemented;
    }

  private:
    ::middleware::core::ServiceId serviceId_;
};
