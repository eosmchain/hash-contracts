#include <aiyunji.mall/mall.hpp>
#include <aiyunji.mall/utils.hpp>
#include <aiyunji.mall/math.hpp>
#include <eosio.token/eosio.token.hpp>

namespace ayj {

inline bool ayj_mall::is_today(const time_point_sec& time) {
	return time.sec_since_epoch() % seconds_per_day == current_time_point().sec_since_epoch() % seconds_per_day;
}

///platform share cache
inline void ayj_mall::update_share_cache() {
	if (!_gstate.executing) _gstate.platform_share_cache = _gstate.platform_share;
	_gstate.share_cache_updated = !_gstate.executing;
}
///user share cache
inline void ayj_mall::update_share_cache(user_t& user) {
	if (!_gstate.executing) {
		user.share_cache = user.share;
		_gstate2.last_cache_update_useridx = user.by_cache_update();
	}
	user.share_cache_updated 	= !_gstate.executing;
}
///shop share cache
inline void ayj_mall::update_share_cache(shop_t& shop) {
	if (!_gstate.executing) {
		shop.share_cache = shop.share;
		_gstate2.last_cache_update_shopidx = shop.by_cache_update();
	}
	shop.share_cache_updated 	= !_gstate.executing;
}
///spend share cache
inline void ayj_mall::update_share_cache(spending_t& spend) {
	if (!_gstate.executing) {
		spend.share_cache = spend.share;
		_gstate2.last_cache_update_spendingidx = spend.by_cache_update();
	}
	spend.share_cache_updated 	= !_gstate.executing;
}

inline void ayj_mall::credit_user(const asset& total_share, user_t& user, const time_point_sec& now) {
	auto share0 				= total_share * _cstate.allocation_ratios[0] / ratio_boost;
	user.share.spending_reward 	+= share0; 	//user:		45%
	user.updated_at 			= now;
	update_share_cache(user);

	_dbc.set( user );
}

inline void ayj_mall::credit_shop(const asset& total_share, shop_t& shop, const time_point_sec& now) {
	auto share1 = total_share * _cstate.allocation_ratios[1] / ratio_boost; 
	auto share2 = total_share * _cstate.allocation_ratios[2] / ratio_boost;
	shop.share.sunshine_share 	+= share1; //shop-all:	15%
	shop.share.top_share 		+= share2; //shop-top:	5%
	shop.updated_at				= now;
	update_share_cache(shop);

	_dbc.set( shop );
}

inline void ayj_mall::credit_certified(const asset& total_share) {
	auto share3 				= total_share * _cstate.allocation_ratios[3] / ratio_boost; 
	_gstate.platform_share.certified_user_share += share3; //certified:	8%
}

inline void ayj_mall::credit_platform_top(const asset& total_share) {
	auto share4 				= total_share * _cstate.allocation_ratios[4] / ratio_boost; 
	_gstate.platform_share.top_share += share4; 	//platform-top: 5%
}

inline void ayj_mall::credit_citycenter(const asset& total_share, const name& citycenter) {
	citycenter_t cc(citycenter);
	CHECK( _dbc.get(cc), "Err: citycenter not found: " + citycenter.to_string() )
	auto share7 				= total_share * _cstate.allocation_ratios[7] / ratio_boost; 
	cc.share 					+= share7; //citycenter:	3%

	_dbc.set( cc );
}

inline void ayj_mall::credit_referrer(const asset& total_share, const user_t& user, const shop_t& shop, const time_point_sec& now) {
	auto share5 = total_share * _cstate.allocation_ratios[5] / ratio_boost; //direct-referral:	10%
	auto share6 = total_share * _cstate.allocation_ratios[6] / ratio_boost;	//agent-referral:	5%

	if (is_account( user.referral_account) && user.referral_account != shop.owner_account) {
		user_t ext_referrer(user.referral_account);
		CHECK( _dbc.get(ext_referrer), "customer referrer not registered: " + user.referral_account.to_string() )
		ext_referrer.share.customer_referral_reward += share5;
		ext_referrer.updated_at = now;
		update_share_cache(ext_referrer);
		_dbc.set( ext_referrer );

		_gstate.platform_share.lucky_draw_share += share6;

	} else if (user.referral_account == shop.owner_account) { //referred within the shop
		user_t user_referrer(user.referral_account);
		CHECK( _dbc.get(user_referrer), "customer referrer not registered: " + user.referral_account.to_string() )
		user_referrer.share.customer_referral_reward += share5;
		user_referrer.updated_at = now;
		update_share_cache(user_referrer);
		_dbc.set( user_referrer );

		if (is_account(shop.referral_account)) {
			user_t shop_referrer(user.referral_account);
			CHECK( _dbc.get(shop_referrer), "shop referrer not registered: " + shop.referral_account.to_string() )
			shop_referrer.share.customer_referral_reward += share6;
			shop_referrer.updated_at = now;
			update_share_cache(shop_referrer);
			_dbc.set( shop_referrer );

		} else {
			_gstate.platform_share.lucky_draw_share += share6;
		}
	} else {
		_gstate.platform_share.lucky_draw_share += share5 + share6;
	}
}

inline void ayj_mall::credit_ramusage(const asset& total_share) {
	auto share8 = total_share * _cstate.allocation_ratios[8] / ratio_boost; 
	_gstate.platform_share.ram_usage_share += share8; //ram-usage:	4%
}

void ayj_mall::log_spending(const asset& quant, const name& customer, const uint64_t& shop_id) {
	auto spent_at = time_point_sec( current_time_point() );

	_gstate.platform_share.total_share += quant;

	spending_t::tbl_t spends(_self, _self.value);
	auto idx = spends.get_index<"shopcustidx"_n>();
	auto key = ((uint128_t) shop_id << 64) | customer.value; 
	auto itr = idx.lower_bound( key );
	if (itr == idx.end()) {
		spends.emplace(_self, [&](auto& row) {
			row.id 						= spends.available_primary_key();
			row.shop_id 				= shop_id;
			row.customer 				= customer;
			row.share.day_spending		= quant;
			row.share.total_spending 	= quant;
			row.created_at 				= spent_at;

			update_share_cache(row);
		});
	} else {
		spends.modify(*itr, _self, [&](auto& row) {
			row.share.day_spending 		+= quant;
			row.share.total_spending 	+= quant;

			update_share_cache(row);
		});
	}
}

/**
 * 	triggered by transfer event of a token contract
 *
 *  @from: 		only admin user allowed
 *  @to: 		mall contract or self
 *  @quantity:	amount issued and transferred
 *  @memo: 		"<user_account>@<shop_id>"
 * 		
 */
void ayj_mall::ontransfer(const name& from, const name& to, const asset& quantity, const string& memo) {
	if (to != _self) return;

	require_auth(from);
	CHECK( from == _cstate.platform_admin, "non-admin user not allowed" )
	CHECK( quantity.symbol.is_valid(), "Invalid quantity symbol name" )
	CHECK( quantity.is_valid(), "Invalid quantity" )
	CHECK( quantity.symbol == HST_SYMBOL, "Token Symbol not allowed" )
	CHECK( quantity.amount > 0, "ontransfer quanity must be positive" )

	vector<string_view> params = split(memo, "@");	
	CHECK( params.size() == 2, "memo must be of <user_account>@<shop_id>" )

	auto user_acct 			= name( parse_uint64(params[0]) );
	CHECK( is_account(user_acct), "user account not valid: " + std::string(params[0]) )
	user_t user(user_acct);
	CHECK( !_dbc.get( user ), "user not registered: " + user_acct.to_string() );

	auto shop_id 			= parse_uint64(params[1]);
	shop_t shop(shop_id);
	CHECK( _dbc.get(shop), "Err: shop not found: " + to_string(shop_id) )

	auto now				= time_point_sec( current_time_point() );
	log_spending			( quantity, user_acct, shop_id );
	
	credit_user				( quantity, user, now );
	credit_shop				( quantity, shop, now );
	credit_certified		( quantity );
	credit_platform_top		( quantity );
	credit_referrer			( quantity, user, shop, now );
	credit_citycenter		( quantity, shop.citycenter);
	credit_ramusage			( quantity );
}

/**
 *  @issuer: platform admin who helps to register
 *  @the_user: the new user to register
 *  @referrer: one who referrs the user onto this platform, if "0" it means empty referrer
 */
ACTION ayj_mall::registeruser(const name& issuer, const name& the_user, const name& referrer) {
	CHECK( issuer == _cstate.platform_admin, "non-platform admin not allowed" )
	require_auth( issuer );

	user_t user(the_user);
	CHECK( !_dbc.get(user), "user not found: " + the_user.to_string() )

	user.referral_account = referrer;
	user.created_at = time_point_sec( current_time_point() );

	_dbc.set(user);

}
         
ACTION ayj_mall::registershop(const name& issuer, const name& owner, const name& citycenter, const uint64_t& parent_shop_id, const uint64_t& shop_id) {
	CHECK( issuer == _cstate.platform_admin, "non-platform-admin err" )
	require_auth( issuer );

	user_t user(owner);
	CHECK( _dbc.get(user), "user not found: " + issuer.to_string() )
	auto shop_referrer = user.referral_account;
	if (!shop_referrer)
		shop_referrer = _cstate.platform_referrer;
	
	shop_t shop(shop_id);
	CHECK( !_dbc.get(shop), "shop already registered: " + to_string(shop_id) );

	if (parent_shop_id != 0) {
		shop_t p_shop(parent_shop_id);
		CHECK( _dbc.get(p_shop), "parnet shop not registered: " + to_string(parent_shop_id) );
	}

	shop_t::tbl_t shops(_self, _self.value);
	shops.emplace(_self, [&](auto& row) {
		row.id 					= shop_id;
		row.parent_id 			= parent_shop_id;
		row.citycenter			= citycenter;
		row.owner_account 		= owner;
		row.referral_account	= shop_referrer;
	});

}

ACTION ayj_mall::registercc(const name& issuer, const name& cc_name, const name& cc_account) {
	CHECK( issuer == _cstate.platform_admin, "non-admin err" )
	require_auth( issuer );

	citycenter_t cc(cc_name);
	CHECK( !_dbc.get(cc), "citycenter name already registered: " + cc_name.to_string() )

	cc.citycenter_account = cc_account;
	cc.created_at = time_point_sec( current_time_point() );
	
	_dbc.set( cc );
}

ACTION ayj_mall::certifyuser(const name& issuer, const name& user) {
	require_auth( issuer );
	CHECK( issuer == _cstate.platform_admin, "issuer (" + issuer.to_string() + ") not platform admin: " + _cstate.platform_admin.to_string() )
	CHECK( is_account(user), "user account not valid" )

	certification_t certuser(user);
	CHECK( !_dbc.get( certuser ), "user already certified" )
	certuser.user = user;
	certuser.created_at = time_point_sec( current_time_point() );
	_dbc.set(certuser);

	_gstate.platform_share.certified_user_count++;
}

ACTION ayj_mall::init() {
	//init mall symbol
	_cstate.mall_bank = "aiyunji.coin"_n;
	_cstate.platform_account = "masteraychen"_n;
}

ACTION ayj_mall::execute() {
	auto now = current_time_point();
	auto last_executed_at = _gstate2.last_executed_at.sec_since_epoch();
	CHECK( now.sec_since_epoch() > last_executed_at + seconds_per_halfday, "too early to execute: < 12 hours" )
	
	if (!_gstate.executing) {
		if (!update_all_caches()) return;

		_gstate.executing = true;
	}

	if (!reward_shops()) 		return;
	if (!reward_certified()) 	return;
	if (!reward_platform_top()) return;

	_gstate.executing = false; //execute completed
	_gstate2.last_executed_at = now;
}

/**
 * @issuer: either user or platform admin who withdraws for user
 * @to: the targer user to receive withdrawn amount
 * @withdraw_type: 0: spending share; 1: referrer share; 2: agent share
 * @quant: the amount to withdraw, when zero, it means withdraw whole amount
 * @shop_id: the specific shop to withdraw its spending reward when withdraw_type is 0
 *
 */
ACTION ayj_mall::withdraw(const name& issuer, const name& to, const uint8_t& withdraw_type, const uint64_t& shop_id) {
	require_auth( issuer );
	CHECK( is_account(to), "to account not valid: " + to.to_string() )
	CHECK( withdraw_type < 3, "withdraw_type not valid: " + to_string(withdraw_type) )

	user_t user(to);
	CHECK( _dbc.get(user), "to user not found: " + to.to_string() )
	CHECK( issuer == _cstate.platform_admin || issuer == to, "withdraw other's reward not allowed" )
	
	auto now = current_time_point();

	switch( withdraw_type ) {
	case 0: // spending share removal 
	{
		spending_t::tbl_t spends(_self, _self.value);
		auto idx = spends.get_index<"shopcustidx"_n>();
		auto itr = idx.lower_bound( (uint128_t) shop_id << 64 | to.value );
		CHECK( itr != idx.end(), "customer: " + to.to_string() + " @ shop: " + to_string(shop_id) + " not found" )
		CHECK( user.share_cache.spending_reward >= itr->share_cache.total_spending, "Err: user spending smaller than one shop total spending" )
		auto quant = itr->share_cache.total_spending;
		CHECK( _gstate.platform_share.total_share >= quant, "platform total share insufficient to withdraw" )
		_gstate.platform_share.total_share -= quant;
		update_share_cache();
		
		user.share.spending_reward -= quant;
		update_share_cache( user );

		_dbc.set( user );
		idx.erase(itr);

		asset platform_fees = quant * _cstate.withdraw_fee_ratio / ratio_boost;
		CHECK( platform_fees < quant, "Err: withdrawl fees oversized!" )

		TRANSFER( _cstate.mall_bank, _cstate.platform_account, platform_fees, "fees" )
		TRANSFER( _cstate.mall_bank, to, quant - platform_fees, "spend reward" )
	}
	break;
	case 1: //customer referral reward withdrawl
	{
		auto quant = user.share.customer_referral_reward;
		user.share.customer_referral_reward.amount = 0;
		update_share_cache( user );
		_dbc.set( user );

		asset platform_fees = quant * _cstate.withdraw_fee_ratio / ratio_boost;
		CHECK( platform_fees < quant, "Err: withdrawl fees oversized!" )

		TRANSFER( _cstate.mall_bank, _cstate.platform_account, platform_fees, "customer ref fees" )
		TRANSFER( _cstate.mall_bank, to, quant - platform_fees, "customer ref reward" )
		
	}
	break;
	case 2: //shop referral reward withdrawal
	{
		auto quant = user.share.shop_referral_reward;
		user.share.shop_referral_reward.amount = 0;
		update_share_cache( user );
		_dbc.set( user );

		asset platform_fees = quant * _cstate.withdraw_fee_ratio / ratio_boost;
		CHECK( platform_fees < quant, "Err: withdrawl fees oversized!" )

		TRANSFER( _cstate.mall_bank, _cstate.platform_account, platform_fees, "shop ref fees" )
		TRANSFER( _cstate.mall_bank, to, quant - platform_fees, "shop ref reward" )
	}
	break;
	}
}

bool ayj_mall::reward_shop(const uint64_t& shop_id) {
	shop_t shop(shop_id);
	CHECK( _dbc.get(shop), "Err: shop not found: " + to_string(shop_id) )
	CHECK( !is_today(shop.updated_at), "shop sunshine reward already executed" )

	if (shop.top_rewarded_count == 0) {
		shop.share_cache = shop.share;
		shop.share.reset();
	}

	spending_t::tbl_t spends(_self, _self.value);
	auto spend_idx = spends.get_index<"shopspends"_n>();
	auto upper_itr = spend_idx.upper_bound( shop.last_sunshine_reward_spend_idx.get_index() );
	auto itr = upper_itr;
	for (uint8_t step = 0; itr != spend_idx.end() && step < MAX_STEP; itr++, step++) {
		if (itr->shop_id != shop_id) break; //already iterate all spends within the given shop

		shop.last_sunshine_reward_spend_idx = spend_index_t(itr->shop_id, itr->id, itr->share_cache.day_spending);
		auto share_cache = shop.share_cache;
		auto spending_share_cache = itr->share_cache;
		auto sunshine_quant = share_cache.sunshine_share * spending_share_cache.total_spending.amount / share_cache.total_spending.amount;
		user_t user(itr->customer);
		CHECK( _dbc.get(user), "Err: user not found: " + user.account.to_string() )
		TRANSFER( _cstate.mall_bank, user.account, sunshine_quant, "shop sunshine reward" )  /// sunshine reward

		if (shop.top_rewarded_count++ < shop.top_reward_count) {
			auto top_quant = share_cache.top_share / shop.top_reward_count; //choose average value, to avoid sum traverse
			TRANSFER( _cstate.mall_bank, user.account, top_quant, "shop top reward" ) /// shop top-10 reward
		}
	}

	if (itr == spend_idx.end() || itr->shop_id != shop_id) {
		shop.share_cache.reset();
		shop.top_rewarded_count 			= 0;
		// shop.last_sunshine_reward_spend_idx = uint256_default;
		// shop.last_top_reward_spend_idx 		= uint256_default;
		shop.updated_at 					= time_point_sec( current_time_point() );

		_dbc.set( shop );
		return true;
	}

	return false;
}

bool ayj_mall::reward_shops() {
	if ( is_today(_gstate2.last_shops_rewarded_at) ) return true;

	shop_t::tbl_t shops(_self, _self.value);
	auto itr = shops.upper_bound(_gstate2.last_reward_shop_id);
	for (uint8_t step = 0; itr != shops.end() && step < MAX_STEP; itr++, step++) {
		if (!reward_shop(itr->id)) return false;

		_gstate2.last_reward_shop_id = itr->id;			
	}

	if (itr == shops.end()) {
		_gstate2.last_reward_shop_id = 0;
		_gstate2.last_shops_rewarded_at = time_point_sec( current_time_point() );
		return true;
	}

	return false;
}

bool ayj_mall::reward_certified() {
	if ( is_today(_gstate2.last_certification_rewarded_at) ) return true;

	auto quant = _gstate.platform_share_cache.certified_user_share / _gstate.platform_share_cache.certified_user_count;
	certification_t::tbl_t certifications(_self, _self.value);
	auto itr = certifications.begin();
	uint8_t step = 0;
	for (; itr != certifications.end() && step < MAX_STEP; step++) {
		TRANSFER( _cstate.mall_bank, itr->user, quant, "cert reward" )

		itr = certifications.erase(itr);
		_gstate2.last_certification_reward_step++;
	}

	if (itr == certifications.end() || step == _gstate.platform_share_cache.certified_user_count) {
		_gstate.platform_share.certified_user_count = 0;
		_gstate2.last_certification_reward_step = 0;
		_gstate2.last_certification_rewarded_at = time_point_sec( current_time_point() );
		return true;
	}

	return false;
}

bool ayj_mall::reward_platform_top() {
	if ( is_today(_gstate2.last_platform_reward_finished_at) ) return true;

	user_t::tbl_t users(_self, _self.value);
	auto user_idx = users.get_index<"totalrewards"_n>();
	auto itr = user_idx.upper_bound(_gstate2.last_platform_top_reward_id);

	for (uint8_t step = 0; itr != user_idx.end() && step < MAX_STEP; itr++, step++) {
		if (_gstate2.last_platform_top_reward_step++ == _cstate.platform_top_count) break; // top-1000 reward

		auto quant = _gstate.platform_share_cache.top_share / _cstate.platform_top_count;
		TRANSFER( _cstate.mall_bank, itr->account, quant, "platform top reward" )

		_gstate2.last_platform_top_reward_id = itr->by_total_rewards();
	}

	if (itr == user_idx.end() || _gstate2.last_platform_top_reward_step == _cstate.platform_top_count) {
		_gstate2.last_platform_top_reward_step 		= 0;
		_gstate2.last_platform_top_reward_id		= 0;
		_gstate2.last_platform_reward_finished_at 	= time_point_sec( current_time_point() );
		return true;
	}

	return false;
}

bool ayj_mall::update_all_caches() {

	update_share_cache(); // platform share cache

	user_t::tbl_t users(_self, _self.value);
	auto user_idx = users.get_index<"cacheupdt"_n>();
	auto user_itr = user_idx.upper_bound(_gstate2.last_cache_update_useridx);
	for (auto step = 0; user_itr != user_idx.end() && step < MAX_STEP; user_itr++, step++) {
		user_t user(user_itr->account);
		CHECK( _dbc.get(user), "Err: user not found: " + user_itr->account.to_string() )
		update_share_cache(user);
	}
	if (user_itr != user_idx.end() && !user_itr->share_cache_updated) return false;

	shop_t::tbl_t shops(_self, _self.value);
	auto shop_idx = shops.get_index<"cacheupdt"_n>();
	auto shop_itr = shop_idx.upper_bound(_gstate2.last_cache_update_shopidx);
	for (auto step = 0; shop_itr != shop_idx.end() && step < MAX_STEP; shop_itr++, step++) {
		shop_t shop(shop_itr->id);
		CHECK( _dbc.get(shop), "Err: shop not found: " + to_string(shop_itr->id) )
		update_share_cache(shop);
	}
	if (shop_itr != shop_idx.end() && !shop_itr->share_cache_updated) return false;

	spending_t::tbl_t spends(_self, _self.value);
	auto spend_idx = spends.get_index<"cacheupdt"_n>();
	auto spend_itr = spend_idx.upper_bound(_gstate2.last_cache_update_spendingidx);
	for (auto step = 0; spend_itr != spend_idx.end() && step < MAX_STEP; spend_itr++, step++) {
		spending_t spend(spend_itr->id);
		CHECK( _dbc.get(spend), "Err: spend not found: " + to_string(spend_itr->id) )
		update_share_cache(spend);
	}
	if (spend_itr != spend_idx.end() && !spend_itr->share_cache_updated) return false;

	return true;
}

} /// namespace ayj
