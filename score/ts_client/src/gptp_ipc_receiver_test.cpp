/********************************************************************************
 * Copyright (c) 2026 Contributors to the Eclipse Foundation
 *
 * See the NOTICE file(s) distributed with this work for additional
 * information regarding copyright ownership.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Apache License Version 2.0 which is available at
 * https://www.apache.org/licenses/LICENSE-2.0
 *
 * SPDX-License-Identifier: Apache-2.0
 ********************************************************************************/
#include "score/ts_client/src/gptp_ipc_receiver.h"
#include "score/ts_client/src/gptp_ipc_publisher.h"
#include "score/ts_client/src/gptp_ipc_test_utils.h"

#include <gtest/gtest.h>

#include <fcntl.h>
#include <poll.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

namespace score
{
namespace ts
{
namespace details
{

/// Publisher running in a forked child process, as TimeSlave does in production.
/// A second publisher cannot live in the reader's process (and a child forked
/// after the reader opened the segment inherits its registry), because the
/// shared-memory registry rejects two resources with the same path. Therefore
/// construct instances BEFORE the reader opens anything, then call Start().
/// Stop() mimics `kill -9` (no cleanup) and unlinks the segment afterwards.
class ForkedPublisher
{
  public:
    ForkedPublisher(const std::string& name, const GptpIpcData& data) : name_{name}
    {
        int go[2];
        int ready[2];
        if ((::pipe(go) != 0) || (::pipe(ready) != 0))
            return;
        pid_ = ::fork();
        if (pid_ == 0)
        {
            char c = 0;
            if (::read(go[0], &c, 1) != 1)
                ::_exit(1);
            GptpIpcPublisher pub;
            const char ok = pub.Open(name) ? 1 : 0;
            if (ok != 0)
                pub.Publish(data);
            static_cast<void>(::write(ready[1], &ok, 1));
            for (;;)
                ::pause();
        }
        ::close(go[0]);
        ::close(ready[1]);
        go_fd_ = go[1];
        ready_fd_ = ready[0];
    }

    ~ForkedPublisher()
    {
        Stop();
        if (go_fd_ >= 0)
            ::close(go_fd_);
        if (ready_fd_ >= 0)
            ::close(ready_fd_);
    }

    ForkedPublisher(const ForkedPublisher&) = delete;
    ForkedPublisher& operator=(const ForkedPublisher&) = delete;

    /// Lets the child create the segment and publish once; true when it is ready.
    bool Start()
    {
        char c = 1;
        char ok = 0;
        if ((pid_ <= 0) || (::write(go_fd_, &c, 1) != 1))
            return false;
        struct pollfd pfd{ready_fd_, POLLIN, 0};
        // Bounded wait: a child stuck after fork must fail the test, not hang it.
        return (::poll(&pfd, 1, 5000) == 1) && (::read(ready_fd_, &ok, 1) == 1) && (ok == 1);
    }

    void Stop()
    {
        if (pid_ > 0)
        {
            ::kill(pid_, SIGKILL);
            ::waitpid(pid_, nullptr, 0);
            ::shm_unlink(name_.c_str());
            pid_ = -1;
        }
    }

  private:
    std::string name_;
    pid_t pid_{-1};
    int go_fd_{-1};
    int ready_fd_{-1};
};

class GptpIpcReceiverTest : public ::testing::Test
{
  protected:
    void TearDown() override
    {
        rx_.Close();
    }

    GptpIpcReceiver rx_;
};

TEST_F(GptpIpcReceiverTest, Open_ShmNotExist_ReturnsFalse)
{
    EXPECT_FALSE(rx_.Open("/gptp_nonexistent_" + std::to_string(::getpid())));
}

TEST_F(GptpIpcReceiverTest, Close_WithoutInit_DoesNotCrash)
{
    EXPECT_NO_THROW(rx_.Close());
}

TEST_F(GptpIpcReceiverTest, Close_CalledTwice_DoesNotCrash)
{
    EXPECT_NO_THROW(rx_.Close());
    EXPECT_NO_THROW(rx_.Close());
}

TEST_F(GptpIpcReceiverTest, Receive_WithoutInit_ReturnsNullopt)
{
    EXPECT_FALSE(rx_.Receive().has_value());
}

TEST_F(GptpIpcReceiverTest, Open_CalledTwice_ReturnsTrueOnSecondCall)
{
    // region_ != nullptr after first Open → second call returns true immediately.
    GptpIpcPublisher pub;
    const std::string name = UniqueShmName();
    ASSERT_TRUE(pub.Open(name));
    ASSERT_TRUE(rx_.Open(name));
    EXPECT_TRUE(rx_.Open(name));
    pub.Close();
}

TEST_F(GptpIpcReceiverTest, Open_TooSmallShm_ReturnsFalse)
{
    // Create a shm segment smaller than GptpIpcRegion so the fstat size check fails.
    const std::string name = UniqueShmName();
    const int fd = ::shm_open(name.c_str(), O_CREAT | O_RDWR, 0600);
    ASSERT_GE(fd, 0);
    ASSERT_EQ(::ftruncate(fd, 1), 0);
    ::close(fd);

    EXPECT_FALSE(rx_.Open(name));

    ::shm_unlink(name.c_str());
}

TEST_F(GptpIpcReceiverTest, LastReceiveStale_TracksPublisherProgress)
{
    GptpIpcPublisher pub;
    const std::string name = UniqueShmName();
    ASSERT_TRUE(pub.Open(name));
    ASSERT_TRUE(rx_.Open(name));
    EXPECT_FALSE(rx_.LastReceiveStale());

    pub.Publish(GptpIpcData{});
    ASSERT_TRUE(rx_.Receive().has_value());
    EXPECT_FALSE(rx_.LastReceiveStale());  // first read

    ASSERT_TRUE(rx_.Receive().has_value());
    EXPECT_TRUE(rx_.LastReceiveStale());  // nothing new published

    pub.Publish(GptpIpcData{});
    ASSERT_TRUE(rx_.Receive().has_value());
    EXPECT_FALSE(rx_.LastReceiveStale());

    rx_.Close();
    EXPECT_FALSE(rx_.LastReceiveStale());
}

TEST_F(GptpIpcReceiverTest, IsReplaced_NotOpened_ReturnsFalse)
{
    EXPECT_FALSE(rx_.IsReplaced());
}

TEST_F(GptpIpcReceiverTest, IsReplaced_SegmentUnchanged_ReturnsFalse)
{
    GptpIpcPublisher pub;
    const std::string name = UniqueShmName();
    ASSERT_TRUE(pub.Open(name));
    ASSERT_TRUE(rx_.Open(name));
    EXPECT_FALSE(rx_.IsReplaced());
}

TEST_F(GptpIpcReceiverTest, IsReplaced_PublisherClosed_ReturnsFalse)
{
    GptpIpcPublisher pub;
    const std::string name = UniqueShmName();
    ASSERT_TRUE(pub.Open(name));
    ASSERT_TRUE(rx_.Open(name));
    pub.Close();  // name removed, nothing to switch to
    EXPECT_FALSE(rx_.IsReplaced());
}

TEST_F(GptpIpcReceiverTest, IsReplaced_PublisherRestarted_ReturnsTrue)
{
    const std::string name = UniqueShmName();
    ForkedPublisher first{name, GptpIpcData{}};
    ForkedPublisher restarted{name, GptpIpcData{}};

    ASSERT_TRUE(first.Start());
    ASSERT_TRUE(rx_.Open(name));
    EXPECT_FALSE(rx_.IsReplaced());

    first.Stop();  // publisher killed; receiver keeps its mapping
    EXPECT_FALSE(rx_.IsReplaced());

    ASSERT_TRUE(restarted.Start());
    EXPECT_TRUE(rx_.IsReplaced());

    rx_.Close();
    ASSERT_TRUE(rx_.Open(name));
    EXPECT_FALSE(rx_.IsReplaced());
}

}  // namespace details
}  // namespace ts
}  // namespace score
