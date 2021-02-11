#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/transaction.hpp>
#include <eosio/system.hpp>
#include <eosio/crypto.hpp>
#include <eosio/action.hpp>
#include <string>

#include "wasm_db.hpp"
#include "swap_states.hpp"

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

    [[eosio::action]] void inittoken(name user, symbol new_symbol, extended_asset initial_pool1, extended_asset initial_pool2, int initial_fee, name fee_contract);
    [[eosio::on_notify("*::transfer")]] void ontransfer(name from, name to, asset quantity, string memo);
    [[eosio::action]] void openext( const name& user, const name& payer, const extended_symbol& ext_symbol);
    [[eosio::action]] void closeext ( const name& user, const name& to, const extended_symbol& ext_symbol, string memo);
    [[eosio::action]] void withdraw(name user, name to, extended_asset to_withdraw, string memo);
    [[eosio::action]] void addliquidity(name user, asset to_buy, asset max_asset1, asset max_asset2);
    [[eosio::action]] void remliquidity(name user, asset to_sell, asset min_asset1, asset min_asset2);
    [[eosio::action]] void exchange( name user, symbol_code pair_token, extended_asset ext_asset_in, asset min_expected );
    [[eosio::action]] void changefee(symbol_code pair_token, int newfee);

    [[eosio::action]] void transfer(const name& from, const name& to, const asset& quantity, const string&  memo );
    [[eosio::action]] void open( const name& owner, const symbol& symbol, const name& ram_payer );
    [[eosio::action]] void close( const name& owner, const symbol& symbol );
    [[eosio::action]] void indexpair(name user, symbol evo_symbol); // This action is only temporarily useful

private:
    void add_signed_ext_balance( const name& owner, const extended_asset& value );
    void add_signed_liq(name user, asset to_buy, bool is_buying, asset max_asset1, asset max_asset2);
    void memoexchange(name user, extended_asset ext_asset_in, string_view details);
    extended_asset process_exch(symbol_code evo_token, extended_asset paying, asset min_expected);
    int64_t compute(int64_t x, int64_t y, int64_t z, int fee);
    asset string_to_asset(string input);
    void placeindex(name user, symbol evo_symbol, extended_asset pool1, extended_asset pool2 );
    void add_balance( const name& owner, const asset& value, const name& ram_payer );
    void sub_balance( const name& owner, const asset& value );
};

} // namespace ayj