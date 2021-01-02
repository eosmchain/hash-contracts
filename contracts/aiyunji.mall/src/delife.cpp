#include "delife.hpp"
#include "delifedb.hpp"

#include <chrono>
#include <system.hpp>
#include <rpc.hpp>

using std::chrono::system_clock;
using namespace wasm;
using namespace wasm::db;
using namespace wasm::safemath;

ACTION delife::init() {
    require_auth( get_maintainer(get_self()) );

    set_config(CK_TOKEN_BANK_REGID,     wasmio_bank.value);
    set_config(CK_TOKEN_SYMBOL,         symbol_code("DLFCNY").raw());

    set_config(CK_PLATFORMSHARE_REGID,  wasmio_bank.value);

}

ACTION delife::config(string conf_key, uint64_t conf_val) {
    require_auth( get_maintainer(get_self()) );

    set_config(conf_key, conf_val);
}


ACTION delife::add_admin(regid issuer, regid admin, bool is_supper_admin) {
    check( is_account( admin ), "admin account does not exist" );
    check_admin_auth(issuer, true);

    admin_t the_admin(admin, is_supper_admin);
    check( !wasm::db::get(the_admin), "admin already exists" );
    wasm::db::set(the_admin);

    inc_admin_counter();
}

ACTION delife::del_admin(regid issuer, regid admin) {
    check_admin_auth(issuer);

    admin_t the_admin(admin);
    check( wasm::db::get(the_admin), "admin not exist" );

    wasm::db::del(the_admin);

    inc_admin_counter(false);
}

ACTION delife::showid(uint64_t id) {}

WASM_DISPATCH_LIMIT( delife,    (init)(config)
                                (add_admin)(del_admin)
                                (add_project)(del_project)
                                (donate)(accept)(showid) )