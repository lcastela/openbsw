#include "middleware/core/event_sender.h"

#include "middleware/core/icluster_connection.h"
#include "middleware/core/message_allocator.h"
#include "middleware/core/middleware_message.h"
#include "middleware/core/skeleton_base.h"
#include "middleware/core/types.h"

namespace middleware::core
{

[[nodiscard]] HRESULT EventSender::send(const MemberId memberId) const
{
    HRESULT ret = HRESULT::NotRegistered;
    if (skeleton_.isInitialized())
    {
        auto msg = MiddlewareMessage::createEvent(
            skeleton_.getServiceId(), memberId, skeleton_.getInstanceId(), skeleton_.getSourceClusterId());
        ret = sendToClusters_(msg);
    }

    return ret;
}

HRESULT EventSender::sendToClusters_(MiddlewareMessage& msg) const
{
    HRESULT ret = HRESULT::Ok;
    auto const& clusterConnections = skeleton_.getClusterConnections();
    // cross thread check
    skeleton_.checkCrossThreadError(skeleton_.getProcessId());
    for (IClusterConnection* const connection : clusterConnections)
    {
        msg.setTargetClusterId(connection->getTargetClusterId());
        if (connection->sendMessage(msg) != HRESULT::Ok)
        {
            MessageAllocator::deallocate(msg);
            ret = HRESULT::EventNotSendSuccessfully;
        }
    }

    return ret;
}

}  // namespace middleware::core
