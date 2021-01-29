#include <aiyunji.swap/swap.hpp>
#include <eosio.token/eosio.token.hpp>
#include <math.hpp>
#include <utils.hpp>

namespace ayj {

using namespace std;
using namespace eosio;
using namespace wasm::safemath;

/************************ Help functions ****************************/

<<<<<<< Updated upstream
void ayj_swap::_swap(const symbol_code& market_symb, const symbol_code &base_symb, const asset &quant_in, const asset &quant_out, const name &to) {
=======
    market_t market(market_sym, base_sym);
    check( _dbc.get(market), "market not exist:" + market_sym.to_string() + "-" + base_sym.to_string() );
    check( !market.closed, "market already closed: " + market_sym.to_string() + "-" + base_sym.to_string() );

    mint_action_t mint_action(to.value, nonce);
    if (step == UNISWAP_MINT_STEP1) {
        check( quant.symbol == market.reserve0.symbol, "step1: bank0 symbol and reserve0 symbol mismatch" );
        check( get_first_receiver() == market.bank0, "step1: bank and bank0 mismatch" );
        check( !_dbc.get(mint_action), "mint action step1 already exist: " );

        mint_action.nonce       = nonce;
        mint_action.market_sym = market_sym;
        mint_action.base_sym = base_sym;
        mint_action.bank0       = get_first_receiver();
        mint_action.amount0_in  = quant;
        mint_action.closed      = false;
        mint_action.created_at  = time_point_sec( current_time_point() );

        _dbc.set(mint_action);

    } else if (step == UNISWAP_MINT_STEP2) {
        check( _dbc.get(mint_action), "step2 mint action not exist" );
        check( mint_action.market_sym == market_sym && mint_action.base_sym == base_sym,
                "symbol_pair of mint action mismatches from param: " );
        // check(mint_action.create_time == time_point_sec(current_time_point()), "2
        // steps must be in one tx");
        check( !mint_action.closed, "mint action already closed" );
        check( get_first_receiver() == market.bank1, "step2 bank and bank1 mismatch" );
        check( quant.symbol == market.reserve1.symbol, "step2 bank1 symbol and reserve1 symbol mismatch" );

        mint_action.bank1      = get_first_receiver();
        mint_action.amount1_in = quant;
        mint_action.closed     = true;

        _dbc.set(mint_action);

        int64_t liquidity;
        symbol liquidity_symbol = market.total_liquidity.symbol;
        if (market.total_liquidity.amount == 0) {
            liquidity = sqrt(mul(mint_action.amount0_in.amount,
                                              mint_action.amount1_in.amount)) -
                        MINIMUM_LIQUIDITY * PRECISION_1;

            ISSUE( _gstate.liquidity_bank, _self, asset(MINIMUM_LIQUIDITY * PRECISION_1, liquidity_symbol), "" )

            market.total_liquidity =
                asset(MINIMUM_LIQUIDITY * PRECISION_1, liquidity_symbol);

        } else {
            liquidity = min(div(mul(mint_action.amount0_in.amount,
                                                            market.total_liquidity.amount),
                                           market.reserve0.amount),
                            div(mul(mint_action.amount1_in.amount,
                                                            market.total_liquidity.amount),
                                           market.reserve1.amount));
        }

        check( liquidity > 0, "zero or negative liquidity minted" );

        market.total_liquidity      += asset(liquidity, liquidity_symbol);
        market.reserve0             = market.reserve0 + mint_action.amount0_in;
        market.reserve1             = market.reserve1 + mint_action.amount1_in;
        market.last_updated_at      = time_point_sec( current_time_point() );

        _dbc.set(market);

        ISSUE( _gstate.liquidity_bank, to, asset(liquidity, liquidity_symbol), "" )
    }

}

void ayj_swap::_burn(const symbol_code& market_sym, const symbol_code &base_sym, const asset &liquidity, const name &to) {
    // require_auth( to );
    market_t market(market_sym, base_sym);
    check( _dbc.get(market), "market does not exist" );
    check( !market.closed, "market already closed" );
    check( liquidity.symbol.raw() == market.total_liquidity.symbol.raw(), "liquidity symbol and market lptoken symbol mismatch" );
    check( get_first_receiver() == _gstate.liquidity_bank, "liquidity bank and lptoken bank mismatch" );

    asset balance0 = market.reserve0;
    asset balance1 = market.reserve1;

    asset amount0 = asset( div( mul(balance0.amount,
                                   div( mul(liquidity.amount, PRECISION_1),
                                        market.total_liquidity.amount)),
                            PRECISION_1), balance0.symbol);
    asset amount1 = asset( div( mul(balance1.amount,
                                   div(mul(liquidity.amount, PRECISION_1),
                                        market.total_liquidity.amount)),
                            PRECISION_1), balance1.symbol);

    BURN( _gstate.liquidity_bank, _self, liquidity, "" )

    TRANSFER( market.bank0, to, amount0, "" )
    TRANSFER( market.bank1, to, amount1, "" )

    market.total_liquidity      -= liquidity;
    market.reserve0             = balance0 - amount0;
    market.reserve1             = balance1 - amount1;
    market.last_updated_at      = current_time_point();

    _dbc.set(market);
}

void ayj_swap::_swap(const symbol_code& market_sym, const symbol_code &base_sym, const asset &quant_in, const asset &quant_out, const name &to) {
>>>>>>> Stashed changes
    check( quant_in.amount > 0, "input amount must be > 0");

    swap_pair_t pair(market_symb, base_symb);
    check( _dbc.get(pair), "pair does not exist" );
    check( !pair.closed, "pair has been closed" );

    uint64_t swap_fee_ratio = _gstate.swap_fee_ratio;

    asset amount0_out, amount1_out;
    asset amount0_in, amount1_in;
    asset fee0, fee1;

    if (quant_in.symbol == pair.market_reserve.symbol) {
        check( quant_out.symbol == pair.base_reserve.symbol, "quant_out mismatch with base reserve's" );

        amount0_in  = quant_in;
        amount1_out = quant_out;
        amount0_out = asset(0, pair.market_reserve.symbol);
        amount1_in  = asset(0, pair.base_reserve.symbol);

        if (swap_fee_ratio) {
            fee0 = asset( mul(mul(amount0_in.amount, swap_fee_ratio), RATIO_BOOST), amount0_in.symbol );
        }

    } else if (quant_in.symbol == pair.base_reserve.symbol) {
        check( quant_out.symbol == pair.market_reserve.symbol, "quant_out mismatches with market reserve's" );

        amount0_out = quant_out;
        amount1_in  = quant_in;
        amount0_in  = asset(0, pair.market_reserve.symbol);
        amount1_out = asset(0, pair.base_reserve.symbol);

        if (swap_fee_ratio) {
            auto amt = mul(mul(amount1_in.amount, swap_fee_ratio), RATIO_BOOST);
            fee1 = asset( amt, amount1_in.symbol );
        }
    }

    check( amount0_out.amount > 0 || amount1_out.amount > 0, "insufficient output amount");
    check( amount0_out < pair.market_reserve || amount1_out < pair.base_reserve, "insufficient liquidity");

    asset balance0, balance1;
    {
        // check( pair.market_reserve.bank0 != to && market.bank1 != to, "invalid to" );

        if (amount0_out.amount > 0)
<<<<<<< Updated upstream
            TRANSFER( _gstate.swap_pair_bank, to, amount0_out, "" )

        if (amount1_out.amount > 0)
            TRANSFER( _gstate.swap_pair_bank, to, amount1_out, "" )
=======
            TRANSFER( market.bank0, to, amount0_out, "" )

        if (amount1_out.amount > 0)
            TRANSFER( market.bank1, to, amount1_out, "" )
>>>>>>> Stashed changes

        balance0 = pair.market_reserve + amount0_in - amount0_out;
        balance1 = pair.base_reserve + amount1_in - amount1_out;
    }

    {
        asset balance0_adjusted = balance0 * 1000 - amount0_in * 3;
        asset balance1_adjusted = balance1 * 1000 - amount1_in * 3;

        auto index0 = mul( balance0_adjusted.amount, balance1_adjusted.amount );
        auto index1 = mul( pair.market_reserve.amount, pair.base_reserve.amount ) * 1000 * 1000;

        check( index0 >= index1, "invalid amount in balance0_adjusted balance1_adjusted index0 < index1" );
    }

    // process fee
    if (fee0.amount > 0 || fee1.amount > 0) {
        check( is_account(_gstate.swap_fee_receiver), "swap_fee_receiver account not set" );

        if (fee0.amount > 0) {
<<<<<<< Updated upstream
            TRANSFER( market.bank0, _gstate.swap_fee_receiver, fee0, "" )
=======
            TRANSFER( market.bank0, _gstate.owner, fee0, "" )
>>>>>>> Stashed changes

            balance0 -= fee0;
        }
        if (fee1.amount > 0) {
<<<<<<< Updated upstream
            TRANSFER( market.bank1, _gstate.swap_fee_receiver, fee1, "" )
=======
            TRANSFER( market.bank1, _gstate.owner, fee1, "" )
>>>>>>> Stashed changes

            balance1 -= fee1;
        }

    }

    pair.market_reserve     = balance0;
    pair.base_reserve       = balance1;
    pair.updated_at         = time_point_sec( current_time_point() );

    _dbc.set( pair );

}


void ayj_swap::ontransfer(const name& from, const name& to, const asset& quantity, const string& memo) {
    constexpr string_view DEPOSIT_TO = "deposit to:";
    constexpr string_view EXCHANGE   = "exchange:";

    if (from == _self) return;

    check( to == _self, "This transfer is not for swap contract itself");
    check( quantity.amount > 0, "quantity must be positive");

    auto incoming = extended_asset{ quantity, get_first_receiver()};
    string_view memosv(memo);
    if ( starts_with(memosv, EXCHANGE) ) {
        memoexchange(from, incoming, memosv.substr(EXCHANGE.size()) );

    } else {
      if ( starts_with(memosv, DEPOSIT_TO) ) {
          from = name(trim(memosv.substr(DEPOSIT_TO.size())));
          check(from != get_self(), "Donation not accepted");
      }
      add_signed_ext_balance(from, incoming);
    }
}

/** 
 *  LP transfers assets into this contract to add liquidity for token A & B in multiple actions
 */
// [[eosio::on_notify("*::transfer")]]
void ayj_swap::deposit(const name& from, const name& to, const asset& quant, const string& memo) {
    if (to != _self)  return;

    std::vector<string> memo_arr = string_split(memo, ':');
    auto memo_arr_size = memo_arr.size();
    check( memo_arr_size > 1, "params size must > 1 in transfer memo: " + to_string(memo_arr_size) );

    auto action          = name( memo_arr[0] ).value;
    auto market_symb     = symbol_code( memo_arr[1] );
    auto base_symb       = symbol_code( memo_arr[2] );

    switch (action) {
    //by liquidity providers, must be done in two actions for transferring both market and base tokens
    case N(enliq): {// "enliq:$symb0:$symb1:$step:$sn" E.g. "enliq:BTC:ETH:1:0001"
        check( memo_arr_size == 5, "enliq params size must be 5 in transfer memo" + to_string(memo_arr_size));
        auto step = atoi(memo_arr[3].data());
        auto sn = atoi(memo_arr43].data());
        _enliq( market_symb, base_symb, step, from, quant, sn);
        return;
    }
    //by liquidity providers
    case N(deliq): {// "deliq:BTC:ETH"
        check( memo_arr_size == 3, "deliq params size must be 3 in transfer memo" + to_string(memo_arr_size) );
        _deliq( market_symb, base_symb, quant, from);
        return;
    }
<<<<<<< Updated upstream
    //by normal swap traders
    case N(swap): {// "swap:BTC:ETH:$quant_out:$to"
        check( memo_arr_size == 5, "burn params size must be 5 in transfer memo", memo_arr_size );
        asset quant_out = asset_from_string(memo_arr[3]);
        _swap( market_symb, base_symb, quant, quant_out, name(memo_arr[3]));
=======
    case N(swap): { // swap:BTC-ETH:quant_out:to
        check( memo_size == 4, "burn params size % must == 4 in transfer memo", memo_size );
        asset quant_out = asset_from_string(transfer_memo[2]);
        _swap(market_sym, base_sym, quant, quant_out, name(transfer_memo[3]));
>>>>>>> Stashed changes
        return;
    }
    default: {
        check(false, "unsuppport transfer action in memo: " + memo_arr[0]);
        return;
    }
    }

}

/************************ ACTION functions ****************************/
ACTION ayj_swap::init() {
    require_auth( _self );

    _gstate.swap_token_bank     = SYS_BANK;
    _gstate.lp_token_bank       = SYS_BANK;
    _gstate.admin               = "mwalletadmin"_n;
    _gstate.fee_receiver        = "mwalletadmin"_n; //TBD

    // _gstate.liquidity_bank  = liquidity_bank;
    // _gstate.operator        = operator;
    // _gstate.swap_fee_ratio  = swap_fee_ratio;

}

//LP user to open new asset type for his/her account pool
ACTION ayj_swap::openasset( const name& lp_user, const extended_symbol& ext_symbol) {
    require_auth( lp_user );
    
    check( is_account( lp_user ), "user account does not exist" );

    asset_t assets( _self, lp_user.value );

    auto asset_idx = assets.get_index<"assetsymb"_n>();
    auto asset_key = make128key( ext_symbol.get_contract().value, ext_symbol.get_symbol().raw());
    const auto& asset_itr = asset_idx.find( asset_key );
    check( asset_itr == asset_idx.end(), "asset already opened for " + lp_user.to_string() );

    assets.emplace( _self, [&]( auto& a ){
        a.id = assets.available_primary_key();
        a.balance = extended_asset{ 0, ext_symbol};
    });
}

ACTION ayj_swap::closeasset( const name& lp_user, const name& to, const extended_symbol& ext_symbol, const string& memo) {
    require_auth( lp_user );

    asset_t assets( _self, lp_user.value );
    auto asset_idx = assets.get_index<"assetsymb"_n>();
    auto asset_key = make128key( ext_symbol.get_contract().value, ext_symbol.get_symbol().raw());
    const auto& asset_itr = asset_idx.find( asset_key );
    check( asset_itr != asset_idx.end(), "LP user does not have such token" );

    auto ext_balance = asset_itr->balance;
    if (ext_balance.quantity.amount > 0) {
        TRANSFER( ext_balance.contract, to, ext_balance.quantity, memo )
    }
    asset_idx.erase( asset_itr );
}

ACTION ayj_swap::createpair(const symbol& market_symb, const symbol& base_symb, const symbol& lpt_symb) {
    require_auth( _gstate.admin );

    check( _gstate.base_symbols.contain(base_symb.code().to_string()), "base_symb not registered: " + base_symb.code().to_string() );

    swap_pair_t pair( market_symb, base_symb, lpt_symb);
    check( !_dbc.get( pair ), "pair already exists" );

    if ( _gstate.base_symbols.contain(market_symb.code().to_string())) {
        swap_pair_t rev_pair(base_symb, market_symb);
        check( !_dbc.get( rev_pair ), "reverse pair already exists" );
    }

    pair.created_at   = time_point_sec( current_time_point() );
    pair.closed       = false;

    _dbc.set( pair );

}

ACTION ayj_swap::closepair(const symbol_code& market_symb, const symbol_code& base_symb, const bool closed) {
    require_auth( _self );

    swap_pair_t pair(market_sym, base_sym);
    check( _dbc.get(pair), "swap pair not exist: " + pair.to_string() );
    pair.closed = closed;

    _dbc.set( pair );
}

ACTION ayj_swap::addliquidity(const name& user, const asset& to_buy, asset max_asset1, asset max_asset2) {
    require_auth(user);
    check( (to_buy.amount > 0), "to_buy amount must be positive");
    check( (max_asset1.amount >= 0) && (max_asset2.amount >= 0), "assets must be nonnegative");
    
    add_signed_liq(user, to_buy, true, max_asset1, max_asset2);
}

ACTION ayj_swap::addliquidity(const symbol_code& market_symb, const symbol_code &base_symb, uint64_t step, const name &to, const asset &quant, uint64_t nonce) {

    swap_pair_t pair(market_symb, base_symb);
    check( _dbc.get(pair), "pair not exist:" + pair.to_string() );
    check( !pair.closed, "pair already closed: " + pair.to_string() );

    mint_action_t mint_action(to.value, nonce);
    if (step == UNISWAP_MINT_STEP1) {
        check( quant.symbol == pair.reserve0.symbol, "step1: bank0 symbol and reserve0 symbol mismatch" );
        check( get_first_receiver() == pair.bank0, "step1: bank and bank0 mismatch" );
        check( !_dbc.get(mint_action), "mint action step1 already exist: " );

        mint_action.nonce       = nonce;
        mint_action.market_sym  = market_sym;
        mint_action.base_sym    = base_sym;
        mint_action.bank0       = get_first_receiver();
        mint_action.amount0_in  = quant;
        mint_action.closed      = false;
        mint_action.created_at  = time_point_sec( current_time_point() );

        _dbc.set(mint_action);

    } else if (step == UNISWAP_MINT_STEP2) {
        check( _dbc.get(mint_action), "step2 mint action not exist" );
        check( mint_action.market_sym == market_sym && mint_action.base_sym == base_sym, 
                "symbol_pair of mint action mismatches from param: " );
        // check(mint_action.create_time == time_point_sec(current_time_point()), "2
        // steps must be in one tx");
        check( !mint_action.closed, "mint action already closed" );
        check( get_first_receiver() == pair.bank1, "step2 bank and bank1 mismatch" );
        check( quant.symbol == pair.reserve1.symbol, "step2 bank1 symbol and reserve1 symbol mismatch" );

        mint_action.bank1      = get_first_receiver();
        mint_action.amount1_in = quant;
        mint_action.closed     = true;

        _dbc.set(mint_action);

        int64_t liquidity;
        symbol liquidity_symbol = market.total_liquidity.symbol;
        if (pair.total_liquidity.amount == 0) {
            liquidity = sqrt(mul(mint_action.amount0_in.amount,
                                              mint_action.amount1_in.amount)) -
                        MINIMUM_LIQUIDITY * PRECISION_1;

            ISSUE( _gstate.liquidity_bank, _self, asset(MINIMUM_LIQUIDITY * PRECISION_1, liquidity_symbol), "" )

            market.total_liquidity =
                asset(MINIMUM_LIQUIDITY * PRECISION_1, liquidity_symbol);

        } else {
            liquidity = min(div(mul(mint_action.amount0_in.amount,
                                                            market.total_liquidity.amount),
                                           market.reserve0.amount),
                            div(mul(mint_action.amount1_in.amount,
                                                            market.total_liquidity.amount),
                                           market.reserve1.amount));
        }

        check( liquidity > 0, "zero or negative liquidity minted" );

        market.total_liquidity      += asset(liquidity, liquidity_symbol);
        market.reserve0             = market.reserve0 + mint_action.amount0_in;
        market.reserve1             = market.reserve1 + mint_action.amount1_in;
        market.last_updated_at      = time_point_sec( current_time_point() );

        _dbc.set(market);

        ISSUE( _gstate.liquidity_bank, to, asset(liquidity, liquidity_symbol), "" )
    }

}

ACTION ayj_swap::remliquidity(const symbol_code& market_symb, const symbol_code &base_symb, const asset &lpt_quant, const name &to) {
    // require_auth( to );
    swap_pair_t pair(market_sym, base_symb);
    check( _dbc.get(pair), "pair does not exist: " + pair.to_string() );
    check( !pair.closed, "pair already closed" );
    check( pair.lpt_reserve >= lpt_quant, "over burn lpt: " + ltp_quant.to_string() );
    check( lpt_quant.symbol.raw() == pair.lpt_reserve.symbol.raw(), "liquidity symbol and pair lptoken symbol mismatch" );
    check( get_first_receiver() == _gstate.lp_token_bank, "liquidity bank and lptoken bank mismatch" );
    
    asset balance0 = pair.market_reserve;
    asset balance1 = pair.base_reserve;

    asset amount0 = asset( div( mul(balance0.amount,
                                   div( mul(lpt_quant.amount, PRECISION_1),
                                        pair.lpt_reserve.amount)),
                            PRECISION_1), balance0.symbol);

    asset amount1 = asset( div( mul(balance1.amount,
                                   div(mul(lpt_quant.amount, PRECISION_1),
                                        pair.lpt_reserve.amount)),
                            PRECISION_1), balance1.symbol);

    BURN( _gstate.lp_token_bank, _self, lpt_quant, "" )

    TRANSFER( _gstate.swap_token_bank, to, amount0, "" )
    TRANSFER( _gstate.swap_token_bank, to, amount1, "" )

    pair.lpt_reserve            -= lpt_quant;
    pair.market_reserve          = balance0 - amount0;
    pair.base_reserve            = balance1 - amount1;
    pair.updated_at              = time_point_sec( current_time_point() );

    _dbc.set( pair );
}
/*
ACTION ayj_swap::refund(const symbol_code& market_sym, const symbol_code &base_sym, const name& to, const uint64_t& nonce) {

    swap_pair_t pair(market_sym, base_sym);
    check( _dbc.get(market), "market not exist" );
    check( !market.closed, "market already closed" );

    mint_action_t mint_action(to.value, nonce);
    check( _dbc.get(mint_action), "the mint action not exist" );

    require_auth( mint_action.owner );
    check( mint_action.market_sym == market_sym && mint_action.base_sym == base_sym,
            "symbol_pair of mint action mismatches with symbol_pair");

    check( !mint_action.closed, "mint action already closed" );
    check( mint_action.amount0_in.amount > 0 ||  mint_action.amount1_in.amount, "all asset is 0 in mint_action" );

    check( to != _self, "_self is not to account: " + to.to_string() );

    if (mint_action.amount0_in.amount > 0) {
        TRANSFER( mint_action.bank0, to, mint_action.amount0_in, "" )
        mint_action.amount0_in.amount = 0;
    }

    if (mint_action.amount1_in.amount > 0) {
        TRANSFER( mint_action.bank1, to, mint_action.amount1_in, "" )
        mint_action.amount1_in.amount = 1;
    }

    mint_action.closed = true;
    _dbc.set( mint_action );

}
<<<<<<< Updated upstream
*/
=======

ACTION ayj_swap::close(const symbol_code& market_sym, const symbol_code& base_sym, const bool closed) {
    require_auth( _self );

    market_t market(market_sym, base_sym);
    check( _dbc.get(market), "market not exist" + market.get_sympair() );
    market.closed = closed;

    _dbc.set( market );
}

ACTION ayj_swap::skim(const symbol_code& market_sym, const symbol_code& base_sym, const name& to, const asset& balance0, const asset& balance1) {
    require_auth( _gstate.owner );

    market_t market(market_sym, base_sym);
    check( _dbc.get(market), "market not exist" );
    check( market.closed, "market must be closed first" );
    check( balance0.symbol == market.reserve0.symbol, "bank0 symbol mismatch" );
    check( balance1.symbol == market.reserve1.symbol, "bank1 symbol mismatch" );
    check( balance0 >  market.reserve0, "balance0 <= market reserve0" );
    check( balance1 >  market.reserve1, "balance1 <= market reserve1" );

    TRANSFER( market.bank0, to, balance0 - market.reserve0, "" )
    TRANSFER( market.bank1, to, balance1 - market.reserve1, "" )

    auto now                    = current_time_point();

    market.last_updated_at      = now;
    market.last_skimmed_at      = now;

    _dbc.set(market);

}

ACTION ayj_swap::sync(const symbol_code& market_sym, const symbol_code& base_sym, const asset& balance0, const asset& balance1) {
    require_auth( _gstate.owner );

    market_t market(market_sym, base_sym);
    check( _dbc.get(market), "market does not exist");
    check( market.closed, "market must be closed first");
    check( balance0.symbol == market.reserve0.symbol, "bank0 symbol mismatch" );
    check( balance1.symbol == market.reserve1.symbol, "bank1 symbol mismatch" );

    auto now                    = current_time_point();

    market.reserve0             = balance0;
    market.reserve1             = balance1;
    market.last_updated_at      = now;
    market.last_synced_at       = now;

    _dbc.set(market);
}
>>>>>>> Stashed changes

} //namespace ayj