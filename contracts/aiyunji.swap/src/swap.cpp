#include <aiyunji.swap/swap.hpp>
#include <eosio.token/eosio.token.hpp>
#include <math.hpp>
#include <utils.hpp>

namespace ayj {

using namespace std;
using namespace eosio;
using namespace wasm::safemath;

/************************ Help functions ****************************/
void ayj_swap::_mint(const symbol_code& market_sym, const symbol_code &base_sym, uint64_t step, const name &to, const asset &quant, uint64_t nonce) {

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

    TRANSFER( market.bank0, _self, to, amount0 )
    TRANSFER( market.bank1, _self, to, amount1 )

    market.total_liquidity      -= liquidity;
    market.reserve0             = balance0 - amount0;
    market.reserve1             = balance1 - amount1;
    market.last_updated_at      = current_time_point();

    _dbc.set(market);
}

void ayj_swap::_swap(const symbol_code& market_sym, const symbol_code &base_sym, const asset &quant_in, const asset &quant_out, const name &to) {
    check( quant_in.amount > 0, "input amount must be > 0");

    market_t market(market_sym, base_sym);
    check( _dbc.get(market), "market does not exist" );
    check( !market.closed, "market has been closed" );

    uint64_t swap_fee_ratio = _gstate.swap_fee_ratio;

    asset amount0_out, amount1_out;
    asset amount0_in, amount1_in;
    asset fee0, fee1;

    if (quant_in.symbol == market.reserve0.symbol) {
        check( quant_out.symbol == market.reserve1.symbol, "quant_out mismatch with reserve1 of market" );

        amount0_in  = quant_in;
        amount1_out = quant_out;
        amount0_out = asset(0, market.reserve0.symbol);
        amount1_in  = asset(0, market.reserve1.symbol);

        if (swap_fee_ratio) {
            fee0 = asset( mul(mul(amount0_in.amount, swap_fee_ratio), RATIO_BOOST), amount0_in.symbol );
        }

    } else if (quant_in.symbol == market.reserve1.symbol) {
        check( quant_out.symbol == market.reserve0.symbol, "quant_out mismatches with reserve0 of market" );

        amount0_out = quant_out;
        amount1_in  = quant_in;
        amount0_in  = asset(0, market.reserve0.symbol);
        amount1_out = asset(0, market.reserve1.symbol);

        if (swap_fee_ratio) {
            auto amt = mul(mul(amount1_in.amount, swap_fee_ratio), RATIO_BOOST);
            fee1 = asset( amt, amount1_in.symbol );
        }
    }

    check( amount0_out.amount > 0 || amount1_out.amount > 0, "insufficient output amount");
    check( amount0_out < market.reserve0 || amount1_out < market.reserve1, "insufficient liquidity");

    asset balance0, balance1;
    {
        check( market.bank0 != to && market.bank1 != to, "invalid to" );

        if (amount0_out.amount > 0)
            TRANSFER( market.bank0, _self, to, amount0_out )

        if (amount1_out.amount > 0)
            TRANSFER( market.bank1, _self, to, amount1_out )

        balance0 = market.reserve0 + amount0_in - amount0_out;
        balance1 = market.reserve1 + amount1_in - amount1_out;
    }

    {
        asset balance0_adjusted = balance0 * 1000 - amount0_in * 3;
        asset balance1_adjusted = balance1 * 1000 - amount1_in * 3;

        auto index0 = mul(balance0_adjusted.amount, balance1_adjusted.amount);
        auto index1 = mul(market.reserve0.amount, market.reserve1.amount) * 1000 * 1000;

        check( index0 >= index1, "invalid amount in balance0_adjusted balance1_adjusted index0 < index1" );
    }

    // process fee
    if (fee0.amount > 0 || fee1.amount > 0) {
        check( is_account(_gstate.owner), "owner account not set" );

        if (fee0.amount > 0) {
            TRANSFER( market.bank0, _self, _gstate.owner, fee0 )

            balance0 -= fee0;
        }
        if (fee1.amount > 0) {
            TRANSFER( market.bank1, _self, _gstate.owner, fee1 )

            balance1 -= fee1;
        }

    }

    market.reserve0             = balance0;
    market.reserve1             = balance1;
    market.last_updated_at      = time_point_sec( current_time_point() );

    _dbc.set(market);

}

//[[eosio::on_notify("*::transfer")]]
void ayj_swap::deposit(name from, name to, asset quant, string memo) {
    if (to != _self)  return;

    std::vector<string> transfer_memo = string_split(memo, ':');
    auto memo_size = transfer_memo.size();
    check( memo_size > 1, "params size must > 1 in transfer memo: " + to_string(memo_size) );

    uint64_t action         = name(transfer_memo[0]).value;
    symbol_code market_sym  = symbol_code(transfer_memo[1]);
    symbol_code base_sym    = symbol_code(transfer_memo[2]);

    switch (action) {
    case N(mint): {// mint:{symbol0-symbol1}:{step}:{nonce}, example "mint:BTC-ETH:1:100"
        check( memo_size == 4, "mint params size % must == 4 in transfer memo" + to_string(memo_size));
        _mint(market_sym, base_sym, atoi(transfer_memo[2].data()), from, quant, atoi(transfer_memo[3].data()));
        return;
    }
    case N(burn): {// burn:BTC-ETH
        check( memo_size == 2, "burn params size % must == 2 in transfer memo" + to_string(memo_size) );
        _burn(market_sym, base_sym, quant, from);
        return;
    }
    case N(swap): { // swap:BTC-ETH:quant_out:to
        check( memo_size == 4, "burn params size % must == 4 in transfer memo", memo_size );
        asset quant_out = asset::asset_from_string(transfer_memo[2]);
        _swap(market_sym, base_sym, quant, quant_out, name(transfer_memo[3]));
        return;
    }
    default: {
        check(false, "unsuppport transfer action in memo: " + transfer_memo[0]);
        return;
    }
    }

}

/************************ ACTION functions ****************************/
ACTION ayj_swap::init() {
    require_auth( _self );

    // check( is_account(owner), "owner '%' account not exist", owner);
    // check( is_account(liquidity_bank), "liquidity_bank '%' account not exist", liquidity_bank);
    // check( is_account(operator), "operator not found: %s", operator.to_string() );
    // check( swap_fee_ratio <= RATIO_MAX, "fee ratio must <= %", RATIO_MAX);

    // _gstate.owner           = owner;
    // _gstate.liquidity_bank  = liquidity_bank;
    // _gstate.operator        = operator;
    // _gstate.swap_fee_ratio  = swap_fee_ratio;

}

ACTION ayj_swap::create(const name& bank0, const name& bank1, const symbol& symbol0, const symbol& symbol1, const uint64_t& lpcode) {
    require_auth( _gstate.owner );

    string symbol0_str = symbol0.code().to_string();
    string symbol1_str = symbol1.code().to_string();
    string symbol_pair = symbol0_str + "-" + symbol1_str;
    string reverse_symbol_pair = symbol1_str + "-" + symbol0_str;

    market_t market(symbol0.code(), symbol1.code());
    check( !_dbc.get(market), "market of symbol_pair already exist" );

    market_t reverse_market(symbol1.code(), symbol0.code());
    check( !_dbc.get( reverse_market), "market of reverse symbol_pair already exist" );

    static const uint32_t ID_LEN_MAX = 6;
    char buf[ID_LEN_MAX + 1] = {0};
    // format LP token symbol, ex. LP0001
    snprintf(buf, sizeof(buf), "LP%.4d", lpcode);
    symbol_code liquidity_symbol(buf);

    market.total_liquidity = TO_ASSET(0, liquidity_symbol);

    liquidity_index_t liquidity_index(liquidity_symbol, market.market_sym, market.base_sym);
    check( !_dbc.get(liquidity_index), "market with liquidity token symbol already exist" );

    auto now                    = time_point_sec( current_time_point() );

    market.bank0                = bank0;
    market.bank1                = bank1;
    market.reserve0             = asset(0, symbol0);
    market.reserve1             = asset(0, symbol1);
    market.last_updated_at      = now;
    market.last_skimmed_at      = now;
    market.last_synced_at       = now;
    market.closed               = false;

    _dbc.set( market );
    _dbc.set( liquidity_index );

}

ACTION ayj_swap::refund(const symbol_code& market_sym, const symbol_code &base_sym, const name& to, const uint64_t& nonce) {

    market_t market(market_sym, base_sym);
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
        TRANSFER( mint_action.bank0, _self, to, mint_action.amount0_in )
        mint_action.amount0_in.amount = 0;
    }

    if (mint_action.amount1_in.amount > 0) {
        TRANSFER( mint_action.bank1, _self, to, mint_action.amount1_in )
        mint_action.amount1_in.amount = 1;
    }

    mint_action.closed = true;
    _dbc.set( mint_action );

}

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

    TRANSFER( market.bank0, _self, to, balance0 - market.reserve0 )
    TRANSFER( market.bank1, _self, to, balance1 - market.reserve1 )

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

} //namespace ayj