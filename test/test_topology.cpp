#include "baitcast/detail/topology.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <optional>

namespace {
  using baitcast::detail::cpu_count;
  using baitcast::detail::cpu_for;
  using baitcast::detail::cpu_set;
  using baitcast::detail::online_cpus;
  using baitcast::detail::pin_current_thread;
  using baitcast::detail::topology;
} // namespace

TEST(Topology, DetectsAtLeastOneCpu) {
  EXPECT_GE(cpu_count(), 1u);
}

TEST(Topology, OnlineCpusAreNonEmptyAndInRange) {
  cpu_set cpus = online_cpus();
  EXPECT_FALSE(cpus.empty());
  for (std::size_t cpu : cpus) {
    EXPECT_LT(cpu, cpu_count());
  }
}

TEST(Topology, CpuSetDeduplicates) {
  cpu_set set{0, 1, 2, 1, 0};
  EXPECT_EQ(set.size(), 3u);
  EXPECT_TRUE(set.add(3));
  EXPECT_EQ(set.size(), 4u);
  EXPECT_TRUE(set.contains(3));
  EXPECT_FALSE(set.contains(7));
}

TEST(Topology, CpuSetRemoveAndBoundedCount) {
  cpu_set set;
  EXPECT_TRUE(set.add(0));
  EXPECT_TRUE(set.add(1));
  EXPECT_TRUE(set.remove(0));
  EXPECT_EQ(set.size(), 1u);
  EXPECT_FALSE(set.contains(0));

  cpu_set dense(8);
  ASSERT_EQ(dense.size(), 8u);
  for (std::size_t i = 0; i < 8; ++i) {
    EXPECT_TRUE(dense.contains(i));
  }
}

TEST(Topology, CpuForCyclesThroughTheSet) {
  cpu_set set{2, 4, 6};
  ASSERT_EQ(cpu_for(0, set), std::optional<std::size_t>{2});
  ASSERT_EQ(cpu_for(1, set), std::optional<std::size_t>{4});
  ASSERT_EQ(cpu_for(2, set), std::optional<std::size_t>{6});
  ASSERT_EQ(cpu_for(3, set), std::optional<std::size_t>{2});
  ASSERT_EQ(cpu_for(6, set), std::optional<std::size_t>{6});
}

TEST(Topology, EmptySetYieldsNoCpuForPinning) {
  cpu_set set;
  EXPECT_FALSE(cpu_for(0, set).has_value());
  EXPECT_TRUE(pin_current_thread(set));
}

TEST(Topology, PinCurrentThreadToFirstOnlineCpu) {
  cpu_set cpus = online_cpus();
  ASSERT_FALSE(cpus.empty());
  cpu_set single;
  ASSERT_TRUE(single.add(cpus[0]));
// Hard affinity is enforced on Linux; other platforms are best effort.
#if defined(__linux__)
  EXPECT_TRUE(pin_current_thread(single));
#endif
}

TEST(Topology, ConfigurableAffinityPinsCurrentThread) {
  topology topo{cpu_set{0}};
  topo.set_allowed_cpus(online_cpus());
  ASSERT_FALSE(topo.allowed_cpus().empty());
// Hard affinity is enforced on Linux; other platforms are best effort.
#if defined(__linux__)
  EXPECT_TRUE(topo.pin_current_thread());
#endif
}

TEST(Topology, WorkerIndexMapsThroughTopology) {
  topology topo{cpu_set{2, 4, 6}};
  ASSERT_EQ(topo.cpu_for(0), std::optional<std::size_t>{2});
  ASSERT_EQ(topo.cpu_for(1), std::optional<std::size_t>{4});
  ASSERT_EQ(topo.cpu_for(3), std::optional<std::size_t>{2});
}