#include <gtest/gtest.h>

#include <rtype/server/AccountStore.hpp>
#include <rtype/server/PasswordHasher.hpp>

using rtype::server::AccountStore;
using rtype::server::hashPassword;
using rtype::server::randomSalt;
using rtype::server::verifyPassword;

TEST(PasswordHasher, VerifiesOnlyTheRightPassword) {
  const auto salt = randomSalt();
  const auto hash = hashPassword("hunter2", salt);

  EXPECT_TRUE(verifyPassword("hunter2", salt, hash));
  EXPECT_FALSE(verifyPassword("hunter3", salt, hash));
  EXPECT_FALSE(verifyPassword("", salt, hash));
}

TEST(PasswordHasher, SaltsMakeDifferentHashes) {
  const auto first = randomSalt();
  const auto second = randomSalt();

  ASSERT_NE(first, second);
  EXPECT_NE(hashPassword("hunter2", first), hashPassword("hunter2", second));
}

TEST(AccountStore, RegisterThenLogin) {
  AccountStore store(":memory:");

  const auto created = store.create("alice", "hunter2");

  ASSERT_TRUE(created.has_value());
  EXPECT_EQ(store.authenticate("alice", "hunter2"), created);
}

TEST(AccountStore, UsernamesIgnoreCase) {
  AccountStore store(":memory:");
  const auto created = store.create("alice", "hunter2");

  EXPECT_EQ(store.authenticate("ALICE", "hunter2"), created);
  EXPECT_FALSE(store.create("Alice", "other").has_value());
}

TEST(AccountStore, RejectsAWrongPassword) {
  AccountStore store(":memory:");
  ASSERT_TRUE(store.create("alice", "hunter2").has_value());

  EXPECT_FALSE(store.authenticate("alice", "hunter3").has_value());
}

TEST(AccountStore, RejectsAnUnknownUser) {
  AccountStore store(":memory:");

  EXPECT_FALSE(store.authenticate("bob", "hunter2").has_value());
}

TEST(AccountStore, GivesEachAccountItsOwnId) {
  AccountStore store(":memory:");

  const auto alice = store.create("alice", "a");
  const auto bob = store.create("bob", "b");

  ASSERT_TRUE(alice.has_value());
  ASSERT_TRUE(bob.has_value());
  EXPECT_NE(*alice, *bob);
  EXPECT_EQ(store.authenticate("bob", "b"), bob);
}

TEST(AccountStore, ThrowsOnADatabaseItCannotOpen) {
  EXPECT_THROW(AccountStore("/no/such/folder/rtype.db"), std::runtime_error);
}
