#include <stdint.h>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "middleware/core/cluster_connection.h"
#include "middleware/core/icluster_connection_configuration_base.h"
#include "middleware/core/message_allocator.h"
#include "middleware/core/middleware_message.h"
#include "middleware/core/proxy_base.h"
#include "middleware/core/skeleton_base.h"
#include "middleware/core/transceiver_container.h"
#include "middleware/core/types.h"
#include "middleware_instances_database.h"
#include "proxy.h"
#include "skeleton.h"

using testing::_;
using testing::Exactly;
using testing::NiceMock;

namespace middleware
{
namespace core
{
namespace test
{

struct MiddelwareMessageComparator
{
    using MsgType = ::middleware::core::MiddlewareMessage;

    bool checkMsgHeader(const MsgType& other) const
    {
        const MiddlewareMessage::Header& msgHeader = _msg.getHeader();
        const MiddlewareMessage::Header& otherHeader = other.getHeader();
        return _msg.getSourceClusterId() == other.getSourceClusterId() &&
               _msg.getTargetClusterId() == other.getTargetClusterId() &&
               msgHeader.serviceId == otherHeader.serviceId && msgHeader.memberId == otherHeader.memberId &&
               msgHeader.serviceInstanceId == otherHeader.serviceInstanceId &&
               _msg.getAddressId() == other.getAddressId() && msgHeader.requestId == otherHeader.requestId &&
               _msg.getFlags() == other.getFlags() && _msg.isSkeletonTarget() == other.isSkeletonTarget() &&
               _msg.isProxyTarget() == other.isProxyTarget() && _msg.hasError() == other.hasError() &&
               _msg.isEvent() == other.isEvent() && _msg.hasOutArgs() == other.hasOutArgs();
    }

    void setReturnCode(::middleware::core::HRESULT ret) { _ret = ret; }

    ::middleware::core::HRESULT msgReceived(const MsgType& msg)
    {
        _msg = msg;
        return _ret;
    }

  private:
    MsgType _msg;
    ::middleware::core::HRESULT _ret{::middleware::core::HRESULT::Ok};
};

struct ProxyMockStoredMessage : public ProxyMock, MiddelwareMessageComparator
{
    using ProxyMock::ProxyMock;
    ::middleware::core::HRESULT onNewMessageReceived(const ::middleware::core::MiddlewareMessage& msg) override
    {
        return MiddelwareMessageComparator::msgReceived(msg);
    }
};

struct SkeletonMockStoredMessage : public SkeletonMock, MiddelwareMessageComparator
{
    using SkeletonMock::SkeletonMock;
    ::middleware::core::HRESULT onNewMessageReceived(const ::middleware::core::MiddlewareMessage& msg) override
    {
        return MiddelwareMessageComparator::msgReceived(msg);
    }
};

struct TimeoutMock : middleware::core::ITimeout
{
    void updateTimeouts() { _triggered = true; }

    bool hasBeenTriggered() { return _triggered; }

  private:
    bool _triggered{false};
};

struct ClusterConfigurationNoTimeout : public IClusterConnectionConfigurationBase
{

    static const ServiceId serviceId{12};
    static const InstanceId instanceId{1};
    static const AddressId addressId{1};
    static const ClusterId sourceClusterId{1};
    static const ClusterId targetClusterId{2};

    ClusterConfigurationNoTimeout()
    {

        // setup proxies
        (_proxyTransceivers[0]).fContainer->emplace_back(&_proxy);

        etl::sort(_proxyTransceivers[0].fContainer->begin(),
                  _proxyTransceivers[0].fContainer->end(),
                  meta::TransceiverContainer::TransceiverComparator());

        // setup skeletons
        _skeletonTransceivers[0].fContainer->emplace_back(&_skeleton);

        etl::sort(_skeletonTransceivers[0].fContainer->begin(),
                  _skeletonTransceivers[0].fContainer->end(),
                  meta::TransceiverContainer::TransceiverComparator());
    }

    ClusterId getSourceClusterId() const override { return sourceClusterId; }
    ClusterId getTargetClusterId() const override { return targetClusterId; }
    bool write(const MiddlewareMessage& msg) const override { return true; }
    std::size_t registeredTransceiversCount(const ServiceId serviceId) const override { return 0; }
    HRESULT dispatchMessage(const MiddlewareMessage& msg) const override
    {
        return IClusterConnectionConfigurationBase::dispatchMessage(std::begin(_proxyTransceivers),
                                                                    std::end(_proxyTransceivers),
                                                                    std::begin(_skeletonTransceivers),
                                                                    std::end(_skeletonTransceivers),
                                                                    msg);
    }

    // relaying to protected base class methods
    HRESULT dispatchMessageToProxy(const MiddlewareMessage& msg)
    {
        return IClusterConnectionConfigurationBase::dispatchMessageToProxy(
            std::begin(_proxyTransceivers), std::end(_proxyTransceivers), msg);
    }

    // relaying to protected base class methods
    HRESULT dispatchMessageToSkeleton(const MiddlewareMessage& msg)
    {
        return IClusterConnectionConfigurationBase::dispatchMessageToSkeleton(
            std::begin(_skeletonTransceivers), std::end(_skeletonTransceivers), msg);
    }

    ProxyMockStoredMessage& getProxy() { return _proxy; }
    SkeletonMockStoredMessage& getSkeleton() { return _skeleton; }

  private:
    etl::vector<::middleware::core::ITransceiver*, 1U> _proxyTransceiversAlloc{};
    etl::ivector<::middleware::core::ITransceiver*>& _iProxyTransceivers{_proxyTransceiversAlloc};

    etl::vector<::middleware::core::ITransceiver*, 1U> _skeletonTransceiversAlloc{};
    etl::ivector<::middleware::core::ITransceiver*>& _iSkeletonTransceivers{_skeletonTransceiversAlloc};

    ::middleware::core::meta::TransceiverContainer _proxyTransceivers[1]{
        {&_iProxyTransceivers, ClusterConfigurationNoTimeout::serviceId, 0U}};
    ::middleware::core::meta::TransceiverContainer _skeletonTransceivers[1]{
        {&_iSkeletonTransceivers, ClusterConfigurationNoTimeout::serviceId, 0U}};

  protected:
    ProxyMockStoredMessage _proxy{ClusterConfigurationNoTimeout::serviceId,
                                  ClusterConfigurationNoTimeout::instanceId,
                                  ClusterConfigurationNoTimeout::addressId};
    SkeletonMockStoredMessage _skeleton{ClusterConfigurationNoTimeout::serviceId,
                                        ClusterConfigurationNoTimeout::instanceId};
};

struct ClusterConfigurationTimeout : ITimeoutConfiguration
{

    static const size_t MAX_TIMEOUT_RECEIVERS = 2;

    // implementing ITimeoutConfiguration
    ClusterId getSourceClusterId() const override { return static_cast<ClusterId>(1); }
    ClusterId getTargetClusterId() const override { return static_cast<ClusterId>(2); }
    bool write(const MiddlewareMessage& msg) const override { return true; }
    std::size_t registeredTransceiversCount(const ServiceId serviceId) const override { return 0; }
    HRESULT dispatchMessage(const MiddlewareMessage& msg) const override { return ::middleware::core::HRESULT::Ok; }

    void registerTimeoutTransceiver(ITimeout& transceiver) override
    {
        ITimeoutConfiguration::registerTimeoutTransceiver(transceiver, _timeoutTransceiver);
    }
    void unregisterTimeoutTransceiver(ITimeout& transceiver) override
    {
        ITimeoutConfiguration::unregisterTimeoutTransceiver(transceiver, _timeoutTransceiver);
    }
    void updateTimeouts() override { ITimeoutConfiguration::updateTimeouts(_timeoutTransceiver); }

    // helper test functions accessing the container
    size_t numTransceivers() { return _timeoutTransceiver.size(); }
    bool containsTransceiver(const ITimeout& transceiver)
    {
        return _timeoutTransceiver.cend() !=
               etl::find(_timeoutTransceiver.cbegin(), _timeoutTransceiver.cend(), &transceiver);
    }

  private:
    etl::vector<::middleware::core::ITimeout*, MAX_TIMEOUT_RECEIVERS> _timeoutTransceiverAlloc{};
    etl::ivector<::middleware::core::ITimeout*>& _timeoutTransceiver{_timeoutTransceiverAlloc};
};

class ConfigurationBaseTest : public ::testing::Test
{
  public:
    void SetUp() override {}

    void TearDown() override {}

    static MiddlewareMessage createRequestMessage(const MemberId memberId, const RequestId requestId)
    {
        MiddlewareMessage::Header header{
            ClusterConfigurationNoTimeout::serviceId, memberId, requestId, ClusterConfigurationNoTimeout::instanceId};
        MiddlewareMessage msg = MiddlewareMessage::createRequest(header,
                                                                 ClusterConfigurationNoTimeout::sourceClusterId,
                                                                 ClusterConfigurationNoTimeout::targetClusterId,
                                                                 ClusterConfigurationNoTimeout::addressId);
        return msg;
    }

    static MiddlewareMessage createInvalidRequestMessage(const MemberId memberId, const RequestId requestId)
    {
        MiddlewareMessage::Header header{
            ClusterConfigurationNoTimeout::serviceId + 1 /* offset ensuring there is no hit in the DB*/,
            memberId,
            requestId,
            ClusterConfigurationNoTimeout::instanceId};
        MiddlewareMessage msg = MiddlewareMessage::createRequest(
            header,
            ClusterConfigurationNoTimeout::sourceClusterId,
            ClusterConfigurationNoTimeout::targetClusterId,
            ClusterConfigurationNoTimeout::addressId + 1 /* offset ensuring there is no hit in the DB*/);
        return msg;
    }

    static MiddlewareMessage createResponseMessage(const MemberId memberId, const RequestId requestId)
    {
        MiddlewareMessage::Header header{
            ClusterConfigurationNoTimeout::serviceId, memberId, requestId, ClusterConfigurationNoTimeout::instanceId};
        MiddlewareMessage msg = MiddlewareMessage::createResponse(header,
                                                                  ClusterConfigurationNoTimeout::sourceClusterId,
                                                                  ClusterConfigurationNoTimeout::targetClusterId,
                                                                  ClusterConfigurationNoTimeout::addressId);
        return msg;
    }

    static MiddlewareMessage createInvalidResponseMessage(const MemberId memberId, const RequestId requestId)
    {
        MiddlewareMessage::Header header{
            ClusterConfigurationNoTimeout::serviceId + 1 /* offset ensuring there is no hit in the DB*/,
            memberId,
            requestId,
            ClusterConfigurationNoTimeout::instanceId};
        MiddlewareMessage msg = MiddlewareMessage::createResponse(
            header,
            ClusterConfigurationNoTimeout::sourceClusterId,
            ClusterConfigurationNoTimeout::targetClusterId,
            ClusterConfigurationNoTimeout::addressId + 1 /* offset ensuring there is no hit in the DB*/);
        return msg;
    }

    static MiddlewareMessage createEvent(const MemberId memberId)
    {
        MiddlewareMessage msg = MiddlewareMessage::createEvent(ClusterConfigurationNoTimeout::serviceId,
                                                               memberId,
                                                               ClusterConfigurationNoTimeout::instanceId,
                                                               ClusterConfigurationNoTimeout::sourceClusterId);
        msg.setTargetClusterId(ClusterConfigurationNoTimeout::targetClusterId);

        return msg;
    }

  protected:
    ClusterConfigurationNoTimeout _clusterConf;
    ClusterConfigurationTimeout _clusterTimeoutConf;
};

TEST_F(ConfigurationBaseTest, ProxyTargetRouted)
{
    MiddlewareMessage prxyTrgtMsg = createResponseMessage(123, 321);

    EXPECT_EQ(::middleware::core::HRESULT::Ok, _clusterConf.dispatchMessage(prxyTrgtMsg));
}

TEST_F(ConfigurationBaseTest, ProxyTargetRoutedEvent)
{
    MiddlewareMessage eventMsg = createEvent(123);

    // no client side return code for events, check the reception of the msg by looking at the delivered payload
    const uint32_t obj = 0x1234U;
    HRESULT ret = MessageAllocator::getInstance().allocate(obj, eventMsg);

    EXPECT_EQ(ret, HRESULT::Ok);
    EXPECT_TRUE(eventMsg.isEvent());
    EXPECT_EQ(::middleware::core::HRESULT::Ok, _clusterConf.dispatchMessage(eventMsg));
    EXPECT_TRUE(_clusterConf.getProxy().checkMsgHeader(eventMsg));
}

TEST_F(ConfigurationBaseTest, ProxyTargetRoutedFailed)
{
    MiddlewareMessage prxyTrgtMsg = createInvalidResponseMessage(123, 321);

    // expectation: Fall through, returning HRESULT::Ok if adressing params can't be matched to a registered proxy
    EXPECT_EQ(::middleware::core::HRESULT::Ok, _clusterConf.dispatchMessage(prxyTrgtMsg));
}

TEST_F(ConfigurationBaseTest, ProxySkeletonWrongEndpoint)
{
    MiddlewareMessage prxyTrgtMsg = createInvalidRequestMessage(123, 321);

    EXPECT_EQ(::middleware::core::HRESULT::RoutingError, _clusterConf.dispatchMessageToProxy(prxyTrgtMsg));
}

TEST_F(ConfigurationBaseTest, SkeletonTargetRouted)
{
    MiddlewareMessage skltnTrgtMsg = createRequestMessage(123, 321);

    EXPECT_EQ(::middleware::core::HRESULT::Ok, _clusterConf.dispatchMessage(skltnTrgtMsg));
}

TEST_F(ConfigurationBaseTest, SkeletonTargetRoutedMemberNotFound)
{
    MiddlewareMessage skltnTrgtMsg = createRequestMessage(123, 321);

    _clusterConf.getSkeleton().setReturnCode(::middleware::core::HRESULT::ServiceMemberIdNotFound);
    EXPECT_EQ(::middleware::core::HRESULT::ServiceMemberIdNotFound, _clusterConf.dispatchMessage(skltnTrgtMsg));
}

TEST_F(ConfigurationBaseTest, SkeletonTargetRoutedServiceBusy)
{
    MiddlewareMessage skltnTrgtMsg = createRequestMessage(123, 321);

    _clusterConf.getSkeleton().setReturnCode(::middleware::core::HRESULT::ServiceBusy);
    EXPECT_EQ(::middleware::core::HRESULT::ServiceBusy, _clusterConf.dispatchMessage(skltnTrgtMsg));
}

TEST_F(ConfigurationBaseTest, SkeletonTargetRoutedArbitraryReturn)
{
    MiddlewareMessage skltnTrgtMsg = createRequestMessage(123, 321);

    // setting any return code not created by the framework but the receiver. Expecting pass through.
    _clusterConf.getSkeleton().setReturnCode(::middleware::core::HRESULT::NotImplemented);
    EXPECT_EQ(::middleware::core::HRESULT::NotImplemented, _clusterConf.dispatchMessage(skltnTrgtMsg));
}

TEST_F(ConfigurationBaseTest, SkeletonTargetRoutedFailed)
{
    MiddlewareMessage skltnTrgtMsg = createInvalidRequestMessage(123, 321);

    EXPECT_EQ(::middleware::core::HRESULT::ServiceNotFound, _clusterConf.dispatchMessage(skltnTrgtMsg));
}

TEST_F(ConfigurationBaseTest, SkeletonProxyWrongEndpoint)
{
    MiddlewareMessage skltnTrgtMsg = createResponseMessage(123, 321);

    EXPECT_EQ(::middleware::core::HRESULT::RoutingError, _clusterConf.dispatchMessageToSkeleton(skltnTrgtMsg));
}

TEST_F(ConfigurationBaseTest, TimeoutTransceiverAddRemove)
{
    TimeoutMock rec1;
    TimeoutMock rec2;

    EXPECT_EQ(0, _clusterTimeoutConf.numTransceivers());

    // add first receiver
    _clusterTimeoutConf.registerTimeoutTransceiver(rec1);
    EXPECT_EQ(1, _clusterTimeoutConf.numTransceivers());
    EXPECT_TRUE(_clusterTimeoutConf.containsTransceiver(rec1));

    // add second receiver
    _clusterTimeoutConf.registerTimeoutTransceiver(rec2);
    EXPECT_EQ(2, _clusterTimeoutConf.numTransceivers());
    EXPECT_TRUE(_clusterTimeoutConf.containsTransceiver(rec1));
    EXPECT_TRUE(_clusterTimeoutConf.containsTransceiver(rec2));

    // delete second receiver
    _clusterTimeoutConf.unregisterTimeoutTransceiver(rec2);
    EXPECT_EQ(1, _clusterTimeoutConf.numTransceivers());
    EXPECT_TRUE(_clusterTimeoutConf.containsTransceiver(rec1));

    // delete first receiver
    _clusterTimeoutConf.unregisterTimeoutTransceiver(rec1);
    EXPECT_EQ(0, _clusterTimeoutConf.numTransceivers());
}

TEST_F(ConfigurationBaseTest, TimeoutTransceiverAddButFull)
{
    TimeoutMock rec1;
    TimeoutMock rec2;
    TimeoutMock rec3;

    EXPECT_EQ(0, _clusterTimeoutConf.numTransceivers());

    // add first receiver
    _clusterTimeoutConf.registerTimeoutTransceiver(rec1);
    EXPECT_EQ(1, _clusterTimeoutConf.numTransceivers());
    EXPECT_TRUE(_clusterTimeoutConf.containsTransceiver(rec1));

    // add second receiver
    _clusterTimeoutConf.registerTimeoutTransceiver(rec2);
    EXPECT_EQ(2, _clusterTimeoutConf.numTransceivers());
    EXPECT_TRUE(_clusterTimeoutConf.containsTransceiver(rec1));
    EXPECT_TRUE(_clusterTimeoutConf.containsTransceiver(rec2));

    // (try) add third receiver
    _clusterTimeoutConf.registerTimeoutTransceiver(rec3);
    EXPECT_EQ(2, _clusterTimeoutConf.numTransceivers());
    EXPECT_TRUE(_clusterTimeoutConf.containsTransceiver(rec1));
    EXPECT_TRUE(_clusterTimeoutConf.containsTransceiver(rec2));

    EXPECT_FALSE(_clusterTimeoutConf.containsTransceiver(rec3));
}

TEST_F(ConfigurationBaseTest, TimeoutTransceiverDoubleDelete)
{
    TimeoutMock rec1;
    TimeoutMock rec2;

    EXPECT_EQ(0, _clusterTimeoutConf.numTransceivers());

    // add first receiver
    _clusterTimeoutConf.registerTimeoutTransceiver(rec1);
    EXPECT_EQ(1, _clusterTimeoutConf.numTransceivers());
    EXPECT_TRUE(_clusterTimeoutConf.containsTransceiver(rec1));

    // add second receiver
    _clusterTimeoutConf.registerTimeoutTransceiver(rec2);
    EXPECT_EQ(2, _clusterTimeoutConf.numTransceivers());
    EXPECT_TRUE(_clusterTimeoutConf.containsTransceiver(rec1));
    EXPECT_TRUE(_clusterTimeoutConf.containsTransceiver(rec2));

    // delete second receiver
    _clusterTimeoutConf.unregisterTimeoutTransceiver(rec2);
    EXPECT_EQ(1, _clusterTimeoutConf.numTransceivers());
    EXPECT_TRUE(_clusterTimeoutConf.containsTransceiver(rec1));
    EXPECT_FALSE(_clusterTimeoutConf.containsTransceiver(rec2));

    // delete second receiver again
    _clusterTimeoutConf.unregisterTimeoutTransceiver(rec2);
    EXPECT_EQ(1, _clusterTimeoutConf.numTransceivers());
    EXPECT_TRUE(_clusterTimeoutConf.containsTransceiver(rec1));
    EXPECT_FALSE(_clusterTimeoutConf.containsTransceiver(rec2));
}

TEST_F(ConfigurationBaseTest, TimeoutTransceiverDoubleInsert)
{
    TimeoutMock rec1;

    EXPECT_EQ(0, _clusterTimeoutConf.numTransceivers());

    // add first receiver
    _clusterTimeoutConf.registerTimeoutTransceiver(rec1);
    EXPECT_EQ(1, _clusterTimeoutConf.numTransceivers());
    EXPECT_TRUE(_clusterTimeoutConf.containsTransceiver(rec1));

    // add first receiver again
    _clusterTimeoutConf.registerTimeoutTransceiver(rec1);
    EXPECT_EQ(1, _clusterTimeoutConf.numTransceivers());
    EXPECT_TRUE(_clusterTimeoutConf.containsTransceiver(rec1));
}

TEST_F(ConfigurationBaseTest, TimeoutTransceiverTriggerTest)
{
    TimeoutMock rec1;
    TimeoutMock rec2;

    // add receivers
    _clusterTimeoutConf.registerTimeoutTransceiver(rec1);
    _clusterTimeoutConf.registerTimeoutTransceiver(rec2);
    EXPECT_EQ(2, _clusterTimeoutConf.numTransceivers());
    EXPECT_TRUE(_clusterTimeoutConf.containsTransceiver(rec1));
    EXPECT_TRUE(_clusterTimeoutConf.containsTransceiver(rec2));

    // trigger all receivers
    _clusterTimeoutConf.updateTimeouts();

    EXPECT_TRUE(rec1.hasBeenTriggered());
    EXPECT_TRUE(rec2.hasBeenTriggered());
}

}  // namespace test
}  // namespace core
}  // namespace middleware
