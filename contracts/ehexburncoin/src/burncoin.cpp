#include <burncoin/burncoin.hpp>
#include <burncoin/utils.hpp>
#include <eosio.token/eosio.token.hpp>

using namespace eosio;
using namespace std;
using std::string;

//account: ehexburncoin
namespace ehex {

void ehex_burncoin::init() {
	require_auth( _self );

	asset_info_t ehex_info {
		asset(EHEX_MAX_SUPPLY * EHEX_PRECISION, EHEX_SYMBOL),
		asset(0, EHEX_SYMBOL),
		asset(0, EHEX_SYMBOL),
		time_point()
	};

	_gstate.asset_stats.emplace("EHEX", ehex_info);
    
	// check( false, ehex_info.last_burned.to_string() );
}

/**
 * 	transfer & burn
 */
void ehex_burncoin::ontransfer(const name& from, const name& to, const asset& quantity, const string& memo) {
	if (to != _self) return;

	require_auth(from);
	CHECK( quantity.symbol.is_valid(), "Invalid quantity symbol name" )
	CHECK( quantity.is_valid(), "Invalid quantity" )
	CHECK( quantity.symbol == EHEX_SYMBOL, "Token Symbol not allowed" )
	CHECK( quantity.amount > 0, "transferburn quanity must be positive" )
	CHECK( get_first_receiver() == EHEX_BANK, "must transfer by EHEX_BANK: " + EHEX_BANK.to_string() )
	CHECK( _gstate.asset_stats["EHEX"].circulated  > quantity, "Err: quantity overflow" )
	
	auto ehex_stat 				= _gstate.asset_stats["EHEX"];
	ehex_stat.circulated 		-= quantity;
	ehex_stat.total_burned 		+= quantity;
	ehex_stat.last_burned 		= quantity;
	ehex_stat.last_burned_at 	= current_time_point();

	BURN( EHEX_BANK, _self, quantity, memo )


}


} //end of namespace:: ehex