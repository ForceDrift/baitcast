#include "baitcast/detail/scope_state.hpp"

#include <gtest/gtest.h>

TEST(ScopeState, StartsOpenWithZeroChildren) {
  baitcast::detail::scope_state scope;
  EXPECT_FALSE(scope.is_closing());
  EXPECT_EQ(scope.active_children(), 0u);
  EXPECT_FALSE(scope.is_complete());
}

TEST(ScopeState, EmptyClosedScopeCompletesImmediately) {
  baitcast::detail::scope_state scope;
  scope.close();
  EXPECT_TRUE(scope.is_closing());
  EXPECT_TRUE(scope.can_complete());
  EXPECT_TRUE(scope.is_complete());
}

TEST(ScopeState, HoldsCompletionWhileChildrenActive) {
  baitcast::detail::scope_state scope;
  scope.register_child();
  scope.register_child();
  scope.register_child();
  scope.close();
  EXPECT_FALSE(scope.can_complete());
  EXPECT_FALSE(scope.is_complete());
  EXPECT_FALSE(scope.child_completed());
  EXPECT_FALSE(scope.child_completed());
  EXPECT_TRUE(scope.child_completed());
  EXPECT_TRUE(scope.can_complete());
  EXPECT_TRUE(scope.is_complete());
}

TEST(ScopeState, OpenResetsClosingAndCompletion) {
  baitcast::detail::scope_state scope;
  scope.close();
  EXPECT_TRUE(scope.is_complete());
  scope.open();
  EXPECT_FALSE(scope.is_closing());
  EXPECT_FALSE(scope.is_complete());
}

TEST(ScopeState, TracksParentRelationship) {
  baitcast::detail::scope_state parent;
  baitcast::detail::scope_state child;
  EXPECT_EQ(child.parent(), nullptr);
  child.set_parent(&parent);
  EXPECT_EQ(child.parent(), &parent);
  const baitcast::detail::scope_state& cref = child;
  EXPECT_EQ(cref.parent(), &parent);
}