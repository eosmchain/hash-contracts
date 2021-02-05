#include <aiyunji.mall/mall.hpp>
#include <aiyunji.mall/utils.hpp>
#include <aiyunji.mall/math.hpp>
#include <eosio.token/eosio.token.hpp>

namespace ayj {

inline void ayj_mall::credit_user(const asset& total_share, user_t& user) {
	auto share0 = total_share * _cstate.allocation_ratios[0] / ratio_boost;
	user.spending_reward += share0; 	//user:		45%

	_dbc.set( user );
}

inline void ayj_mall::credit_shop(const asset& total_share, shop_t& shop) {
	auto share1 = total_share * _cstate.allocation_ratios[1] / ratio_boost; 
	auto share2 = total_share * _cstate.allocation_ratios[2] / ratio_boost;
	shop.shop_sunshine_share += share1; //shop-all:		15%
	shop.shop_top_share += share2;		//shop-top:		5%
	shop.updated_at = time_point_sec(current_time_point());

	_dbc.set( shop );
}

inline void ayj_mall::credit_certified(const asset& total_share) {
	auto share3 = total_share * _cstate.allocation_ratios[3] / ratio_boost; 
	_gstate.certified_user_share += share3; //certified:	8%
}

inline void ayj_mall::credit_platform_top(const asset& total_share) {
	auto share4 = total_share * _cstate.allocation_ratios[4] / ratio_boost; 
	_gstate.platform_top_share += share4; 	//platform-top: 5%
}

inline void ayj_mall::credit_citycenter(const asset& total_share, const uint64_t& citycenter_id) {
	citycenter_t cc(citycenter_id);
	CHECK( _dbc.get(cc), "Err: citycenter not found: " + to_string(citycenter_id) )
	auto share7 = total_share * _cstate.allocation_ratios[7] / ratio_boost; 
	cc.share += share7; //citycenter:	3%

	_dbc.set( cc );
}

inline void ayj_mall::credit_referrer(const asset& total_share, const user_t& user, const shop_t& shop) {
	auto share5 = total_share * _cstate.allocation_ratios[5] / ratio_boost; //direct-referral:	10%
	auto share6 = total_share * _cstate.allocation_ratios[6] / ratio_boost;	//agent-referral:	5%

	if (is_account( user.referral_account) && user.referral_account != shop.shop_account) {
		user_t referrer(user.referral_account);
		CHECK( _dbc.get(referrer), "customer referrer not registered: " + user.referral_account.to_string() )
		referrer.customer_referral_reward += share5;
		_dbc.set( referrer );

		_gstate.lucky_draw_share += share6;

	} else if (user.referral_account == shop.shop_account) { //referred within the shop
		user_t referrer(user.referral_account);
		CHECK( _dbc.get(referrer), "customer referrer not registered: " + user.referral_account.to_string() )
		referrer.customer_referral_reward += share5;
		_dbc.set( referrer );

		if (is_account(shop.referral_account)) {
			user_t referrer(user.referral_account);
			CHECK( _dbc.get(referrer), "shop referrer not registered: " + user.referral_account.to_string() )
			referrer.customer_referral_reward += share6;
			_dbc.set( referrer );

		} else {
			_gstate.lucky_draw_share += share6;
		}
	} else {
		_gstate.lucky_draw_share += share5 + share6;
	}
}

inline void ayj_mall::credit_ramusage(const asset& total_share) {
	auto share8 = total_share * _cstate.allocation_ratios[8] / ratio_boost; 
	_gstate.ram_usage_share += share8; //ram-usage:	4%
}

void ayj_mall::log_spending(const asset& quant, const name& customer, const uint64_t& shop_id) {
	auto spent_at = time_point_sec( current_time_point() );

	/// day spending
	day_spending_t::tbl_t day_spends(_self, shop_id);
	auto idx_ds = day_spends.get_index<"daycustomer"_n>();
	auto key_ds = (uint128_t (spent_at.sec_since_epoch() % seconds_per_day) << 64) | customer.value; 
	auto itr_ds = idx_ds.lower_bound( key_ds );
	if (itr_ds == idx_ds.end()) {
		day_spends.emplace(_self, [&](auto& row) {
			row.id 			= day_spends.available_primary_key();
			row.shop_id 	= shop_id;
			row.customer 	= customer;
			row.spending 	= quant;
			row.spent_at 	= spent_at;
		});
	} else {
		day_spends.modify(*itr_ds, _self, [&](auto& row) {
			row.spending 	+= quant;
			row.spent_at 	= spent_at;
		});
	}

	/// total spending (accumulated)
	total_spending_t::tbl_t total_spends(_self, _self.value);
	auto idx_ts = total_spends.get_index<"shopcustomer"_n>();
	auto key_ts = (uint128_t (spent_at.sec_since_epoch() % seconds_per_day) << 64) | customer.value; 
	auto itr_ts = idx_ts.lower_bound( key_ts );
	if (itr_ts == idx_ts.end()) {
		total_spends.emplace(_self, [&](auto& row)  {
			row.id          = total_spends.available_primary_key();
			row.shop_id     = shop_id;
			row.customer    = customer;
			row.spending    = quant;
			row.spent_at    = spent_at;
		});
	} else {
		total_spends.modify(*itr_ts, _self, [&](auto& row) {
			row.spending    += quant;
			row.spent_at    = spent_at;
		});
	}

	_gstate.platform_total_share += quant;
}

/**
 * 	@from: 		only admin user allowed
 *  @to: 		mall contract or self
 *  @quantity:	amount issued and transferred
 *  @memo: 		"<user_account>@<shop_id>"
 * 		
 */
void ayj_mall::creditspend(const name& from, const name& to, const asset& quantity, const string& memo) {
	if (to != _self) return;

	require_auth(from);
	CHECK( from == _cstate.platform_admin, "non-admin user not allowed" )
	CHECK( quantity.symbol.is_valid(), "Invalid quantity symbol name" )
	CHECK( quantity.is_valid(), "Invalid quantity" )
	CHECK( quantity.symbol == HST_SYMBOL, "Token Symbol not allowed" )
	CHECK( quantity.amount > 0, "creditspend quanity must be positive" )

	vector<string_view> params = split(memo, "@");	
	CHECK( params.size() == 2, "memo must be of <user_account>@<shop_id>" )

	auto user_acct 			= name( parse_uint64(params[0]) );
	CHECK( is_account(user_acct), "user account not valid: " + std::string(params[0]) )
	user_t user(user_acct);
	CHECK( !_dbc.get( user ), "user not registered: " + user_acct.to_string() );

	auto shop_id 			= parse_uint64(params[1]);
	shop_t shop(shop_id);
	CHECK( _dbc.get(shop), "Err: shop not found: " + to_string(shop_id) )
	auto cc_id 				= shop.citycenter_id;

	log_spending			( quantity, user_acct, shop_id );
	credit_user				( quantity, user );
	credit_shop				( quantity, shop );
	credit_certified		( quantity );
	credit_platform_top		( quantity );
	credit_referrer			( quantity, user, shop );
	credit_citycenter		( quantity, cc_id );
	credit_ramusage			( quantity );

}

/**
 *  @issuer: platform admin who helps to register
 *  @the_user: the new user to register
 *  @referrer: one who referrs the user onto this platform, if "0" it means empty referrer
 */
ACTION ayj_mall::registeruser(const name& issuer, const name& the_user, const name& referrer) {
	CHECK( issuer == _cstate.platform_admin, "" )
	require_auth( issuer );

	user_t user(the_user);
	CHECK( !_dbc.get(user), "user not found: " + the_user.to_string() )

	user.referral_account = referrer;
	user.created_at = time_point_sec( current_time_point() );

	_dbc.set(user);

}
         
ACTION ayj_mall::registershop(const name& issuer, const name& referrer, const uint64_t& citycenter_id, const uint64_t& parent_shop_id, const name& shop_account) {
	CHECK( issuer == _cstate.platform_admin, "non-admin err" )
	require_auth( issuer );

	shop_t::tbl_t shops(_self, _self.value);
	shops.emplace(_self, [&](auto& row) {
		row.id 					= shops.available_primary_key();
		row.citycenter_id		= citycenter_id;
		row.parent_id 			= parent_shop_id;
		row.shop_account 		= shop_account;
		row.referral_account	= referrer;
	});

}

ACTION ayj_mall::registercc(const name& issuer, const name& cc_name, const name& cc_account) {
	CHECK( issuer == _cstate.platform_admin, "non-admin err" )
	require_auth( issuer );

	citycenter_t::tbl_t ccs(_self, _self.value);
	auto idx = ccs.get_index<"ccname"_n>();
	auto itr = idx.lower_bound(cc_name.value);

	CHECK( itr != idx.end(), "citycenter name already registered: " + cc_name.to_string() )

	ccs.emplace(_self, [&](auto& row)  {
		row.id 					= ccs.available_primary_key();
		row.citycenter_name 	= cc_name;
		row.citycenter_account 	= cc_account;
		row.created_at 			= time_point_sec( current_time_point() );
	});
}

ACTION ayj_mall::certifyuser(const name& issuer, const name& user) {
	require_auth( issuer );
	CHECK( issuer == _cstate.platform_admin, "issuer not platform admin: " )
	CHECK( is_account(user), "user account not valid" )

	certified_user_t certuser(user);
	CHECK( !_dbc.get( certuser ), "user already certified" )
	certuser.user = user;
	certuser.certified_at = time_point_sec( current_time_point() );
	_dbc.set(certuser);
}

ACTION ayj_mall::init() {
	//init mall symbol
	_cstate.mall_bank = "aiyunji.coin"_n;
	_cstate.platform_account = "masteraychen"_n;
}

ACTION ayj_mall::execute() {
	if (!reward_shops()) return;

	if (!reward_certified()) return;

	reward_platform_top();
}

/**
 * @issuer: either user or platform admin who withdraws for user
 * @to: the targer user to receive withdrawn amount
 * @withdraw_type: 0: spending share; 1: referrer share; 2: agent share
 * @quant: the amount to withdraw, when zero, it means withdraw whole amount
 */
ACTION ayj_mall::withdraw(const name& issuer, const name& to, const uint8_t& withdraw_type, asset& quant) {
	require_auth( issuer );
	CHECK( is_account(to), "to account not valid: " + to.to_string() )
	CHECK( withdraw_type < 3, "withdraw_type not valid: " + to_string(withdraw_type) )
	CHECK( quant.amount >= 0 && quant.symbol == HST_SYMBOL, "invalid withdraw quantity: " + quant.to_string() )

	user_t user(to);
	CHECK( _dbc.get(user), "to user not found: " + to.to_string() )
	CHECK( issuer == _cstate.platform_admin || issuer == to, "withdraw other's reward not allowed" )
	
	CHECK( _gstate.platform_total_share >= quant, "platform total share insufficient to withdraw" )
	_gstate.platform_total_share -= quant;

	auto now = current_time_point();

	switch( withdraw_type ) {
	case 0: // spending share removal 
	{
		if (quant.amount == 0) quant = user.spending_reward;
		CHECK( user.spending_reward >= quant, "spending_reward amount is over-withdrawn" )

		auto share_to_remove = quant;
		user.spending_reward -= quant;
		_dbc.set( user );

		asset platform_fees = share_to_remove * _cstate.withdraw_fee_ratio / ratio_boost;
		CHECK( platform_fees < share_to_remove, "Err: withdrawl fees oversized!" )
		share_to_remove -= platform_fees;

		TRANSFER( _cstate.mall_bank, _cstate.platform_account, platform_fees, "fees" )
		TRANSFER( _cstate.mall_bank, to, share_to_remove, "spend reward" )
	}
	break;
	case 1: //customer referral reward withdrawl
	{
		if (quant.amount == 0) quant = user.customer_referral_reward;
		CHECK( user.customer_referral_reward >= quant, "customer_referral_reward amount is over-withdrawn" )

		asset platform_fees = quant * _cstate.withdraw_fee_ratio / ratio_boost;
		CHECK( platform_fees < user.customer_referral_reward, "Err: withdrawl fees oversized!" )
		user.customer_referral_reward -= platform_fees;

		TRANSFER( _cstate.mall_bank, _cstate.platform_account, platform_fees, "customer ref fees" )
		TRANSFER( _cstate.mall_bank, to, quant, "customer ref reward" )
		user.customer_referral_reward -= quant;
		_dbc.set( user );
	}
	break;
	case 2: //shop referral reward withdrawal
	{
		if (quant.amount == 0) quant = user.shop_referral_reward;
		CHECK( user.shop_referral_reward >= quant, "shop_referral_reward amount is over-withdrawn" )

		asset platform_fees = quant * _cstate.withdraw_fee_ratio / ratio_boost;
		CHECK( platform_fees < user.shop_referral_reward, "Err: withdrawl fees oversized!" )
		user.shop_referral_reward -= platform_fees;

		TRANSFER( _cstate.mall_bank, _cstate.platform_account, platform_fees, "shop ref fees" )
		TRANSFER( _cstate.mall_bank, to, quant, "shop ref reward" )
		user.shop_referral_reward -= quant;
		_dbc.set( user );
	}
	break;
	}
}

bool ayj_mall::reward_shop(const uint64_t& shop_id) {
	auto finished = false;

	shop_t shop(shop_id);
	CHECK( _dbc.get(shop), "Err: shop not found: " + to_string(shop_id) )
	CHECK( shop.rewarded_at.sec_since_epoch() % seconds_per_day < current_time_point().sec_since_epoch() % seconds_per_day, "shop sunshine reward already executed" )

	auto spend_key = ((uint128_t) shop.id << 64) | _gstate2.last_sunshine_reward_spending_id;
	total_spending_t::tbl_t total_spends(_self, _self.value);
	auto spend_idx = total_spends.get_index<"shopspends"_n>();
	auto lower_itr = spend_idx.lower_bound( spend_key );
	uint8_t step = 0;
	auto itr = lower_itr;
	for (; itr != spend_idx.end() && step < MAX_STEP; itr++, step++) { /// sunshine reward
		shop.last_sunshine_reward_spending_id = itr->id;
		_gstate2.last_sunshine_reward_spending_id = itr->id;
		auto quant = (shop.shop_sunshine_share / shop.total_customer_spending) * itr->spending;

		user_t user(itr->customer);
		CHECK( _dbc.get(user), "Err: user not found: " + user.account.to_string() )

		TRANSFER( _cstate.mall_bank, user.account, quant, "shop reward" )
	}

	if (itr == spend_idx.end()) { /// top 10 reward
		day_spending_t::tbl_t day_spends(_self, shop.id);
		auto spend_idx = day_spends.get_index<"spends"_n>();
		uint8_t step = 0;
		for (auto itr = spend_idx.begin(); itr != spend_idx.end() && step < _cstate.shop_top_count; itr++, step++) {
			user_t user(itr->customer);
			CHECK( _dbc.get(user), "Err: user not found: " + user.account.to_string() )

			auto quant = (shop.shop_top_share / shop.total_customer_day_spending) * itr->spending;

			TRANSFER( _cstate.mall_bank, user.account, quant, "shop top reward" )
		}
		shop.rewarded_at = time_point_sec( current_time_point() );
		_dbc.set( shop );

		finished = true;
		_gstate2.last_sunshine_reward_spending_id = 0;
	}

	return finished;
}

bool ayj_mall::reward_shops() {
	if (_gstate2.last_shop_reward_finished_at.sec_since_epoch() % seconds_per_day == current_time_point().sec_since_epoch() % seconds_per_day )
		return true; //shops already rewarded

	shop_t::tbl_t shops(_self, _self.value);
	auto itr = shops.find(_gstate2.last_reward_shop_id);
	CHECK( itr++ != shops.end(), "shop not found" + to_string(itr->id) )

	uint8_t step = 0;
	for (; itr != shops.end() && step < MAX_STEP; itr++, step++) {
		if (!reward_shop(itr->id)) return false;

		_gstate2.last_reward_shop_id = itr->id;			
	}

	if (itr == shops.end()) {
		_gstate2.last_reward_shop_id = 0;
		_gstate2.last_shop_reward_finished_at = time_point_sec( current_time_point() );
		return true;
	}

	return false;
}

bool ayj_mall::reward_certified() {
	CHECK( _gstate2.last_certified_reward_finished_at.sec_since_epoch() % seconds_per_day != current_time_point().sec_since_epoch() % seconds_per_day, "certified users already rewarded" )

	bool finished = false;
	auto quant = _gstate.certified_user_share / _gstate.certified_user_count;
	certified_user_t::tbl_t new_users(_self, _self.value);
	uint8_t step = 0;
	auto itr = new_users.begin();
	for (; itr != new_users.end() && step++ < MAX_STEP;) {
		TRANSFER( _cstate.mall_bank, itr->user, quant, "cert reward" )

		itr = new_users.erase(itr);
	}

	if (itr == new_users.end()) {
		_gstate.certified_user_count = 0;
		_gstate2.last_certified_reward_finished_at = time_point_sec( current_time_point() );
		finished = true;
	}

	return finished;
}

bool ayj_mall::reward_platform_top() {
	CHECK( _gstate2.last_platform_reward_finished_at.sec_since_epoch() % seconds_per_day 
			!= current_time_point().sec_since_epoch() % seconds_per_day, "platform top users already rewarded" )

	auto finished = false;
	user_t::tbl_t users(_self, _self.value);
	auto user_idx = users.get_index<"totalassets"_n>();
	auto itr = user_idx.begin();
	for (uint8_t step = 0; itr != user_idx.end() && step < MAX_STEP; itr++) { /// sunshine reward
		if ( step++ < _gstate2.last_platform_top_reward_step) continue;
		
		_gstate2.last_platform_top_reward_step++;
		if (_gstate2.last_platform_top_reward_step == _cstate.platform_top_count) break;

		//TODO: need to freeze total assets, platform top share, platform total share
		auto amt = _gstate.platform_top_share.amount * itr->total_assets().amount / _gstate.platform_total_share.amount;
		TRANSFER( _cstate.mall_bank, itr->account, asset(amt, HST_SYMBOL), "shop reward" )
	}

	if (itr == user_idx.end() || _gstate2.last_platform_top_reward_step == _cstate.platform_top_count) {
		_gstate2.last_platform_reward_finished_at = time_point_sec( current_time_point() );
		_gstate2.last_platform_top_reward_step = 0;
		finished = true;
	}

	return true;
}

} /// namespace ayj
