#pragma once

#include <cstdint>

#include <etl/iterator.h>
#include <etl/memory.h>
#include <etl/optional.h>
#include <etl/span.h>
#include <etl/type_traits.h>

#include "middleware/core/Message.h"
#include "middleware/core/types.h"

namespace middleware::core
{

class MessageAllocator
{
public:
    ~MessageAllocator()                                        = default;
    MessageAllocator(MessageAllocator const& other)            = delete;
    MessageAllocator(MessageAllocator&& other)                 = delete;
    MessageAllocator& operator=(MessageAllocator const& other) = delete;
    MessageAllocator& operator=(MessageAllocator&& other)      = delete;

    /**
     * @brief Decides how \p obj will be allocated in shared memory based on if T is trivially
     * copyable and if T can be directly copied inside Message's internal payload or if
     * needs to be externally allocated.
     *
     * @tparam T
     * @param obj
     * @param msg
     * @param numberOfReferences
     * @return HRESULT
     */
    // Overload 1: Non-trivially copyable types
    template<typename T>
    [[nodiscard]] typename etl::enable_if<!etl::is_trivially_copyable<T>::value, HRESULT>::type
    allocate(T const& obj, Message& msg, uint8_t const numberOfReferences = 1U)
    {
        HRESULT ret = HRESULT::CannotAllocatePayload;

        size_t const payloadSize = T::AllocationPolicy::getNeededSize(obj);
        etl::optional<etl::span<uint8_t>> buffer
            = MessageAllocator::allocateNonTrivialType_(payloadSize, msg, numberOfReferences);
        if (buffer.has_value())
        {
            T::AllocationPolicy::serialize(obj, buffer.value());
            ret = HRESULT::Ok;
        }

        return ret;
    }

    // Overload 2: Trivially copyable types that fit in MAX_PAYLOAD_SIZE
    template<typename T>
    [[nodiscard]] typename etl::enable_if<
        etl::is_trivially_copyable<T>::value && (sizeof(T) <= Message::MAX_PAYLOAD_SIZE),
        HRESULT>::type
    allocate(T const& obj, Message& msg, uint8_t const)
    {
        msg.constructObjectAtPayload(obj);
        return HRESULT::Ok;
    }

    // Overload 3: Trivially copyable types that don't fit in MAX_PAYLOAD_SIZE
    template<typename T>
    [[nodiscard]] typename etl::enable_if<
        etl::is_trivially_copyable<T>::value && (sizeof(T) > Message::MAX_PAYLOAD_SIZE),
        HRESULT>::type
    allocate(T const& obj, Message& msg, uint8_t const numberOfReferences = 1U)
    {
        return MessageAllocator::allocateTrivialType(&obj, sizeof(obj), msg, numberOfReferences);
    }

    /**
     * @brief Reads an object of type T from the content of \p msg.
     * @details The reading of T will depend on the properties of T. If it is not trivially
     * copyable, we know that it was externally allocated and that it needs to be deserialized. If T
     * is trivially copyable we know we can read it directly from either inside the
     * Message's internal payload or from some shared memory space where the payload was
     * stored.
     *
     * @tparam T
     * @param msg
     * @return T
     */
    // Overload 1: Non-trivially copyable types
    template<typename T>
    typename etl::enable_if<!etl::is_trivially_copyable<T>::value, T>::type
    readPayload(Message const& msg)
    {
        uint8_t* const ptr = getAllocatorPointerFromMessage(msg);
        T obj              = T::AllocationPolicy::deserialize(
            etl::span<uint8_t>(ptr, msg.getExternalHandle().size));
        return obj;
    }

    // Overload 2: Trivially copyable types that fit in MAX_PAYLOAD_SIZE
    template<typename T>
    typename etl::enable_if<
        etl::is_trivially_copyable<T>::value && (sizeof(T) <= Message::MAX_PAYLOAD_SIZE),
        T>::type
    readPayload(Message const& msg)
    {
        return msg.getObjectStoredInPayload<T>();
    }

    // Overload 3: Trivially copyable types that don't fit in MAX_PAYLOAD_SIZE
    template<typename T>
    typename etl::enable_if<
        etl::is_trivially_copyable<T>::value && (sizeof(T) > Message::MAX_PAYLOAD_SIZE),
        T>::type
    readPayload(Message const& msg)
    {
        uint8_t* const ptr = getAllocatorPointerFromMessage(msg);
        T obj              = etl::get_object_at<T>(ptr);
        return obj;
    }

    /**
     * @brief Releases any external resources if \p msg contains a payload that is stored
     * externally.
     *
     * @param msg
     */
    static void deallocate(Message const& msg);

    /**
     * @brief Get the MessageAllocator instance.
     *
     * @return MessageAllocator&
     */
    static MessageAllocator& getInstance() { return gInstance; }

private:
    /**
     * @brief Specialization function to allocate space on the middleware allocators for a trivially
     * copyable type, meaning that we can use memcpy to safely copy the entire payload.
     *
     * @param objPtr
     * @param objSize
     * @param msg
     * @param numberOfReferences
     * @return HRESULT
     */
    static HRESULT allocateTrivialType(
        void const* objPtr, size_t objSize, Message& msg, uint8_t numberOfReferences);

    /**
     * @brief Specialization function to allocate space on the middleware allocators for a non
     * trivially copyable type.
     *
     * @param payloadSize
     * @param msg
     * @param numberOfReferences
     * @return etl::optional<etl::span<uint8_t>>
     */
    static etl::optional<etl::span<uint8_t>>
    allocateNonTrivialType_(size_t payloadSize, Message& msg, uint8_t numberOfReferences);

    /**
     * @brief Retrieves the allocator's pointer, stored in the message, which is being used to store
     * the actual payload.
     *
     * @param msg
     * @return uint8_t*
     */
    static uint8_t* getAllocatorPointerFromMessage(Message const& msg);

    MessageAllocator() = default;

    static MessageAllocator gInstance;
};

} // namespace middleware::core
