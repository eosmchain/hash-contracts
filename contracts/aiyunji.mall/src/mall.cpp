#include <aiyunji.mall/mall.hpp>
#include <aiyunji.mall/utils.hpp>

namespace ayj {

void ayj_mall::init() {

}

/**
 * 	@from: 		admin user
 *  @to: 		mall contract or self
 *  @quantity:	amount issued and transferred
 *  @memo: 		"<shop_id>"
 * 		
 */
void ayj_mall::deposit(const name& from, const name& to, const asset& quantity, const string& memo) {
	if (to != _self) return;

	CHECK( quantity.symbol.is_valid(), "Invalid quantity symbol name" )
	CHECK( quantity.is_valid(), "Invalid quantity" )
	CHECK( quantity.symbol == _cstate.mall_symb, "Token Symbol not allowed" )
	CHECK( quantity.amount > 0, "deposit quanity must be positive" )

	vector<string_view> params = split(memo, ":");
	CHECK( params.size() == 2, "memo must be of <user_id>:<shop_id>" )
	auto user_id = parse_uint64(params[0]);
	auto shop_id = parse_uint64(params[1]);

	auto N = quantity;

	//user:		45%
	auto r0 = _cstate.allocation_ratios[0];
	auto share0 = N * r0 / share_boost;
	user_t user(from);
	_dbc.get( user );
	user.share += share0;
	_dbc.set( user );
	
	shop_t shop(shop_id);
	CHECK( _dbc.get(shop), "Err: shop not found: " + to_string(shop_id) )

	auto r1 = _cstate.allocation_ratios[1];
	auto share1 = N * r1 / share_boost; 
	auto r2 = _cstate.allocation_ratios[1];
	auto share2 = N * r2 / share_boost;
	shop.shop_sunshine_share += share1; //shop-all:		15%
	shop.shop_top_share += share2;	//shop-top:		5%
	shop.updated_at = time_point_sec(current_time_point());
	_dbc.set( shop );

	//certified:	8%
	auto r3 = _cstate.allocation_ratios[3];
	auto share3 = N * r3 / share_boost; 
	_gstate.certified_user_share += share3;

	//platform-top: 5%
	auto r4 = _cstate.allocation_ratios[4];
	auto share4 = N * r4 / share_boost; 
	_gstate.platform_top_share += share4;

	//direct-referral:	20%
	//agent:		5%
	
	auto r6 = _cstate.allocation_ratios[6]; //citycenter:	3%
	auto share6 = N * r6 / share_boost; 
	citycenter_t cc(shop.citycenter_id);
	CHECK( _dbc.get(cc), "Err: citycenter not found: " + to_string(shop.citycenter_id) )
	cc.share += share6;
	_dbc.set( cc );

	//ram-usage:	4%
	auto r7 = _cstate.allocation_ratios[7];
	auto share7 = N * r7 / share_boost; 
	_gstate.ram_usage_share += share7;
    
    
}

} /// namespace ayj
