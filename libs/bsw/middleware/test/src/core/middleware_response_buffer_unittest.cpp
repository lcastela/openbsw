#include <etl/array.h>
#include <etl/delegate.h>
#include <etl/span.h>
#include <etl/type_traits.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "middleware/core/message_allocator.h"
#include "middleware/core/middleware_message.h"
#include "middleware/core/response_buffer.h"
#include "middleware/core/response_buffer_base.h"
#include "middleware/core/types.h"
#include "middleware_instances_database.h"

namespace middleware::core::test
{

class SkeletonMock : public SkeletonBase
{
  public:
    using Base = SkeletonBase;

    HRESULT init(const InstanceId instanceId)
    {
        Base::setInstanceId(instanceId);
        return Base::initFromInstancesDatabase(instanceId, etl::span(INSTANCESDATABASE));
    }

    MOCK_METHOD(ServiceId, getServiceId, (), (const, final));
    MOCK_METHOD(HRESULT, sendMessage, (MiddlewareMessage&), (const, final));
    MOCK_METHOD(HRESULT, onNewMessageReceived, (const MiddlewareMessage&), (final));
};

class ResponseBufferTestSuite : public ::testing::Test
{
  public:
    using Traits = ResponseTraits<uint32_t, 0U>;
    static constexpr uint16_t RESPONSE_LIMIT = 10U;
    static constexpr InstanceId SKELETON_SERVICE_ID = 4096U;
    static constexpr InstanceId SKELETON_INSTANCE_ID = 1U;
    static constexpr ClusterId TARGET_CLUSTER_ID = static_cast<ClusterId>(2U);

    ResponseBufferTestSuite() : skeletonMock_(), responseBuffer(skeletonMock_) {}

    void SetUp() final
    {
        ON_CALL(skeletonMock_, getServiceId).WillByDefault(::testing::Return(SKELETON_SERVICE_ID));
        skeletonMock_.init(SKELETON_INSTANCE_ID);
    }

    HRESULT doRespond(ResponseBufferBase::SkeletonResponseInfo& response,
                      const typename Traits::ResponseType& result,
                      const bool handleResponseFailure = true)
    {
        return responseBuffer.respond(response, result, handleResponseFailure);
    }

    HRESULT doCancelResponse(ResponseBufferBase::SkeletonResponseInfo& response)
    {
        return responseBuffer.cancelResponse(response);
    }

    bool doIsResponseIteratorValid(ResponseBufferBase::SkeletonResponseInfo* const iterator)
    {
        return responseBuffer.isResponseIteratorValid(iterator);
    }

    ResponseBufferBase::SkeletonResponseInfo* doGetAvailableResponse(const AddressId addressId,
                                                                     const RequestId requestId)
    {
        return responseBuffer.getAvailableResponse(addressId, TARGET_CLUSTER_ID, requestId);
    }

    void checkMessageHeader(const MiddlewareMessage& msg,
                            const AddressId expectedAddressId,
                            const RequestId expectedRequestId)
    {
        EXPECT_EQ(msg.getHeader().serviceId, SKELETON_SERVICE_ID);
        EXPECT_EQ(msg.getHeader().serviceInstanceId, SKELETON_INSTANCE_ID);
        EXPECT_EQ(msg.getHeader().memberId, Traits::METHOD_MEMBER_ID);
        EXPECT_EQ(msg.isProxyTarget(), true);
        EXPECT_EQ(msg.isEvent(), false);
        EXPECT_EQ(msg.getTargetClusterId(), TARGET_CLUSTER_ID);
        EXPECT_EQ(msg.getSourceClusterId(), skeletonMock_.getSourceClusterId());
        EXPECT_EQ(msg.getAddressId(), expectedAddressId);
        EXPECT_EQ(msg.getHeader().requestId, expectedRequestId);
    }

  protected:
    ::testing::NiceMock<SkeletonMock> skeletonMock_;

  private:
    ResponseBuffer<Traits, RESPONSE_LIMIT> responseBuffer;
};

TEST_F(ResponseBufferTestSuite, TestCancelResponseWithValidSkeletonResponseInfo)
{
    // ARRANGE
    const AddressId proxyAddress = 0U;
    const RequestId requestId = 1U;
    ResponseBufferBase::SkeletonResponseInfo* response = this->doGetAvailableResponse(proxyAddress, requestId);

    // ASSERT
    EXPECT_EQ(this->doCancelResponse(*response), HRESULT::Ok);
}

TEST_F(ResponseBufferTestSuite, TestCancelResponseWithInvalidSkeletonResponseInfo)
{
    // A future not known to the ResponseBuffer should not be handled
    ResponseBufferBase::SkeletonResponseInfo responseInfo{};

    EXPECT_EQ(this->doCancelResponse(responseInfo), HRESULT::ResponseBufferFutureNotFound);
}

TEST_F(ResponseBufferTestSuite, TestRespondWithValidResponse)
{
    // ARRANGE
    Traits::ResponseType responseData{0xF1D9EE37U};
    const AddressId proxyAddress = 0U;
    const RequestId requestId = 1U;

    // ACT
    ResponseBufferBase::SkeletonResponseInfo* response = this->doGetAvailableResponse(proxyAddress, requestId);

    // ASSERT
    EXPECT_CALL(this->skeletonMock_, sendMessage).WillRepeatedly([this](MiddlewareMessage& msg) -> HRESULT {
        this->checkMessageHeader(msg, proxyAddress, requestId);
        return HRESULT::Ok;
    });
    EXPECT_EQ(this->doRespond(*response, responseData), HRESULT::Ok);
}

TEST_F(ResponseBufferTestSuite, TestRespondWithHandleResponseFailureFalse)
{
    // ARRANGE
    Traits::ResponseType responseData{0xF1D9EE37U};
    const AddressId proxyAddress = 0U;
    const RequestId requestId = 1U;

    // ACT
    ResponseBufferBase::SkeletonResponseInfo* response = this->doGetAvailableResponse(proxyAddress, requestId);

    // ASSERT
    EXPECT_CALL(this->skeletonMock_, sendMessage).WillRepeatedly([this](MiddlewareMessage& msg) -> HRESULT {
        this->checkMessageHeader(msg, proxyAddress, requestId);
        return HRESULT::Ok;
    });
    EXPECT_EQ(this->doRespond(*response, responseData, false), HRESULT::Ok);
}

TEST_F(ResponseBufferTestSuite, TestRespondWithInvalidResponse)
{
    // ARRANGE
    Traits::ResponseType responseData{0xF1D9EE37U};
    ResponseBufferBase::SkeletonResponseInfo response{};

    // ASSERT
    EXPECT_CALL(this->skeletonMock_, sendMessage).Times(0U);
    EXPECT_EQ(this->doRespond(response, responseData), HRESULT::ResponseBufferFutureNotFound);
}

TEST_F(ResponseBufferTestSuite, TestRespondWithInvalidsendResponseResult)
{
    // ARRANGE
    Traits::ResponseType responseData{0xF1D9EE37U};
    const AddressId proxyAddress = 0U;
    const RequestId requestId = 1U;

    // ACT
    ResponseBufferBase::SkeletonResponseInfo* response = this->doGetAvailableResponse(proxyAddress, requestId);

    // ASSERT
    ON_CALL(this->skeletonMock_, sendMessage).WillByDefault(testing::Return(HRESULT::QueueFull));
    EXPECT_EQ(this->doRespond(*response, responseData, false), HRESULT::QueueFull);
    EXPECT_EQ(this->doIsResponseIteratorValid(response), true);
}

TEST_F(ResponseBufferTestSuite, TestGetAvailableResponseExhaustion)
{
    // ARRANGE
    etl::vector<ResponseBufferBase::SkeletonResponseInfo*, RESPONSE_LIMIT> responses{};
    AddressId addressId{0U};
    RequestId requestId{0U};

    // ACT
    for (size_t counter = 0U; counter < RESPONSE_LIMIT; ++counter)
    {
        responses.push_back(this->doGetAvailableResponse(addressId, requestId));
        addressId++;
        requestId++;
    }

    // ASSERT
    EXPECT_EQ(this->doGetAvailableResponse(addressId, requestId), nullptr);
}

}  // namespace middleware::core::test
