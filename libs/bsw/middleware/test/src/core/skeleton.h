#include "gtest/gtest.h"
#include "middleware/core/SkeletonBase.h"
#include "middleware/core/types.h"

class SkeletonMock : public ::middleware::core::SkeletonBase
{
public:
    SkeletonMock(uint16_t serviceId, uint16_t instanceId)
    : serviceId_(serviceId), middleware::core::SkeletonBase()
    {
        this->setInstanceId(instanceId);
    }

    uint16_t getServiceId() const final { return serviceId_; }

    virtual ::middleware::core::HRESULT onNewMessageReceived(::middleware::core::Message const& msg)
    {
        return ::middleware::core::HRESULT::NotImplemented;
    }

private:
    uint16_t serviceId_;
};
