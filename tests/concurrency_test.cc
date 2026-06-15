// MIT License
//
// Copyright (c) 2026 Ioannis Kaliakatsos
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "database.h"

#include <gtest/gtest.h>

#include <atomic>
#include <cstdint>
#include <set>
#include <thread>
#include <vector>

#include "person.h"

using namespace sqlite_reflection;

// These tests exercise the shared singleton connection from many threads at once.
// Before the connection access was serialized (issue #9), concurrent writers raced
// on a single sqlite3* and each operation's BEGIN/COMMIT interleaved, producing
// "cannot start a transaction within a transaction" failures and intermittent
// crashes. They deterministically pass only once access is serialized.
class ConcurrencyTest : public ::testing::Test {
    void SetUp() override {
        Database::Initialize("");
    }

    void TearDown() override {
        Database::Finalize();
    }
};

TEST_F(ConcurrencyTest, ConcurrentAutoIncrementInsertsAllSucceedWithUniqueIds) {
    const auto db = Database::Instance();

    constexpr int kThreads = 8;
    constexpr int kInsertsPerThread = 100;
    constexpr int kExpected = kThreads * kInsertsPerThread;

    std::atomic<int> failures{0};
    std::vector<std::thread> workers;
    workers.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        workers.emplace_back([&] {
            for (int i = 0; i < kInsertsPerThread; ++i) {
                try {
                    Person p{L"john", L"doe", 30};
                    db->SaveAutoIncrement(p);
                } catch (...) {
                    failures.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }
    for (auto& w : workers) {
        w.join();
    }

    // No operation may fail (e.g. with a nested-transaction error).
    EXPECT_EQ(0, failures.load());

    // Every insert must be persisted exactly once.
    const auto all = db->FetchAll<Person>();
    EXPECT_EQ(kExpected, static_cast<int>(all.size()));

    // Every row must have received a distinct, database-assigned id.
    std::set<int64_t> ids;
    for (const auto& person : all) {
        ids.insert(person.id);
    }
    EXPECT_EQ(kExpected, static_cast<int>(ids.size()));
}

TEST_F(ConcurrencyTest, ConcurrentReadsAndWritesDoNotThrow) {
    const auto db = Database::Instance();

    constexpr int kWriters = 4;
    constexpr int kReaders = 4;
    constexpr int kInsertsPerWriter = 100;
    constexpr int kReadsPerReader = 100;
    constexpr int kExpected = kWriters * kInsertsPerWriter;

    std::atomic<int> failures{0};
    std::vector<std::thread> workers;
    workers.reserve(kWriters + kReaders);

    for (int t = 0; t < kWriters; ++t) {
        workers.emplace_back([&] {
            for (int i = 0; i < kInsertsPerWriter; ++i) {
                try {
                    Person p{L"jane", L"roe", 41};
                    db->SaveAutoIncrement(p);
                } catch (...) {
                    failures.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }

    for (int t = 0; t < kReaders; ++t) {
        workers.emplace_back([&] {
            for (int i = 0; i < kReadsPerReader; ++i) {
                try {
                    // Reading concurrently with writers must not crash or throw.
                    volatile auto count = db->FetchAll<Person>().size();
                    (void)count;
                } catch (...) {
                    failures.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }

    for (auto& w : workers) {
        w.join();
    }

    EXPECT_EQ(0, failures.load());

    const auto all = db->FetchAll<Person>();
    EXPECT_EQ(kExpected, static_cast<int>(all.size()));
}

TEST_F(ConcurrencyTest, ConnectionOutlivesFinalizeWhileAHandleIsHeld) {
    // A caller holding an Instance() handle must be able to keep using the database even if
    // another thread calls Finalize() in the meantime: the connection is reference-counted
    // and only closed once the last handle is released. With the previous raw-pointer
    // lifecycle, Finalize() deleted the object and closed the connection immediately, so the
    // operations below would have been a use-after-free.
    auto db = Database::Instance();

    Person first{L"ada", L"lovelace", 36};
    db->SaveAutoIncrement(first);

    // Drop the singleton's own reference while we still hold one.
    Database::Finalize();

    // The held handle must still be valid: the connection is alive and usable. If Finalize()
    // had closed/freed it, these calls would crash or throw.
    Person second{L"grace", L"hopper", 85};
    db->SaveAutoIncrement(second);
    const auto all = db->FetchAll<Person>();
    EXPECT_EQ(2, static_cast<int>(all.size()));
}

TEST_F(ConcurrencyTest, ReinitializeWhileAHandleIsHeldIsRejected) {
    // Hold a handle to the current database, then finalize. The database stays alive through
    // the handle, so re-initializing now would create a second live database/connection that
    // does not share the first's lock. That must be rejected until the handle is released.
    auto db = Database::Instance();
    Database::Finalize();

    EXPECT_THROW(Database::Initialize(""), std::runtime_error);

    // Once the last handle is gone, the previous database is destroyed and re-initialization
    // is allowed again.
    db.reset();
    Database::Initialize("");
    const auto fresh = Database::Instance();
    EXPECT_EQ(0, static_cast<int>(fresh->FetchAll<Person>().size()));
}
