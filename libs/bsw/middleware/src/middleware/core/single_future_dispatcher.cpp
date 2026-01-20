#include "middleware/core/single_future_dispatcher.h"

#include <cstdint>

#include "middleware/core/abstract_future_timeout.h"
#include "middleware/core/logger_api.h"
#include "middleware/core/middleware_message.h"
#include "middleware/core/types.h"
#include "middleware/logger/logger.h"

namespace middleware::core
{

void SingleFutureDispatcher::freeAll()
{
    if (nullptr != future_)
    {
        future_->setState(IFuture::State::Invalid);
        future_->setRequestId(INVALID_REQUEST_ID);
        future_->setFutureDispatcherSlot(INVALID_REQUEST_ID);
        future_ = nullptr;
    }
    actualRequestId_ = 0U;
}
IFuture* SingleFutureDispatcher::releaseRequestId(const MiddlewareMessage& msg)
{
    IFuture* ret = nullptr;

    if ((nullptr != future_) && (msg.getHeader().requestId == future_->getRequestId()))
    {
        ret = future_;
        future_ = nullptr;
        ret->setRequestId(INVALID_REQUEST_ID);
        ret->setFutureDispatcherSlot(INVALID_REQUEST_ID);
        const IFuture::State newSate =
            msg.hasError() ? static_cast<IFuture::State>(msg.getErrorState()) : IFuture::State::Ready;
        ret->setState(newSate);
    }
    else
    {
        logger::logMessageSendingFailure(
            logger::LogLevel::Error, logger::Error::SendMessage, HRESULT::FutureNotFound, msg);
    }
    return ret;
}

HRESULT SingleFutureDispatcher::obtainRequestId(uint16_t& requestId, IFuture& future)
{
    bool isFutureFree = false;
    HRESULT ret = HRESULT::RequestPoolDepleted;
    requestId = INVALID_REQUEST_ID;

    if (future_ == &future)
    {
        // future already in use - reuse it
        isFutureFree = true;
        static_cast<void>(invalidateFuture(future));
    }
    else if (future.getState() == IFuture::State::Pending)
    {
        // future is in use by some other proxy-> not allowed
        ret = HRESULT::FutureAlreadyInUse;  // suppress misra 0.1.6: ret is always used at the end of the
                                            // function
    }
    else
    {
        isFutureFree = true;
    }
    if (isFutureFree && (future_ == nullptr))
    {
        future_ = &future;
        // the id is also a position of the future array, so simply store the future there O(1)
        future.setRequestId(actualRequestId_);
        future.setFutureDispatcherSlot(0U);
        future.setState(IFuture::State::Pending);
        requestId = actualRequestId_;
        ++actualRequestId_;
        if (actualRequestId_ == INVALID_REQUEST_ID)
        {
            actualRequestId_ = 0U;
        }
        ret = HRESULT::Ok;
    }
    return ret;
}

HRESULT SingleFutureDispatcher::invalidateFuture(IFuture& future)
{
    HRESULT ret = HRESULT::InstanceNotFound;

    if (future_ == &future)
    {
        future.setState(IFuture::State::Invalid);
        future.setRequestId(INVALID_REQUEST_ID);
        future.setFutureDispatcherSlot(INVALID_REQUEST_ID);
        future_ = nullptr;
        ret = HRESULT::Ok;
    }

    return ret;
}

void SingleFutureDispatcher::updateTimeouts(const uint32_t now, const uint32_t timeoutDuration)
{
    if (nullptr != future_)
    {
        // NOLINTBEGIN(cppcoreguidelines-pro-type-static-cast-downcast): IPBD-49847 (Deviation Approved)
        if ((static_cast<const AbstractFutureTimeout*>(future_)->getCallerTimestamp() - now) > timeoutDuration)
        {
            IFuture* future = future_;
            future_ = nullptr;
            future->setState(IFuture::State::Timeout);
            future->setRequestId(INVALID_REQUEST_ID);
            future->setFutureDispatcherSlot(INVALID_REQUEST_ID);
            static_cast<AbstractFutureTimeout*>(future)->timeoutExpired();
        }
        // NOLINTEND(cppcoreguidelines-pro-type-static-cast-downcast): IPBD-49847 (Deviation Approved)
    }
}

}  // namespace middleware::core
