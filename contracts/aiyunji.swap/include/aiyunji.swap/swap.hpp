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

    [[eosio::action]] void init();
    [[eosio::action]] void openasset( const name& user, const name& payer, const extended_symbol& ext_symbol);
    [[eosio::action]] void closeasset( const name& user, const name& to, const extended_symbol& ext_symbol, const string& memo);
    [[eosio::action]] void openpair( const symbol& market_symb, const symbol& base_symb, const symbol& lpt_symb);
    [[eosio::action]] void closepair( const symbol_code& market_sym, const symbol_code& base_sym, const bool closed);

    [[eosio::on_notify("*::transfer")]]
    void deposit(const name& from, const name& to, const asset& quant, const string& memo);

    using init_action           = action_wrapper<name("init"),          &ayj_swap::init           >;
    using createpair_action     = action_wrapper<name("createpair"),    &ayj_swap::createpair     >;
    using closepair_action      = action_wrapper<name("closepair"),     &ayj_swap::closepair      >;
    using addliquidity_action   = action_wrapper<name("addliquidity"),  &ayj_swap::addliquidity   >;
    using remliquidity_action   = action_wrapper<name("remliquidity"),  &ayj_swap::remliquidity   >;
    using transfer_action       = action_wrapper<name("transfer"),      &ayj_swap::deposit          >;

public:
    uint128_t make128key(uint64_t a, uint64_t b) {
        uint128_t aa = a;
        uint128_t bb = b;
        return (aa << 64) + bb;
    }

    checksum256 make256key(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
        if (make128key(a,b) < make128key(c,d))
            return checksum256::make_from_word_sequence<uint64_t>(a,b,c,d);
        else
            return checksum256::make_from_word_sequence<uint64_t>(c,d,a,b);
    }

private:
    void _addliquidity(const symbol_code& market_sym, const symbol_code &base_sym, uint64_t step, const name &to, const asset &quant, uint64_t nonce);
    void _deliq(const symbol_code& market_sym, const symbol_code &base_sym, const asset &liquidity, const name &to);
    void _swap(const symbol_code& market_sym, const symbol_code &base_sym, const asset &quant_in, const asset &quant_out, const name &to);

};
} // namespace ayj