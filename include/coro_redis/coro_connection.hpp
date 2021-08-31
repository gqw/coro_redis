//
// Copyright (c) 2020 Gu.Qiwei(gqwmail@qq.com)
//
// Distributed under the MIT Software License
// See accompanying file LICENSE.txt or copy at
// https://opensource.org/licenses/MIT
//
#pragma once

#include <fmt/format.h>

#include <coro_redis/impl/coro_connection.ipp>

namespace coro_redis {

class sync_connection;
///
/// @brief connection of corotine
///
class coro_connection {
  public:
    using cptr = std::shared_ptr<coro_connection>;

    ///
    /// @brief construction
    ///
    /// @param actx Redis asynchronous context
    ///
    coro_connection(redisAsyncContext* actx) : impl_(actx) {}


    /// @brief Send redis command.
    /// @param cmd Redis command.
    /// @return Redis return.
    template <typename CORO_RET = std::string>
    awaiter_t<CORO_RET> command(std::string_view cmd) const {
        return impl_.command<CORO_RET>(cmd);
    }

    /// @brief Send redis command.
    /// @param cmd Redis command.
    /// @param reply_op Callback for deal redis reply.
    /// @return Redis return.
    template <typename CORO_RET>
    awaiter_t<CORO_RET> command(
        std::string_view cmd,
        std::function<std::optional<CORO_RET>(redisReply*)>&& reply_op) const {
        return impl_.command<CORO_RET>(cmd, reply_op);
    }

    /// @brief Send password to Redis.
    /// @param password Password.
    /// @note Normally, you should not call this method.
    ///       Instead, you should set password with `ConnectionOptions` or URI.
    /// @see https://redis.io/commands/auth
    inline awaiter_t<std::string> auth(std::string_view password) {
        return impl_.command<std::string>(fmt::format("auth {}", password));
    }

    /// @brief Send user and password to Redis.
    /// @param user User name.
    /// @param password Password.
    /// @note Normally, you should not call this method.
    ///       Instead, you should set password with `ConnectionOptions` or URI.
    ///       Also this overload only works with Redis 6.0 or later.
    /// @see https://redis.io/commands/auth
    inline awaiter_t<std::string> auth(std::string_view user,
                                       std::string_view password) {
        return impl_.command<std::string>(
                   fmt::format("auth {} {}", user, password));
    }

    /// @brief Ask Redis to return the given message.
    /// @param msg Message to be sent.
    /// @return Return the given message.
    /// @see https://redis.io/commands/echo
    inline awaiter_t<std::string> echo(std::string_view msg) const {
        return impl_.command<std::string>(fmt::format("echo {}", msg));
    }

    /// @brief Test if the connection is alive.
    /// @return Always return *PONG*.
    /// @see https://redis.io/commands/ping
    inline awaiter_t<std::string> ping() {
        return impl_.command<std::string>("ping");
    }

    /// @brief Test if the connection is alive.
    /// @param msg Message sent to Redis.
    /// @return Return the given message.
    /// @see https://redis.io/commands/ping
    inline awaiter_t<std::string> ping(std::string_view msg) {
        return impl_.command<std::string>(fmt::format("ping {}", msg));
    }

    /// @brief After sending QUIT, only the current connection will be close,
    /// while other connections in the pool is still open.
    /// @see https://redis.io/commands/quit
    inline awaiter_t<std::string> quit() {
        return impl_.command<std::string>(fmt::format("quit"));
    }

    /// @brief Select the Redis logical database
    /// @see https://redis.io/commands/select
    inline awaiter_t<std::string> select(uint64_t idx) {
        return impl_.command<std::string>(fmt::format("select {}", idx));
    }

    /// @brief Swap two Redis databases.
    /// @param idx1 The index of the first database.
    /// @param idx2 The index of the second database.
    /// @see https://redis.io/commands/swapdb
    inline awaiter_t<std::string> swapdb(uint64_t idx1, uint64_t idx2) {
        return impl_.command<std::string>(fmt::format("swapdb {} {}", idx1, idx2));
    }

    // SERVER commands.

    /// @brief Rewrite AOF in the background.
    /// @see https://redis.io/commands/bgrewriteaof
    inline awaiter_t<std::string> bgrewriteaof() {
        return impl_.command<std::string>("bgrewriteaof");
    }

    /// @brief Save database in the background.
    /// @see https://redis.io/commands/bgsave
    inline awaiter_t<std::string> bgsave() {
        return impl_.command<std::string>("bgsave");
    }

    /// @brief Get the size of the currently selected database.
    /// @return Number of keys in currently selected database.
    /// @see https://redis.io/commands/dbsize
    inline awaiter_t<uint64_t> dbsize() {
        return impl_.command<uint64_t>("dbsize");
    }

    /// @brief Remove keys of all databases.
    /// @param async Whether flushing databases asynchronously, i.e. without
    /// blocking the server.
    /// @see https://redis.io/commands/flushall
    inline awaiter_t<std::string> flushall(bool async = false) {
        return impl_.command<std::string>(async ? "FLUSHALL ASYNC"
                                          : "FLUSHALL SYNC");
    }

    /// @brief Remove keys of current databases.
    /// @param async Whether flushing databases asynchronously, i.e. without
    /// blocking the server.
    /// @see https://redis.io/commands/flushdb
    inline awaiter_t<std::string> flushdb(bool async = false) {
        return impl_.command<std::string>(async ? "FLUSHDB ASYNC" : "FLUSHDB SYNC");
    }

    /// @brief Get the info about the server.
    /// @return Server info.
    /// @see https://redis.io/commands/info
    inline awaiter_t<std::string> info() {
        return impl_.command<std::string>("info");
    }

    /// @brief Get the info about the server on the given section.
    /// @param section Section.
    /// @return Server info.
    /// @see https://redis.io/commands/info
    inline awaiter_t<std::string> info(std::string_view section) {
        return impl_.command<std::string>(fmt::format("info {}", section));
    }

    /// @brief Get the UNIX timestamp in seconds, at which the database was saved
    /// successfully.
    /// @return The last saving time.
    /// @see https://redis.io/commands/lastsave
    inline awaiter_t<uint64_t> lastsave() {
        return impl_.command<uint64_t>("lastsave");
    }

    /// @brief Save databases into RDB file **synchronously**, i.e. block the
    /// server during saving.
    /// @see https://redis.io/commands/save
    inline awaiter_t<std::string> save() {
        return impl_.command<std::string>("save");
    }

    // KEY commands.

    /// @brief Delete the given key.
    /// @param key Key.
    /// @return Number of keys removed.
    /// @retval 1 If the key exists, and has been removed.
    /// @retval 0 If the key does not exist.
    /// @see https://redis.io/commands/del
    template <typename... Args>
    inline awaiter_t<uint64_t> del(Args&&... keys) {
        std::string cmd("del");
        (cmd.append(" ").append(keys), ...);
        return impl_.command<uint64_t>(std::move(cmd));
    }

    /// @brief Get the serialized valued stored at key.
    /// @param key Key.
    /// @return The serialized value.
    /// @note If key does not exist, `dump` returns `OptionalString{}`
    /// (`std::nullopt`).
    /// @see https://redis.io/commands/dump
    inline awaiter_t<std::string> dump(std::string_view key) {
        return impl_.command<std::string>(fmt::format("dump {}", key));
    }

    /// @brief Check if the given key exists.
    /// @param key Key.
    /// @return Whether the given key exists.
    /// @retval 1 If key exists.
    /// @retval 0 If key does not exist.
    /// @see https://redis.io/commands/exists
    template <typename... Args>
    inline awaiter_t<uint64_t> exists(Args&&... keys) {
        std::string cmd("exists");
        (cmd.append(" ").append(keys), ...);
        return impl_.command<uint64_t>(std::move(cmd));
    }

    /// @brief Set a timeout on key.
    /// @param key Key.
    /// @param timeout Timeout in seconds.
    /// @return Whether timeout has been set.
    /// @retval 1 If timeout has been set.
    /// @retval 0 If key does not exist.
    /// @see https://redis.io/commands/expire
    inline awaiter_t<uint64_t> expire(std::string_view key, uint64_t timeout) {
        return impl_.command<uint64_t>(fmt::format("expire {} {}", key, timeout));
    }

    /// @brief Set a timeout on key, i.e. expire the key at a future time point.
    /// @param key Key.
    /// @param timestamp Time in seconds since UNIX epoch.
    /// @return Whether timeout has been set.
    /// @retval true If timeout has been set.
    /// @retval false If key does not exist.
    /// @see https://redis.io/commands/expireat
    inline awaiter_t<uint64_t> expireat(std::string_view key,
                                        uint64_t timestamp) {
        return impl_.command<uint64_t>(
                   fmt::format("expireat {} {}", key, timestamp));
    }

    /// @brief Get keys matching the given pattern.
    /// @param pattern Pattern.
    /// @param output Output iterator to the destination where the returned keys
    /// are stored.
    /// @note It's always a bad idea to call `keys`, since it might block Redis
    /// for a long time,
    ///       especially when the data set is very big.
    /// @see `Redis::scan`
    /// @see https://redis.io/commands/keys
    // inline awaiter_t<std::vector<std::string>> keys(std::string_view pattern) {
    //	return impl_.command<std::vector<std::string>>(fmt::format("keys {}",
    //pattern));
    //}

    /// @brief Move a key to the given database.
    /// @param key Key.
    /// @param db The destination database.
    /// @return Whether key has been moved.
    /// @retval true If key has been moved.
    /// @retval false If key was not moved.
    /// @see https://redis.io/commands/move
    inline awaiter_t<uint64_t> move(std::string_view key, uint64_t db) {
        return impl_.command<uint64_t>(fmt::format("move {} {}", key, db));
    }

    /// @brief Remove timeout on key.
    /// @param key Key.
    /// @return Whether timeout has been removed.
    /// @retval true If timeout has been removed.
    /// @retval false If key does not exist, or does not have an associated
    /// timeout.
    /// @see https://redis.io/commands/persist
    inline awaiter_t<uint64_t> persist(std::string_view key) {
        return impl_.command<uint64_t>(fmt::format("persist {}", key));
    }

    /// @brief Set a timeout on key.
    /// @param key Key.
    /// @param timeout Timeout in milliseconds.
    /// @return Whether timeout has been set.
    /// @retval true If timeout has been set.
    /// @retval false If key does not exist.
    /// @see https://redis.io/commands/pexpire
    inline awaiter_t<uint64_t> pexpire(std::string_view key, uint64_t timeout) {
        return impl_.command<uint64_t>(fmt::format("pexpire {} {}", key, timeout));
    }

    /// @brief Set a timeout on key, i.e. expire the key at a future time point.
    /// @param key Key.
    /// @param timestamp Time in milliseconds since UNIX epoch.
    /// @return Whether timeout has been set.
    /// @retval true If timeout has been set.
    /// @retval false If key does not exist.
    /// @see https://redis.io/commands/pexpireat
    inline awaiter_t<uint64_t> pexpireat(std::string_view key,
                                         uint64_t timestamp) {
        return impl_.command<uint64_t>(
                   fmt::format("pexpireat {} {}", key, timestamp));
    }

    /// @brief Get the TTL of a key in milliseconds.
    /// @param key Key.
    /// @return TTL of the key in milliseconds.
    /// @see https://redis.io/commands/pttl
    inline awaiter_t<uint64_t> pttl(std::string_view key) {
        return impl_.command<uint64_t>(fmt::format("pttl {}", key));
    }

    /// @brief Get a random key from current database.
    /// @return A random key.
    /// @note If the database is empty, `randomkey` returns `OptionalString{}`
    /// (`std::nullopt`).
    /// @see https://redis.io/commands/randomkey
    inline awaiter_t<std::string> randomkey() {
        return impl_.command<std::string>("randomkey");
    }

    /// @brief Rename `key` to `newkey`.
    /// @param key Key to be renamed.
    /// @param newkey The new name of the key.
    /// @see https://redis.io/commands/rename
    inline awaiter_t<std::string> rename(std::string_view key,
                                         std::string_view newkey) {
        return impl_.command<std::string>(fmt::format("rename {} {}", key, newkey));
    }

    /// @brief Rename `key` to `newkey` if `newkey` does not exist.
    /// @param key Key to be renamed.
    /// @param newkey The new name of the key.
    /// @return Whether key has been renamed.
    /// @retval true If key has been renamed.
    /// @retval false If newkey already exists.
    /// @see https://redis.io/commands/renamenx
    inline awaiter_t<uint64_t> renamenx(std::string_view key,
                                        std::string_view newkey) {
        return impl_.command<uint64_t>(fmt::format("renamenx {} {}", key, newkey));
    }

    /// @brief Create a key with the value obtained by `Redis::dump`.
    /// @param key Key.
    /// @param val Value obtained by `Redis::dump`.
    /// @param ttl Timeout of the created key in milliseconds. If `ttl` is 0, set
    /// no timeout.
    /// @param replace Whether to overwrite an existing key.
    ///                If `replace` is `true` and key already exists, throw an
    ///                exception.
    /// @see https://redis.io/commands/restore
    inline awaiter_t<std::string> restore(std::string_view key,
                                          std::string_view val, uint64_t ttl,
                                          bool replace = false) {
        return impl_.command<std::string>(
                   fmt::format("restore {} {} {} {}", key, ttl, val, replace));
    }

    // TODO: sort

    /// @brief Scan keys of the database matching the given pattern.
    ///
    /// Example:
    /// @code{.cpp}
    /// @endcode
    /// @param cursor Cursor.
    /// @param pattern Pattern of the keys to be scanned.
    /// @param count A hint for how many keys to be scanned.
    /// @param output Output iterator to the destination where the returned keys
    /// are stored.
    /// @return The cursor to be used for the next scan operation.
    /// @see https://redis.io/commands/scan
    /// TODO: support the TYPE option for Redis 6.0.

    inline awaiter_t<scan_ret_t> scan(uint64_t cursor, uint64_t count = 0) {
        return scan(cursor, "", count);
    }

    inline awaiter_t<scan_ret_t> scan(uint64_t cursor, std::string_view pattern,
                                      uint64_t count = 0) {
        return impl_.scan(cursor, pattern, count);
    }

    /// @brief Update the last access time of the given key.
    /// @param key Key.
    /// @return Whether last access time of the key has been updated.
    /// @retval 1 If key exists, and last access time has been updated.
    /// @retval 0 If key does not exist.
    /// @see https://redis.io/commands/touch
    template <typename... Args>
    inline awaiter_t<uint64_t> touch(Args&&... keys) {
        std::string cmd("touch");
        (cmd.append(" ").append(keys), ...);
        return impl_.command<uint64_t>(std::move(cmd));
    }

    /// @brief Get the remaining Time-To-Live of a key.
    /// @param key Key.
    /// @return TTL in seconds.
    /// @retval TTL If the key has a timeout.
    /// @retval -1 If the key exists but does not have a timeout.
    /// @retval -2 If the key does not exist.
    /// @note In Redis 2.6 or older, `ttl` returns -1 if the key does not exist,
    ///       or if the key exists but does not have a timeout.
    /// @see https://redis.io/commands/ttl
    inline awaiter_t<uint64_t> ttl(std::string_view key) {
        return impl_.command<uint64_t>(fmt::format("ttl {}", key));
    }

    /// @brief Get the type of the value stored at key.
    /// @param key Key.
    /// @return The type of the value.
    /// @see https://redis.io/commands/type
    inline awaiter_t<std::string> type(std::string_view key) {
        return impl_.command<std::string>(fmt::format("type {}", key));
    }

    /// @brief Remove the given key asynchronously, i.e. without blocking Redis.
    /// @param key Key.
    /// @return Whether the key has been removed.
    /// @retval 1 If key exists, and has been removed.
    /// @retval 0 If key does not exist.
    /// @see https://redis.io/commands/unlink
    template <typename... Args>
    inline awaiter_t<uint64_t> unlink(Args&&... keys) {
        std::string cmd("unlink");
        (cmd.append(" ").append(keys), ...);
        return impl_.command<uint64_t>(std::move(cmd));
    }

    /// @brief Wait until previous write commands are successfully replicated to
    /// at
    ///        least the specified number of replicas or the given timeout has
    ///        been reached.
    /// @param numslaves Number of replicas.
    /// @param timeout Timeout in milliseconds. If timeout is 0ms, wait forever.
    /// @return Number of replicas that have been successfully replicated these
    /// write commands.
    /// @note The return value might be less than `numslaves`, because timeout has
    /// been reached.
    /// @see https://redis.io/commands/wait
    inline awaiter_t<uint64_t> wait(uint64_t numslaves, uint64_t timeout) {
        return impl_.command<uint64_t>(
                   fmt::format("wait {} {}", numslaves, timeout));
    }

    // STRING commands.

    /// @brief Append the given string to the string stored at key.
    /// @param key Key.
    /// @param str String to be appended.
    /// @return The length of the string after the append operation.
    /// @see https://redis.io/commands/append
    inline awaiter_t<uint64_t> append(std::string_view key,
                                      std::string_view str) {
        return impl_.command<uint64_t>(fmt::format("append {} {}", key, str));
    }

    /// @brief Get the number of bits that have been set for the given range of
    /// the string.
    /// @param key Key.
    /// @param start Start index (inclusive) of the range. 0 means the beginning
    /// of the string.
    /// @param end End index (inclusive) of the range. -1 means the end of the
    /// string.
    /// @return Number of bits that have been set.
    /// @note The index can be negative to index from the end of the string.
    /// @see https://redis.io/commands/bitcount
    inline awaiter_t<uint64_t> bitcount(std::string_view key, uint64_t start = 0,
                                        uint64_t end = -1) {
        return impl_.command<uint64_t>(
                   fmt::format("bitcount {} {} {}", key, start, end));
    }

    enum BitOp { AND, OR, XOR, NOT };
    /// @brief Do bit operation on the string stored at `key`, and save the result
    /// to `destination`.
    /// @param op Bit operations.
    /// @param destination The destination key where the result is saved.
    /// @param key The key where the string to be operated is stored.
    /// @return The length of the string saved at `destination`.
    /// @see https://redis.io/commands/bitop
    /// @see `BitOp`
    template <typename... Args>
    inline awaiter_t<uint64_t> bitop(BitOp op, Args&&... keys) {
        std::string cmd("bitop");
        switch (op) {
        case BitOp::AND:
            cmd.append(" AND ");
            break;
        case BitOp::OR:
            cmd.append(" OR ");
            break;
        case BitOp::XOR:
            cmd.append(" XOR ");
            break;
        case BitOp::NOT:
            cmd.append(" NOT ");
            break;
        }
        (cmd.append(" ").append(keys), ...);
        return impl_.command<uint64_t>(std::move(cmd));
    }

    /// @brief Get the position of the first bit set to 0 or 1 in the given range
    /// of the string.
    /// @param key Key.
    /// @param bit 0 or 1.
    /// @param start Start index (inclusive) of the range. 0 means the beginning
    /// of the string.
    /// @param end End index (inclusive) of the range. -1 means the end of the
    /// string.
    /// @return The position of the first bit set to 0 or 1.
    /// @see https://redis.io/commands/bitpos
    inline awaiter_t<uint64_t> bitpos(std::string_view key, uint64_t bit,
                                      uint64_t start = 0, uint64_t end = -1) {
        return impl_.command<uint64_t>(
                   fmt::format("bitpos {} {} {} {}", key, bit, start, end));
    }

    /// @brief Decrement the integer stored at key by 1.
    /// @param key Key.
    /// @return The value after the decrement.
    /// @see https://redis.io/commands/decr
    inline awaiter_t<uint64_t> decr(std::string_view key) {
        return impl_.command<uint64_t>(fmt::format("decr {}", key));
    }

    /// @brief Decrement the integer stored at key by `decrement`.
    /// @param key Key.
    /// @param decrement Decrement.
    /// @return The value after the decrement.
    /// @see https://redis.io/commands/decrby
    inline awaiter_t<uint64_t> decrby(std::string_view key, uint64_t decrement) {
        return impl_.command<uint64_t>(fmt::format("decrby {} {}", key, decrement));
    }

    /// @brief Get the string value stored at key.
    ///
    /// Example:
    /// @code{.cpp}
    /// auto val = redis.get("key");
    /// if (val)
    ///     std::cout << *val << std::endl;
    /// else
    ///     std::cout << "key not exist" << std::endl;
    /// @endcode
    /// @param key Key.
    /// @return The value stored at key.
    /// @note If key does not exist, `get` returns `OptionalString{}`
    /// (`std::nullopt`).
    /// @see https://redis.io/commands/get
    inline awaiter_t<std::string> get(std::string_view key) const {
        return impl_.command<std::string>(fmt::format("get {}", key));
    }

    /// @brief Get the bit value at offset in the string.
    /// @param key Key.
    /// @param offset Offset.
    /// @return The bit value.
    /// @see https://redis.io/commands/getbit
    inline awaiter_t<uint64_t> getbit(std::string_view key, uint64_t offset) {
        return impl_.command<uint64_t>(fmt::format("getbit {} {}", key, offset));
    }

    /// @brief Get the substring of the string stored at key.
    /// @param key Key.
    /// @param start Start index (inclusive) of the range. 0 means the beginning
    /// of the string.
    /// @param end End index (inclusive) of the range. -1 means the end of the
    /// string.
    /// @return The substring in range [start, end]. If key does not exist, return
    /// an empty string.
    /// @see https://redis.io/commands/getrange
    inline awaiter_t<std::string> getrange(std::string_view key, uint64_t start,
                                           uint64_t end) {
        return impl_.command<std::string>(
                   fmt::format("getrange {} {} {}", key, start, end));
    }

    /// @brief Atomically set the string stored at `key` to `val`, and return the
    /// old value.
    /// @param key Key.
    /// @param val Value to be set.
    /// @return The old value stored at key.
    /// @note If key does not exist, `getset` returns `OptionalString{}`
    /// (`std::nullopt`).
    /// @see https://redis.io/commands/getset
    /// @see `OptionalString`
    inline awaiter_t<std::string> getset(std::string_view key,
                                         std::string_view val) {
        return impl_.command<std::string>(fmt::format("getset {} {}", key, val));
    }

    /// @brief Increment the integer stored at key by 1.
    /// @param key Key.
    /// @return The value after the increment.
    /// @see https://redis.io/commands/incr
    inline awaiter_t<uint64_t> incr(std::string_view key) {
        return impl_.command<uint64_t>(fmt::format("incr {}", key));
    }

    /// @brief Increment the integer stored at key by `increment`.
    /// @param key Key.
    /// @param increment Increment.
    /// @return The value after the increment.
    /// @see https://redis.io/commands/incrby
    inline awaiter_t<uint64_t> incrby(std::string_view key, uint64_t increment) {
        return impl_.command<uint64_t>(fmt::format("incrby {} {}", key, increment));
    }

    /// @brief Increment the floating point number stored at key by `increment`.
    /// @param key Key.
    /// @param increment Increment.
    /// @return The value after the increment.
    /// @see https://redis.io/commands/incrbyfloat
    inline awaiter_t<std::string> incrbyfloat(std::string_view key,
            double increment) {
        return impl_.command<std::string>(
                   fmt::format("incrby {} {}", key, increment));
    }

    /// @brief Get the values of multiple keys atomically.
    ///
    /// Example:
    /// @code{.cpp}
    /// std::vector<std::string> keys = {"k1", "k2", "k3"};
    /// std::vector<OptionalString> vals;
    /// redis.mget(keys.begin(), keys.end(), std::back_inserter(vals));
    /// for (const auto &val : vals) {
    ///     if (val)
    ///         std::cout << *val << std::endl;
    ///     else
    ///         std::cout << "key does not exist" << std::endl;
    /// }
    /// @endcode
    /// @param first Iterator to the first key of the given range.
    /// @param last Off-the-end iterator to the given range.
    /// @param output Output iterator to the destination where the values are
    /// stored.
    /// @note The destination should be a container of `OptionalString` type,
    ///       since the given key might not exist (in this case, the value of the
    ///       corresponding key is `OptionalString{}` (`std::nullopt`)).
    /// @see https://redis.io/commands/mget
    template <typename... Args>
    inline awaiter_t<std::vector<std::string>> mget(Args&&... keys) {
        std::string cmd("mget");
        (cmd.append(" ").append(keys), ...);
        return impl_.command<std::vector<std::string>>(std::move(cmd));
    }

    /// @brief Set multiple key-value pairs.
    ///
    /// Example:
    /// @code{.cpp}
    /// redis.mset({std::make_pair("k1", "v1"), std::make_pair("k2", "v2")});
    /// @endcode
    /// @param il Initializer list of key-value pairs.
    /// @see https://redis.io/commands/mset
    template <typename... Args>
    // requires requires (Args&&...keys) {	((std::string_view(keys)), ...);
    // }
    inline awaiter_t<std::string> mset(Args&&... keys) {
        std::string cmd("mset");
        (cmd.append(" ").append(keys), ...);
        return impl_.command<std::string>(std::move(cmd));
    }

    /// @brief Set the given key-value pairs if all specified keys do not exist.
    ///
    /// Example:
    /// @code{.cpp}
    /// std::vector<std::pair<std::string, std::string>> kvs1;
    /// redis.msetnx(kvs1.begin(), kvs1.end());
    /// std::unordered_map<std::string, std::string> kvs2;
    /// redis.msetnx(kvs2.begin(), kvs2.end());
    /// @endcode
    /// @param first Iterator to the first key-value pair.
    /// @param last Off-the-end iterator of the given range.
    /// @return Whether all keys have been set.
    /// @retval true If all keys have been set.
    /// @retval false If no key was set, i.e. at least one key already exist.
    /// @see https://redis.io/commands/msetnx
    template <typename... Args>
    // requires requires (Args&&...keys) {	((std::string_view(keys)), ...);
    // }
    inline awaiter_t<std::string> msetnx(Args&&... keys) {
        std::string cmd("msetnx");
        (cmd.append(" ").append(keys), ...);
        return impl_.command<std::string>(std::move(cmd));
    }

    /// @brief Set key-value pair with the given timeout in milliseconds.
    /// @param key Key.
    /// @param ttl Time-To-Live in milliseconds.
    /// @param val Value.
    /// @see https://redis.io/commands/psetex
    inline awaiter_t<std::string> psetex(std::string_view key, uint64_t ttl,
                                         std::string_view val) {
        return impl_.command<std::string>(
                   fmt::format("psetex {} {} {}", key, ttl, val));
    }

    /// @brief Set a key-value pair.
    ///
    /// Example:
    /// @code{.cpp}
    /// // Set a key-value pair.
    /// redis.set("key", "value");
    /// // Set a key-value pair, and expire it after 10 seconds.
    /// redis.set("key", "value", std::chrono::seconds(10));
    /// // Set a key-value pair with a timeout, only if the key already exists.
    /// if (redis.set("key", "value", std::chrono::seconds(10),
    /// UpdateType::EXIST))
    ///     std::cout << "OK" << std::endl;
    /// else
    ///     std::cout << "key does not exist" << std::endl;
    /// @endcode
    /// @param key Key.
    /// @param val Value.
    /// @param ttl Timeout on the key. If `ttl` is 0ms, do not set timeout.
    /// @param type Options for set command:
    ///             - UpdateType::EXIST: Set the key only if it already exists.
    ///             - UpdateType::NOT_EXIST: Set the key only if it does not
    ///             exist.
    ///             - UpdateType::ALWAYS: Always set the key no matter whether it
    ///             exists.
    /// @return Whether the key has been set.
    /// @retval true If the key has been set.
    /// @retval false If the key was not set, because of the given option.
    /// @see https://redis.io/commands/set
    // TODO: Support KEEPTTL option for Redis 6.0
    template <RedisSetValueType T>
    inline awaiter_t<std::string> set(std::string_view key, T val,
                                      uint64_t ttl = 0,
                                      RedisTTLType ttl_type = RedisTTLType::EX,
                                      UpdateType type = UpdateType::ALWAYS) {
        return impl_.set(key, val, ttl, ttl_type, type);
    }

    // TODO: add SETBIT command.

    /// @brief Set key-value pair with the given timeout in seconds.
    /// @param key Key.
    /// @param ttl Time-To-Live in seconds.
    /// @param val Value.
    /// @see https://redis.io/commands/setex
    inline awaiter_t<std::string> setex(std::string_view key, uint64_t ttl,
                                        std::string_view val) {
        return impl_.command<std::string>(
                   fmt::format("setex {} {} {}", key, ttl, val));
    }

    /// @brief Set the key if it does not exist.
    /// @param key Key.
    /// @param val Value.
    /// @return Whether the key has been set.
    /// @retval true If the key has been set.
    /// @retval false If the key was not set, i.e. the key already exists.
    /// @see https://redis.io/commands/setnx
    inline awaiter_t<uint64_t> setnx(std::string_view key, std::string_view val) {
        return impl_.command<uint64_t>(fmt::format("setnx {} {}", key, val));
    }

    /// @brief Set the substring starting from `offset` to the given value.
    /// @param key Key.
    /// @param offset Offset.
    /// @param val Value.
    /// @return The length of the string after this operation.
    /// @see https://redis.io/commands/setrange
    inline awaiter_t<uint64_t> setrange(std::string_view key, uint64_t offset,
                                        std::string_view val) {
        return impl_.command<uint64_t>(
                   fmt::format("setrange {} {} {}", key, offset, val));
    }

    /// @brief Get the length of the string stored at key.
    /// @param key Key.
    /// @return The length of the string.
    /// @note If key does not exist, `strlen` returns 0.
    /// @see https://redis.io/commands/strlen
    inline awaiter_t<uint64_t> strlen(std::string_view key) {
        return impl_.command<uint64_t>(fmt::format("strlen {}", key));
    }
    /*
            // LIST commands.

            /// @brief Pop the first element of the list in a blocking way.
            /// @param key Key where the list is stored.
            /// @param timeout Timeout in seconds. 0 means block forever.
            /// @return Key-element pair.
            /// @note If list is empty and timeout reaches, return
       `OptionalStringPair{}` (`std::nullopt`).
            /// @see `Redis::lpop`
            /// @see https://redis.io/commands/blpop
            inline awaiter_t<std::vector<std::string>> blpop(std::string_view key,
       uint64_t timeout = 0) { return
       impl_.command<std::vector<std::string>>(fmt::format("blpop {} {}", key,
       timeout));
            }

            /// @brief Pop the first element of the list in a blocking way.
            /// @param key Key where the list is stored.
            /// @param timeout Timeout in seconds. 0 means block forever.
            /// @return Key-element pair.
            /// @note If list is empty and timeout reaches, return
       `OptionalStringPair{}` (`std::nullopt`).
            /// @see `Redis::lpop`
            /// @see https://redis.io/commands/blpop
            OptionalStringPair blpop(std::string_view key,
                    const std::chrono::seconds& timeout = std::chrono::seconds{ 0
       });

            /// @brief Pop the first element of multiple lists in a blocking way.
            /// @param first Iterator to the first key.
            /// @param last Off-the-end iterator to the key range.
            /// @param timeout Timeout in seconds. 0 means block forever.
            /// @return Key-element pair.
            /// @note If all lists are empty and timeout reaches, return
       `OptionalStringPair{}` (`std::nullopt`).
            /// @see `Redis::lpop`
            /// @see https://redis.io/commands/blpop
            template <typename Input>
            OptionalStringPair blpop(Input first, Input last, uint64_t timeout);

            /// @brief Pop the first element of multiple lists in a blocking way.
            /// @param il Initializer list of keys.
            /// @param timeout Timeout in seconds. 0 means block forever.
            /// @return Key-element pair.
            /// @note If all lists are empty and timeout reaches, return
       `OptionalStringPair{}` (`std::nullopt`).
            /// @see `Redis::lpop`
            /// @see https://redis.io/commands/blpop
            template <typename T>
            OptionalStringPair blpop(std::initializer_list<T> il, uint64_t
       timeout) { return blpop(il.begin(), il.end(), timeout);
            }

            /// @brief Pop the first element of multiple lists in a blocking way.
            /// @param first Iterator to the first key.
            /// @param last Off-the-end iterator to the key range.
            /// @param timeout Timeout in seconds. 0 means block forever.
            /// @return Key-element pair.
            /// @note If all lists are empty and timeout reaches, return
       `OptionalStringPair{}` (`std::nullopt`).
            /// @see `Redis::lpop`
            /// @see https://redis.io/commands/blpop
            template <typename Input>
            OptionalStringPair blpop(Input first,
                    Input last,
                    const std::chrono::seconds& timeout = std::chrono::seconds{ 0
       });

            /// @brief Pop the first element of multiple lists in a blocking way.
            /// @param il Initializer list of keys.
            /// @param timeout Timeout in seconds. 0 means block forever.
            /// @return Key-element pair.
            /// @note If all lists are empty and timeout reaches, return
       `OptionalStringPair{}` (`std::nullopt`).
            /// @see `Redis::lpop`
            /// @see https://redis.io/commands/blpop
            template <typename T>
            OptionalStringPair blpop(std::initializer_list<T> il,
                    const std::chrono::seconds& timeout = std::chrono::seconds{ 0
       }) { return blpop(il.begin(), il.end(), timeout);
            }

            /// @brief Pop the last element of the list in a blocking way.
            /// @param key Key where the list is stored.
            /// @param timeout Timeout in seconds. 0 means block forever.
            /// @return Key-element pair.
            /// @note If list is empty and timeout reaches, return
       `OptionalStringPair{}` (`std::nullopt`).
            /// @see `Redis::rpop`
            /// @see https://redis.io/commands/brpop
            OptionalStringPair brpop(std::string_view key, uint64_t timeout);

            /// @brief Pop the last element of the list in a blocking way.
            /// @param key Key where the list is stored.
            /// @param timeout Timeout in seconds. 0 means block forever.
            /// @return Key-element pair.
            /// @note If list is empty and timeout reaches, return
       `OptionalStringPair{}` (`std::nullopt`).
            /// @see `Redis::rpop`
            /// @see https://redis.io/commands/brpop
            OptionalStringPair brpop(std::string_view key,
                    const std::chrono::seconds& timeout = std::chrono::seconds{ 0
       });

            /// @brief Pop the last element of multiple lists in a blocking way.
            /// @param first Iterator to the first key.
            /// @param last Off-the-end iterator to the key range.
            /// @param timeout Timeout in seconds. 0 means block forever.
            /// @return Key-element pair.
            /// @note If all lists are empty and timeout reaches, return
       `OptionalStringPair{}` (`std::nullopt`).
            /// @see `Redis::rpop`
            /// @see https://redis.io/commands/brpop
            template <typename Input>
            OptionalStringPair brpop(Input first, Input last, uint64_t timeout);

            /// @brief Pop the last element of multiple lists in a blocking way.
            /// @param il Initializer list of lists.
            /// @param timeout Timeout in seconds. 0 means block forever.
            /// @return Key-element pair.
            /// @note If all lists are empty and timeout reaches, return
       `OptionalStringPair{}` (`std::nullopt`).
            /// @see `Redis::rpop`
            /// @see https://redis.io/commands/brpop
            template <typename T>
            OptionalStringPair brpop(std::initializer_list<T> il, uint64_t
       timeout) { return brpop(il.begin(), il.end(), timeout);
            }

            /// @brief Pop the last element of multiple lists in a blocking way.
            /// @param first Iterator to the first list.
            /// @param last Off-the-end iterator to the list range.
            /// @param timeout Timeout in seconds. 0 means block forever.
            /// @return Key-element pair.
            /// @note If all lists are empty and timeout reaches, return
       `OptionalStringPair{}` (`std::nullopt`).
            /// @see `Redis::rpop`
            /// @see https://redis.io/commands/brpop
            template <typename Input>
            OptionalStringPair brpop(Input first,
                    Input last,
                    const std::chrono::seconds& timeout = std::chrono::seconds{ 0
       });

            /// @brief Pop the last element of multiple lists in a blocking way.
            /// @param il Initializer list of list keys.
            /// @param timeout Timeout in seconds. 0 means block forever.
            /// @return Key-element pair.
            /// @note If all lists are empty and timeout reaches, return
       `OptionalStringPair{}` (`std::nullopt`).
            /// @see `Redis::rpop`
            /// @see https://redis.io/commands/brpop
            template <typename T>
            OptionalStringPair brpop(std::initializer_list<T> il,
                    const std::chrono::seconds& timeout = std::chrono::seconds{ 0
       }) { return brpop(il.begin(), il.end(), timeout);
            }

            /// @brief Pop last element of one list and push it to the left of
       another list in blocking way.
            /// @param source Key of the source list.
            /// @param destination Key of the destination list.
            /// @param timeout Timeout. 0 means block forever.
            /// @return The popped element.
            /// @note If the source list does not exist, `brpoplpush` returns
       `OptionalString{}` (`std::nullopt`).
            /// @see `Redis::rpoplpush`
            /// @see https://redis.io/commands/brpoplpush
            inline awaiter_t<std::string> brpoplpush(std::string_view source,
                    std::string_view destination,
                    uint64_t timeout);

            /// @brief Pop last element of one list and push it to the left of
       another list in blocking way.
            /// @param source Key of the source list.
            /// @param destination Key of the destination list.
            /// @param timeout Timeout. 0 means block forever.
            /// @return The popped element.
            /// @note If the source list does not exist, `brpoplpush` returns
       `OptionalString{}` (`std::nullopt`).
            /// @see `Redis::rpoplpush`
            /// @see https://redis.io/commands/brpoplpush
            inline awaiter_t<std::string> brpoplpush(std::string_view source,
                    std::string_view destination,
                    const std::chrono::seconds& timeout = std::chrono::seconds{ 0
       });

            /// @brief Get the element at the given index of the list.
            /// @param key Key where the list is stored.
            /// @param index Zero-base index, and -1 means the last element.
            /// @return The element at the given index.
            /// @see https://redis.io/commands/lindex
            inline awaiter_t<std::string> lindex(std::string_view key, uint64_t
       index);

            /// @brief Insert an element to a list before or after the pivot
       element.
            ///
            /// Example:
            /// @code{.cpp}
            /// // Insert 'hello' before 'world'
            /// auto len = redis.linsert("list", InsertPosition::BEFORE, "world",
       "hello");
            /// if (len == -1)
            ///     std::cout << "there's no 'world' in the list" << std::endl;
            /// else
            ///     std::cout << "after the operation, the length of the list is "
       << len << std::endl;
            /// @endcode
            /// @param key Key where the list is stored.
            /// @param position Before or after the pivot element.
            /// @param pivot The pivot value. The `pivot` is the value of the
       element, not the index.
            /// @param val Element to be inserted.
            /// @return The length of the list after the operation.
            /// @note If the pivot value is not found, `linsert` returns -1.
            /// @see `InsertPosition`
            /// @see https://redis.io/commands/linsert
            inline awaiter_t<uint64_t> linsert(std::string_view key,
                    InsertPosition position,
                    std::string_view pivot,
                    std::string_view val);
    */

    /// @brief Get the length of the list.
    /// @param key Key where the list is stored.
    /// @return The length of the list.
    /// @see https://redis.io/commands/llen
    inline awaiter_t<uint64_t> llen(std::string_view key) {
        return impl_.command<uint64_t>(fmt::format("llen {}", key));
    }

    /// @brief Pop the first element of the list.
    ///
    /// Example:
    /// @code{.cpp}
    /// auto element = redis.lpop("list");
    /// if (element)
    ///     std::cout << *element << std::endl;
    /// else
    ///     std::cout << "list is empty, i.e. list does not exist" << std::endl;
    /// @endcode
    /// @param key Key where the list is stored.
    /// @return The popped element.
    /// @note If list is empty, i.e. key does not exist, return `OptionalString{}`
    /// (`std::nullopt`).
    /// @see https://redis.io/commands/lpop
    awaiter_t<std::string> lpop(std::string_view key) {
        return impl_.command<std::string>(fmt::format("lpop {}", key));
    }

    /// @brief Push an element to the beginning of the list.
    /// @param key Key where the list is stored.
    /// @param val Element to be pushed.
    /// @return The length of the list after the operation.
    /// @see https://redis.io/commands/lpush
    inline awaiter_t<uint64_t> lpush(std::string_view key, std::string_view val) {
        return impl_.command<uint64_t>(fmt::format("lpush {} {}", key, val));
    }

    /// @brief Push multiple elements to the beginning of the list.
    ///
    /// Example:
    /// @code{.cpp}
    /// std::vector<std::string> elements = {"e1", "e2", "e3"};
    /// redis.lpush("list", elements.begin(), elements.end());
    /// @endcode
    /// @param key Key where the list is stored.
    /// @param first Iterator to the first element to be pushed.
    /// @param last Off-the-end iterator to the given element range.
    /// @return The length of the list after the operation.
    /// @see https://redis.io/commands/lpush
    template <typename... Args>
    inline awaiter_t<uint64_t> lpush(std::string_view key, Args&&... keys) {
        std::string cmd("lpush ");
        cmd.append(key);
        (cmd.append(" ").append(keys), ...);
        return impl_.command<uint64_t>(std::move(cmd));
    }

    /// @brief Push an element to the beginning of the list, only if the list
    /// already exists.
    /// @param key Key where the list is stored.
    /// @param val Element to be pushed.
    /// @return The length of the list after the operation.
    /// @see https://redis.io/commands/lpushx
    // TODO: add a multiple elements overload.
    inline awaiter_t<uint64_t> lpushx(std::string_view key,
                                      std::string_view val) {
        return impl_.command<uint64_t>(fmt::format("lpushx {} {}", key, val));
    }

    /// @brief Get elements in the given range of the given list.
    ///
    /// Example:
    /// @code{.cpp}
    /// std::vector<std::string> elements;
    /// // Save all elements of a Redis list to a vector of string.
    /// redis.lrange("list", 0, -1, std::back_inserter(elements));
    /// @endcode
    /// @param key Key where the list is stored.
    /// @param start Start index of the range. Index can be negative, which mean
    /// index from the end.
    /// @param stop End index of the range.
    /// @param output Output iterator to the destination where the results are
    /// saved.
    /// @see https://redis.io/commands/lrange
    inline awaiter_t<std::vector<std::string>> lrange(std::string_view key,
                                            uint64_t start,
    uint64_t stop) {
        return impl_.command<std::vector<std::string>>(
                   fmt::format("lrange {} {} {}", key, start, stop));
    }
    /// @brief Remove the first `count` occurrences of elements equal to `val`.
    /// @param key Key where the list is stored.
    /// @param count Number of occurrences to be removed.
    /// @param val Value.
    /// @return Number of elements removed.
    /// @note `count` can be positive, negative and 0. Check the reference for
    /// detail.
    /// @see https://redis.io/commands/lrem
    inline awaiter_t<uint64_t> lrem(std::string_view key, uint64_t count,
                                    std::string_view val) {
        return impl_.command<uint64_t>(
                   fmt::format("lrem {} {} {}", key, count, val));
    }

    /// @brief Set the element at the given index to the specified value.
    /// @param key Key where the list is stored.
    /// @param index Index of the element to be set.
    /// @param val Value.
    /// @see https://redis.io/commands/lset
    inline awaiter_t<std::string> lset(std::string_view key, uint64_t index,
                                       std::string_view val) {
        return impl_.command<std::string>(
                   fmt::format("lset {} {} {}", key, index, val));
    }

    /// @brief Trim a list to keep only element in the given range.
    /// @param key Key where the key is stored.
    /// @param start Start of the index.
    /// @param stop End of the index.
    /// @see https://redis.io/commands/ltrim
    inline awaiter_t<std::string> ltrim(std::string_view key, uint64_t start,
                                        uint64_t stop) {
        return impl_.command<std::string>(
                   fmt::format("ltrim {} {} {}", key, start, stop));
    }

    /// @brief Pop the last element of a list.
    /// @param key Key where the list is stored.
    /// @return The popped element.
    /// @note If the list is empty, i.e. key does not exist, `rpop` returns
    /// `OptionalString{}` (`std::nullopt`).
    /// @see https://redis.io/commands/rpop
    inline awaiter_t<std::string> rpop(std::string_view key) {
        return impl_.command<std::string>(fmt::format("rpop {}", key));
    }

    /// @brief Pop last element of one list and push it to the left of another
    /// list.
    /// @param source Key of the source list.
    /// @param destination Key of the destination list.
    /// @return The popped element.
    /// @note If the source list does not exist, `rpoplpush` returns
    /// `OptionalString{}` (`std::nullopt`).
    /// @see https://redis.io/commands/brpoplpush
    inline awaiter_t<std::string> rpoplpush(std::string_view source,
                                            std::string_view destination) {
        return impl_.command<std::string>(
                   fmt::format("rpoplpush {} {}", source, destination));
    }

    /// @brief Push an element to the end of the list.
    /// @param key Key where the list is stored.
    /// @param val Element to be pushed.
    /// @return The length of the list after the operation.
    /// @see https://redis.io/commands/rpush
    template <typename... Args>
    inline awaiter_t<uint64_t> rpush(std::string_view key, std::string_view val,
                                     Args&&... vals) {
        std::string cmd("rpush ");
        cmd.append(val);
        (cmd.append(" ").append(vals), ...);
        return impl_.command<uint64_t>(std::move(cmd));
    }

    /// @brief Push an element to the end of the list, only if the list already
    /// exists.
    /// @param key Key where the list is stored.
    /// @param val Element to be pushed.
    /// @return The length of the list after the operation.
    /// @see https://redis.io/commands/rpushx
    inline awaiter_t<uint64_t> rpushx(std::string_view key,
                                      std::string_view val) {
        return impl_.command<uint64_t>(fmt::format("rpushx {} {}", key, val));
    }

    // HASH commands.

    /// @brief Remove the given field from hash.
    /// @param key Key where the hash is stored.
    /// @param field Field to be removed.
    /// @return Whether the field has been removed.
    /// @retval 1 If the field exists, and has been removed.
    /// @retval 0 If the field does not exist.
    /// @see https://redis.io/commands/hdel
    template <typename... Args>
    inline awaiter_t<uint64_t> hdel(std::string_view key, std::string_view field,
                                    Args&&... fields) {
        std::string cmd("hdel");
        cmd.append(" ").append(field);
        (cmd.append(" ").append(fields), ...);
        return impl_.command<uint64_t>(std::move(cmd));
    }

    /// @brief Check if the given field exists in hash.
    /// @param key Key where the hash is stored.
    /// @param field Field.
    /// @return Whether the field exists.
    /// @retval true If the field exists in hash.
    /// @retval false If the field does not exist.
    /// @see https://redis.io/commands/hexists
    inline awaiter_t<uint64_t> hexists(std::string_view key,
                                       std::string_view field) {
        return impl_.command<uint64_t>(fmt::format("hexists {} {}", key, field));
    }

    /// @brief Get the value of the given field.
    /// @param key Key where the hash is stored.
    /// @param field Field.
    /// @return Value of the given field.
    /// @note If field does not exist, `hget` returns `OptionalString{}`
    /// (`std::nullopt`).
    /// @see https://redis.io/commands/hget
    inline awaiter_t<std::string> hget(std::string_view key,
                                       std::string_view field) {
        return impl_.command<std::string>(fmt::format("hget {} {}", key, field));
    }

    /// @brief Get all field-value pairs of the given hash.
    ///
    /// Example:
    /// @code{.cpp}
    /// std::unordered_map<std::string, std::string> results;
    /// // Save all field-value pairs of a Redis hash to an unordered_map<string,
    /// string>. redis.hgetall("hash", std::inserter(results, results.begin()));
    /// @endcode
    /// @param key Key where the hash is stored.
    /// @param output Output iterator to the destination where the result is
    /// saved.
    /// @note It's always a bad idea to call `hgetall` on a large hash, since it
    /// will block Redis.
    /// @see `Redis::hscan`
    /// @see https://redis.io/commands/hgetall
    inline awaiter_t<std::vector<std::string>> hgetall(std::string_view key) {
        return impl_.command<std::vector<std::string>>(
                   fmt::format("hgetall {}", key));
    }

    /// @brief Increment the integer stored at the given field.
    /// @param key Key where the hash is stored.
    /// @param field Field.
    /// @param increment Increment.
    /// @return The value of the field after the increment.
    /// @see https://redis.io/commands/hincrby
    inline awaiter_t<uint64_t> hincrby(std::string_view key,
                                       std::string_view field,
                                       uint64_t increment) {
        return impl_.command<uint64_t>(
                   fmt::format("hincrby {} {} {}", key, field, increment));
    }

    /// @brief Increment the floating point number stored at the given field.
    /// @param key Key where the hash is stored.
    /// @param field Field.
    /// @param increment Increment.
    /// @return The value of the field after the increment.
    /// @see https://redis.io/commands/hincrbyfloat
    inline awaiter_t<double> hincrbyfloat(std::string_view key,
                                          std::string_view field,
                                          double increment) {
        return impl_.command<double>(
                   fmt::format("hincrbyfloat {} {} {}", key, field, increment));
    }

    /// @brief Get all fields of the given hash.
    /// @param key Key where the hash is stored.
    /// @param output Output iterator to the destination where the result is
    /// saved.
    /// @note It's always a bad idea to call `hkeys` on a large hash, since it
    /// will block Redis.
    /// @see `Redis::hscan`
    /// @see https://redis.io/commands/hkeys
    inline awaiter_t<std::vector<std::string>> hkeys(std::string_view key) {
        return impl_.command<std::vector<std::string>>(
                   fmt::format("hkeys {}", key));
    }

    /// @brief Get the number of fields of the given hash.
    /// @param key Key where the hash is stored.
    /// @return Number of fields.
    /// @see https://redis.io/commands/hlen
    inline awaiter_t<uint64_t> hlen(std::string_view key) {
        return impl_.command<uint64_t>(fmt::format("hlen {}", key));
    }

    /// @brief Get values of multiple fields.
    ///
    /// Example:
    /// @code{.cpp}
    /// std::vector<std::string> fields = {"f1", "f2"};
    /// std::vector<OptionalString> vals;
    /// redis.hmget("hash", fields.begin(), fields.end(),
    /// std::back_inserter(vals)); for (const auto &val : vals) {
    ///     if (val)
    ///         std::cout << *val << std::endl;
    ///     else
    ///         std::cout << "field not exist" << std::endl;
    /// }
    /// @endcode
    /// @param key Key where the hash is stored.
    /// @param first Iterator to the first field.
    /// @param last Off-the-end iterator to the given field range.
    /// @param output Output iterator to the destination where the result is
    /// saved.
    /// @note The destination should be a container of `OptionalString` type,
    ///       since the given field might not exist (in this case, the value of
    ///       the corresponding field is `OptionalString{}` (`std::nullopt`)).
    /// @see https://redis.io/commands/hmget
    template <typename... Args>
    inline awaiter_t<std::vector<std::string>> hmget(std::string_view key,
                                            std::string_view field,
    Args&&... fields) {
        std::string cmd("hmget ");
        cmd.append(" ").append(key);
        cmd.append(" ").append(field);
        (cmd.append(" ").append(fields), ...);
        return impl_.command<std::vector<std::string>>(std::move(cmd));
    }

    /// @brief Set multiple field-value pairs of the given hash.
    ///
    /// Example:
    /// @code{.cpp}
    /// std::unordered_map<std::string, std::string> m = {{"f1", "v1"}, {"f2",
    /// "v2"}}; redis.hmset("hash", m.begin(), m.end());
    /// @endcode
    /// @param key Key where the hash is stored.
    /// @param first Iterator to the first field-value pair.
    /// @param last Off-the-end iterator to the range.
    /// @see https://redis.io/commands/hmset
    template <typename... Args>
    inline awaiter_t<std::string> hmset(std::string_view key,
                                        std::string_view field,
                                        std::string_view value, Args&&... args) {
        std::string cmd("hmset ");
        cmd.append(" ").append(key);
        cmd.append(" ").append(field);
        cmd.append(" ").append(value);
        (cmd.append(" ").append(args), ...);
        return impl_.command<std::string>(std::move(cmd));
    }

    /// @brief Scan fields of the given hash matching the given pattern.
    ///
    /// Example:
    /// @code{.cpp}
    /// auto cursor = 0LL;
    /// std::unordered_map<std::string, std::string> kvs;
    /// while (true) {
    ///     cursor = redis.hscan("hash", cursor, "pattern:*", 10,
    ///     std::inserter(kvs, kvs.begin())); if (cursor == 0) {
    ///         break;
    ///     }
    /// }
    /// @endcode
    /// @param key Key where the hash is stored.
    /// @param cursor Cursor.
    /// @param pattern Pattern of fields to be scanned.
    /// @param count A hint for how many fields to be scanned.
    /// @param output Output iterator to the destination where the result is
    /// saved.
    /// @return The cursor to be used for the next scan operation.
    /// @see https://redis.io/commands/hscan
    inline awaiter_t<scan_ret_t> hscan(std::string_view key, uint64_t cursor,
                                       uint64_t count) {
        return hscan(key, cursor, "", count);
    }

    /// @brief Scan fields of the given hash matching the given pattern.
    /// @param key Key where the hash is stored.
    /// @param cursor Cursor.
    /// @param pattern Pattern of fields to be scanned.
    /// @param output Output iterator to the destination where the result is
    /// saved.
    /// @return The cursor to be used for the next scan operation.
    /// @see https://redis.io/commands/hscan
    inline awaiter_t<scan_ret_t> hscan(std::string_view key, uint64_t cursor,
                                       std::string_view pattern,
                                       uint64_t count = 0) {
        return impl_.hscan(key, cursor, pattern, count);
    }

    /// @brief Set hash field to value.
    /// @param key Key where the hash is stored.
    /// @param field Field.
    /// @param val Value.
    /// @return Whether the given field is a new field.
    /// @retval true If the given field didn't exist, and a new field has been
    /// added.
    /// @retval false If the given field already exists, and its value has been
    /// overwritten.
    /// @note When `hset` returns false, it does not mean that the method failed
    /// to set the field.
    ///       Instead, it means that the field already exists, and we've
    ///       overwritten its value. If `hset` fails, it will throw an exception
    ///       of `Exception` type.
    /// @see https://github.com/sewenew/redis-plus-plus/issues/9
    /// @see https://redis.io/commands/hset
    template <typename... Args>
    inline awaiter_t<uint64_t> hset(std::string_view key, std::string_view field,
                                    std::string_view val, Args&&... args) {
        std::string cmd("hset ");
        cmd.append(" ").append(field);
        cmd.append(" ").append(val);
        (cmd.append(" ").append(args), ...);
        return impl_.command<uint64_t>(std::move(cmd));
    }

    /// @brief Set multiple fields of the given hash.
    ///
    /// Example:
    /// @code{.cpp}
    /// std::unordered_map<std::string, std::string> m = {{"f1", "v1"}, {"f2",
    /// "v2"}}; redis.hset("hash", m.begin(), m.end());
    /// @endcode
    /// @param key Key where the hash is stored.
    /// @param first Iterator to the first field to be set.
    /// @param last Off-the-end iterator to the given range.
    /// @return Number of fields that have been added, i.e. fields that not
    /// existed before.
    /// @see https://redis.io/commands/hset
    inline awaiter_t<uint64_t> hset(
        std::string_view key,
        const std::vector<std::pair<std::string_view, std::string_view>>& kvs);

    /// @brief Set hash field to value, only if the given field does not exist.
    /// @param key Key where the hash is stored.
    /// @param field Field.
    /// @param val Value.
    /// @return Whether the field has been set.
    /// @retval true If the field has been set.
    /// @retval false If failed to set the field, i.e. the field already exists.
    /// @see https://redis.io/commands/hsetnx
    inline awaiter_t<uint64_t> hsetnx(std::string_view key,
                                      std::string_view field,
                                      std::string_view val) {
        return impl_.command<uint64_t>(
                   fmt::format("hsetnx {} {} {}", key, field, val));
    }

    /// @brief Get the length of the string stored at the given field.
    /// @param key Key where the hash is stored.
    /// @param field Field.
    /// @return Length of the string.
    /// @see https://redis.io/commands/hstrlen
    inline awaiter_t<uint64_t> hstrlen(std::string_view key,
                                       std::string_view field) {
        return impl_.command<uint64_t>(fmt::format("hstrlen {} {}", key, field));
    }

    /// @brief Get values of all fields stored at the given hash.
    /// @param key Key where the hash is stored.
    /// @param output Output iterator to the destination where the result is
    /// saved.
    /// @note It's always a bad idea to call `hvals` on a large hash, since it
    /// might block Redis.
    /// @see `Redis::hscan`
    /// @see https://redis.io/commands/hvals
    inline awaiter_t<std::vector<std::string>> hvals(std::string_view key) {
        return impl_.command<std::vector<std::string>>(
                   fmt::format("hvals {}", key));
    }

    // SET commands.

    /// @brief Add a member to the given set.
    /// @param key Key where the set is stored.
    /// @param member Member to be added.
    /// @return Whether the given member is a new member.
    /// @retval 1 The member did not exist before, and it has been added now.
    /// @retval 0 The member already exists before this operation.
    /// @see https://redis.io/commands/sadd
    template <typename... Args>
    inline awaiter_t<uint64_t> sadd(std::string_view key, std::string_view member,
                                    Args&&... members) {
        std::string cmd("sadd");
        cmd.append(" ").append(key);
        cmd.append(" ").append(member);
        (cmd.append(" ").append(members), ...);
        return impl_.command<uint64_t>(std::move(cmd));
    }

    /// @brief Get the number of members in the set.
    /// @param key Key where the set is stored.
    /// @return Number of members.
    /// @see https://redis.io/commands/scard
    inline awaiter_t<uint64_t> scard(std::string_view key) {
        return impl_.command<uint64_t>(fmt::format("scard {}", key));
    }

    /// @brief Get the difference between the first set and all successive sets.
    /// @param first Iterator to the first set.
    /// @param last Off-the-end iterator to the range.
    /// @param output Output iterator to the destination where the result is
    /// saved.
    /// @see https://redis.io/commands/sdiff
    // TODO: `void sdiff(const StringView &key, Input first, Input last, Output
    // output)` is better.
    template <typename... Args>
    inline awaiter_t<std::vector<std::string>> sdiff(std::string_view key,
    Args&&... keys) {
        std::string cmd("sdiff");
        cmd.append(" ").append(key);
        (cmd.append(" ").append(keys), ...);
        return impl_.command<std::vector<std::string>>(std::move(cmd));
    }

    /// @brief Copy set stored at `key` to `destination`.
    /// @param destination Key of the destination set.
    /// @param key Key of the source set.
    /// @return Number of members of the set.
    /// @see https://redis.io/commands/sdiffstore
    template <typename... Args>
    inline awaiter_t<uint64_t> sdiffstore(std::string_view destination,
                                          std::string_view key, Args&&... keys) {
        std::string cmd("sdiffstore");
        cmd.append(" ").append(destination);
        cmd.append(" ").append(key);
        (cmd.append(" ").append(keys), ...);
        return impl_.command<uint64_t>(std::move(cmd));
    }

    /// @brief Get the intersection between the first set and all successive sets.
    /// @param first Iterator to the first set.
    /// @param last Off-the-end iterator to the range.
    /// @param output Output iterator to the destination where the result is
    /// saved.
    /// @see https://redis.io/commands/sinter
    // TODO: `void sinter(const StringView &key, Input first, Input last, Output
    // output)` is better.
    template <typename... Args>
    inline awaiter_t<std::vector<std::string>> sinter(std::string_view key,
    Args&&... keys) {
        std::string cmd("sinter");
        cmd.append(" ").append(key);
        (cmd.append(" ").append(keys), ...);
        return impl_.command<std::vector<std::string>>(std::move(cmd));
    }

    /// @brief Copy set stored at `key` to `destination`.
    /// @param destination Key of the destination set.
    /// @param key Key of the source set.
    /// @return Number of members of the set.
    /// @see https://redis.io/commands/sinter
    template <typename... Args>
    inline awaiter_t<uint64_t> sinterstore(std::string_view destination,
                                           std::string_view key, Args&&... keys) {
        std::string cmd("sinterstore");
        cmd.append(" ").append(destination);
        cmd.append(" ").append(key);
        (cmd.append(" ").append(keys), ...);
        return impl_.command<uint64_t>(std::move(cmd));
    }

    /// @brief Test if `member` exists in the set stored at key.
    /// @param key Key where the set is stored.
    /// @param member Member to be checked.
    /// @return Whether `member` exists in the set.
    /// @retval true If it exists in the set.
    /// @retval false If it does not exist in the set, or the given key does not
    /// exist.
    /// @see https://redis.io/commands/sismember
    inline awaiter_t<uint64_t> sismember(std::string_view key,
                                         std::string_view member) {
        return impl_.command<uint64_t>(fmt::format("sismember {} {}", key, member));
    }

    /// @brief Get all members in the given set.
    ///
    /// Example:
    /// @code{.cpp}
    /// std::unordered_set<std::string> members1;
    /// redis.smembers("set", std::inserter(members1, members1.begin()));
    /// std::vector<std::string> members2;
    /// redis.smembers("set", std::back_inserter(members2));
    /// @endcode
    /// @param key Key where the set is stored.
    /// @param output Iterator to the destination where the result is saved.
    /// @see https://redis.io/commands/smembers
    inline awaiter_t<std::vector<std::string>> smembers(std::string_view key) {
        return impl_.command<std::vector<std::string>>(
                   fmt::format("smembers {}", key));
    }

    /// @brief Move `member` from one set to another.
    /// @param source Key of the set in which the member currently exists.
    /// @param destination Key of the destination set.
    /// @return Whether the member has been moved.
    /// @retval true If the member has been moved.
    /// @retval false If `member` does not exist in `source`.
    /// @see https://redis.io/commands/smove
    inline awaiter_t<uint64_t> smove(std::string_view source,
                                     std::string_view destination,
                                     std::string_view member) {
        return impl_.command<uint64_t>(
                   fmt::format("smove {} {} {}", source, destination, member));
    }

    /// @brief Remove a random member from the set.
    /// @param key Key where the set is stored.
    /// @return The popped member.
    /// @note If the set is empty, `spop` returns `OptionalString{}`
    /// (`std::nullopt`).
    /// @see `Redis::srandmember`
    /// @see https://redis.io/commands/spop
    inline awaiter_t<std::vector<std::string>> spop(std::string_view key,
    uint64_t count = 1) {
        return impl_.command<std::vector<std::string>>(
                   fmt::format("spop {} {}", key, count));
    }

    /// @brief Get a random member of the given set.
    /// @param key Key where the set is stored.
    /// @return A random member.
    /// @note If the set is empty, `srandmember` returns `OptionalString{}`
    /// (`std::nullopt`).
    /// @note This method won't remove the member from the set.
    /// @see `Redis::spop`
    /// @see https://redis.io/commands/srandmember
    inline awaiter_t<std::string> srandmember(std::string_view key) {
        return impl_.command<std::string>(fmt::format("srandmember {}", key));
    }

    /// @brief Get multiple random members of the given set.
    /// @param key Key where the set is stored.
    /// @param count Number of members to be returned.
    /// @param output Output iterator to the destination where the result is
    /// saved.
    /// @note This method won't remove members from the set.
    /// @see `Redis::spop`
    /// @see https://redis.io/commands/srandmember
    inline awaiter_t<std::vector<std::string>> srandmember(std::string_view key,
    uint64_t count) {
        return impl_.command<std::vector<std::string>>(
                   fmt::format("srandmember {} {}", key, count));
    }

    /// @brief Remove a member from set.
    /// @param key Key where the set is stored.
    /// @param member Member to be removed.
    /// @return Whether the member has been removed.
    /// @retval 1 If the given member exists, and has been removed.
    /// @retval 0 If the given member does not exist.
    /// @see https://redis.io/commands/srem
    template <typename... Args>
    inline awaiter_t<uint64_t> srem(std::string_view key, std::string_view member,
                                    Args&&... members) {
        std::string cmd("srem");
        cmd.append(" ").append(key);
        cmd.append(" ").append(member);
        (cmd.append(" ").append(members), ...);
        return impl_.command<uint64_t>(std::move(cmd));
    }

    /// @brief Scan members of the set matching the given pattern.
    ///
    /// Example:
    /// @code{.cpp}
    /// auto cursor = 0LL;
    /// std::unordered_set<std::string> members;
    /// while (true) {
    ///     cursor = redis.sscan("set", cursor, "pattern:*",
    ///         10, std::inserter(members, members.begin()));
    ///     if (cursor == 0) {
    ///         break;
    ///     }
    /// }
    /// @endcode
    /// @param key Key where the set is stored.
    /// @param cursor Cursor.
    /// @param pattern Pattern of fields to be scanned.
    /// @param count A hint for how many fields to be scanned.
    /// @param output Output iterator to the destination where the result is
    /// saved.
    /// @return The cursor to be used for the next scan operation.
    /// @see https://redis.io/commands/sscan
    inline awaiter_t<scan_ret_t> sscan(std::string_view key, uint64_t cursor,
                                       uint64_t count) {
        return sscan(key, cursor, "", count);
    }

    inline awaiter_t<scan_ret_t> sscan(std::string_view key, uint64_t cursor,
                                       std::string_view pattern,
                                       uint64_t count = 0) {
        return impl_.sscan(key, cursor, pattern, count);
    }

    /// @brief Get the union between the first set and all successive sets.
    /// @param first Iterator to the first set.
    /// @param last Off-the-end iterator to the range.
    /// @param output Output iterator to the destination where the result is
    /// saved.
    /// @see https://redis.io/commands/sunion
    // TODO: `void sunion(const StringView &key, Input first, Input last, Output
    // output)` is better.
    template <typename... Args>
    inline awaiter_t<uint64_t> sunion(std::string_view key, Args&&... keys) {
        std::string cmd("sunion");
        cmd.append(" ").append(key);
        (cmd.append(" ").append(keys), ...);
        return impl_.command<uint64_t>(std::move(cmd));
    }

    /// @brief Copy set stored at `key` to `destination`.
    /// @param destination Key of the destination set.
    /// @param key Key of the source set.
    /// @return Number of members of the set.
    /// @see https://redis.io/commands/sunionstore
    template <typename... Args>
    inline awaiter_t<uint64_t> sunionstore(std::string_view destination,
                                           std::string_view key, Args&&... keys) {
        std::string cmd("sunionstore");
        cmd.append(" ").append(destination);
        cmd.append(" ").append(key);
        (cmd.append(" ").append(keys), ...);
        return impl_.command<uint64_t>(std::move(cmd));
    }

    // SORTED SET commands.

    /// @brief Pop the member with highest score from sorted set in a blocking
    /// way.
    /// @param key Key where the sorted set is stored.
    /// @param timeout Timeout in seconds. 0 means block forever.
    /// @return Key-member-score tuple with the highest score.
    /// @see `Redis::zpopmax`
    /// @see https://redis.io/commands/bzpopmax
    inline awaiter_t<std::vector<std::string>> bzpopmax(
    std::initializer_list<std::string_view> keys, uint64_t timeout = 0) {
        if (keys.size() == 0) return {};
        std::string cmd("bzpopmax");
        for (const auto& key : keys) {
            cmd.append(" ").append(key);
        }
        cmd.append(" ").append(std::to_string(timeout));
        return impl_.command<std::vector<std::string>>(std::move(cmd));
    }

    /// @brief Pop the member with lowest score from sorted set in a blocking way.
    /// @param key Key where the sorted set is stored.
    /// @param timeout Timeout in seconds. 0 means block forever.
    /// @return Key-member-score tuple with the lowest score.
    /// @note If sorted set is empty and timeout reaches, `bzpopmin` returns
    ///       `Optional<std::tuple<std::string, std::string, double>>{}`
    ///       (`std::nullopt`).
    /// @see `Redis::zpopmin`
    /// @see https://redis.io/commands/bzpopmin
    inline awaiter_t<std::vector<std::string>> bzpopmin(
    std::initializer_list<std::string_view> keys, uint64_t timeout = 0) {
        if (keys.size() == 0) return {};
        std::string cmd("bzpopmin");
        for (const auto& key : keys) {
            cmd.append(" ").append(key);
        }
        cmd.append(" ").append(std::to_string(timeout));
        return impl_.command<std::vector<std::string>>(std::move(cmd));
    }

    /// @brief Add or update a member with score to sorted set.
    /// @param key Key where the sorted set is stored.
    /// @param member Member to be added.
    /// @param score Score of the member.
    /// @param type Options for zadd command:
    ///             - UpdateType::EXIST: Add the member only if it already exists.
    ///             - UpdateType::NOT_EXIST: Add the member only if it does not
    ///             exist.
    ///             - UpdateType::ALWAYS: Always add the member no matter whether
    ///             it exists.
    /// @param changed Whether change the return value from number of newly added
    /// member to
    ///                number of members changed (i.e. added and updated).
    /// @return Number of added members or number of added and updated members
    /// depends on `changed`.
    /// @note We don't support the INCR option, because in this case, the return
    /// value of zadd
    ///       command is NOT of type uint64_t. However, you can always use the
    ///       generic interface to send zadd command with INCR option: `auto score
    ///       = redis.command<OptionalDouble>("ZADD", "key", "XX", "INCR", 10,
    ///       "mem");`
    /// @see `UpdateType`
    /// @see https://redis.io/commands/zadd
    inline awaiter_t<uint64_t> zadd(std::string_view key, std::string_view member,
                                    double score,
                                    UpdateType type = UpdateType::ALWAYS) {
        return impl_.zadd(key, member, score, type);
    }

    /// @brief Add or update multiple members with score to sorted set.
    ///
    /// Example:
    /// @code{.cpp}
    /// std::unordered_map<std::string, double> m = {{"m1", 1.2}, {"m2", 2.3}};
    /// redis.zadd("zset", m.begin(), m.end());
    /// @endcode
    /// @param key Key where the sorted set is stored.
    /// @param first Iterator to the first member-score pair.
    /// @param last Off-the-end iterator to the member-score pairs range.
    /// @param type Options for zadd command:
    ///             - UpdateType::EXIST: Add the member only if it already exists.
    ///             - UpdateType::NOT_EXIST: Add the member only if it does not
    ///             exist.
    ///             - UpdateType::ALWAYS: Always add the member no matter whether
    ///             it exists.
    /// @param changed Whether change the return value from number of newly added
    /// member to
    ///                number of members changed (i.e. added and updated).
    /// @return Number of added members or number of added and updated members
    /// depends on `changed`.
    /// @note We don't support the INCR option, because in this case, the return
    /// value of zadd
    ///       command is NOT of type uint64_t. However, you can always use the
    ///       generic interface to send zadd command with INCR option: `auto score
    ///       = redis.command<OptionalDouble>("ZADD", "key", "XX", "INCR", 10,
    ///       "mem");`
    /// @see `UpdateType`
    /// @see https://redis.io/commands/zadd
    inline awaiter_t<uint64_t> zadd(
        std::string_view key,
        std::vector<std::pair<std::string_view, double>> kvs,
        UpdateType type = UpdateType::ALWAYS) {
        return impl_.zadd(key, kvs, type);
    }

    /// @brief Add or update multiple members with score to sorted set.
    ///
    /// Example:
    /// @code{.cpp}
    /// redis.zadd("zset", {std::make_pair("m1", 1.4),
    /// std::make_pair("m2", 2.3)});
    /// @endcode
    /// @param key Key where the sorted set is stored.
    /// @param first Iterator to the first member-score pair.
    /// @param last Off-the-end iterator to the member-score pairs range.
    /// @param type Options for zadd command:
    ///             - UpdateType::EXIST: Add the member only if it already exists.
    ///             - UpdateType::NOT_EXIST: Add the member only if it does not
    ///             exist.
    ///             - UpdateType::ALWAYS: Always add the member no matter whether
    ///             it exists.
    /// @param changed Whether change the return value from number of newly added
    /// member to
    ///                number of members changed (i.e. added and updated).
    /// @return Number of added members or number of added and updated members
    /// depends on `changed`.
    /// @note We don't support the INCR option, because in this case, the return
    /// value of zadd
    ///       command is NOT of type uint64_t. However, you can always use the
    ///       generic interface to send zadd command with INCR option: `auto score
    ///       = redis.command<OptionalDouble>("ZADD", "key", "XX", "INCR", 10,
    ///       "mem");`
    /// @see `UpdateType`
    /// @see https://redis.io/commands/zadd
    template <typename T>
    inline awaiter_t<uint64_t> zadd(std::string_view key,
                                    std::initializer_list<T> il,
                                    UpdateType type = UpdateType::ALWAYS,
                                    bool changed = false) {
        return zadd(key, il.begin(), il.end(), type, changed);
    }

    /// @brief Get the number of members in the sorted set.
    /// @param key Key where the sorted set is stored.
    /// @return Number of members in the sorted set.
    /// @see https://redis.io/commands/zcard
    inline awaiter_t<uint64_t> zcard(std::string_view key) {
        return impl_.command<uint64_t>(fmt::format("zcard {}", key));
    }

    /// @brief Get the number of members with score between a min-max score range.
    ///
    /// Example:
    /// @code{.cpp}
    /// @endcode
    /// @param key Key where the sorted set is stored.
    /// @param interval The min-max score range.
    /// @return Number of members with score between a min-max score range.
    /// @see `BoundedInterval`
    /// @see `LeftBoundedInterval`
    /// @see `RightBoundedInterval`
    /// @see `UnboundedInterval`
    /// @see `BoundType`
    /// @see https://redis.io/commands/zcount
    // TODO: add a string version of Interval: zcount("key", "2.3", "5").
    inline awaiter_t<uint64_t> zcount(std::string_view key, std::string_view min,
                                      std::string_view max) {
        return impl_.command<uint64_t>(
                   fmt::format("zcount {} {} {}", key, min, max));
    }

    /// @brief Increment the score of given member.
    /// @param key Key where the sorted set is stored.
    /// @param increment Increment.
    /// @param member Member.
    /// @return The score of the member after the operation.
    /// @see https://redis.io/commands/zincrby
    inline awaiter_t<double> zincrby(std::string_view key, double increment,
                                     std::string_view member) {
        return impl_.command<double>(
                   fmt::format("zincrby {} {} {}", key, increment, member));
    }
    /*
            /// @brief Copy a sorted set to another one with the scores being
       multiplied by a factor.
            /// @param destination Key of the destination sorted set.
            /// @param key Key of the source sorted set.
            /// @param weight Weight to be multiplied to the score of each member.
            /// @return The number of members in the sorted set.
            /// @note  There's no aggregation type parameter for single key
       overload, since these 3 types
            ///         have the same effect.
            /// @see `Redis::zunionstore`
            /// @see https://redis.io/commands/zinterstore
            //inline awaiter_t<uint64_t> zinterstore(std::string_view destination,
       std::string_view key, double weight) {
            //	return impl_.command<double>(fmt::format("zinterstore {} {} {}",
       key, increment, member));
            //}



            /// @brief Get intersection of multiple sorted sets, and store the
       result to another one.
            ///
            /// Example:
            /// @code{.cpp}
            /// // Use the default weight, i.e. 1,
            /// // and use the sum of the all scores as the score of the result:
            /// redis.zinterstore("destination", {"k1", "k2"});
            /// // Each sorted set has a different weight,
            /// // and the score of the result is the min of all scores.
            /// redis.zinterstore("destination",
            ///     {std::make_pair("k1", 1), std::make_pair("k2", 2)},
       Aggregation::MIN);
            /// @endcode
            /// @param destination Key of the destination sorted set.
            /// @param il Initializer list of sorted set.
            /// @param type How the scores are aggregated.
            ///             - Aggregation::SUM: Score of a member is the sum of
       all scores.
            ///             - Aggregation::MIN: Score of a member is the min of
       all scores.
            ///             - Aggregation::MAX: Score of a member is the max of
       all scores.
            /// @return The number of members in the resulting sorted set.
            /// @note The score of each member can be multiplied by a factor, i.e.
       weight. If `T` is
            ///       of type `std::string`, we use the default weight, i.e. 1,
       and send
            ///       *ZINTERSTORE dest numkeys key [key ...] [AGGREGATE
       SUM|MIN|MAX]* command.
            ///       If `T` is of type `std::pair<std::string, double>`, i.e.
       key-weight pair,
            ///       we send the command with the given weights:
            ///       *ZINTERSTORE dest numkeys key [key ...] [WEIGHTS weight
       [weight ...]] [AGGREGATE SUM|MIN|MAX]*.
            ///       See the *Example* part for examples on how to use this
       command.
            /// @see `Redis::zunionstore`
            /// @see https://redis.io/commands/zinterstore
            template <typename T>
            inline awaiter_t<uint64_t> zinterstore(std::string_view destination,
                    std::initializer_list<T> il,
                    Aggregation type = Aggregation::SUM) {
                    return zinterstore(destination, il.begin(), il.end(), type);
            }
    */
    /// @brief Get the number of members between a min-max range in
    /// lexicographical order.
    ///
    /// @param key Key where the sorted set is stored.
    /// @param interval The min-max range in lexicographical order.
    /// @return Number of members between a min-max range in lexicographical
    /// order.
    /// @see `BoundedInterval`
    /// @see `LeftBoundedInterval`
    /// @see `RightBoundedInterval`
    /// @see `UnboundedInterval`
    /// @see `BoundType`
    /// @see https://redis.io/commands/zlexcount
    // TODO: add a string version of Interval: zlexcount("key", "(abc", "abd").
    inline awaiter_t<uint64_t> zlexcount(std::string_view key,
                                         std::string_view min,
                                         std::string_view max) {
        return impl_.command<uint64_t>(
                   fmt::format("zlexcount {} {} {}", key, min, max));
    }

    /// @brief Pop the member with highest score from sorted set.
    /// @param key Key where the sorted set is stored.
    /// @return Member-score pair with the highest score.
    /// @note If sorted set is empty `zpopmax` returns
    ///       `Optional<std::pair<std::string, double>>{}` (`std::nullopt`).
    /// @see `Redis::bzpopmax`
    /// @see https://redis.io/commands/zpopmax
    inline awaiter_t<std::vector<std::string>> zpopmax(std::string_view key,
    uint64_t count = 1) {
        return impl_.command<std::vector<std::string>>(
                   fmt::format("zpopmax {} {}", key, count));
    }

    /// @brief Pop the member with lowest score from sorted set.
    /// @param key Key where the sorted set is stored.
    /// @return Member-score pair with the lowest score.
    /// @note If sorted set is empty `zpopmin` returns
    ///       `Optional<std::pair<std::string, double>>{}` (`std::nullopt`).
    /// @see `Redis::bzpopmin`
    /// @see https://redis.io/commands/zpopmin
    inline awaiter_t<std::vector<std::string>> zpopmin(std::string_view key) {
        return impl_.command<std::vector<std::string>>(
                   fmt::format("zpopmin {}", key));
    }

    /// @brief Get a range of members by rank (ordered from lowest to highest).
    ///
    /// Example:
    /// @code{.cpp}
    /// // send *ZRANGE* command without the *WITHSCORES* option:
    /// std::vector<std::string> result;
    /// redis.zrange("zset", 0, -1, std::back_inserter(result));
    /// // send command with *WITHSCORES* option:
    /// std::unordered_map<std::string, double> with_score;
    /// redis.zrange("zset", 0, -1, std::inserter(with_score, with_score.end()));
    /// @endcode
    /// @param key Key where the sorted set is stored.
    /// @param start Start rank. Inclusive and can be negative.
    /// @param stop Stop rank. Inclusive and can be negative.
    /// @param output Output iterator to the destination where the result is
    /// saved.
    /// @note This method can also return the score of each member. If `output` is
    /// an iterator
    ///       to a container of `std::string`, we send *ZRANGE key start stop*
    ///       command. If it's an iterator to a container of
    ///       `std::pair<std::string, double>`, we send *ZRANGE key start stop
    ///       WITHSCORES* command. See the *Example* part on how to use this
    ///       method.
    /// @see `Redis::zrevrange`
    /// @see https://redis.io/commands/zrange
    inline awaiter_t<std::vector<std::string>> zrange(std::string_view key,
                                            std::string_view min,
    std::string_view max) {
        return impl_.command<std::vector<std::string>>(
                   fmt::format("zrange {} {} {}", key, min, max));
    }

    /// @brief Get a range of members by lexicographical order (from lowest to
    /// highest).
    ///
    /// Example:
    /// @code{.cpp}
    /// std::vector<std::string> result;
    /// // Get members between [abc, abd].
    /// redis.zrangebylex("zset", BoundedInterval<std::string>("abc", "abd",
    /// BoundType::CLOSED),
    ///     std::back_inserter(result));
    /// @endcode
    /// @param key Key where the sorted set is stored.
    /// @param interval the min-max range by lexicographical order.
    /// @param output Output iterator to the destination where the result is
    /// saved.
    /// @note See `Redis::zlexcount`'s *Example* part for how to set `interval`
    /// parameter.
    /// @see `Redis::zlexcount`
    /// @see `BoundedInterval`
    /// @see `LeftBoundedInterval`
    /// @see `RightBoundedInterval`
    /// @see `UnboundedInterval`
    /// @see `BoundType`
    /// @see `Redis::zrevrangebylex`
    /// @see https://redis.io/commands/zrangebylex
    ///
    inline awaiter_t<std::vector<std::string>> zrangebylex(std::string_view key,
                                            std::string_view min,
    std::string_view max) {
        return impl_.command<std::vector<std::string>>(
                   fmt::format("zrangebylex {} {} {}", key, min, max));
    }

    /// @brief Get a range of members by score (ordered from lowest to highest).
    ///
    /// Example:
    /// @code{.cpp}
    /// // Send *ZRANGEBYSCORE* command without the *WITHSCORES* option:
    /// std::vector<std::string> result;
    /// // Get members whose score between (3, 6).
    /// redis.zrangebyscore("zset", BoundedInterval<double>(3, 6,
    /// BoundType::OPEN),
    ///     std::back_inserter(result));
    /// // Send command with *WITHSCORES* option:
    /// std::unordered_map<std::string, double> with_score;
    /// // Get members whose score between [3, +inf).
    /// redis.zrangebyscore("zset", LeftBoundedInterval<double>(3,
    /// BoundType::RIGHT_OPEN),
    ///     std::inserter(with_score, with_score.end()));
    /// @endcode
    /// @param key Key where the sorted set is stored.
    /// @param interval the min-max range by score.
    /// @param output Output iterator to the destination where the result is
    /// saved.
    /// @note This method can also return the score of each member. If `output` is
    /// an iterator
    ///       to a container of `std::string`, we send *ZRANGEBYSCORE key min max*
    ///       command. If it's an iterator to a container of
    ///       `std::pair<std::string, double>`, we send *ZRANGEBYSCORE key min max
    ///       WITHSCORES* command. See the *Example* part on how to use this
    ///       method.
    /// @note See `Redis::zcount`'s *Example* part for how to set the `interval`
    /// parameter.
    /// @see `Redis::zrevrangebyscore`
    /// @see https://redis.io/commands/zrangebyscore
    inline awaiter_t<std::vector<std::string>> zrangebyscore(
    std::string_view key, std::string_view min, std::string_view max) {
        return impl_.command<std::vector<std::string>>(
                   fmt::format("zrangebyscore {} {} {}", key, min, max));
    }

    /// @brief Get the rank (from low to high) of the given member in the sorted
    /// set.
    /// @param key Key where the sorted set is stored.
    /// @param member Member.
    /// @return The rank of the given member.
    /// @note If the member does not exist, `zrank` returns `OptionalLongLong{}`
    /// (`std::nullopt`).
    /// @see https://redis.io/commands/zrank
    inline awaiter_t<uint64_t> zrank(std::string_view key,
                                     std::string_view member) {
        return impl_.command<uint64_t>(fmt::format("zrank {} {}", key, member));
    }

    /// @brief Remove the given member from sorted set.
    /// @param key Key where the sorted set is stored.
    /// @param member Member to be removed.
    /// @return Whether the member has been removed.
    /// @retval 1 If the member exists, and has been removed.
    /// @retval 0 If the member does not exist.
    /// @see https://redis.io/commands/zrem
    template <typename... Args>
    inline awaiter_t<uint64_t> zrem(std::string_view key, std::string_view member,
                                    Args&&... members) {
        std::string cmd("zrem");
        cmd.append(" ").append(key);
        cmd.append(" ").append(member);
        (cmd.append(" ").append(members), ...);
        return impl_.command<uint64_t>(std::move(cmd));
    }

    /// @brief Remove members in the given range of lexicographical order.
    /// @param key Key where the sorted set is stored.
    /// @param interval the min-max range by lexicographical order.
    /// @note See `Redis::zlexcount`'s *Example* part for how to set `interval`
    /// parameter.
    /// @return Number of members removed.
    /// @see `Redis::zlexcount`
    /// @see `BoundedInterval`
    /// @see `LeftBoundedInterval`
    /// @see `RightBoundedInterval`
    /// @see `UnboundedInterval`
    /// @see `BoundType`
    /// @see https://redis.io/commands/zremrangebylex
    inline awaiter_t<uint64_t> zremrangebylex(std::string_view key,
            std::string_view min,
            std::string_view max) {
        return impl_.command<uint64_t>(
                   fmt::format("zremrangebylex {} {} {}", key, min, max));
    }

    /// @brief Remove members in the given range ordered by rank.
    /// @param key Key where the sorted set is stored.
    /// @param start Start rank.
    /// @param stop Stop rank.
    /// @return Number of members removed.
    /// @see https://redis.io/commands/zremrangebyrank
    inline awaiter_t<uint64_t> zremrangebyrank(std::string_view key,
            uint64_t start, uint64_t stop) {
        return impl_.command<uint64_t>(
                   fmt::format("zremrangebyrank {} {} {}", key, start, stop));
    }

    /// @brief Remove members in the given range ordered by score.
    /// @param key Key where the sorted set is stored.
    /// @param interval the min-max range by score.
    /// @return Number of members removed.
    /// @note See `Redis::zcount`'s *Example* part for how to set the `interval`
    /// parameter.
    /// @see https://redis.io/commands/zremrangebyscore
    inline awaiter_t<uint64_t> zremrangebyscore(std::string_view key,
            std::string_view min,
            std::string_view max) {
        return impl_.command<uint64_t>(
                   fmt::format("zremrangebyscore {} {} {}", key, min, max));
    }

    /// @brief Get a range of members by rank (ordered from highest to lowest).
    ///
    /// Example:
    /// @code{.cpp}
    /// // send *ZREVRANGE* command without the *WITHSCORES* option:
    /// std::vector<std::string> result;
    /// redis.zrevrange("key", 0, -1, std::back_inserter(result));
    /// // send command with *WITHSCORES* option:
    /// std::unordered_map<std::string, double> with_score;
    /// redis.zrevrange("key", 0, -1, std::inserter(with_score,
    /// with_score.end()));
    /// @endcode
    /// @param key Key where the sorted set is stored.
    /// @param start Start rank. Inclusive and can be negative.
    /// @param stop Stop rank. Inclusive and can be negative.
    /// @param output Output iterator to the destination where the result is
    /// saved.
    /// @note This method can also return the score of each member. If `output` is
    /// an iterator
    ///       to a container of `std::string`, we send *ZREVRANGE key start stop*
    ///       command. If it's an iterator to a container of
    ///       `std::pair<std::string, double>`, we send *ZREVRANGE key start stop
    ///       WITHSCORES* command. See the *Example* part on how to use this
    ///       method.
    /// @see `Redis::zrange`
    /// @see https://redis.io/commands/zrevrange
    inline awaiter_t<std::string> zrevrange(std::string_view key, uint64_t start,
                                            uint64_t stop,
                                            bool withscores = false) {
        return impl_.command<std::string>(
                   fmt::format("zrevrange {} {} {} {}", key, start, stop,
                               (withscores ? "WITHSCORES" : "")));
    }

    /// @brief Get a range of members by lexicographical order (from highest to
    /// lowest).
    ///
    /// Example:
    /// @code{.cpp}
    /// std::vector<std::string> result;
    /// // Get members between [abc, abd] in reverse order.
    /// redis.zrevrangebylex("zset", BoundedInterval<std::string>("abc", "abd",
    /// BoundType::CLOSED),
    ///     std::back_inserter(result));
    /// @endcode
    /// @param key Key where the sorted set is stored.
    /// @param interval the min-max range by lexicographical order.
    /// @param output Output iterator to the destination where the result is
    /// saved.
    /// @note See `Redis::zlexcount`'s *Example* part for how to set `interval`
    /// parameter.
    /// @see `Redis::zlexcount`
    /// @see `BoundedInterval`
    /// @see `LeftBoundedInterval`
    /// @see `RightBoundedInterval`
    /// @see `UnboundedInterval`
    /// @see `BoundType`
    /// @see `Redis::zrangebylex`
    /// @see https://redis.io/commands/zrevrangebylex
    inline awaiter_t<std::string> zrevrangebylex(std::string_view key,
            std::string_view min,
            std::string_view max) {
        return impl_.command<std::string>(
                   fmt::format("zrevrangebylex {} {} {}", key, min, max));
    }

    /// @brief Get a range of members by score (ordered from highest to lowest).
    ///
    /// Example:
    /// @code{.cpp}
    /// // Send *ZREVRANGEBYSCORE* command without the *WITHSCORES* option:
    /// std::vector<std::string> result;
    /// // Get members whose score between (3, 6) in reverse order.
    /// redis.zrevrangebyscore("zset", BoundedInterval<double>(3, 6,
    /// BoundType::OPEN),
    ///     std::back_inserter(result));
    /// // Send command with *WITHSCORES* option:
    /// std::unordered_map<std::string, double> with_score;
    /// // Get members whose score between [3, +inf) in reverse order.
    /// redis.zrevrangebyscore("zset", LeftBoundedInterval<double>(3,
    /// BoundType::RIGHT_OPEN),
    ///     std::inserter(with_score, with_score.end()));
    /// @endcode
    /// @param key Key where the sorted set is stored.
    /// @param interval the min-max range by score.
    /// @param output Output iterator to the destination where the result is
    /// saved.
    /// @note This method can also return the score of each member. If `output` is
    /// an iterator
    ///       to a container of `std::string`, we send *ZREVRANGEBYSCORE key min
    ///       max* command. If it's an iterator to a container of
    ///       `std::pair<std::string, double>`, we send *ZREVRANGEBYSCORE key min
    ///       max WITHSCORES* command. See the *Example* part on how to use this
    ///       method.
    /// @note See `Redis::zcount`'s *Example* part for how to set the `interval`
    /// parameter.
    /// @see `Redis::zrangebyscore`
    /// @see https://redis.io/commands/zrevrangebyscore
    template <typename Interval, typename Output>
    inline awaiter_t<std::string> zrevrangebyscore(std::string_view key,
            std::string_view min,
            std::string_view max) {
        return impl_.command<std::string>(
                   fmt::format("zrevrangebyscore {} {} {}", key, min, max));
    }

    /// @brief Get the rank (from high to low) of the given member in the sorted
    /// set.
    /// @param key Key where the sorted set is stored.
    /// @param member Member.
    /// @return The rank of the given member.
    /// @note If the member does not exist, `zrevrank` returns
    /// `OptionalLongLong{}` (`std::nullopt`).
    /// @see https://redis.io/commands/zrevrank
    inline awaiter_t<uint64_t> zrevrank(std::string_view key,
                                        std::string_view member) {
        return impl_.command<uint64_t>(fmt::format("zrevrank {} {}", key, member));
    }

    /// @brief Scan all members of the given sorted set.
    /// @param key Key where the sorted set is stored.
    /// @param cursor Cursor.
    /// @param count A hint for how many members to be scanned.
    /// @param output Output iterator to the destination where the result is
    /// saved.
    /// @return The cursor to be used for the next scan operation.
    /// @see https://redis.io/commands/zscan
    inline awaiter_t<uint64_t> zscan(std::string_view key, uint64_t cursor,
                                     uint64_t count) {
        return zscan(key, cursor, count);
    }

    /// @brief Scan all members of the given sorted set.
    /// @param key Key where the sorted set is stored.
    /// @param cursor Cursor.
    /// @param output Output iterator to the destination where the result is
    /// saved.
    /// @return The cursor to be used for the next scan operation.
    /// @see https://redis.io/commands/zscan
    inline awaiter_t<scan_ret_t> zscan(std::string_view key, uint64_t cursor,
                                       std::string_view pattern, uint64_t count) {
        return impl_.zscan(key, cursor, pattern, count);
    }

    /// @brief Get the score of the given member.
    /// @param key Key where the sorted set is stored.
    /// @param member Member.
    /// @return The score of the member.
    /// @note If member does not exist, `zscore` returns `OptionalDouble{}`
    /// (`std::nullopt`).
    /// @see https://redis.io/commands/zscore
    inline awaiter_t<double> zscore(std::string_view key,
                                    std::string_view member) {
        return impl_.command<double>(fmt::format("zscore {} {}", key, member));
    }

    /// @brief Copy a sorted set to another one with the scores being multiplied
    /// by a factor.
    /// @param destination Key of the destination sorted set.
    /// @param key Key of the source sorted set.
    /// @param weight Weight to be multiplied to the score of each member.
    /// @return The number of members in the sorted set.
    /// @note  There's no aggregation type parameter for single key overload,
    /// since these 3 types
    ///         have the same effect.
    /// @see `Redis::zinterstore`
    /// @see https://redis.io/commands/zinterstore
    inline awaiter_t<uint64_t> zunionstore(
        std::string_view destination,
        std::initializer_list<std::string_view> keys,
        std::string_view aggregate = "SUM") {
        return impl_.zunionstore(destination, keys, aggregate);
    }

    /// @brief Get union of multiple sorted sets, and store the result to another
    /// one.
    ///
    /// Example:
    /// @code{.cpp}
    /// // Use the default weight, i.e. 1,
    /// // and use the sum of the all scores as the score of the result:
    /// std::vector<std::string> keys = {"k1", "k2", "k3"};
    /// redis.zunionstore("destination", keys.begin(), keys.end());
    /// // Each sorted set has a different weight,
    /// // and the score of the result is the min of all scores.
    /// std::vector<std::pair<std::string, double>> keys_with_weights = {{"k1",
    /// 1}, {"k2", 2}}; redis.zunionstore("destination",
    /// keys_with_weights.begin(),
    ///     keys_with_weights.end(), Aggregation::MIN);
    /// // NOTE: `keys_with_weights` can also be of type
    /// `std::unordered_map<std::string, double>`.
    /// // However, it will be slower than std::vector<std::pair<std::string,
    /// double>>, since we use
    /// // `std::distance(first, last)` to calculate the *numkeys* parameter.
    /// @endcode
    /// @param destination Key of the destination sorted set.
    /// @param first Iterator to the first sorted set (might with weight).
    /// @param last Off-the-end iterator to the sorted set range.
    /// @param type How the scores are aggregated.
    ///             - Aggregation::SUM: Score of a member is the sum of all
    ///             scores.
    ///             - Aggregation::MIN: Score of a member is the min of all
    ///             scores.
    ///             - Aggregation::MAX: Score of a member is the max of all
    ///             scores.
    /// @return The number of members in the resulting sorted set.
    /// @note The score of each member can be multiplied by a factor, i.e. weight.
    /// If `Input` is an
    ///       iterator to a container of `std::string`, we use the default weight,
    ///       i.e. 1, and send *ZUNIONSTORE dest numkeys key [key ...] [AGGREGATE
    ///       SUM|MIN|MAX]* command.
    ///        If `Input` is an iterator to a container of `std::pair<std::string,
    ///        double>`, i.e. key-weight pair, we send the command with the given
    ///        weights:
    ///       *ZUNIONSTORE dest numkeys key [key ...] [WEIGHTS weight [weight
    ///       ...]] [AGGREGATE SUM|MIN|MAX]*. See the *Example* part for examples
    ///       on how to use this command.
    /// @see `Redis::zinterstore`
    /// @see https://redis.io/commands/zunionstore
    inline awaiter_t<uint64_t> zunionstore(
        std::string_view destination,
        std::vector<std::pair<std::string_view, double>> kvs,
        std::string_view aggregate = "SUM") {
        return impl_.zunionstore(destination, kvs, aggregate);
    }

    // HYPERLOGLOG commands.

    /// @brief Add the given element to a hyperloglog.
    /// @param key Key of the hyperloglog.
    /// @param element Element to be added.
    /// @return Whether any of hyperloglog's internal register has been altered.
    /// @retval true If at least one internal register has been altered.
    /// @retval false If none of internal registers has been altered.
    /// @note When `pfadd` returns false, it does not mean that this method failed
    /// to add
    ///       an element to the hyperloglog. Instead it means that the internal
    ///       registers were not altered. If `pfadd` fails, it will throw an
    ///       exception of `Exception` type.
    /// @see https://redis.io/commands/pfadd
    template <typename... Args>
    inline awaiter_t<uint64_t> pfadd(std::string_view key, Args&&... elements) {
        std::string cmd("pfadd");
        cmd.append(" ").append(key);
        (cmd.append(" ").append(elements), ...);
        return impl_.command<uint64_t>(std::move(cmd));
    }

  private:
    coro_connection() = default;
    inline awaiter_t<scan_ret_t> send_scan_cmd(std::string_view cmd) {
        return impl_.send_scan_cmd(cmd);
    }

  private:
    impl::coro_connection_impl impl_;

};  // class connectin
}  // namespace coro_redis
