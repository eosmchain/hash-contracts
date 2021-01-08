#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/transaction.hpp>
#include <eosio/system.hpp>
#include <eosio/crypto.hpp>
#include <eosio/action.hpp>
#include <string>

#include "wasm_db.hpp"
#include "entities.hpp"

using namespace wasm::db;

namespace ayj {

using eosio::asset;
using eosio::check;
using eosio::datastream;
using eosio::name;
using eosio::symbol;
using eosio::symbol_code;
using eosio::unsigned_int;

using std::string;


class [[eosio::contract("aiyunji.swap")]] ayj_swap: public eosio::contract {
private:
    global_singleton    _global;
    global_t            _gstate;
    dbc                 _dbc;

public:
    using contract::contract;

    ayj_swap(eosio::name receiver, eosio::name code, datastream<const char*> ds):
        contract(receiver, code, ds), _global(get_self(), get_self().value), _dbc(get_self()) 
    {
      _gstate = _global.exists() ? _global.get() : global_t{};
    }

    ~ayj_swap() { _global.set( _gstate, get_self() ); }

    [[eosio::action]]
    void init();

    [[eosio::action]]
    void create(const name& bank0, const name& bank1, const symbol& symbol0, const symbol& symbol1, const uint64_t& lpcode);

    [[eosio::action]]
    void skim(const symbol_code& market_sym, const symbol_code& base_sym, const name& to, const asset& balance0, const asset& balance1);

    [[eosio::action]]
    void sync(const symbol_code& market_sym, const symbol_code& base_sym, const asset& balance0, const asset& balance1);

    [[eosio::action]]
    void close(const symbol_code& market_sym, const symbol_code& base_sym, const bool closed);

    [[eosio::action]]
    void refund(const symbol_code& market_sym, const symbol_code &base_sym, const name& to, const uint64_t& nonce);
    
    [[eosio::on_notify("*::transfer")]]
    void deposit(name from, name to, asset quant, std::string memo);

    using init_action       = action_wrapper<name("init"),      &ayj_swap::init     >;
    using create_action     = action_wrapper<name("create"),    &ayj_swap::create   >;
    using skim_action       = action_wrapper<name("skim"),      &ayj_swap::skim     >;
    using sync_action       = action_wrapper<name("sync"),      &ayj_swap::sync     >;
    using refund_action     = action_wrapper<name("refund"),    &ayj_swap::refund   >;
    using transfer_action   = action_wrapper<name("transfer"),  &ayj_swap::deposit  >;

private:
    void _mint(const symbol_code& market_sym, const symbol_code &base_sym, uint64_t step, const name &to, const asset &quant, uint64_t nonce);
    void _burn(const symbol_code& market_sym, const symbol_code &base_sym, const asset &liquidity, const name &to);
    void _swap(const symbol_code& market_sym, const symbol_code &base_sym, const asset &quant_in, const asset &quant_out, const name &to);

};
} // namespace ayj