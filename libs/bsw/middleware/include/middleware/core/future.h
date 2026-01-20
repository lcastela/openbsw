#pragma once

#include <cstdint>

#include <etl/delegate.h>
#include <etl/expected.h>
#include <etl/type_traits.h>

#include "middleware/core/abstract_future_timeout.h"
#include "middleware/core/ifuture.h"
#include "middleware/core/message_allocator.h"
#include "middleware/core/middleware_message.h"
#include "middleware/core/types.h"

namespace middleware::core
{

template <typename Arg, MemberId MemberIdValue, bool CopyOnSend, uint32_t TIMEOUT = 0U>
struct FutureTraits
{
    using ArgumentType = Arg;
    static constexpr MemberId METHOD_MEMBER_ID = MemberIdValue;
    static constexpr bool COPY = CopyOnSend;
    static constexpr uint32_t TIMEOUT_VALUE = TIMEOUT;
};

namespace internal
{

template <typename FutureTraits, typename Specialization = void>
struct CallbackHelper;

/**
 * \brief Partial specialization of \a CallbackHelper with non-void \a ArgumentType specified
 *
 * \tparam FutureTraits
 */
template <typename FutureTraits>
struct CallbackHelper<FutureTraits,
                      etl::enable_if_t<(etl::is_void<typename FutureTraits::ArgumentType>::value == false)>>
{
    using Callback = ::etl::delegate<void(etl::expected<typename FutureTraits::ArgumentType, IFuture::State>&&)>;

    /**
     * \brief Calls the \a callback of the user with the result according to the current future's state.
     *
     * \param cb the user callback.
     * \param msg the payload for the user.
     * \param state the current future state.
     */
    static void call(const Callback& cb, const MiddlewareMessage& msg, const IFuture::State state)
    {
        if (cb.is_valid())
        {
            if (state != IFuture::State::Ready)
            {
                // call notification
                cb(etl::unexpected<IFuture::State>(etl::in_place, state));
            }
            else
            {
                cb(etl::expected<typename FutureTraits::ArgumentType, IFuture::State>(
                    etl::in_place,
                    MessageAllocator::getInstance().readPayload<typename FutureTraits::ArgumentType>(msg)));
            }
        }
    }
};

/**
 * \brief Partial specialization of \a CallbackHelper with void \a ArgumentType specified
 *
 * \tparam FutureTraits
 */
template <typename FutureTraits>
struct CallbackHelper<FutureTraits, etl::enable_if_t<etl::is_void<typename FutureTraits::ArgumentType>::value>>
{
    using Callback = ::etl::delegate<void(etl::expected<void, IFuture::State>&&)>;

    /**
     * \brief Calls the \a callback of the user with the result according to the current future's state.
     *
     * \param cb the user callback.
     * \param msg the payload for the user.
     * \param state the current future state.
     */
    static void call(const Callback& cb, const MiddlewareMessage&, const IFuture::State state)
    {
        if (cb.is_valid())
        {
            if (state != IFuture::State::Ready)
            {
                cb(etl::unexpected<IFuture::State>(etl::in_place, state));
            }
            else
            {
                cb(etl::expected<void, IFuture::State>());
            }
        }
    }
};

}  // namespace internal

template <typename FutureTraits, typename TimeoutEnabled = void>
class Future;

/**
 * \brief Partial specialization of future with no Timeout specified
 *
 * \tparam FutureTraits
 */
template <typename FutureTraits>
class Future<FutureTraits, etl::enable_if_t<(FutureTraits::TIMEOUT_VALUE == 0U)>> final : public IFuture
{
  public:
    using FutureType = Future<FutureTraits>;
    using Type = typename FutureTraits::ArgumentType;
    using Base = IFuture;
    using Traits = FutureTraits;
    using Callback = typename internal::CallbackHelper<FutureTraits>::Callback;

    constexpr explicit Future(const Callback callback = Callback()) : Base(), fCallback(callback) {}

    ~Future() { unsetReceiveHandler(); }

    /**
     * \brief Set the \a fCallback attribute
     *
     * \param callback
     */
    void setReceiveHandler(const Callback callback) { fCallback = callback; }

    /**
     * \brief Unset the \a fCallback attribute, which will become an empty function
     *
     */
    void unsetReceiveHandler() { fCallback = Callback(); }

    /**
     * \brief Calls the \a fCallback of the user, if it has been setted, with the current result.
     *
     * \param msg contains the payload for the user.
     */
    void setResult(const MiddlewareMessage& msg)
    {
        if (fCallback)
        {
            internal::CallbackHelper<Traits>::call(fCallback, msg, getState());
        }
    }

  private:
    Callback fCallback;
};

/**
 * \brief Partial specialization of future with \a Timeout specified
 *
 * \tparam FutureTraits
 */
template <typename FutureTraits>
class Future<FutureTraits, etl::enable_if_t<(FutureTraits::TIMEOUT_VALUE > 0U)>> final : public AbstractFutureTimeout
{
  public:
    using FutureType = Future<FutureTraits>;
    using Type = typename FutureTraits::ArgumentType;
    using Base = AbstractFutureTimeout;
    using Traits = FutureTraits;
    using Callback = typename internal::CallbackHelper<FutureTraits>::Callback;

    explicit Future(const Callback callback = Callback()) : Base(), fCallback(callback) {}
    ~Future() { unsetReceiveHandler(); }

    /**
     * \brief Getter for \a callerTimestamp_
     *
     * \return uint32_t
     */
    inline uint32_t getCallerTimestamp() const final { return callerTimestamp_; };

    /**
     * \brief Set the \a fCallback attribute
     *
     * \param callback
     */
    void setReceiveHandler(const Callback callback) { fCallback = callback; }

    /**
     * \brief Unset the \a fCallback attribute, which will become an empty function
     *
     */
    void unsetReceiveHandler() { fCallback = Callback(); }

    /**
     * \brief Setter for \a callerTimestamp_
     *
     * \param timestamp
     */
    inline void setCallerTimestamp(const uint32_t timestamp) final { callerTimestamp_ = timestamp; }

    /**
     * \brief Call the \a fCallback of the user, if it has been setted, with state = State::Timeout
     *
     */
    void timeoutExpired() final
    {
        setState(IFuture::State::Timeout);
        if (fCallback)
        {
            internal::CallbackHelper<Traits>::call(fCallback, MiddlewareMessage(), getState());
        }
    }

    /**
     * \brief Calls the \a fCallback of the user, if it has been setted, with the current result.
     *
     * \param msg contains the payload for the user.
     */
    void setResult(const MiddlewareMessage& msg)
    {
        if (fCallback)
        {
            internal::CallbackHelper<Traits>::call(fCallback, msg, getState());
        }
    }

  private:
    uint32_t callerTimestamp_{0U};
    Callback fCallback;
};

}  // namespace middleware::core
