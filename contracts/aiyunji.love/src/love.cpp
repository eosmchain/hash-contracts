// #include <eosio.token/eosio.token.hpp>
#include "aiyunji.love/love.hpp"
#include "aiyunji.love/wasm_db.hpp"

namespace ayj {

using namespace std;
using namespace eosio;


ACTION ayj_love::init() {
    require_auth( _self );

    _gstate.bank = "hst.token"_n;

}

ACTION ayj_love::addproj(const name& issuer, const string& proj_code) {
    _require_admin( issuer );

    project_t::tbl_t projects(_self, _self.value);
    auto idx = projects.get_index<"projcode"_n>();
    auto pc = sha256(const_cast<char*>(proj_code.c_str()), proj_code.size());
    auto itr = idx.lower_bound( pc );
    check( itr == idx.end(), "project already exists: " + proj_code );

    projects.emplace( _self, [&]( auto& row ) {
        row.proj_code   = proj_code;
        row.owner       = issuer;
        row.created_at  = time_point_sec( current_time_point() );
    });

}

ACTION ayj_love::delproj(const name& issuer, const uint64_t& proj_id) {
    _require_admin( issuer );

    project_t proj(proj_id);
    check( _dbc.get(proj), "proj not found: " + to_string(proj_id) );
    check( proj.owner == issuer, issuer.to_string() + " is not an project owner: " + proj.owner.to_string() );

    _dbc.del( proj );

}


/**
 * Usage:  to mint new CNY tokens upon receiving fiat CNY from another channel
 */
ACTION ayj_love::donate(const name& issuer, const name& donor, const uint64_t& proj_id, const asset& quantity, const string& pay_sn, const string& memo) {
    require_auth( issuer );
    check( is_account(donor), "donor account not exist" );

    project_t proj(proj_id);
    check( _dbc.get(proj), "proj not exist for " + to_string(proj_id) );

    proj.total_donated += quantity;
    _dbc.set(proj);

    donation_t donation(_self, _self.value);
    _dbc.get(donation);

    donation.donor      = donor;
    donation.proj_id    = proj_id;
    donation.quantity   = quantity;
    donation.pay_sn     = pay_sn;
    donation.donated_at = time_point_sec( current_time_point() );

    _dbc.set(donation);

    ISSUE( _gstate.bank, _self, quantity, "" )

}

/**
 * Usage:  transfer tokens to the contract for charity/shopping etc. purposes
 */
ACTION ayj_love::accept(const name& issuer, const name& to, const uint64_t& proj_id, const asset& quantity, const string& pay_sn, const string& memo) {
    require_auth( issuer );

    check( is_account(to), "to is not an account" );

    acception_t acception(_self, _self.value);
    _dbc.get(acception);

    project_t proj(proj_id);
    check( _dbc.get(proj), "proj: " + to_string(proj_id) + " not exist" );
    check( proj.total_donated >= proj.total_withdrawn, "proj over-withdrawn" );
    auto available_quant = proj.total_donated - proj.total_withdrawn;
    check( quantity <= available_quant, "insufficient to donate" );
    proj.total_withdrawn   += quantity;
    _dbc.set(proj);

    acception.donee         = to;
    acception.proj_id       = proj_id;
    acception.pay_sn        = pay_sn;
    acception.received      = quantity;
    acception.received_at   = time_point_sec( current_time_point() );

    BURN( _gstate.bank, _self, quantity )

}

} //ayj namespace