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

    [[eosio::on_notify("*::transfer")]] void ontransfer(const name& from, const name& to, const asset& quant, const string& memo);

    [[eosio::action]] void initpair( const name& user, const symbol& new_symbol, const extended_asset& initial_pool1, const extended_asset& initial_pool2, const int initial_fee, const name& fee_contract);
    [[eosio::action]] void openext( const name& user, const name& payer, const extended_symbol& ext_symbol);
    [[eosio::action]] void closeext( const name& user, const name& to, const extended_symbol& ext_symbol, const string& memo);
    [[eosio::action]] void withdraw( const name& user, const name& to, const extended_asset& to_withdraw, const string& memo);
    [[eosio::action]] void addliquidity( const name& user, const asset& to_buy, const asset& max_asset1, const asset& max_asset2);
    [[eosio::action]] void remliquidity( const name& user, const asset& to_sell, const asset& min_asset1, const asset& min_asset2);
    [[eosio::action]] void exchange( const name& user, const symbol_code& pair_token, const extended_asset& ext_asset_in, const asset& min_expected );
    [[eosio::action]] void changefee( const symbol_code& pair_token, const int newfee);

    [[eosio::action]] void transfer( const name& from, const name& to, const asset& quantity, const string& memo );
    [[eosio::action]] void open( const name& owner, const symbol& symbol, const name& ram_payer );
    [[eosio::action]] void close( const name& owner, const symbol& symbol );
    [[eosio::action]] void indexpair( const name& user, const symbol& lp_symbol); // This action is only temporarily useful

private:
    void add_lp_balance( const name& owner, const extended_asset& value );
    void add_signed_liq( const name& user, const asset& to_buy, const bool is_buying, const asset& max_asset1, const asset& max_asset2);
    void memoexchange( const name& user, const extended_asset& ext_asset_in, const string_view& details);
    extended_asset process_exch(symbol_code evo_token, extended_asset paying, asset min_expected);
    int64_t compute(int64_t x, int64_t y, int64_t z, int fee);
    asset string_to_asset(string input);
    void place_index(const name& user, const symbol& lp_symbol, const extended_asset& pool1, const extended_asset& pool2 );
    void add_balance( const name& owner, const asset& value, const name& ram_payer );
    void sub_balance( const name& owner, const asset& value );
};

} // namespace ayj