#include "middleware/core/future_dispatcher_base.h"

#include <cstdint>

#include <etl/algorithm.h>
#include <etl/array.h>
#include <etl/iterator.h>
#include <etl/limits.h>
#include <etl/vector.h>

#include "middleware/core/abstract_future_timeout.h"
#include "middleware/core/logger_api.h"
#include "middleware/core/middleware_message.h"
#include "middleware/core/types.h"
#include "middleware/logger/logger.h"

namespace middleware::core
{

FutureDispatcherBase::~FutureDispatcherBase()
{
    freeAll();
}

void FutureDispatcherBase::freeAll()
{
    for (auto* const future : futures_)
    {
        future->setState(IFuture::State::Invalid);
        future->setRequestId(INVALID_REQUEST_ID);
        future->setFutureDispatcherSlot(INVALID_REQUEST_ID);
    }
    futures_.clear();
    etl::fill(kRequestdIds.begin(), kRequestdIds.end(), 0U);
    actualRequestId_ = 0U;
}

HRESULT FutureDispatcherBase::invalidateFuture(IFuture& future)
{
    HRESULT ret = HRESULT::InstanceNotFound;

    auto* const activeFuture = etl::find(futures_.begin(), futures_.end(), &future);
    if (activeFuture != futures_.end())
    {
        future.setState(IFuture::State::Invalid);
        future.setRequestId(INVALID_REQUEST_ID);
        static_cast<void>(futures_.erase(activeFuture));
        freeRequestId_(future.getFutureDispatcherSlot());

        future.setFutureDispatcherSlot(INVALID_REQUEST_ID);
        ret = HRESULT::Ok;
    }

    return ret;
}

HRESULT FutureDispatcherBase::obtainRequestId(uint16_t& requestId, IFuture& future)
{
    bool isFutureFree = false;
    HRESULT ret = HRESULT::RequestPoolDepleted;

    auto* const futureIterator = etl::find(futures_.begin(), futures_.end(), &future);
    // future already in use - reuse it
    if (futureIterator != futures_.end())
    {
        isFutureFree = true;
        static_cast<void>(invalidateFuture(future));
    }
    else if (future.getState() == IFuture::State::Pending)
    {
        // future is in use by some other proxy-> not allowed
        requestId = INVALID_REQUEST_ID;
        ret = HRESULT::FutureAlreadyInUse;
    }
    else
    {
        isFutureFree = true;
    }

    if (isFutureFree)
    {
        auto const newSlot = getRequestId_();
        if (newSlot == INVALID_REQUEST_ID)
        {
            requestId = INVALID_REQUEST_ID;
            ret = HRESULT::RequestPoolDepleted;
        }
        else
        {
            futures_.push_back(&future);
            // the id is also a position of the future array, so simply store the future there O(1)
            future.setRequestId(actualRequestId_);
            future.setFutureDispatcherSlot(newSlot);
            future.setState(IFuture::State::Pending);
            requestId = actualRequestId_;
            ++actualRequestId_;
            if (actualRequestId_ == INVALID_REQUEST_ID)
            {
                actualRequestId_ = 0U;
            }
            ret = HRESULT::Ok;
        }
    }
    return ret;
}

uint16_t FutureDispatcherBase::getPositionOfRightMostUnsetBit_(uint32_t freeBlockValue)
{
    const uint32_t negativeBlock = (~(freeBlockValue));
    const uint32_t blockPlusOne = (freeBlockValue + 1U);
    // right most unset bit ~N & (N+1) -> always power of 2
    const uint32_t rightMostUnsetBit = (negativeBlock & blockPlusOne);

    // http://graphics.stanford.edu/~seander/bithacks.html
    static const etl::array<uint32_t, 32U> kmultiplyDeBruijnBitPosition2 = {
        0U,  1U,  28U, 2U,  29U, 14U, 24U, 3U, 30U, 22U, 20U, 15U, 25U, 17U, 4U,  8U,
        31U, 27U, 13U, 23U, 21U, 19U, 16U, 7U, 26U, 12U, 18U, 6U,  11U, 5U,  10U, 9U};

    // http://graphics.stanford.edu/~seander/bithacks.html
    const uint32_t deBruijnSequence = 0x077CB531U;
    const uint32_t position =
        kmultiplyDeBruijnBitPosition2[static_cast<uint32_t>(rightMostUnsetBit * deBruijnSequence) >> 27U];

    return static_cast<uint16_t>(position);
}

uint16_t FutureDispatcherBase::getRequestId_()
{
    uint16_t newRequestId{INVALID_REQUEST_ID};

    decltype(kRequestdIds)::iterator freeBlock =
        etl::find_if(kRequestdIds.begin(), kRequestdIds.end(), [](uint32_t const block) -> bool {
            return block != etl::numeric_limits<uint32_t>::max();
        });

    if (freeBlock != kRequestdIds.end())
    {
        const auto index = static_cast<uint32_t>(etl::distance(kRequestdIds.begin(), freeBlock));
        const uint16_t rightMostUnsetBitPosition = getPositionOfRightMostUnsetBit_(*freeBlock);

        newRequestId = rightMostUnsetBitPosition + index * FUTURE_DISPATCHER_OFFSET;

        // request limit can be smaller than 32 (1-31) -> we need to fail in this case
        if (newRequestId < futures_.max_size())
        {
            obtainRequestId_(newRequestId);  // set this position to 0
        }
        else
        {
            newRequestId = INVALID_REQUEST_ID;
        }
    }

    return newRequestId;
}

void FutureDispatcherBase::freeRequestId_(const uint16_t requestId)
{
    const uint32_t mask = ~(static_cast<uint32_t>(1U)
                            << (static_cast<uint32_t>(requestId) % static_cast<uint32_t>(FUTURE_DISPATCHER_OFFSET)));
    kRequestdIds[requestId / FUTURE_DISPATCHER_OFFSET] &= static_cast<uint32_t>(mask);
}

void FutureDispatcherBase::obtainRequestId_(const uint16_t requestId)
{
    kRequestdIds[requestId / FUTURE_DISPATCHER_OFFSET] |=
        (static_cast<uint32_t>(1U) << (static_cast<uint32_t>(requestId) %
                                       static_cast<uint32_t>(FUTURE_DISPATCHER_OFFSET)));
}

IFuture* FutureDispatcherBase::releaseRequestId(const MiddlewareMessage& msg)
{
    IFuture* ret = nullptr;
    auto const requestId = msg.getHeader().requestId;
    auto* const activeFuture =
        etl::find_if(futures_.begin(), futures_.end(), [requestId](const IFuture* const future) -> bool {
            return (future->getRequestId() == requestId);
        });

    // another sanity check, maybe future was already invalidated by client before answer was
    // received
    if (activeFuture != futures_.end())
    {
        freeRequestId_((*activeFuture)->getFutureDispatcherSlot());
        (*activeFuture)->setRequestId(INVALID_REQUEST_ID);
        (*activeFuture)->setFutureDispatcherSlot(INVALID_REQUEST_ID);
        const IFuture::State newState =
            msg.hasError() ? static_cast<IFuture::State>(msg.getErrorState()) : IFuture::State::Ready;
        (*activeFuture)->setState(newState);
        ret = *activeFuture;
        /* KW_SUPPRESS_START:MISRA.CAST.PTR.UNRELATED: IPBD-68166 (Deviation Approved) */
        static_cast<void>(futures_.erase(activeFuture));
        /* KW_SUPPRESS_END:MISRA.CAST.PTR.UNRELATED: IPBD-68166 (Deviation Approved) */
    }
    else
    {
        // suppress misra 0.1.9 next_line: This is not a null statement in production.
        logger::logMessageSendingFailure(
            logger::LogLevel::Error, logger::Error::SendMessage, HRESULT::FutureNotFound, msg);
    }

    return ret;
}

void FutureDispatcherBase::updateTimeouts(const uint32_t now,
                                          const uint32_t timeoutDuration,
                                          etl::ivector<IFuture*>& futuresToErase)
{
    // NOLINTBEGIN(cppcoreguidelines-pro-type-static-cast-downcast): IPBD-49847 (Deviation Approved)
    /* KW_SUPPRESS_START:MISRA.CAST.PTR.UNRELATED: IPBD-68166 (Deviation Approved) */
    IFuture* const* const firstNonExpiredFuture =
        etl::upper_bound(futures_.cbegin(),
                         futures_.cend(),
                         now,
                         [timeoutDuration](const uint32_t timeNow, const IFuture* const future) -> bool {
                             const auto* futureWithTimeout = static_cast<const AbstractFutureTimeout*>(future);
                             return ((futureWithTimeout->getCallerTimestamp() - timeNow) <= timeoutDuration);
                         });
    // nothing to do
    if (firstNonExpiredFuture != futures_.cbegin())
    {
        static_cast<void>(etl::move(futures_.cbegin(), firstNonExpiredFuture, etl::back_inserter(futuresToErase)));
        static_cast<void>(futures_.erase(futures_.cbegin(), firstNonExpiredFuture));
        IFuture** toErase = futuresToErase.begin();
        while (toErase != futuresToErase.end())
        {
            IFuture* future = *toErase;
            toErase = futuresToErase.erase(toErase);
            future->setState(IFuture::State::Timeout);
            freeRequestId_(future->getFutureDispatcherSlot());
            future->setRequestId(INVALID_REQUEST_ID);
            future->setFutureDispatcherSlot(INVALID_REQUEST_ID);
            static_cast<AbstractFutureTimeout*>(future)->timeoutExpired();
        }
    }
    /* KW_SUPPRESS_END:MISRA.CAST.PTR.UNRELATED: IPBD-68166 (Deviation Approved) */
    // NOLINTEND(cppcoreguidelines-pro-type-static-cast-downcast): IPBD-49847 (Deviation Approved)
}

}  // namespace middleware::core
