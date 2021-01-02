#include "delife.hpp"
#include "delifedb.hpp"
#include "delifedb_charity.hpp"

#include <chrono>
#include <system.hpp>
#include <rpc.hpp>

using std::chrono::system_clock;
using namespace wasm;
using namespace wasm::db;
using namespace wasm::safemath;

/**
 *  Usage: consumer pays the platform with fiat and receives the on-chain credit points
 */
ACTION delife::credit(regid issuer, regid consumer, uint64_t consume_points, string memo) {
    check_admin_auth(issuer);
    check( is_account(consumer), to_string(consumer) + " account not exist" );

    customer_t customer(consumer);
    check( db::get(customer), to_string(consumer) + " account not found" );

}

/**
 *  Usage: consumer pays the platform and receives the credit points
 */
ACTION delife::decredit(regid issuer, regid consumer, uint64_t consume_points, string memo) {

}

/**
 *  Usage: consumer pays the platform and receives the credit points
 */
ACTION delife::withdraw(regid issuer, regid to, asset quantity);
ACTION delife::invest(regid issuer, regid investor, uint64_t invest_points);


// settle all daily rewards
ACTION delife::emit_reward() {

}